/*
 *			B U T T O N S . C
 *
 * Functions -
 *	buttons		Process button-box functions
 *	press		button function request
 *	state_err	state error printer
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

#include <math.h>
#include "./machine.h"	/* special copy */
#include "vmath.h"
#include "db.h"
#include "./ged.h"
#include "./objdir.h"
#include "./solid.h"
#include "./menu.h"
#include "./dm.h"
#include "./sedit.h"

extern void	perror();
extern int	printf(), read();
extern long	lseek();

int	adcflag;	/* angle/distance cursor in use */

/* This flag indicates that SOLID editing is in effect.
 * edobj may not be set at the same time.
 * It is set to the 0 if off, or the value of the button function
 * that is currently in effect (eg, BE_S_SCALE).
 */
static	edsol;

/* This flag indicates that OBJECT editing is in effect.
 * edsol may not be set at the same time.
 * Value is 0 if off, or the value of the button function currently
 * in effect (eg, BE_O_XY).
 */
int	edobj;		/* object editing */
int	movedir;	/* RARROW | UARROW | SARROW | ROTARROW */

/*
 * The "accumulation" solid rotation matrix and scale factor
 */
mat_t	acc_rot_sol;
float	acc_sc_sol;
float	acc_sc[3];	/* local object scale factors --- accumulations */

static void bv_top(), bv_bottom(), bv_right();
static void bv_left(), bv_front(), bv_rear();
static void bv_vrestore(), bv_vsave(), bv_adcursor(), bv_reset();
static void bv_90_90(), bv_35_25();
static void be_o_illuminate(), be_s_illuminate();
static void be_o_scale(), be_o_x(), be_o_y(), be_o_xy(), be_o_rotate();
static void be_accept(), be_reject(), bv_slicemode();
static void be_s_edit(), be_s_rotate(), be_s_trans(), be_s_scale();
static void be_o_xscale(), be_o_yscale(), be_o_zscale();

struct buttons  {
	int	bu_code;	/* dm_values.dv_button */
	char	*bu_name;	/* keyboard string */
	void	(*bu_func)();	/* function to call */
}  button_table[] = {
	BV_TOP,		"top",		bv_top,
	BV_BOTTOM,	"bottom",	bv_bottom,
	BV_RIGHT,	"right",	bv_right,
	BV_LEFT,	"left",		bv_left,
	BV_FRONT,	"front",	bv_front,
	BV_REAR,	"rear",		bv_rear,
	BV_VRESTORE,	"restore",	bv_vrestore,
	BV_VSAVE,	"save",		bv_vsave,
	BV_ADCURSOR,	"adc",		bv_adcursor,
	BV_RESET,	"reset",	bv_reset,
	BV_90_90,	"90,90",	bv_90_90,
	BV_35_25,	"35,25",	bv_35_25,
	BE_O_ILLUMINATE,"oill",		be_o_illuminate,
	BE_S_ILLUMINATE,"sill",		be_s_illuminate,
	BE_O_SCALE,	"oscale",	be_o_scale,
	BE_O_XSCALE,	"oxscale",	be_o_xscale,
	BE_O_YSCALE,	"oyscale",	be_o_yscale,
	BE_O_ZSCALE,	"ozscale",	be_o_zscale,
	BE_O_X,		"ox",		be_o_x,
	BE_O_Y,		"oy",		be_o_y,
	BE_O_XY,	"oxy",		be_o_xy,
	BE_O_ROTATE,	"orot",		be_o_rotate,
	BE_ACCEPT,	"accept",	be_accept,
	BE_REJECT,	"reject",	be_reject,
	BV_SLICEMODE,	"slice",	bv_slicemode,
	BE_S_EDIT,	"sedit",	be_s_edit,
	BE_S_ROTATE,	"srot",		be_s_rotate,
	BE_S_TRANS,	"sxy",		be_s_trans,
	BE_S_SCALE,	"sscale",	be_s_scale,
	-1,		"-end-",	be_reject
};

