/*!
@file TrickHLA/LagCompensationInteg.cpp
@ingroup TrickHLA
@brief This class provides the implementation for a TrickHLA latency/lag
compensation class using integration.

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
@trick_link_dependency{LagCompensationInteg.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/LagCompensationInteg.hh"

// Uncomment this define if you want additional debug information.
#define TRICK_HLA_DEBUG_INTEG

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
LagCompensationInteg::LagCompensationInteg()
   : integ_t( 0.0 ),
     integ_dt( 0.05 ),
     integ_tol( 1.0e-8 ),
     integrator(NULL)
{
   return;
}


/*!
 * @job_class{shutdown}
 */
LagCompensationInteg::~LagCompensationInteg()
{
   return;
}

/*!
 * @job_class{integration}
 */
int LagCompensationInteg::integrate(
   const double t_begin,
   const double t_end   )
{
   int ipass;
   double compensate_dt = t_end - t_begin;
   double dt_go = compensate_dt;

#ifdef TRICK_HLA_DEBUG_INTEG
   cout<< "Compensate: t_begin, t_end, dt_go: " << t_begin << ", " << t_end
       << ", " << dt_go << endl;
#endif

   // Propagate the current RefFrame state to the desired time.
   // Set the current integration time for the integrator.
   this->integ_t = t_begin;
   this->integrator->time = 0.0;

   // Loop through integrating the state forward to the current scenario time.
   while( (dt_go >= 0.0) && (fabs(dt_go) > this->integ_tol) ) {

      // Print out debug information if requested.
#ifdef TRICK_HLA_DEBUG_INTEG
      cout << "Integ dt, tol, t, dt_go: " << this->integ_dt << ", "
           << this->integ_tol << ", " << integ_t << ", " << dt_go << endl;
#endif

      // Integration inner loop.
      // Step through the integrator's integration steps.
      do {

         // Initialize the integration pass.
         ipass = 0;

         // Compute the derivatives of the lag compensation state vector.
         this->derivative_first();

         // Load the integration states and derivatives.
         this->load();

         // Perform the integration propagation one integration step.
         if ( dt_go > this->integ_dt ){
            // Not near the end; so, use the defined integration step size.
            this->integrator->dt = this->integ_dt;
         }
         else {
            // Near the end; so, use the remainder of the time step.
            this->integrator->dt = dt_go;
         }

         // Call the integrator.
         // Was: ipass |= this->integrator->integrate();
         // Only need the |= if using multiple integrators.
         ipass = this->integrator->integrate();

         // Unload the integrated states.
         this->unload();

      } while ( ipass );

      // Update the integration time.
      this->integ_t = t_begin + this->integrator->time;

      // Compute the remaining time in the compensation step.
      dt_go = compensate_dt - this->integrator->time;

   }

   // Update the lag compensated time,
   this->update_time();

   // Compute the derivatives of the lag compensation state vector.
   this->derivative_first();

   return( 0 );
}
