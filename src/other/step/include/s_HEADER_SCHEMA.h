#ifndef  S_HEADER_SCHEMA_H
#define  S_HEADER_SCHEMA_H

/*
* NIST STEP Editor Class Library
* cleditor/s_HEADER_SCHEMA.h
* April 1997
* David Sauder
* K. C. Morris

* Development of this software was funded by the United States Government,
* and is not subject to copyright.
*/

// This file was generated by fedex_plus.  You probably don't want to edit
// it since your modifications will be lost if fedex_plus is used to
// regenerate it.

/* $Id: s_HEADER_SCHEMA.h,v 3.0.1.4 1997/11/05 22:11:43 sauderd DP3.1 $ */

#ifdef __O3DB__
#include <OpenOODB.h>
#endif
#include <sdai.h>
#include <Registry.h>
#include <STEPaggregate.h>
//#include <STEPentity.h>

/*	**************  TYPES  	*/
extern TypeDescriptor *HEADER_SCHEMAt_SCHEMA_NAME;
typedef 	SCLP23(String)  	p21DIS_Schema_name;

/*	**************  ENTITIES  	*/
extern EntityDescriptor *HEADER_SCHEMAe_FILE_IDENTIFICATION;
extern AttrDescriptor *a_0FILE_NAME;
extern AttrDescriptor *a_1DATE;
extern AttrDescriptor *a_2AUTHOR;
extern AttrDescriptor *a_3ORGANIZATION;
extern AttrDescriptor *a_4PREPROCESSOR_VERSION;
extern AttrDescriptor *a_5ORIGINATING_SYSTEM;

class s_N279_file_identification  :    public SCLP23(Application_instance) {
  protected:
	SCLP23(String)  _file_name ;
	SCLP23(String)  _date ;
	StringAggregate  _author ;
	StringAggregate  _organization ;
	SCLP23(String)  _preprocessor_version ;
	SCLP23(String)  _originating_system ;
  public:  

	s_N279_file_identification ( ); 
	s_N279_file_identification (s_N279_file_identification& e ); 
	~s_N279_file_identification ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "File_Identification"; }
	int opcode ()  { return 0 ; } 
	const SCLP23(String) & file_name() const;
	void file_name (const char * x);
	const SCLP23(String) & date() const;
	void date (const char * x);
	StringAggregate & author();
	void author (StringAggregate & x);
	StringAggregate & organization();
	void organization (StringAggregate & x);
	const SCLP23(String) & preprocessor_version() const;
	void preprocessor_version (const char * x);
	const SCLP23(String) & originating_system() const;
	void originating_system (const char * x);
};
inline SCLP23(Application_instance) *
create_s_N279_file_identification () {  return (SCLP23(Application_instance)*) new s_N279_file_identification ;  }
extern EntityDescriptor *HEADER_SCHEMAe_IMP_LEVEL;
extern AttrDescriptor *a_6IMPLEMENTATION_LEVEL;

class s_N279_imp_level  :    public SCLP23(Application_instance) {
  protected:
	SCLP23(String)  _implementation_level ;
  public:  

	s_N279_imp_level ( ); 
	s_N279_imp_level (s_N279_imp_level& e ); 
	~s_N279_imp_level ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "Imp_Level"; }
	int opcode ()  { return 1 ; } 
	const SCLP23(String) & implementation_level() const;
	void implementation_level (const char * x);
};
inline SCLP23(Application_instance) *
create_s_N279_imp_level () {  return (SCLP23(Application_instance)*) new s_N279_imp_level ;  }
extern EntityDescriptor *HEADER_SCHEMAe_FILE_NAME;
extern AttrDescriptor *a_7NAME;
extern AttrDescriptor *a_8TIME_STAMP;
extern AttrDescriptor *a_9AUTHOR;
extern AttrDescriptor *a_10ORGANIZATION;
extern AttrDescriptor *a_11STEP_VERSION;
extern AttrDescriptor *a_12PREPROCESSOR_VERSION;
extern AttrDescriptor *a_13ORIGINATING_SYSTEM;
extern AttrDescriptor *a_14AUTHORISATION;

class p21DIS_File_name  :    public SCLP23(Application_instance) {
  protected:
	SCLP23(String)  _name ;
	SCLP23(String)  _time_stamp ;
	StringAggregate  _author ;
	StringAggregate  _organization ;
//	SCLP23(String)  _step_version ;
	SCLP23(String)  _preprocessor_version ;
	SCLP23(String)  _originating_system ;
	SCLP23(String)  _authorisation ;
  public:  

	p21DIS_File_name ( ); 
	p21DIS_File_name (p21DIS_File_name& e ); 
	~p21DIS_File_name ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "File_Name"; }
	int opcode ()  { return 2 ; } 
	const SCLP23(String) & name() const;
	void name (const char * x);
	const SCLP23(String) & time_stamp() const;
	void time_stamp (const char * x);
	StringAggregate & author();
	void author (StringAggregate & x);
	StringAggregate & organization();
	void organization (StringAggregate & x);
