/*!
@file SpaceFOM/PhysicalEntityLagCompSA.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
PhysicalEntity latency/lag compensation class.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../source/TrickHLA/LagCompensation.cpp}
@trick_link_dependency{PhysicalEntityLagCompSA.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <string>
#include <float.h>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h" // for send_hs
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Attribute.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityLagCompSA.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityLagCompSA::PhysicalEntityLagCompSA( PhysicalEntityBase & entity_ref ) // RETURN: -- None.
   : PhysicalEntityLagCompBase( entity_ref ),
     integ_t( 0.0 ),
     integ_dt( 0.05 ),
     integ_tol( 1.0e-8 ),
     integrator( this->integ_dt, 13, this->integ_states, this->integ_states, this->derivatives, this )
{
   // Initialize the acceleration values.
   for( int iinc = 0 ; iinc < 3 ; iinc++ ){
      this->accel[iinc]     = 0.0;
      this->rot_accel[iinc] = 0.0;
   }

   // Assign the integrator state references.
   // Translational position
   integ_states[0] = &(this->lag_comp_data.pos[0]);
   integ_states[1] = &(this->lag_comp_data.pos[1]);
   integ_states[2] = &(this->lag_comp_data.pos[2]);
   // Translational velocity
   integ_states[3] = &(this->lag_comp_data.vel[0]);
   integ_states[4] = &(this->lag_comp_data.vel[1]);
   integ_states[5] = &(this->lag_comp_data.vel[2]);
   // Rotational position
   integ_states[6] = &(this->lag_comp_data.quat_scalar);
   integ_states[7] = &(this->lag_comp_data.quat_vector[0]);
   integ_states[8] = &(this->lag_comp_data.quat_vector[1]);
   integ_states[9] = &(this->lag_comp_data.quat_vector[2]);
   // Rotational velocity
   integ_states[10] = &(this->lag_comp_data.ang_vel[0]);
   integ_states[11] = &(this->lag_comp_data.ang_vel[1]);
   integ_states[12] = &(this->lag_comp_data.ang_vel[2]);

}


/*!
 * @job_class{shutdown}
 */
PhysicalEntityLagCompSA::~PhysicalEntityLagCompSA() // RETURN: -- None.
{

}


/*!
 * @job_class{initialization}
 */
void PhysicalEntityLagCompSA::initialize()
{

   // Return to calling routine.
   return;
}


/*! @brief Sending side latency compensation callback interface from the
 *  TrickHLALagCompensation class. */
void PhysicalEntityLagCompSA::send_lag_compensation()
{
   double begin_t = get_scenario_time();
   double end_t;

   // Save the compensation time step.
   this->compensate_dt = get_lookahead().get_time_in_seconds();
   end_t = begin_t + this->compensate_dt;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "******* PhysicalEntityLagCompSA::send_lag_compensation():" << __LINE__ << endl
           << " scenario-time:" << get_scenario_time() << endl
           << "     lookahead:" << this->compensate_dt << endl
           << " adjusted-time:" << end_t << endl;
   }

   // Compensate the data
   this->compensate( begin_t, end_t );

   // Return to calling routine.
   return;
}


/*! @brief Receive side latency compensation callback interface from the
 *  TrickHLALagCompensation class. */
void PhysicalEntityLagCompSA::receive_lag_compensation()
{
   double end_t  = get_scenario_time();
   double data_t = entity.get_time();

   // Save the compensation time step.
   this->compensate_dt = end_t - data_t;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "******* PhysicalEntityLagCompSA::receive_lag_compensation():" << __LINE__ << endl
           << "  scenario-time:" << end_t  << endl
           << "      data-time:" << data_t << endl
           << " comp-time-step:" << this->compensate_dt  << endl;
   }

   // Because of ownership transfers and attributes being sent at different
   // rates we need to check to see if we received attribute data.
   if ( this->state_attr->is_received() ) {
      // Compensate the data
      this->compensate( data_t, end_t );
   }

   // Return to calling routine.
   return;
}


