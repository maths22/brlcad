
#ifndef _STEPattributeList_h
#define _STEPattributeList_h 1

/*
* NIST STEP Core Class Library
* clstepcore/STEPattributeList.h
* April 1997
* K. C. Morris
* David Sauder

* Development of this software was funded by the United States Government,
* and is not subject to copyright.
*/

/* $Id: STEPattributeList.h,v 3.0.1.3 1997/11/05 21:59:25 sauderd DP3.1 $ */

#ifdef __OSTORE__
#include <ostore/ostore.hh>    // Required to access ObjectStore Class Library
#endif

#ifdef __O3DB__
#include <OpenOODB.h>
#endif

//#ifndef _STEPattribute_typedefs
//#define _STEPattribute_typedefs 1

//#include <STEPattribute.h>
class STEPattribute;
#include <SingleLinkList.h>

//class STEPattribute;
class STEPattributeList;
//class AttrListNode;

class AttrListNode :  public SingleLinkNode 
{
  friend class STEPattributeList;

  protected:
    STEPattribute *attr;

  public:
    AttrListNode(STEPattribute *a);
    virtual ~AttrListNode();

#ifdef __OSTORE__
    static os_typespec* get_os_typespec();
#endif
};

class STEPattributeList : public SingleLinkList
{
  public:
    STEPattributeList();
    virtual ~STEPattributeList();

    STEPattribute& operator [] (int n);
    int list_length();
    void push(STEPattribute *a);

#ifdef __OSTORE__
    static os_typespec* get_os_typespec();
#endif
};

/*****************************************************************
**                                                              **
**      This file defines the type STEPattributeList -- a list  **
**      of pointers to STEPattribute objects.  The nodes on the **
**      list point to STEPattributes.  
**                                                              **
		USED TO BE - DAS
**      The file was generated by using GNU's genclass.sh       **
**      script with the List prototype definitions.  The        **
**      command to generate it was as follows:                  **

        genclass.sh STEPattribute ref List STEPattribute

**      The file is dependent on the file "STEPattribute.h"     **
**      which contains the definition of STEPattribute.         **
**                                                              **
**      1/15/91  kcm                                            **
*****************************************************************/

#endif