static mat_t sav_viewrot, sav_toviewcenter;
static float sav_viewscale;
static int	vsaved = 0;	/* set iff view saved */

void button_menu(), button_hit_menu();

static struct menu_item first_menu[] = {
	{ "BUTTON MENU", button_menu, 1 },		/* chg to 2nd menu */
	{ "", (void (*)())NULL, 0 }
};
static struct menu_item second_menu[] = {
	{ "***BUTTON MENU***", button_menu, 0 },	/* chg to 1st menu */
	{ "REJECT Edit", button_hit_menu, BE_REJECT },
	{ "ACCEPT Edit", button_hit_menu, BE_ACCEPT },
	{ "35,25", button_hit_menu, BV_35_25 },
	{ "Top", button_hit_menu, BV_TOP },
	{ "Right", button_hit_menu, BV_RIGHT },
	{ "Front", button_hit_menu, BV_FRONT },
	{ "90,90", button_hit_menu, BV_90_90 },
	{ "Restore View", button_hit_menu, BV_VRESTORE },
	{ "Save View", button_hit_menu, BV_VSAVE },
	{ "Angle/Dist Cursor", button_hit_menu, BV_ADCURSOR },
	{ "Reset Viewsize", button_hit_menu, BV_RESET },
	{ "Solid Illum", button_hit_menu, BE_S_ILLUMINATE },
	{ "Object Illum", button_hit_menu, BE_O_ILLUMINATE },
	{ "", (void (*)())NULL, 0 }
};
static struct menu_item sed_menu[] = {
	{ "***SOLID EDIT***", button_menu, 2 },		/* turn off menu */
	{ "edit menu", button_hit_menu, BE_S_EDIT },
	{ "Rotate", button_hit_menu, BE_S_ROTATE },
	{ "Translate", button_hit_menu, BE_S_TRANS },
	{ "Scale", button_hit_menu, BE_S_SCALE },
	{ "", (void (*)())NULL, 0 }
};
static struct menu_item oed_menu[] = {
	{ "***OBJ EDIT***", button_menu, 2 },		/* turn off menu */
	{ "Scale", button_hit_menu, BE_O_SCALE },
	{ "X move", button_hit_menu, BE_O_X },
	{ "Y move", button_hit_menu, BE_O_Y },
	{ "XY move", button_hit_menu, BE_O_XY },
	{ "Rotate", button_hit_menu, BE_O_ROTATE },
	{ "Scale X", button_hit_menu, BE_O_XSCALE },
	{ "Scale Y", button_hit_menu, BE_O_YSCALE },
	{ "Scale Z", button_hit_menu, BE_O_ZSCALE },
	{ "", (void (*)())NULL, 0 }
};

/*
 *			B U T T O N
 */
void
button( bnum )
register int bnum;
{
	register struct buttons *bp;

	if( edsol && edobj )
		(void)printf("WARNING: State error: edsol=%x, edobj=%x\n", edsol, edobj );

	/* Process the button function requested. */
	for( bp = button_table; bp->bu_code >= 0; bp++ )  {
		if( bnum != bp->bu_code )
			continue;
		bp->bu_func();
		return;
	}
	(void)printf("button(%d):  Not a defined operation\n", bnum);
}

/*
 *  			P R E S S
 */
void
press( str )
char *str;{
	register struct buttons *bp;

	if( edsol && edobj )
		(void)printf("WARNING: State error: edsol=%x, edobj=%x\n", edsol, edobj );

	if( str[0] == '?' )  {
		register int i=0;
		for( bp = button_table; bp->bu_code >= 0; bp++ )  {
			(void)printf("%s, ", bp->bu_name );
			if( ++i > 4 )  {
				(void)putchar('\n');
				i = 0;
			}
		}
		return;
	}

	/* Process the button function requested. */
	for( bp = button_table; bp->bu_code >= 0; bp++ )  {
		if( strcmp( str, bp->bu_name) )
			continue;
		bp->bu_func();
		return;
	}
	(void)printf("press(%s):  Unknown operation, type 'press ?' for help\n",
		str);
}
/*
 *  			L A B E L _ B U T T O N
 *  
 *  For a given GED button number, return the "press" ID string.
 *  Useful for displays with programable button lables, etc.
 */
