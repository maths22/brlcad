#ifndef  SDAIHEADER_SECTION_SCHEMA_CC
#define  SDAIHEADER_SECTION_SCHEMA_CC
// This file was generated by fedex_plus.  You probably don't want to edit
// it since your modifications will be lost if fedex_plus is used to
// regenerate it.

#ifdef  SC_LOGGING
#include <fstream.h>
extern ofstream * logStream;
#define SCLLOGFILE "scl.log"
#endif

#include <ExpDict.h>
#include <STEPattribute.h>
#include <SdaiHeaderSchema.h>
#include "sc_memmgr.h"

Schema * s_header_section_schema = 0;

/*        **************  TYPES          */
TypeDescriptor     *    header_section_schemat_time_stamp_text;
TypeDescriptor     *    header_section_schemat_section_name;
TypeDescriptor     *    header_section_schemat_context_name;
TypeDescriptor     *    header_section_schemat_schema_name;
TypeDescriptor     *    header_section_schemat_language_name;
TypeDescriptor     *    header_section_schemat_exchange_structure_identifier;

/*        **************  ENTITIES          */

/////////         ENTITY section_language

EntityDescriptor * header_section_schemae_section_language = 0;
AttrDescriptor * a_0section = 0;
AttrDescriptor * a_1default_language = 0;
SdaiSection_language::SdaiSection_language( ) {

    /*  no SuperTypes */

    eDesc = header_section_schemae_section_language;
    STEPattribute * a = new STEPattribute( *a_0section,  &_section );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_1default_language,  &_default_language );
    a -> set_null();
    attributes.push( a );
}
SdaiSection_language::SdaiSection_language( SdaiSection_language & e ) {
    CopyAs( ( SDAI_Application_instance_ptr ) &e );
}
SdaiSection_language::~SdaiSection_language() {  }

SdaiSection_language::SdaiSection_language( SDAI_Application_instance * se, int * addAttrs ) {
    /* Set this to point to the head entity. */
    HeadEntity( se );

    /*  no SuperTypes */

    eDesc = header_section_schemae_section_language;
    STEPattribute * a = new STEPattribute( *a_0section,  &_section );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_1default_language,  &_default_language );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
}

const SdaiSection_name
SdaiSection_language::section_() const {
    return ( const SdaiSection_name ) _section;
}

void
SdaiSection_language::section_( const SdaiSection_name x )

{
    _section = x;
}

const SdaiLanguage_name
SdaiSection_language::default_language_() const {
    return ( const SdaiLanguage_name ) _default_language;
}

void
SdaiSection_language::default_language_( const SdaiLanguage_name x )

{
    _default_language = x;
}

/////////         END_ENTITY section_language


/////////         ENTITY file_population

EntityDescriptor * header_section_schemae_file_population = 0;
AttrDescriptor * a_2governing_schema = 0;
AttrDescriptor * a_3determination_method = 0;
AttrDescriptor * a_4governed_sections = 0;
SdaiFile_population::SdaiFile_population( ) {

    /*  no SuperTypes */

    eDesc = header_section_schemae_file_population;
    STEPattribute * a = new STEPattribute( *a_2governing_schema,  &_governing_schema );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_3determination_method,  &_determination_method );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_4governed_sections,  &_governed_sections );
    a -> set_null();
    attributes.push( a );
}
SdaiFile_population::SdaiFile_population( SdaiFile_population & e ) {
    CopyAs( ( SDAI_Application_instance_ptr ) &e );
}
SdaiFile_population::~SdaiFile_population() {  }

SdaiFile_population::SdaiFile_population( SDAI_Application_instance * se, int * addAttrs ) {
    /* Set this to point to the head entity. */
    HeadEntity( se );

    /*  no SuperTypes */

    eDesc = header_section_schemae_file_population;
    STEPattribute * a = new STEPattribute( *a_2governing_schema,  &_governing_schema );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_3determination_method,  &_determination_method );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_4governed_sections,  &_governed_sections );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
}