/*!
 * @job_class{derivative}
 */
void PhysicalEntityLagCompSA::derivatives(
   double   t,
   double   states[],
   double   derivs[],
   void   * udata)
{
   double * omega = &(states[10]);
   double   quat_scalar = states[6];
   double * quat_vector = &(states[7]);
   double   qdot_scalar;
   double   qdot_vector[3];

   // Cast the user data to a PhysicalEntityLagCompSA instance.
   PhysicalEntityLagCompSA * lag_comp_data_ptr = static_cast<PhysicalEntityLagCompSA *>(udata);

   // Compute the derivatives based on time, state, and user data.
   // Translational state derivatives.
   derivs[0] = states[3];
   derivs[1] = states[4];
   derivs[2] = states[5];
   derivs[3] = lag_comp_data_ptr->accel[0];
   derivs[4] = lag_comp_data_ptr->accel[1];
   derivs[5] = lag_comp_data_ptr->accel[2];

   // We need to compute the quaternion rate (Q_dot) from the current value
   // of the attitude quaternion and the angular velocity vector.
   compute_Q_dot( quat_scalar,
                  quat_vector,
                  omega,
                  qdot_scalar,
                  qdot_vector );

   // Rotational state derivatives.
   derivs[6]  = qdot_scalar;
   derivs[7]  = qdot_vector[0];
   derivs[8]  = qdot_vector[1];
   derivs[9]  = qdot_vector[2];
   derivs[10] = lag_comp_data_ptr->rot_accel[0];
   derivs[11] = lag_comp_data_ptr->rot_accel[1];
   derivs[12] = lag_comp_data_ptr->rot_accel[2];

   // Return to calling routine.
   return;
}


/*!
 * @job_class{derivative}
 */
int PhysicalEntityLagCompSA::compensate(
   const double t_begin,
   const double t_end   )
{
   double dt_go  = t_end - t_begin;

   // Copy the current PhysicalEntity state over to the lag compensated state.
   this->copy_state_from_entity();
   compute_Q_dot( this->lag_comp_data.quat_scalar,
                  this->lag_comp_data.quat_vector,
                  this->lag_comp_data.ang_vel,
                  this->Q_dot.scalar,
                  this->Q_dot.vector );

   // Print out debug information if desired.
   if ( debug ) {
      cout << "Receive data before compensation: " << endl;
      this->print_lag_comp_data();
   }

   // Propagate the current PhysicalEntity state to the desired time.
   // Set the current integration time for the integrator.
   this->integ_t = t_begin;
   this->integrator.setIndyVar( this->integ_t );
   // Compute and save the size of this compensation step.
   this->compensate_dt = dt_go;

   // Loop through integrating the state forward to the current scenario time.
   while( (dt_go >= 0.0) && (fabs(dt_go) > this->integ_tol) ) {

      // Load the integration states and derivatives.
      this->integrator.load();

      // Perform the integration propagation one integration step.
      if ( dt_go > this->integ_dt ){
         // Not near the end; so, use the defined integration step size.
         this->integrator.variable_step(this->integ_dt);
      }
      else {
         // Near the end; so, integrate to the end of the compensation step.
         this->integrator.variable_step(dt_go);
      }

      // Unload the integrated states and derivatives.
      this->integrator.unload();

      // Update the integration time.
      this->integ_t = this->integrator.getIndyVar();

      // Compute the remaining time in the compensation step.
      dt_go = t_end - this->integ_t;

      if ( debug ) {
         cout<< "Integ t, dt_go: " << integ_t << ", " << dt_go << endl;
      }

   }

   // Update the lag compensated time,
   lag_comp_data.time = integ_t;

   // Print out debug information if desired.
   if ( debug ) {
      cout << "Receive data after compensation: " << endl;
      this->print_lag_comp_data();
   }

   return( 0 );
}



