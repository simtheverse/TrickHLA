/*!
@file TrickHLA/LagCompensation.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for Trick HLA lag compensation.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{Attribute.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64Interval.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{LagCompensation.cpp}
@trick_link_dependency{Object.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2006, --, DSES Initial Lag Compensation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Constants.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/LagCompensation.hh"
#include "TrickHLA/Object.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
void LagCompensation::initialize_callback(
   Object *obj )
{
   this->object = obj;
}

void LagCompensation::send_lag_compensation()
{
   ostringstream errmsg;
   errmsg << "LagCompensation::send_lag_compensation():" << __LINE__
          << " ERROR: Your class that extends LagCompensation must implement"
          << " the 'virtual void send_lag_compensation()' function!"
          << THLA_ENDL;
   DebugHandler::terminate_with_message( errmsg.str() );
}

void LagCompensation::receive_lag_compensation()
{
   ostringstream errmsg;
   errmsg << "LagCompensation::receive_lag_compensation():" << __LINE__
          << " ERROR: Your class that extends LagCompensation must implement"
          << " the 'virtual void receive_lag_compensation()' function!"
          << THLA_ENDL;
   DebugHandler::terminate_with_message( errmsg.str() );
}

Attribute *LagCompensation::get_attribute(
   const char *attr_FOM_name )
{
   return object->get_attribute( attr_FOM_name );
}

/*!
 *  @details If the attribute is not found then an error message is displayed
 *  then exec-terminate is called.
 */
Attribute *LagCompensation::get_attribute_and_validate(
   const char *attr_FOM_name )
{
   // Make sure the FOM name is not NULL.
   if ( attr_FOM_name == NULL ) {
      ostringstream errmsg;
      errmsg << "LagCompensation::get_attribute_and_validate():" << __LINE__
             << " ERROR: Unexpected NULL attribute FOM name specified."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the attribute by FOM name.
   Attribute *attr = get_attribute( attr_FOM_name );

   // Make sure we have found the attribute.
   if ( attr == NULL ) {
      ostringstream errmsg;
      errmsg << "LagCompensation::get_attribute_and_validate():" << __LINE__
             << " ERROR: For FOM object '" << object->get_FOM_name()
             << "', failed to find the Attribute for an attribute named"
             << " '" << attr_FOM_name << "'. Make sure the FOM attribute name is"
             << " correct, the FOM contains an attribute named '"
             << attr_FOM_name << "' and that your input file is properly"
             << " configured for this attribute." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return attr;
}

Int64Interval LagCompensation::get_lookahead() const
{
   if ( object != NULL ) {
      return object->get_lookahead();
   } else {
      Int64Interval di( -1.0 );
      return di;
   }
}

Int64Time LagCompensation::get_granted_time() const
{
   if ( object != NULL ) {
      return object->get_granted_time();
   } else {
      Int64Time dt( MAX_LOGICAL_TIME_SECONDS );
      return dt;
   }
}

double LagCompensation::get_scenario_time()
{
   if ( ( object != NULL ) && ( object->get_federate() != NULL ) ) {
      ExecutionControlBase *execution_control = object->get_federate()->get_execution_control();
      return ( execution_control->get_scenario_time() );
   }
   return -std::numeric_limits< double >::max();
}

double LagCompensation::get_cte_time()
{
   if ( ( object != NULL ) && ( object->get_federate() != NULL ) ) {
      ExecutionControlBase *execution_control = object->get_federate()->get_execution_control();
      if ( execution_control->does_cte_timeline_exist() ) {
         return execution_control->get_cte_time();
      }
   }
   return -std::numeric_limits< double >::max();
}