//	const SCLP23(String) & step_version() const;
//	void step_version (const char * x);
	const SCLP23(String) & preprocessor_version() const;
	void preprocessor_version (const char * x);
	const SCLP23(String) & originating_system() const;
	void originating_system (const char * x);
	const SCLP23(String) & authorisation() const;
	void authorisation (const char * x);
};
inline SCLP23(Application_instance) *
create_p21DIS_File_name () {  return (SCLP23(Application_instance)*) new p21DIS_File_name ;  }
extern EntityDescriptor *HEADER_SCHEMAe_FILE_DESCRIPTION;
extern AttrDescriptor *a_15DESCRIPTION;
extern AttrDescriptor *a_16IMPLEMENTATION_LEVEL;

class p21DIS_File_description  :    public SCLP23(Application_instance) {
  protected:
	StringAggregate  _description ;
	SCLP23(String)  _implementation_level ;
  public:  

	p21DIS_File_description ( ); 
	p21DIS_File_description (p21DIS_File_description& e ); 
	~p21DIS_File_description ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "File_Description"; }
	int opcode ()  { return 3 ; } 
	StringAggregate & description();
	void description (StringAggregate & x);
	const SCLP23(String) & implementation_level() const;
	void implementation_level (const char * x);
};
inline SCLP23(Application_instance) *
create_p21DIS_File_description () {  return (SCLP23(Application_instance)*) new p21DIS_File_description ;  }
extern EntityDescriptor *HEADER_SCHEMAe_S_CLASSIFICATION;
extern AttrDescriptor *a_17SECURITY_CLASSIFICATION;

class s_N279_classification  :    public SCLP23(Application_instance) {
  protected:
	SCLP23(String)  _security_classification ;
  public:  

	s_N279_classification ( ); 
	s_N279_classification (s_N279_classification& e ); 
	~s_N279_classification ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "S_Classification"; }
	int opcode ()  { return 4 ; } 
	const SCLP23(String) & security_classification() const;
	void security_classification (const char * x);
};
inline SCLP23(Application_instance) *
create_s_N279_classification () {  return (SCLP23(Application_instance)*) new s_N279_classification ;  }
extern EntityDescriptor *HEADER_SCHEMAe_S_FILE_DESCRIPTION;
extern AttrDescriptor *a_18DESCRIPTION;

class s_N279_file_description  :    public SCLP23(Application_instance) {
  protected:
	SCLP23(String)  _description ;
  public:  

	s_N279_file_description ( ); 
	s_N279_file_description (s_N279_file_description& e ); 
	~s_N279_file_description ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "S_File_Description"; }
	int opcode ()  { return 5 ; } 
	const SCLP23(String) & description() const;
	void description (const char * x);
};
inline SCLP23(Application_instance) *
create_s_N279_file_description () {  return (SCLP23(Application_instance)*) new s_N279_file_description ;  }
extern EntityDescriptor *HEADER_SCHEMAe_MAXSIG;
extern AttrDescriptor *a_19MAXIMUM_SIGNIFICANT_DIGIT;

class p21DIS_Maxsig  :    public SCLP23(Application_instance) {
  protected:
	SCLP23(Integer)  _maximum_significant_digit ;
  public:  

	p21DIS_Maxsig ( ); 
	p21DIS_Maxsig (p21DIS_Maxsig& e ); 
	~p21DIS_Maxsig ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "Maxsig"; }
	int opcode ()  { return 6 ; } 
	SCLP23(Integer)  maximum_significant_digit();
	void maximum_significant_digit (SCLP23(Integer)  x);
};
inline SCLP23(Application_instance) *
create_p21DIS_Maxsig () {  return (SCLP23(Application_instance)*) new p21DIS_Maxsig ;  }
extern EntityDescriptor *HEADER_SCHEMAe_CLASSIFICATION;
extern AttrDescriptor *a_20SECURITY_CLASSIFICATION;

class p21DIS_Classification  :    public SCLP23(Application_instance) {
  protected:
	SCLP23(String)  _security_classification ;
  public:  

	p21DIS_Classification ( ); 
	p21DIS_Classification (p21DIS_Classification& e ); 
	~p21DIS_Classification ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "Classification"; }
	int opcode ()  { return 7 ; } 
	const SCLP23(String) & security_classification() const;
	void security_classification (const char * x);
};
inline SCLP23(Application_instance) *
create_p21DIS_Classification () {  return (SCLP23(Application_instance)*) new p21DIS_Classification ;  }
extern EntityDescriptor *HEADER_SCHEMAe_FILE_SCHEMA;
extern AttrDescriptor *a_21SCHEMA_IDENTIFIERS;

class p21DIS_File_schema  :    public SCLP23(Application_instance) {
  protected:
	StringAggregate  _schema_identifiers ;	  //  of  SCHEMA_NAME

  public:  

	p21DIS_File_schema ( ); 
	p21DIS_File_schema (p21DIS_File_schema& e ); 
	~p21DIS_File_schema ();
#ifdef __O3DB__
  void oodb_reInit();
#endif
	char *Name () { return "File_Schema"; }
	int opcode ()  { return 8 ; } 
	StringAggregate & schema_identifiers();
	void schema_identifiers (StringAggregate & x);
};
inline SCLP23(Application_instance) *
create_p21DIS_File_schema () {  return (SCLP23(Application_instance)*) new p21DIS_File_schema ;  }

#endif
