/*
 *			M A T E R I A L . C
 *
 *  Routines to coordinate the implementation of material properties
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "../h/machine.h"
#include "../h/vmath.h"
#include "../h/raytrace.h"
#include "material.h"

extern int plastic_setup();
extern int texture_setup();
extern int testmap_setup();
extern int cloud_setup();

struct matlib {
	char	*ml_name;
	int	(*ml_setup)();
} matlib[] = {
	"plastic",	plastic_setup,
	"texture",	texture_setup,
	"testmap",	testmap_setup,
	"cloud",	cloud_setup,
	(char *)0,	0			/* END */
};

/*
 *			M A T L I B _ S E T U P
 *
 *  Returns -
 *	0	failed
 *	!0	success
 */
int
matlib_setup( rp )
register struct region *rp;
{
	register struct matlib *mlp;

	if( rp->reg_ufunc )  {
		rt_log("matlib_setup:  region %s already setup\n", rp->reg_name );
		return(0);
	}
	if( rp->reg_mater.ma_matname[0] == '\0' )
		goto def;
	for( mlp=matlib; mlp->ml_name != (char *)0; mlp++ )  {
		if( rp->reg_mater.ma_matname[0] != mlp->ml_name[0]  ||
		    strcmp( rp->reg_mater.ma_matname, mlp->ml_name ) != 0 )
			continue;
		return( mlp->ml_setup( rp ) );
	}
	rt_log("matlib_setup(%s):  material not known, default assumed\n",
		rp->reg_mater.ma_matname );
def:
	return( plastic_setup( rp ) );
}

/*
 *			M A T L I B _ P A R S E
 */
matlib_parse( cp, parsetab, base )
register char *cp;
struct matparse *parsetab;
char *base;		/* base address of users structure */
{
	register struct matparse *mp;
	char *name;
	char *value;

	while( *cp )  {
		/* NAME = VALUE separator (comma, space, tab) */

		/* skip any leading whitespace */
		while( *cp != '\0' && 
		    (*cp == ',' || *cp == ' ' || *cp == '\t' ) )
			cp++;

		/* Find equal sign */
		name = cp;
		while( *cp != '\0' && *cp != '=' )  cp++;
		if( *cp == '\0' )  {
			rt_log("name %s without value\n", name );
			break;
		}
		*cp++ = '\0';

		/* Find end of value */
		value = cp;
		while( *cp != '\0' && *cp != ',' &&
		    *cp != ' ' && *cp != '\t' )
			cp++;
		if( *cp != '\0' )
			*cp++ = '\0';

		/* Lookup name in parsetab table */
		for( mp = parsetab; mp->mp_name != (char *)0; mp++ )  {
			if( strcmp( mp->mp_name, name ) == 0 )  {
				(void)sscanf( value,
					mp->mp_fmt, 
					base + mp->mp_offset );
				goto out;
			}
		}
		rt_log("matlib_parse:  %s=%s not a valid arg\n", name, value);
out:		;
	}
	matlib_print( "matlib_parse", parsetab, base );
}

/*
 *			M A T L I B _ P R I N T
 */
matlib_print( title, parsetab, base )
char *title;
struct matparse *parsetab;
char *base;		/* base address of users structure */
{
	register struct matparse *mp;

	rt_log( "%s\n", title );
	for( mp = parsetab; mp->mp_name != (char *)0; mp++ )  {
		rt_log( " %s=", mp->mp_name );
		switch( mp->mp_fmt[1] )  {
		case 's':
			rt_log( "%s\n", base + mp->mp_offset );
			break;
		case 'd':
			rt_log( "%d\n", *((int *)(base + mp->mp_offset)) );
			break;
		case 'f':
			rt_log( "%f\n", *((double *)(base + mp->mp_offset)) );
			break;
		default:
			rt_log( " %s??\n", mp->mp_fmt );
			break;
		}
	}
}
