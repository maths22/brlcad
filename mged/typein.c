/*
 *  			T Y P E I N
 *
 * This module contains functions which allow solid parameters to
 * be entered by keyboard.
 *
 * Functions -
 *	f_in		decides what solid needs to be entered and
 *			calls the appropriate solid parameter reader
 *	arb_in		reads ARB params from keyboard
 *	sph_in		reads sphere params from keyboard
 *	ell_in		reads params for all ELLs
 *	tor_in		gets params for torus from keyboard
 *	cyl_in		reads params for all cylinders
 *	box_in		gets params for BOX and RAW from keyboard
 *	rpp_in		gets params for RPP from keyboard
 *	ars_in		gets ARS param from keyboard
 *	checkv		checks for zero vector from keyboard
 *	getcmd		reads and parses input parameters from keyboard
 *	cvt_ged		converts typed in params to GED database format
 *
 * Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include "./machine.h"	/* special copy */
#include "../h/vmath.h"
#include "../h/db.h"
#include "ged.h"
#include "objdir.h"

void	aexists();
extern double atof();
extern void f_quit();

int		args;		/* total number of args available */
int		argcnt;		/* holder for number of args added later */
int		vals;		/* number of args for s_values[] */
int		newargs;	/* number of args from getcmd() */
extern int	numargs;	/* number of args */
extern char	*cmd_args[];	/* array of pointers to args */
char		**promp;	/* pointer to a pointer to a char */

#define MAXLINE		512	/* Maximum number of chars per line */

char *p_arb[] = {
	"Enter X, Y, Z for point 1: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 2: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 3: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 4: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 5: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 6: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 7: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z for point 8: ",
	"Enter Y: ",
	"Enter Z: "
};

char *p_sph[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius: "
};

char *p_ell[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector A: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector B: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector C: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius of revolution: ",		/* 12 ELL1 Lookout! */
	"Enter X, Y, Z of focus point 1: ",	/* 13 ELL  Lookout! */
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of focus point 2: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter axis length L: "
};

char *p_tor[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of normal vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter radius 1: ",
	"Enter radius 2: "
};

char *p_tgc[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of height (H) vector: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector A: ",		/*  6         REC TEC TGC */
	"Enter Y: ",				/*  7         REC TEC TGC */
	"Enter Z: ",				/*  8         REC TEC TGC */
	"Enter X, Y, Z of vector B: ",		/*  9         REC TEC TGC */
	"Enter Y: ",				/* 10         REC TEC TGC */
	"Enter Z: ",				/* 11         REC TEC TGC */
	"Enter scalar c: ",			/* 12                 TGC */
	"Enter scalar d: ",			/* 13                 TGC */
	"Enter radius: ",			/* 14 RCC                 */
	"Enter radius of base: ",		/* 15     TRC             */
	"Enter radius of top: ",		/* 16     TRC             */
	"Enter ratio: "				/* 17             TEC     */
};

char *p_box[] = {
	"Enter X, Y, Z of vertex: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector H: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector W: ",
	"Enter Y: ",
	"Enter Z: ",
	"Enter X, Y, Z of vector D: ",
	"Enter Y: ",
	"Enter Z: "
};

char *p_rpp[] = {
	"Enter XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX: ",
	"Enter XMAX: ",
	"Enter YMIN, YMAX, ZMIN, ZMAX: ",
	"Enter YMAX: ",
	"Enter ZMIN, ZMAX: ",
	"Enter ZMAX: "
};

/*	F _ I N ( ) :  	decides which solid reader to call
 *			Used for manual entry of solids.
 */