const SdaiSchema_name
SdaiFile_population::governing_schema_() const {
    return ( const SdaiSchema_name ) _governing_schema;
}

void
SdaiFile_population::governing_schema_( const SdaiSchema_name x )

{
    _governing_schema = x;
}

const SdaiExchange_structure_identifier
SdaiFile_population::determination_method_() const {
    return ( const SdaiExchange_structure_identifier ) _determination_method;
}

void
SdaiFile_population::determination_method_( const SdaiExchange_structure_identifier x )

{
    _determination_method = x;
}

const StringAggregate_ptr
SdaiFile_population::governed_sections_() const {
    return ( StringAggregate_ptr ) &_governed_sections;
}

void
SdaiFile_population::governed_sections_( const StringAggregate_ptr x )

{
    _governed_sections.ShallowCopy( *x );
}

/////////         END_ENTITY file_population


/////////         ENTITY file_name

EntityDescriptor * header_section_schemae_file_name = 0;
AttrDescriptor * a_5name = 0;
AttrDescriptor * a_6time_stamp = 0;
AttrDescriptor * a_7author = 0;
AttrDescriptor * a_8organization = 0;
AttrDescriptor * a_9preprocessor_version = 0;
AttrDescriptor * a_10originating_system = 0;
AttrDescriptor * a_11authorization = 0;
SdaiFile_name::SdaiFile_name( ) {

    /*  no SuperTypes */

    eDesc = header_section_schemae_file_name;
    STEPattribute * a = new STEPattribute( *a_5name,  &_name );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_6time_stamp,  &_time_stamp );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_7author,  &_author );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_8organization,  &_organization );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_9preprocessor_version,  &_preprocessor_version );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_10originating_system,  &_originating_system );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_11authorization,  &_authorization );
    a -> set_null();
    attributes.push( a );
}
SdaiFile_name::SdaiFile_name( SdaiFile_name & e ) {
    CopyAs( ( SDAI_Application_instance_ptr ) &e );
}
SdaiFile_name::~SdaiFile_name() {  }

SdaiFile_name::SdaiFile_name( SDAI_Application_instance * se, int * addAttrs ) {
    /* Set this to point to the head entity. */
    HeadEntity( se );

    /*  no SuperTypes */

    eDesc = header_section_schemae_file_name;
    STEPattribute * a = new STEPattribute( *a_5name,  &_name );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_6time_stamp,  &_time_stamp );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_7author,  &_author );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_8organization,  &_organization );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_9preprocessor_version,  &_preprocessor_version );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_10originating_system,  &_originating_system );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_11authorization,  &_authorization );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
}

const SDAI_String
SdaiFile_name::name_() const {
    return ( const SDAI_String ) _name;
}

void
SdaiFile_name::name_( const SDAI_String x )

{
    _name = x;
}

const SdaiTime_stamp_text
SdaiFile_name::time_stamp_() const {
    return ( const SdaiTime_stamp_text ) _time_stamp;
}

void
SdaiFile_name::time_stamp_( const SdaiTime_stamp_text x )

{
    _time_stamp = x;
}

const StringAggregate_ptr
SdaiFile_name::author_() const {
    return ( StringAggregate_ptr ) &_author;
}

void
SdaiFile_name::author_( const StringAggregate_ptr x )

{
    _author.ShallowCopy( *x );
}

const StringAggregate_ptr
SdaiFile_name::organization_() const {
    return ( StringAggregate_ptr ) &_organization;
}

void
SdaiFile_name::organization_( const StringAggregate_ptr x )

{
    _organization.ShallowCopy( *x );
}

const SDAI_String
SdaiFile_name::preprocessor_version_() const {
    return ( const SDAI_String ) _preprocessor_version;
}

void
SdaiFile_name::preprocessor_version_( const SDAI_String x )

{
    _preprocessor_version = x;
}

const SDAI_String
SdaiFile_name::originating_system_() const {
    return ( const SDAI_String ) _originating_system;
}