char *
label_button(bnum)
{
	register struct buttons *bp;

	/* Process the button function requested. */
	for( bp = button_table; bp->bu_code >= 0; bp++ )  {
		if( bnum != bp->bu_code )
			continue;
		return( bp->bu_name );
	}
	(void)printf("label_button(%d):  Not a defined operation\n", bnum);
	return("");
}

static void bv_top()  {
	/* Top view */
	setview( 0, 0, 0 );
}

static void bv_bottom()  {
	/* Bottom view */
	setview( 180, 0, 0 );
}

static void bv_right()  {
	/* Right view */
	setview( 270, 0, 0 );
}

static void bv_left()  {
	/* Left view */
	setview( 270, 0, 180 );
}

static void bv_front()  {
	/* Front view */
	setview( 270, 0, 270 );
}

static void bv_rear()  {
	/* Rear view */
	setview( 270, 0, 90 );
}

static void bv_vrestore()  {
	/* restore to saved view */
	if ( vsaved )  {
		Viewscale = sav_viewscale;
		mat_copy( Viewrot, sav_viewrot );
		mat_copy( toViewcenter, sav_toviewcenter );
		new_mats();
		dmaflag++;
	}
}

static void bv_vsave()  {
	/* save current view */
	sav_viewscale = Viewscale;
	mat_copy( sav_viewrot, Viewrot );
	mat_copy( sav_toviewcenter, toViewcenter );
	vsaved = 1;
	dmp->dmr_light( LIGHT_ON, BV_VRESTORE );
}

static void bv_adcursor()  {
	if (adcflag)  {
		/* Was on, turn off */
		adcflag = 0;
		dmp->dmr_light( LIGHT_OFF, BV_ADCURSOR );
	}  else  {
		/* Was off, turn on */
		adcflag = 1;
		dmp->dmr_light( LIGHT_ON, BV_ADCURSOR );
	}
	dmaflag = 1;
}

static void bv_reset()  {
	/* Reset to initial viewing conditions */
	mat_idn( toViewcenter );
	Viewscale = maxview;
	setview( 0, 0, 0 );
}

static void bv_90_90()  {
	/* azm 90   elev 90 */
	setview( 360, 0, 180 );
}

static void bv_35_25()  {
	/* Use Azmuth=35, Elevation=25 in Keith's backwards space */
	setview( 295, 0, 235 );
}

/* returns 0 if error, !0 if success */
static int ill_common()  {
	/* Common part of illumination */
	dmp->dmr_light( LIGHT_ON, BE_REJECT );
	if( HeadSolid.s_forw == &HeadSolid )  {
		(void)printf("no solids in view\n");
		return(0);	/* BAD */
	}
	illump = HeadSolid.s_forw;/* any valid solid would do */
	edobj = 0;		/* sanity */
	edsol = 0;		/* sanity */
	movedir = 0;		/* No edit modes set */
	mat_idn( modelchanges );	/* No changes yet */
	dmaflag++;
	return(1);		/* OK */
}

static void be_o_illuminate()  {
	if( not_state( ST_VIEW, "Object Illuminate" ) )
		return;

	dmp->dmr_light( LIGHT_ON, BE_O_ILLUMINATE );
	if( ill_common() )  {
		(void)chg_state( ST_VIEW, ST_O_PICK, "Object Illuminate" );
		new_mats();
	}
	/* reset accumulation local scale factors */
	acc_sc[0] = acc_sc[1] = acc_sc[2] = 1.0;
}