void
f_in()
{
	register int i;
	register struct directory *dp;
	union record record;

	(void)signal( SIGINT, sig2);	/* allow interrupts */

	/* Save the number of args loaded initially */
	args = numargs;
	argcnt = 0;
	vals = 0;

	/* Get the name of the solid to be created */
	while( args < 2 )  {
		(void)printf("Enter name of solid: ");
		argcnt = getcmd(args);
		/* Add any new args slurped up */
		args += argcnt;
	}
	if( lookup( cmd_args[1], LOOKUP_QUIET ) != DIR_NULL )  {
		(void)aexists( cmd_args[1] );
		return;
	}
	if( strlen(cmd_args[1]) >= NAMESIZE )  {
		(void)printf("ERROR, names are limited to %d characters\n", NAMESIZE-1);
		return;
	}
	/* Save the solid name since cmd_args[] might get bashed */
	NAMEMOVE( cmd_args[1], record.s.s_name );

	/* Make sure to note this is a solid record */
	record.s.s_id = ID_SOLID;

	/* Get the solid type to be created and make it */
	while( args < 3 )  {
		(void)printf("Enter solid type: ");
		argcnt = getcmd(args);
		/* Add any new args slurped up */
		args += argcnt;
	}
	/*
	 * Decide which solid to make and get the rest of the args
	 * make name <arb[4-8]|sph|ell|ellg|ell1|tor|tgc|tec|rec|trc|rcc|box|raw|rpp>
	 */
	if( strcmp( cmd_args[2], "arb8" ) == 0 )  {
		record.s.s_type = GENARB8;
		record.s.s_cgtype = ARB8;
		promp = &p_arb[0];		/* or promp = p_arb */
		if( arb_in( 8 ) != 0 )  {
			(void)printf("ERROR, arb8 not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "arb7" ) == 0 )  {
		record.s.s_type = GENARB8;
		record.s.s_cgtype = ARB7;
		promp = &p_arb[0];
		if( arb_in( 7 ) != 0 )  {
			(void)printf("ERROR, arb7 not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "arb6" ) == 0 )  {
		record.s.s_type = GENARB8;
		record.s.s_cgtype = ARB6;
		promp = &p_arb[0];
		if( arb_in( 6 ) != 0 )  {
			(void)printf("ERROR, arb6 not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "arb5" ) == 0 )  {
		record.s.s_type = GENARB8;
		record.s.s_cgtype = ARB5;
		promp = &p_arb[0];
		if( arb_in( 5 ) != 0 )  {
			(void)printf("ERROR, arb5 not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "arb4" ) == 0 )  {
		record.s.s_type = GENARB8;
		record.s.s_cgtype = ARB4;
		promp = &p_arb[0];
		if( arb_in( 4 ) != 0 )  {
			(void)printf("ERROR, arb4 not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "sph" ) == 0 )  {
		record.s.s_type = GENELL;
		record.s.s_cgtype = SPH;
		promp = &p_sph[0];
		if( sph_in() != 0 )  {
			(void)printf("ERROR, sphere not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "ell" ) == 0 )  {
		record.s.s_type = GENELL;
		record.s.s_cgtype = ELL;
		promp = &p_ell[0];
		if( ell_in( ELL ) != 0 )  {
			(void)printf("ERROR, ell not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "ellg" ) == 0 )  {
		record.s.s_type = GENELL;
		record.s.s_cgtype = ELLG;
		promp = &p_ell[0];
		if( ell_in( ELLG ) != 0 )  {
			(void)printf("ERROR, ellg not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "ell1" ) == 0 )  {
		record.s.s_type = GENELL;
		record.s.s_cgtype = ELL1;
		promp = &p_ell[0];
		if( ell_in( ELL1 ) != 0 )  {
			(void)printf("ERROR, ell1 not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "tor" ) == 0 )  {
		record.s.s_type = TOR;
		record.s.s_cgtype = TOR;
		promp = &p_tor[0];
		if( tor_in() != 0 )  {
			(void)printf("ERROR, tor not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "tgc" ) == 0 )  {
		record.s.s_type = GENTGC;
		record.s.s_cgtype = TGC;
		promp = &p_tgc[0];
		if( cyl_in( TGC ) != 0 )  {
			(void)printf("ERROR, tgc not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "tec" ) == 0 )  {
		record.s.s_type = GENTGC;
		record.s.s_cgtype = TEC;
		promp = &p_tgc[0];
		if( cyl_in( TEC ) != 0 )  {
			(void)printf("ERROR, tec not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "rec" ) == 0 )  {
		record.s.s_type = GENTGC;
		record.s.s_cgtype = REC;
		promp = &p_tgc[0];
		if( cyl_in( REC ) != 0 )  {
			(void)printf("ERROR, rec not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "trc" ) == 0 )  {
		record.s.s_type = GENTGC;
		record.s.s_cgtype = TRC;
		promp = &p_tgc[0];
		if( cyl_in( TRC ) != 0 )  {
			(void)printf("ERROR, trc not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "rcc" ) == 0 )  {
		record.s.s_type = GENTGC;
		record.s.s_cgtype = RCC;
		promp = &p_tgc[0];
		if( cyl_in( RCC ) != 0 )  {
			(void)printf("ERROR, rcc not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "box" ) == 0 )  {
		record.s.s_type = GENARB8;
		record.s.s_cgtype = BOX;
		promp = &p_box[0];
		if( box_in() != 0 )  {
			(void)printf("ERROR, box not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "raw" ) == 0 )  {
		record.s.s_type = GENARB8;
		record.s.s_cgtype = RAW;
		promp = &p_box[0];
		if( box_in() != 0 )  {
			(void)printf("ERROR, raw not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "rpp" ) == 0 )  {
		record.s.s_type = GENARB8;
		record.s.s_cgtype = RPP;
		promp = &p_rpp[0];
		if( rpp_in() != 0 )  {
			(void)printf("ERROR, rpp not made!\n");
			return;
		}
	} else if( strcmp( cmd_args[2], "ars" ) == 0 )  {
		(void)printf("typein ars not implimented yet\n");
		return;
	} else {
		(void)printf("f_in:  %s is not a known primitive\n", cmd_args[2]);
		return;
	}

	/* Zero out record.s.s_values[] */
	for( i = 0; i < 24; i++ )  {
		record.s.s_values[i] = 0.0;
	}

	/* Convert and copy cmd_args[] to record.s.s_values[] */
	for( i = 0; i < vals; i++ )  {
		record.s.s_values[i] = atof(cmd_args[3+i])*local2base;
	}

	/* Convert to GED notion of database */
	if( cvt_ged( &record.s ) )  {
		return;
	}

	/* don't allow interrupts while we update the database! */
	(void)signal( SIGINT, SIG_IGN);
 
	/* Add to in-core directory */
	if( (dp = dir_add( record.s.s_name, -1, DIR_SOLID, 0 )) == DIR_NULL )
		return;
	db_alloc( dp, 1 );

	db_putrec( dp, &record, 0 );
	/* draw the "typed-in" solid */
	drawHobj(dp, ROOT, 0, identity);
	dmaflag = 1;
}

/*	A R B _ I N ( ) :    	reads arb parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
arb_in( numpts )
int numpts;
{
	if( numpts > 8 || numpts < 4 )  {
		(void)printf("ERROR arb_in: numpts out of range!\n");
		return(1);	/* failure */
	}

	while( args < (3 + (3*numpts)) )  {
		(void)printf("%s", promp[args-3] );
		if( (argcnt = getcmd(args)) < 0 )  {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	vals = 24;
	return(0);	/* success */
}

/*   S P H _ I N ( ) :   	reads sphere parameters from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
sph_in()
{
	/* Read vertex and radius */
	while( args < (3 + (3*1 + 1)) )  {
		(void)printf("%s", promp[args-3]);
		if( (argcnt = getcmd(args)) < 0 )  {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	/* Check for zero radius */
	if( atof(cmd_args[6]) <= 0.0 )  {
		(void)printf("ERROR, radius must be greater than zero!\n");
		return(1);	/* failure */
	}
	vals = 4;
	return(0);	/* success */
}

/*	E L L _ I N ( ) :	reads parameters for ells
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
ell_in( type )
int type;
{
	if( type == ELL )  {
		while( args < (3 + 7) )  {
			(void)printf("%s", promp[args-3+13]);	/* Lookout! */
			if( (argcnt = getcmd(args)) < 0 )  {
				return(1);	/* failure */
			}
			args += argcnt;
		}
		/* Check for zero axis length */
		if( atof(cmd_args[9]) <= 0.0 )  {
			(void)printf("ERROR, length must be greater than zero!\n");
			return(1);	/* failure */
		}
		vals = 7;
		return(0);	/* success */
	}

	/* Have ELL1 or ELLG.  Get Vertex(X, Y, Z)  and vector A(X, Y, Z) */
	while( args < (3 + 6) )  {
		(void)printf("%s", promp[args-3]);
		if( (argcnt = getcmd(args)) < 0 )  {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	/* Check for zero length vector A */
	if( checkv(6) )  {
		return(1);	/* failure */
	}
	if( type == ELL1 )  {
		while( args < (3 + 7) )  {
			(void)printf("%s", promp[12]);		/* Lookout! */
			if( (argcnt = getcmd(args)) < 0 )  {
				return(1);	/* failure */
			}
			args += argcnt;
		}
		/* Check for zero radius */
		if( atof(cmd_args[9]) <= 0.0 )  {
			(void)printf("ERROR, radius must be greater than zero!\n");
			return(1);	/* failure */
		}
		vals = 7;
		return(0);	/* success */
	}

	/* Should have ELLG.  Get vector B and C */
	if( type == ELLG )  {
		while( args < (3 + 12) )  {
			(void)printf("%s", promp[args-3]);
			if( (argcnt = getcmd(args)) < 0 )  {
				return(1);	/* failure */
			}
			args += argcnt;
		}
		/* Check for zero length vector B */
		if( checkv(9) )  {
			return(1);	/* failure */
		}
		/* Check for zero length vector C */
		if( checkv(12) )  {
			return(1);	/* failure */
		}
		vals = 12;
		return(0);	/* success */
	}

	/* Protect ourselves */
	(void)printf("ERROR ell_in(): uknown type of GENELL\n");
	return(1);	/* failure */
}

/*	T O R _ I N ( ) :	gets parameters of torus from keyboard
 *				returns 0 if successful read
 *					1 if unsuccessful read
 */
int
tor_in()
{
	float rad1, rad2;

	while( args < (3 + 8) )  {
		(void)printf("%s", promp[args-3]);
		if( (argcnt = getcmd(args)) < 0 )  {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	/* Check for zero length normal vector */
	if( checkv(6) )  {
		return(1);	/* failure */
	}
	/* Check for zero radius 1 */
	if( (rad1 = atof(cmd_args[9])) <= 0.0 )  {
		(void)printf("ERROR, radius 1 must be greater than zero!\n");
		return(1);	/* failure */
	}
	/* Check for zero radius 2 */
	if( (rad1 = atof(cmd_args[10])) <= 0.0 )  {
		(void)printf("ERROR, radius 2 must be greater than zero!\n");
		return(1);	/* failure */
	}
	/* Check for radius 2 >= radius 1 */
	if( rad1 <= rad2 )  {
		(void)printf("ERROR, radius 2 >= radius 1 ....\n");
		return(1);	/* failure */
	}
	vals = 8;
	return(0);	/* success */
}

/*   C Y L _ I N ( ) :		reads parameters for all cylinders 
 */
int
cyl_in( type )
int type;
{
	float rad1, rad2;

	/* Get vertex and height vectors */
	while( args < (3 + 6) )  {
		(void)printf("%s", promp[args-3]);
		if( (argcnt = getcmd(args)) < 0 )  {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	/* Check for zero length height vector */
	if( checkv(6) )  {
		return(1);	/* failure */
	}

	if( type == RCC )  {
		/* Get radius */
		while( args < (3 + 7) )  {
			(void)printf("%s", promp[14]);		/* Lookout! */
			if( (argcnt = getcmd(args)) < 0 )  {
				return(1);	/* failure */
			}
			args += argcnt;
		}
		/* Check for zero radius */
		if( (rad1 = atof(cmd_args[9])) <= 0.0 )  {
			(void)printf("ERROR, radius must be greater than zero!\n");
			return(1);	/* failure */
		}
		vals = 7;
		return(0);	/* success */
	}

	if( type == TRC )  {
		/* Get radius of base and top */
		while( args < (3 + 7) )  {
			(void)printf("%s", promp[15]);		/* Lookout! */
			if( (argcnt = getcmd(args)) < 0 )  {
				return(1);	/* failure */
			}
			args += argcnt;
		}
		/* Check for zero top radius */
		if( (rad1 = atof(cmd_args[9])) <= 0.0 )  {
			(void)printf("ERROR, radius must be greater than zero!\n");
			return(1);	/* failure */
		}
		while( args < (3 + 8) )  {
			(void)printf("%s", promp[16]);		/* Lookout! */
			if( (argcnt = getcmd(args)) < 0 )  {
				return(1);	/* failure */
			}
			args += argcnt;
		}
		/* Check for zero bottom radius */
		if( (rad2 = atof(cmd_args[10])) <= 0.0 )  {
			(void)printf("ERROR, radius must be greater than zero!\n");
			return(1);	/* failure */
		}
		vals = 8;
		return(0);	/* success */
	}

	/*
	 * Must have REC, TEC, or TGC
	 * Get A and B vectors
	 */
	while( args < (3 + 12) )  {
		(void)printf("%s", promp[args-3]);
		if( (argcnt = getcmd(args)) < 0 )  {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	/* Check for zero length A vector */
	if( checkv(9) )  {
		return(1);	/* failure */
	}
	/* Check for zero length B vector */
	if( checkv(12) )  {
		return(1);	/* failure */
	}

	if( type == REC )  {
		vals = 12;
		return(0);	/* success */
	}

	if( type == TEC )  {
		while( args < (3 + 13) )  {
			(void)printf("%s", promp[17]);		/* Lookout! */
			if( (argcnt = getcmd(args)) < 0 )  {
				return(1);	/* failure */
			}
			args += argcnt;
		}
		/* Check for ratio greater than 1.0 */
		if( (rad1 = atof(cmd_args[15])) < 1.0 )  {
			(void)printf("ERROR, ratio must be greater than one!\n");
			return(1);	/* failure */
		}
		vals = 13;
		return(0);	/* success */
	}

	if( type == TGC )  {
		while( args < (3 + 14) )  {
			(void)printf("%s", promp[args-3]);	/* Lookout! */
			if( (argcnt = getcmd(args)) < 0 )  {
				return(1);	/* failure */
			}
			args += argcnt;
		}
		/* Check for zero radius */
		if( (rad1 = atof(cmd_args[15])) <= 0.0 )  {
			(void)printf("ERROR, must be greater than zero!\n");
			return(1);	/* failure */
		}
		/* Check for zero radius */
		if( (rad1 = atof(cmd_args[16])) <= 0.0 )  {
			(void)printf("ERROR, must be greater than zero!\n");
			return(1);	/* failure */
		}
		vals = 14;
		return(0);	/* success */
	}

	/* Protect ourselves */
	(void)printf("ERROR cyl_in(): uknown type of GENTGC\n");
	return(1);	/* failure */
}

/*   B O X _ I N ( ) :		reads parameters for BOX and RAW
 *				returns 0 if successful read
 *				        1 if unsuccessful read
 */
int
box_in()
{
	while( args < (3 + 12) )  {
		(void)printf("%s", promp[args-3]);
		if( (argcnt = getcmd(args)) < 0 )  {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	/* Check for zero H vector */
	if( checkv(6) )  {
		return(1);	/* failure */
	}
	/* Check for zero W vector */
	if( checkv(9) )  {
		return(1);	/* failure */
	}
	/* Check for zero D vector */
	if( checkv(12) )  {
		return(1);	/* failure */
	}
	vals = 12;
	return(0);	/* success */
}

/*   R P P _ I N ( ) :		reads parameters for RPP
 *				returns 0 if successful read
 *				        1 if unsuccessful read
 */
int
rpp_in()
{
	while( args < (3 + 6) )  {
		(void)printf("%s", promp[args-3]);
		if( (argcnt = getcmd(args)) < 0 )  {
			return(1);	/* failure */
		}
		args += argcnt;
	}
	/* Check input */
	if( atof(cmd_args[3]) >= atof(cmd_args[4]) )  {
		(void)printf("ERROR, XMIN greater than XMAX!\n");
		return(1);	/* failure */
	}
	if( atof(cmd_args[5]) >= atof(cmd_args[6]) )  {
		(void)printf("ERROR, YMIN greater than YMAX!\n");
		return(1);	/* failure */
	}
	if( atof(cmd_args[7]) >= atof(cmd_args[8]) )  {
		(void)printf("ERROR, ZMIN greater than ZMAX!\n");
		return(1);	/* failure */
	}
	vals = 6;
	return(0);	/* success */
}

/*   C H E C K V ( ) :		checks for zero vector at cmd_args[loc]
 *				returns 0 if vector non-zero
 *				       -1 if vector is zero
 */
int
checkv( loc )
int loc;
{
	int i;
	float work[3];

	for(i=0; i<3; i++) {
		work[i] = atof(cmd_args[(loc+i)]);
	}
	if( MAGNITUDE(work) == 0.0 ) {
		(void)printf("ERROR, zero vector ....\n");
		return(-1);	/* zero vector */
	}
	return(0);	/* vector is non-zero */
}

/*   G E T C M D ( ) :	gets keyboard input command lines, parses and
 *			saves pointers to the beginning of each element
 *			starting at cmd_args[pos] and returns:
 *				the number of args	if successful
 *						-1	if unsuccessful
 */
int
getcmd(pos)
int pos;
{
#define MAXARGS		200	/* Maximum number of args per line */
	register char *lp;
	register char *lp1;
	static char line[MAXLINE];

	newargs = 0;
	/*
	 * Now we go to the last argument string read and then position
	 * ourselves at the end of the string.  This is IMPORTANT so we
	 * don't overwrite what we've already read into line[].
	 */
	lp = cmd_args[pos-1];		/* Beginning of last arg string */
	while( *lp++ != '\0' )  {	/* Get positioned at end of string */
		;
	}

	/* Read input line */
	(void)fgets( lp, MAXLINE, stdin );

	/* Check for Control-D (EOF) */
	if( feof( stdin ) )  {
		/* Control-D typed, let's hit the road */
		(void)f_quit();
		/* NOTREACHED */
	}

	cmd_args[newargs + pos] = lp;

	if( *lp == '\n' )
		return(0);		/* NOP */

	/* In case first character is not "white space" */
	if( (*lp != ' ') && (*lp != '\t') && (*lp != '\0') )
		newargs++;		/* holds # of args */

	for( ; *lp != '\0'; lp++ )  {
		if( (*lp == ' ') || (*lp == '\t') || (*lp == '\n') )  {
			*lp = '\0';
			lp1 = lp + 1;
			if( (*lp1 != ' ') && (*lp1 != '\t') &&
			    (*lp1 != '\n') && (*lp1 != '\0') )  {
				if( (newargs + pos) >= MAXARGS )  {
					(void)printf("More than %d arguments, excess flushed\n", MAXARGS);
					cmd_args[MAXARGS] = (char *)0;
					return(MAXARGS - pos);
				}
				cmd_args[newargs + pos] = lp1;
			    	newargs++;
			}
		}
		/* Finally, a non-space char */
	}
	/* Null terminate pointer array */
	cmd_args[newargs + pos] = (char *)0;
	return(newargs);
}

/*   C V T _ G E D ( ) :	converts solid parameters to GED format
 *				Adapted from convert.c of CVT
 *				returns		-1 if not successful
 *						 0 if successful
 */

#define Xmin	iv[0]
#define Xmax	iv[1]
#define Ymin	iv[2]
#define Ymax	iv[3]
#define Zmin	iv[4]
#define Zmax	iv[5]

/*
 * Input Vector Fields
 */
#define Fi	iv+(i-1)*3
#define F1	iv+(1-1)*3
#define F2	iv+(2-1)*3
#define F3	iv+(3-1)*3
#define F4	iv+(4-1)*3
#define F5	iv+(5-1)*3
#define F6	iv+(6-1)*3
#define F7	iv+(7-1)*3
#define F8	iv+(8-1)*3
/*
 * Output vector fields
 */
#define Oi	ov+(i-1)*3
#define O1	ov+(1-1)*3
#define O2	ov+(2-1)*3
#define O3	ov+(3-1)*3
#define O4	ov+(4-1)*3
#define O5	ov+(5-1)*3
#define O6	ov+(6-1)*3
#define O7	ov+(7-1)*3
#define O8	ov+(8-1)*3
#define O9	ov+(9-1)*3
#define O10	ov+(10-1)*3
#define O11	ov+(11-1)*3
#define O12	ov+(12-1)*3
#define O13	ov+(13-1)*3
#define O14	ov+(14-1)*3
#define O15	ov+(15-1)*3
#define O16	ov+(16-1)*3

int
cvt_ged( in )
struct solidrec *in;
{
	static struct solidrec out;
	register float *iv;
	register float *ov;
	register int i;
	float r1, r2, r3, r4;
	float work[3];
	float m1, m2, m3;
	float m5, m6;
	short cgtype;
	static float pi = 3.14159265358979323264;

	/* Get positioned at s_values[0] to begin conversion */
	iv = &in->s_values[0];
	ov = &out.s_values[0];
	cgtype = in->s_cgtype;

	switch( cgtype )  {

	case RPP:
		VSET( O1, Xmax, Ymin, Zmin );
		VSET( O2, Xmax, Ymax, Zmin );
		VSET( O3, Xmax, Ymax, Zmax );
		VSET( O4, Xmax, Ymin, Zmax );
		VSET( O5, Xmin, Ymin, Zmin );
		VSET( O6, Xmin, Ymax, Zmin );
		VSET( O7, Xmin, Ymax, Zmax );
		VSET( O8, Xmin, Ymin, Zmax );
		goto ccommon;

	case BOX:
		VMOVE( O1, F1 );
		VADD2( O2, F1, F3 );
		VADD3( O3, F1, F3, F2 );
		VADD2( O4, F1, F2 );
		VADD2( O5, F1, F4 );
		VADD3( O6, F1, F4, F3 );
		VADD4( O7, F1, F4, F3, F2 );
		VADD3( O8, F1, F4, F2 );
		goto ccommon;

	case RAW:
		VMOVE( O1, F1 );
		VADD2( O2, F1, F3 );
		VMOVE( O3, O2 );
		VADD2( O4, F1, F2 );
		VADD2( O5, F1, F4 );
		VADD3( O6, F1, F4, F3 );
		VMOVE( O7, O6 );
		VADD3( O8, F1, F4, F2 );
	ccommon:
		VMOVE( F1, O1 );
		for( i=2; i<=8; i++ )  {
			VSUB2( Fi, Oi, O1 );
		}
		return(0);

	case ARB8:
	arbcommon:
		for( i=2; i<=8; i++ )  {
			VSUB2( Fi, Fi, F1 );
		}
		return(0);

	case ARB7:
		VMOVE( F8, F5 );
		goto arbcommon;

	case ARB6:
		/* NOTE: the ordering is important, as data is in F5, F6 */
		VMOVE( F8, F6 );
		VMOVE( F7, F6 );
		VMOVE( F6, F5 );
		goto arbcommon;

	case ARB5:
		VMOVE( F6, F5 );
		VMOVE( F7, F5 );
		VMOVE( F8, F5 );
		goto arbcommon;

	case ARB4:
		/* Order is important, data is in F4 */
		VMOVE( F8, F4 );
		VMOVE( F7, F4 );
		VMOVE( F6, F4 );
		VMOVE( F5, F4 );
		VMOVE( F4, F1 );
		goto arbcommon;
	case RCC:
		r1 = iv[6];	/* Radius */
		r2 = iv[6];
		goto trccommon;		/* sorry */

	case REC:
		VMOVE( F5, F3 );
		VMOVE( F6, F4 );
		return(0);

		/*
		 * For the TRC, if the V vector (F1) is of zero length,
		 * a divide by zero will occur when scaling by the magnitude.
		 * We add the vector [pi, pi, pi] to V to produce a unique
		 * (and most likely non-zero) resultant vector.  This will
		 * do nicely for purposes of cross-product.
		 * THIS DOES NOT GO OUT INTO THE FILE!!
		 * work[] must NOT be colinear with F2[].  We check for this
		 * later.
		 */

	case TRC:
		r1 = iv[6];	/* Radius 1 */
		r2 = iv[7];	/* Radius 2 */
	trccommon:
		VMOVE( work, F1 );
		work[0] += pi;
		work[1] += pi;
		work[2] += pi;
		VCROSS( F3, work, F2 );
		m1 = MAGNITUDE( F3 );
		if( m1 == 0.0 )  {
			work[1] = 0.0;		/* Vector is colinear, so */
			work[2] = 0.0;		/* make it different */
			VCROSS( F3, work, F2 );
			m1 = MAGNITUDE( F3 );
			if( m1 == 0.0 )  {
				(void)printf("ERROR, magnitude is zero!\n");
				return(-1);
			}
		}
		VSCALE( F3, F3, r1/m1 );

		VCROSS( F4, F2, F3 );
		m2 = MAGNITUDE( F4 );
		if( m2 == 0.0 )  {
			(void)printf("ERROR, magnitude is zero!\n");
			return(-1);
		}
		VSCALE( F4, F4, r1/m2 );

		if( r1 == 0.0 )  {
			(void)printf("ERROR, magnitude is zero!\n");
			return(-1);
		}
		VSCALE( F5, F3, r2/r1 );
		VSCALE( F6, F4, r2/r1 );
		return(0);

	case TEC:
		r1 = iv[12];	/* P */
		if( r1 == 0.0 )  {
			(void)printf("ERROR, magnitude is zero!\n");
			return(-1);
		}
		VSCALE( F5, F3, (1.0/r1) );
		VSCALE( F6, F4, (1.0/r1) );
		return(0);

	case TGC:
		/* This should have been checked earlier but we'll check */
		if( (MAGNITUDE( F3 ) == 0.0) || (MAGNITUDE( F4) == 0.0) )  {
			(void)printf("ERROR, magnitude is zero!\n");
			return(-1);
		}
		r1 = iv[12] / MAGNITUDE( F3 );	/* A/|A| * C */
		r2 = iv[13] / MAGNITUDE( F4 );	/* B/|B| * D */
		VSCALE( F5, F3, r1 );
		VSCALE( F6, F4, r2 );
		return(0);

	case SPH:
		/* SPH format is V, r */
		r1 = iv[3];		/* Radius */
		VSET( F2, r1,  0,  0 );
		VSET( F3,  0, r1,  0 );
		VSET( F4,  0,  0, r1 );
		return(0);

	case ELL:
		/*
		 * For simplicity, an ELL is converted to an ELL1, then
		 * falls through to the ELL1 code.
		 * ELL format is F1, F2, l and ELL1 format is V, A, r
		 */
		r1 = iv[6];		/* Length */
		VADD2( O1, F1, F2 );
		VSCALE( O1, O1, 0.5 );	/* O1 holds V */

		VSUB2( O3, F2, F1 );	/* O3 holds F2 - F1 */
		m1 = MAGNITUDE(O3);
		/* XXX check this later */
		if( m1 == 0.0 )  {
			(void)printf("ERROR, magnitude is zero!\n");
			return(-1);
		}
		r2 = 0.5 * r1 / m1;
		VSCALE( O2, O3, r2 );	/* O2 holds A */

		iv[6] = sqrt( MAGSQ( O2 ) - (m1 * 0.5)*(m1 * 0.5) );	/* r */
		VMOVE( F1, O1 );	/* Move V */
		VMOVE( F2, O2 );	/* Move A */
		/* fall through */

	case ELL1:
		/* GENELL format is V, A, B, C */
		r1 = iv[6];		/* Radius */

		/*
		 * To allow for V being (0,0,0), for VCROSS purposes only,
		 * we add (pi,pi,pi).  THIS DOES NOT GO OUT INTO THE FILE!!
		 * work[] must NOT be colinear with F2[].  We check for this
		 * later.
		 */
		VMOVE( work, F1 );
		work[0] += pi;
		work[1] += pi;
		work[2] += pi;

		VCROSS( F3, work, F2 );
		m1 = MAGNITUDE( F3 );
		if( m1 == 0.0 )  {
			work[1] = 0.0;		/* Vector is colinear, so */
			work[2] = 0.0;		/* make it different */
			VCROSS( F3, work, F2 );
			m1 = MAGNITUDE( F3 );
			if( m1 == 0.0 )  {
				(void)printf("ERROR, magnitude is zero!\n");
				return(-1);
			}
		}
		VSCALE( F3, F3, r1/m1 );

		VCROSS( F4, F2, F3 );
		m2 = MAGNITUDE( F4 );
		if( m2 == 0.0 )  {
			(void)printf("ERROR, magnitude is zero!\n");
			return(-1);
		}
		VSCALE( F4, F4, r1/m2 );
		return(0);

	case ELLG:
		/* Everything is already okay.  ELLG format is V, A, B, C */
		return(0);

	case TOR:
		/* TOR format is V, N, r1, r2 */
		r1=iv[6];	/* Dist from end of V to center of (solid portion) of TORUS */
		r2=iv[7];	/* Radius of solid portion of TORUS */
		r3=r1-r2;	/* Radius to inner circular edge */
		r4=r1+r2;	/* Radius to outer circular edge */

		/*
		 * To allow for V being (0,0,0), for VCROSS purposes only,
		 * we add (pi,pi,pi).  THIS DOES NOT GO OUT INTO THE FILE!!
		 * work[] must NOT be colinear with N[].  We check for this
		 * later.
		 */
		VMOVE(work,F1);
		work[0] +=pi;
		work[1] +=pi;
		work[2] +=pi;

		m2 = MAGNITUDE( F2 );	/* F2 is NORMAL to Torus, with Radius length */
		if( m2 == 0.0 )  {
			(void)printf("ERROR, normal magnitude is zero!\n");
			return(-1);
		}
		VSCALE( F2, F2, r2/m2 );

		/* F3, F4 are perpendicular, goto center of Torus (solid part), for top/bottom */
		VCROSS(F3,work,F2);
		m1=MAGNITUDE(F3);
		if( m1 == 0.0 )  {
			work[1] = 0.0;		/* Vector is colinear, so */
			work[2] = 0.0;		/* make it different */
			VCROSS(F3,work,F2);
			m1=MAGNITUDE(F3);
			if( m1 == 0.0 )  {
				(void)printf("ERROR, cross product vector is zero!\n");
				return(-1);
			}
		}
		VSCALE(F3,F3,r1/m1);

		VCROSS(F4,F3,F2);
		m3=MAGNITUDE(F4);
		if( m3 == 0.0 )  {
			(void)printf("ERROR, magnitude is zero!\n");
			return(-1);
		}

		VSCALE(F4,F4,r1/m3);

		m5 = MAGNITUDE(F3);
		m6 = MAGNITUDE( F4 );
		if( m5 == 0.0 || m6 == 0.0 )  {
			(void)printf("ERROR, magnitude is zero!\n");
			return(-1);
		}

		/* F5, F6 are perpindicular, goto inner edge of ellipse */
		VSCALE( F5, F3, r3/m5 );
		VSCALE( F6, F4, r3/m6 );

		/* F7, F8 are perpendicular, goto outer edge of ellipse */
		VSCALE( F7, F3, r4/m5 );
		VSCALE( F8, F4, r4/m6 );
 
		return(0);

	default:
		(void)printf("cvt_ged(): unknown solid type\n");
		return(-1);
	}
}