void
SdaiFile_name::originating_system_( const SDAI_String x )

{
    _originating_system = x;
}

const SDAI_String
SdaiFile_name::authorization_() const {
    return ( const SDAI_String ) _authorization;
}

void
SdaiFile_name::authorization_( const SDAI_String x )

{
    _authorization = x;
}

/////////         END_ENTITY file_name


/////////         ENTITY section_context

EntityDescriptor * header_section_schemae_section_context = 0;
AttrDescriptor * a_12section = 0;
AttrDescriptor * a_13context_identifiers = 0;
SdaiSection_context::SdaiSection_context( ) {

    /*  no SuperTypes */

    eDesc = header_section_schemae_section_context;
    STEPattribute * a = new STEPattribute( *a_12section,  &_section );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_13context_identifiers,  &_context_identifiers );
    a -> set_null();
    attributes.push( a );
}
SdaiSection_context::SdaiSection_context( SdaiSection_context & e ) {
    CopyAs( ( SDAI_Application_instance_ptr ) &e );
}
SdaiSection_context::~SdaiSection_context() {  }

SdaiSection_context::SdaiSection_context( SDAI_Application_instance * se, int * addAttrs ) {
    /* Set this to point to the head entity. */
    HeadEntity( se );

    /*  no SuperTypes */

    eDesc = header_section_schemae_section_context;
    STEPattribute * a = new STEPattribute( *a_12section,  &_section );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_13context_identifiers,  &_context_identifiers );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
}

const SdaiSection_name
SdaiSection_context::section_() const {
    return ( const SdaiSection_name ) _section;
}

void
SdaiSection_context::section_( const SdaiSection_name x )

{
    _section = x;
}

const StringAggregate_ptr
SdaiSection_context::context_identifiers_() const {
    return ( StringAggregate_ptr ) &_context_identifiers;
}

void
SdaiSection_context::context_identifiers_( const StringAggregate_ptr x )

{
    _context_identifiers.ShallowCopy( *x );
}

/////////         END_ENTITY section_context


/////////         ENTITY file_description

EntityDescriptor * header_section_schemae_file_description = 0;
AttrDescriptor * a_14description = 0;
AttrDescriptor * a_15implementation_level = 0;
SdaiFile_description::SdaiFile_description( ) {

    /*  no SuperTypes */

    eDesc = header_section_schemae_file_description;
    STEPattribute * a = new STEPattribute( *a_14description,  &_description );
    a -> set_null();
    attributes.push( a );
    a = new STEPattribute( *a_15implementation_level,  &_implementation_level );
    a -> set_null();
    attributes.push( a );
}
SdaiFile_description::SdaiFile_description( SdaiFile_description & e ) {
    CopyAs( ( SDAI_Application_instance_ptr ) &e );
}
SdaiFile_description::~SdaiFile_description() {  }

SdaiFile_description::SdaiFile_description( SDAI_Application_instance * se, int * addAttrs ) {
    /* Set this to point to the head entity. */
    HeadEntity( se );

    /*  no SuperTypes */

    eDesc = header_section_schemae_file_description;
    STEPattribute * a = new STEPattribute( *a_14description,  &_description );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
    a = new STEPattribute( *a_15implementation_level,  &_implementation_level );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
}

const StringAggregate_ptr
SdaiFile_description::description_() const {
    return ( StringAggregate_ptr ) &_description;
}

void
SdaiFile_description::description_( const StringAggregate_ptr x )

{
    _description.ShallowCopy( *x );
}

const SDAI_String
SdaiFile_description::implementation_level_() const {
    return ( const SDAI_String ) _implementation_level;
}

void
SdaiFile_description::implementation_level_( const SDAI_String x )

{
    _implementation_level = x;
}

/////////         END_ENTITY file_description


/////////         ENTITY file_schema