static void be_s_illuminate()  {
	if( not_state( ST_VIEW, "Solid Illuminate" ) )
		return;

	dmp->dmr_light( LIGHT_ON, BE_S_ILLUMINATE );
	if( ill_common() )  {
		(void)chg_state( ST_VIEW, ST_S_PICK, "Solid Illuminate" );
		new_mats();
	}
}

static void be_o_scale()  {
	if( not_state( ST_O_EDIT, "Object Scale" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edobj );
	dmp->dmr_light( LIGHT_ON, edobj = BE_O_SCALE );
	movedir = SARROW;
	dmaflag++;
}

static void be_o_xscale()  {
	if( not_state( ST_O_EDIT, "Object Local X Scale" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edobj );
	dmp->dmr_light( LIGHT_ON, edobj = BE_O_XSCALE );
	movedir = SARROW;
	dmaflag++;
}

static void be_o_yscale()  {
	if( not_state( ST_O_EDIT, "Object Local Y Scale" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edobj );
	dmp->dmr_light( LIGHT_ON, edobj = BE_O_YSCALE );
	movedir = SARROW;
	dmaflag++;
}

static void be_o_zscale()  {
	if( not_state( ST_O_EDIT, "Object Local Z Scale" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edobj );
	dmp->dmr_light( LIGHT_ON, edobj = BE_O_ZSCALE );
	movedir = SARROW;
	dmaflag++;
}

static void be_o_x()  {
	if( not_state( ST_O_EDIT, "Object X Motion" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edobj );
	dmp->dmr_light( LIGHT_ON, edobj = BE_O_X );
	movedir = RARROW;
	dmaflag++;
}

static void be_o_y()  {
	if( not_state( ST_O_EDIT, "Object Y Motion" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edobj );
	dmp->dmr_light( LIGHT_ON, edobj = BE_O_Y );
	movedir = UARROW;
	dmaflag++;
}


static void be_o_xy()  {
	if( not_state( ST_O_EDIT, "Object XY Motion" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edobj );
	dmp->dmr_light( LIGHT_ON, edobj = BE_O_XY );
	movedir = UARROW | RARROW;
	dmaflag++;
}

static void be_o_rotate()  {
	if( not_state( ST_O_EDIT, "Object Rotation" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edobj );
	dmp->dmr_light( LIGHT_ON, edobj = BE_O_ROTATE );
	movedir = ROTARROW;
	dmaflag++;
}

static void be_accept()  {
	register struct solid *sp;
	/* matrices used to accept editing done from a depth
	 *	>= 2 from the top of the illuminated path
	 */
	mat_t topm;	/* accum matrix from pathpos 0 to i-2 */
	mat_t inv_topm;	/* inverse */
	mat_t deltam;	/* final "changes":  deltam = (inv_topm)(modelchanges)(topm) */
	mat_t tempm;


	menu_array[MENU_L2] = MENU_NULL;
	if( state == ST_S_EDIT )  {
		/* Accept a solid edit */
		dmp->dmr_light( LIGHT_OFF, BE_ACCEPT );
		dmp->dmr_light( LIGHT_OFF, BE_REJECT );
		/* write editing changes out to disc */
		db_putrec( illump->s_path[illump->s_last], &es_rec, 0 );

		dmp->dmr_light( LIGHT_OFF, edsol );
		edsol = 0;
		menu_array[MENU_L1] = MENU_NULL;
		menu_array[MENU_L2] = MENU_NULL;
		dmp->dmr_light( LIGHT_OFF, BE_S_EDIT );
		es_edflag = -1;
		menuflag = 0;

		FOR_ALL_SOLIDS( sp )
			sp->s_iflag = DOWN;

		illump = SOLID_NULL;
		dmp->dmr_colorchange();
		(void)chg_state( ST_S_EDIT, ST_VIEW, "Edit Accept" );
		dmaflag = 1;		/* show completion */
	}  else if( state == ST_O_EDIT )  {
		/* Accept an object edit */
		dmp->dmr_light( LIGHT_OFF, BE_ACCEPT );
		dmp->dmr_light( LIGHT_OFF, BE_REJECT );
		dmp->dmr_light( LIGHT_OFF, edobj );
		edobj = 0;
		movedir = 0;	/* No edit modes set */
		switch( ipathpos )  {
		case 0:
			moveHobj( illump->s_path[ipathpos], modelchanges );
			break;
		case 1:
			moveHinstance(
				illump->s_path[ipathpos-1],
				illump->s_path[ipathpos],
				modelchanges
			);
			break;
		default:
			mat_idn( topm );
			mat_idn( inv_topm );
			mat_idn( deltam );
			mat_idn( tempm );

			pathHmat( illump, topm, ipathpos-2 );

			mat_inv( inv_topm, topm );

			mat_mul( tempm, modelchanges, topm );
			mat_mul( deltam, inv_topm, tempm );

			moveHinstance(
				illump->s_path[ipathpos-1],
				illump->s_path[ipathpos],
				deltam
			);
			break;
		}

		/*
		 *  Redraw all solids affected by this edit.
		 *  Regenerate a new control list which does not
		 *  include the solids about to be replaced,
		 *  so we can safely fiddle the displaylist.
		 */
		modelchanges[15] = 1000000000;	/* => small ratio */
		dmaflag=1;
		refresh();

		/* Now, recompute new chunks of displaylist */
		FOR_ALL_SOLIDS( sp )  {
			if( sp->s_iflag == DOWN )
				continue;
			/* Rip off es_mat and es_rec for redraw() */
			pathHmat( sp, es_mat, sp->s_last-1 );
			if( sp->s_Eflag )  {
				(void)printf("Unable to-redraw evaluated things\n");
			} else {
				db_getrec(sp->s_path[sp->s_last], &es_rec, 0);
				illump = sp;	/* flag to drawHsolid! */
				redraw( sp, &es_rec );
			}
			sp->s_iflag = DOWN;
		}
		mat_idn( modelchanges );

		illump = SOLID_NULL;
		dmp->dmr_colorchange();
		(void)chg_state( ST_O_EDIT, ST_VIEW, "Edit Accept" );
		dmaflag = 1;		/* show completion */
	} else {
		(void)not_state( ST_S_EDIT, "Edit Accept" );
		return;
	}
}

static void be_reject()  {
	register struct solid *sp;

	/* Reject edit */
	dmp->dmr_light( LIGHT_OFF, BE_ACCEPT );
	dmp->dmr_light( LIGHT_OFF, BE_REJECT );

	switch( state )  {
	default:
		state_err( "Edit Reject" );
		return;

	case ST_S_EDIT:
		/* Reject a solid edit */
		if( edsol )
			dmp->dmr_light( LIGHT_OFF, edsol );
		menu_array[MENU_L1] = MENU_NULL;
		menu_array[MENU_L2] = MENU_NULL;

		/* Restore the saved original solid */
		illump = redraw( illump, &es_orig );
		break;

	case ST_O_EDIT:
		if( edobj )
			dmp->dmr_light( LIGHT_OFF, edobj );
		menu_array[MENU_L1] = MENU_NULL;
		menu_array[MENU_L2] = MENU_NULL;
		break;
	case ST_O_PICK:
		dmp->dmr_light( LIGHT_OFF, BE_O_ILLUMINATE );
		break;
	case ST_S_PICK:
		dmp->dmr_light( LIGHT_OFF, BE_S_ILLUMINATE );
		break;
	case ST_O_PATH:
		break;
	}

	menuflag = 0;
	movedir = 0;
	edsol = 0;
	edobj = 0;
	es_edflag = -1;
	illump = SOLID_NULL;		/* None selected */
	dmaflag = 1;

	/* Clear illumination flags */
	FOR_ALL_SOLIDS( sp )
		sp->s_iflag = DOWN;
	dmp->dmr_colorchange();
	(void)chg_state( state, ST_VIEW, "Edit Reject" );
}

static void bv_slicemode() {
}

static void be_s_edit()  {
	/* solid editing */
	if( not_state( ST_S_EDIT, "Solid Edit (Menu)" ) )
		return;

	if( edsol )
		dmp->dmr_light( LIGHT_OFF, edsol );
	dmp->dmr_light( LIGHT_ON, edsol = BE_S_EDIT );
	es_edflag = MENU;
	sedit_menu();		/* Install appropriate menu */
	dmaflag++;
}

static void be_s_rotate()  {
	/* rotate solid */
	if( not_state( ST_S_EDIT, "Solid Rotate" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edsol );
	dmp->dmr_light( LIGHT_ON, edsol = BE_S_ROTATE );
	menuflag = 0;
	menu_array[MENU_L1] = MENU_NULL;
	es_edflag = SROT;
	mat_idn(acc_rot_sol);
	dmaflag++;
}

static void be_s_trans()  {
	/* translate solid */
	if( not_state( ST_S_EDIT, "Solid Translate" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edsol );
	dmp->dmr_light( LIGHT_ON, edsol = BE_S_TRANS );
	menuflag = 0;
	es_edflag = STRANS;
	movedir = UARROW | RARROW;
	menu_array[MENU_L1] = MENU_NULL;
	dmaflag++;
}

static void be_s_scale()  {
	/* scale solid */
	if( not_state( ST_S_EDIT, "Solid Scale" ) )
		return;

	dmp->dmr_light( LIGHT_OFF, edsol );
	dmp->dmr_light( LIGHT_ON, edsol = BE_S_SCALE );
	menuflag = 0;
	es_edflag = SSCALE;
	menu_array[MENU_L1] = MENU_NULL;
	acc_sc_sol = 1.0;
	dmaflag++;
}

/*
 *			N O T _ S T A T E
 *  
 *  Returns 0 if current state is as desired,
 *  Returns !0 and prints error message if state mismatch.
 */
int
not_state( desired, str )
int desired;
char *str;
{
	if( state != desired ) {
		(void)printf("Unable to do <%s> from %s state.\n",
			str, state_str[state] );
		return(1);	/* BAD */
	}
	return(0);		/* GOOD */
}

/*
 *  			C H G _ S T A T E
 *
 *  Returns 0 if state change is OK,
 *  Returns !0 and prints error message if error.
 */
int
chg_state( from, to, str )
int from, to;
char *str;
{
	if( state != from ) {
		(void)printf("Unable to do <%s> going from %s to %s state.\n",
			str, state_str[from], state_str[to] );
		return(1);	/* BAD */
	}
	state = to;
	/* Advise display manager of state change */
	dmp->dmr_statechange( from, to );
	return(0);		/* GOOD */
}

state_err( str )
char *str;
{
	(void)printf("Unable to do <%s> from %s state.\n",
		str, state_str[state] );
}


/*
 *  "Button Menu" stuff
 */
void button_hit_menu(arg)  {
	button(arg);
	menuflag = 0;
}

void
button_menu(i)  {
	switch(i)  {
	case 0:
		menu_array[MENU_GEN] = first_menu;
		break;
	case 1:
		menu_array[MENU_GEN] = second_menu;
		break;
	case 2:
		menu_array[MENU_L2] = MENU_NULL;
		break;
	default:
		printf("button_menu(%d): bad arg\n", i);
		break;
	}
	menuflag = 0;	/* no selected menu item */
	dmaflag = 1;
}

void
chg_l2menu(i)  {
	switch( i )  {
	case ST_S_EDIT:
		menu_array[MENU_L2] = sed_menu;
		break;
	case ST_O_EDIT:
		menu_array[MENU_L2] = oed_menu;
		break;
	default:
		(void)printf("chg_l2menu(%d): bad arg\n");
		break;
	}
}
