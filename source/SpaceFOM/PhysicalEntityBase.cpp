/*!
@file SpaceFOM/PhysicalEntityBase.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM PhysicalEntities.

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
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{PhysicalEntityBase.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, SISO, Sept 2010, --, Smackdown implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Cleaned up and filled out.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/matrix_macros.h"
#include "trick/message_proto.h"
#include "trick/vector_macros.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityBase::PhysicalEntityBase() // RETURN: -- None.
   : debug(false),
     initialized(false),
     name_attr(NULL),
     type_attr(NULL),
     status_attr(NULL),
     parent_frame_attr(NULL),
     state_attr(NULL),
     accel_attr(NULL),
     rot_accel_attr(NULL),
     cm_attr(NULL),
     body_frame_attr(NULL),
     stc_encoder(),
     quat_encoder(),
     name(NULL),
     type(NULL),
     status(NULL),
     parent_frame(NULL),
     state(stc_encoder.get_data()),
     body_wrt_struct(quat_encoder.get_data())
{
   V_INIT( accel );
   V_INIT( rot_accel );
   V_INIT( cm );
}

/*!
 * @job_class{shutdown}
 */
PhysicalEntityBase::~PhysicalEntityBase() // RETURN: -- None.
{
   if ( name != (char *)NULL ) {
      trick_MM->delete_var( (void *)name );
      name = (char *)NULL;
   }
   if ( type != (char *)NULL ) {
      trick_MM->delete_var( (void *)type );
      type = (char *)NULL;
   }
   if ( status != (char *)NULL ) {
      trick_MM->delete_var( (void *)status );
      status = (char *)NULL;
   }
   if ( parent_frame != (char *)NULL ) {
      trick_MM->delete_var( (void *)parent_frame );
      parent_frame = (char *)NULL;
   }
   initialized = false;
   name_attr = NULL;
   state_attr = NULL;
   body_frame_attr = NULL;
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{default_data}
 */
void PhysicalEntityBase::default_data(
   TrickHLA::Object *mngr_object,
   char const       *sim_obj_name,
   char const       *entity_obj_name,
   char const       *entity_name,
   char const       *parent_entity_name,
   bool              publishes )
{
   string entity_name_str = string( sim_obj_name ) + "." + string( entity_obj_name );
   string trick_name_str;

   // Associate the instantiated Manager object with this packing object.
   this->object = mngr_object;

   // Set the frame name and parent frame name.
   if( parent_entity_name != NULL ){
      parent_frame = trick_MM->mm_strdup( parent_entity_name );
   }
   else{
      parent_frame = trick_MM->mm_strdup( "" );
   }
   if( entity_name != NULL ){
      name = trick_MM->mm_strdup( entity_name );
   }
   else{
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalEntityBase::default_data():" << __LINE__
             << " WARNING: Unexpected NULL federation instance PhysicalEntity name!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   object->FOM_name            = allocate_input_string( "PhysicalEntity" );
   object->name                = allocate_input_string( entity_name );
   object->create_HLA_instance = publishes;
   object->packing             = this;
   // Allocate the attributes for the PhysicalEntity HLA object.
   object->attr_count = 9;
   object->attributes = (TrickHLA::Attribute *)trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count );

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name      = allocate_input_string( "name" );
   trick_name_str                      = entity_name_str + string( ".name" );
   object->attributes[0].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[0].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[0].publish       = publishes;
   object->attributes[0].subscribe     = !publishes;
   object->attributes[0].locally_owned = publishes;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name      = allocate_input_string( "type" );
   trick_name_str                      = entity_name_str + string( ".type" );
   object->attributes[1].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[1].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[1].publish       = publishes;
   object->attributes[1].subscribe     = !publishes;
   object->attributes[1].locally_owned = publishes;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[2].FOM_name      = allocate_input_string( "status" );
   trick_name_str                      = entity_name_str + string( ".status" );
   object->attributes[2].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[2].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[2].publish       = publishes;
   object->attributes[2].subscribe     = !publishes;
   object->attributes[2].locally_owned = publishes;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[3].FOM_name      = allocate_input_string( "parent_reference_frame" );
   trick_name_str                      = entity_name_str + string( ".parent_frame" );
   object->attributes[3].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[3].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[3].publish       = publishes;
   object->attributes[3].subscribe     = !publishes;
   object->attributes[3].locally_owned = publishes;
   object->attributes[3].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[4].FOM_name      = allocate_input_string( "state" );
   trick_name_str                      = entity_name_str + string( ".stc_encoder.buffer" );
   object->attributes[4].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[4].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[4].publish       = publishes;
   object->attributes[4].subscribe     = !publishes;
   object->attributes[4].locally_owned = publishes;
   object->attributes[4].rti_encoding  = TrickHLA::ENCODING_OPAQUE_DATA;

   object->attributes[5].FOM_name      = allocate_input_string( "acceleration" );
   trick_name_str                      = entity_name_str + string( ".accel" );
   object->attributes[5].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[5].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[5].publish       = publishes;
   object->attributes[5].subscribe     = !publishes;
   object->attributes[5].locally_owned = publishes;
   object->attributes[5].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[6].FOM_name      = allocate_input_string( "rotational_acceleration" );
   trick_name_str                      = entity_name_str + string( ".rot_accel" );
   object->attributes[6].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[6].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[6].publish       = publishes;
   object->attributes[6].subscribe     = !publishes;
   object->attributes[6].locally_owned = publishes;
   object->attributes[6].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[7].FOM_name      = allocate_input_string( "center_of_mass" );
   trick_name_str                      = entity_name_str + string( ".cm" );
   object->attributes[7].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[7].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[7].publish       = publishes;
   object->attributes[7].subscribe     = !publishes;
   object->attributes[7].locally_owned = publishes;
   object->attributes[7].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[8].FOM_name      = allocate_input_string( "body_wrt_structural" );
   trick_name_str                      = entity_name_str + string( ".quat_encoder.buffer" );
   object->attributes[8].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[8].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[8].publish       = publishes;
   object->attributes[8].subscribe     = !publishes;
   object->attributes[8].locally_owned = publishes;
   object->attributes[8].rti_encoding  = TrickHLA::ENCODING_OPAQUE_DATA;

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityBase::initialize()
{
   ostringstream errmsg;

   // Must have federation instance name.
   if ( name == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL entity name!"
             << "  Setting frame name to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      name = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance type.
   if ( type == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL entity type!"
             << "  Setting type to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      type = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance status.
   if ( status == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL entity status!"
             << "  Setting status to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      status = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance parent_ref_frame.
   if ( parent_frame == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL entity parent_ref_frame!"
             << "  Setting parent_ref_frame to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      parent_frame = trick_MM->mm_strdup( "" );
   }

   // Mark this as initialized.
   this->initialized = true;

   // Return to calling routine.
   return;
}

/*!
 * @details From the TrickHLA::Packing class. We override this function so
 * that we can initialize references to the TrickHLA::Attribute's that are
 * used in the unpack function to handle attribute ownership and different
 * attribute data rates.
 *
 * Use the initialize callback function as a way to setup TrickHLA::Attribute
 * references which are use to determine ownership or if data for an attribute
 * was received.
 *
 * @job_class{initialization}
 */
void PhysicalEntityBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   this->TrickHLA::Packing::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   name_attr         = get_attribute_and_validate( "name" );
   type_attr         = get_attribute_and_validate( "type" );
   status_attr       = get_attribute_and_validate( "status" );
   parent_frame_attr = get_attribute_and_validate( "parent_reference_frame" );
   state_attr        = get_attribute_and_validate( "state" );
   accel_attr        = get_attribute_and_validate( "acceleration" );
   rot_accel_attr    = get_attribute_and_validate( "rotational_acceleration" );
   cm_attr           = get_attribute_and_validate( "center_of_mass" );
   body_frame_attr   = get_attribute_and_validate( "body_wrt_structural" );

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityBase::set_name( char const *new_name )
{
   if ( name != NULL ) {
      trick_MM->delete_var( (void *)name );
   }
   name = trick_MM->mm_strdup( new_name );
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityBase::set_type( char const *new_name )
{
   if ( type != NULL ) {
      trick_MM->delete_var( (void *)type );
   }
   type = trick_MM->mm_strdup( new_name );
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityBase::set_status( char const *new_name )
{
   if ( status != NULL ) {
      trick_MM->delete_var( (void *)status );
   }
   status = trick_MM->mm_strdup( new_name );
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityBase::set_parent_ref_frame( char const *new_name )
{
   if ( parent_frame != NULL ) {
      trick_MM->delete_var( (void *)parent_frame );
   }
   parent_frame = trick_MM->mm_strdup( new_name );

   return;
}