EntityDescriptor * header_section_schemae_file_schema = 0;
AttrDescriptor * a_16schema_identifiers = 0;
SdaiFile_schema::SdaiFile_schema( ) {

    /*  no SuperTypes */

    eDesc = header_section_schemae_file_schema;
    STEPattribute * a = new STEPattribute( *a_16schema_identifiers,  &_schema_identifiers );
    a -> set_null();
    attributes.push( a );
}
SdaiFile_schema::SdaiFile_schema( SdaiFile_schema & e ) {
    CopyAs( ( SDAI_Application_instance_ptr ) &e );
}
SdaiFile_schema::~SdaiFile_schema() {  }

SdaiFile_schema::SdaiFile_schema( SDAI_Application_instance * se, int * addAttrs ) {
    /* Set this to point to the head entity. */
    HeadEntity( se );

    /*  no SuperTypes */

    eDesc = header_section_schemae_file_schema;
    STEPattribute * a = new STEPattribute( *a_16schema_identifiers,  &_schema_identifiers );
    a -> set_null();
    /* Put attribute on this class' attributes list so the */
    /*access functions still work. */
    attributes.push( a );
    /* Put attribute on the attributes list for the */
    /* main inheritance heirarchy. */
    if( !addAttrs || addAttrs[0] ) {
        se->attributes.push( a );
    }
}

const StringAggregate_ptr
SdaiFile_schema::schema_identifiers_() const {
    return ( StringAggregate_ptr ) &_schema_identifiers;
}

void
SdaiFile_schema::schema_identifiers_( const StringAggregate_ptr x )

{
    _schema_identifiers.ShallowCopy( *x );
}

/////////         END_ENTITY file_schema


SDAI_Model_contents_ptr create_SdaiModel_contents_header_section_schema() {
    return new SdaiModel_contents_header_section_schema ;
}

SdaiModel_contents_header_section_schema::SdaiModel_contents_header_section_schema() {
    SDAI_Entity_extent_ptr eep = ( SDAI_Entity_extent_ptr )0;

    eep = new SDAI_Entity_extent;
    eep->definition_( header_section_schemae_section_language );
    _folders.Append( eep );

    eep = new SDAI_Entity_extent;
    eep->definition_( header_section_schemae_file_population );
    _folders.Append( eep );

    eep = new SDAI_Entity_extent;
    eep->definition_( header_section_schemae_file_name );
    _folders.Append( eep );

    eep = new SDAI_Entity_extent;
    eep->definition_( header_section_schemae_section_context );
    _folders.Append( eep );

    eep = new SDAI_Entity_extent;
    eep->definition_( header_section_schemae_file_description );
    _folders.Append( eep );

    eep = new SDAI_Entity_extent;
    eep->definition_( header_section_schemae_file_schema );
    _folders.Append( eep );

}

SdaiSection_language__set_var SdaiModel_contents_header_section_schema::SdaiSection_language_get_extents() {
    return ( SdaiSection_language__set_var )( ( _folders.retrieve( 0 ) )->instances_() );
}

SdaiFile_population__set_var SdaiModel_contents_header_section_schema::SdaiFile_population_get_extents() {
    return ( SdaiFile_population__set_var )( ( _folders.retrieve( 1 ) )->instances_() );
}

SdaiFile_name__set_var SdaiModel_contents_header_section_schema::SdaiFile_name_get_extents() {
    return ( SdaiFile_name__set_var )( ( _folders.retrieve( 2 ) )->instances_() );
}

SdaiSection_context__set_var SdaiModel_contents_header_section_schema::SdaiSection_context_get_extents() {
    return ( SdaiSection_context__set_var )( ( _folders.retrieve( 3 ) )->instances_() );
}

SdaiFile_description__set_var SdaiModel_contents_header_section_schema::SdaiFile_description_get_extents() {
    return ( SdaiFile_description__set_var )( ( _folders.retrieve( 4 ) )->instances_() );
}

SdaiFile_schema__set_var SdaiModel_contents_header_section_schema::SdaiFile_schema_get_extents() {
    return ( SdaiFile_schema__set_var )( ( _folders.retrieve( 5 ) )->instances_() );
}
#endif
