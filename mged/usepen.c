/*
 *			U S E P E N . C
 *
 * Functions -
 *	usepen		Use x,y data from data tablet
 *	buildHrot	Generate rotation matrix
 *	wrt_view	Modify xform matrix with respect to current view
 */

#include	<math.h>
#include "ged_types.h"
#include "ged.h"
#include "solid.h"
#include "menu.h"
#include "dm.h"
#include "vmath.h"
#include "3d.h"
#include "sedit.h"

/*	Degree <-> Radian conversion factors	*/
double	degtorad =  0.01745329251994329573;
double	radtodeg = 57.29577951308232098299;

int	sedraw;			/* apply solid editing changes */
struct solid	*illump;	/* == 0 if none, else points to ill. solid */
int		ipathpos;	/* path index of illuminated element */
				/* set by e9.c, cleared here */
static void	illuminate();

/*
 *			U S E P E N
 *
 * X and Y are expected to be in -2048 <= x,y <= +2047 range.
 * Press is !0 when pen is pressed.
 *
 * Note -
 *  The data tablet is the focus of much of the editing activity in GED.
 *  The editor operates in one of seven basic editing states, recorded
 *  in the variable called "state".  When no editing is taking place,
 *  the editor is in state ST_VIEW.  There are two paths out of ST_VIEW:
 *  
 *  BE_S_ILLUMINATE, when pressed, takes the editor into ST_S_PICK,
 *  where the tablet is used to pick a solid to edit, using our
 *  unusual "illuminate" technique.  Moving the pen varies the solid
 *  being illuminated.  When the pen is pressed, the editor moves into
 *  state ST_S_EDIT, and solid editing may begin.  Solid editing is
 *  terminated via BE_ACCEPT and BE_REJECT.
 *  
 *  BE_O_ILLUMINATE, when pressed, takes the editor into ST_O_PICK,
 *  again performing the illuminate procedure.  When the pen is pressed,
 *  the editor moves into state ST_O_PATH.  Now, moving the pen allows
 *  the user to choose the portion of the path relation to be edited.
 *  When the pen is pressed, the editor moves into state ST_O_EDIT,
 *  and object editing may begin.  Object editing is terminated via
 *  BE_ACCEPT and BE_REJECT.
 *  
 *  The only way to exit the intermediate states (non-VIEW, non-EDIT)
 *  is by completing the sequence, or pressing BE_REJECT.
 */
void
usepen( x, y, press )
register int x, y;
{
	static float scale;
	register struct solid *sp;
	static int j;
	static vect_t pos_view;	 	/* Unrotated view space pos */
	static vect_t pos_model;	/* Rotated screen space pos */
	static vect_t tr_temp;		/* temp translation vector */
	static vect_t tabvec;		/* tablet vector */

	/*
	 * Keep the pen input from doing other things to
	 * ged when the MENU_ON is active.  This test may want
	 * to include some other stuff for illumination, which
	 * might still be done in a vertical direction, with
	 * the menu active under the illumination info. Bob S
	 */
	if(  menu_on == TRUE  &&
		menu_list != (struct menu_item *) NULL  &&
		menu_select( x, y, press )
	) return;

	if( state == ST_VIEW )  {
		if( press == 0 )
			return;

		/*
		 * Use the DT for moving view center.
		 * Make indicated point be new view center (NEW).
		 */
		tabvec[X] =  x / 2047.0;
		tabvec[Y] =  y / 2047.0;
		tabvec[Z] = 0;

		slewview( tabvec );
		return;
	}

	if( state == ST_O_PICK || state == ST_S_PICK )  {
		/*
		 * We are in "illuminate" mode, with no object selected.
		 */
		if( press )  {
			/* Currently illuminated object is selected */
			if( state == ST_O_PICK )  {
				ipathpos = 0;
				state = ST_O_PATH;
			} else {
				/* Check details, Init menu, set state */
				init_sedit();
			}
			dmaflag = 1;
			return;
		}

		/*
		 * Use the tablet for illuminating a solid
		 */
		illuminate( y );

		/*
		 * People seem to find the cross-hair distracting in
		 * illuminate mode, so force it to the center.
		 */
		xcross = ycross = 0;
		return;
	}

	if( state == ST_O_PATH )  {
		/* Select path element for Object Edit */
		/* If pressed, use selected path element */
		if( press )  {
			/*
			 * Set combination "illuminate" mode.  This code
			 * assumes that the user has already illuminated
			 * a single solid, and wishes to move a collection of
			 * objects of which the illuminated solid is a part.
			 * The whole combination will not illuminate (to save
			 * vector drawing time), but all the objects should
			 * move/scale in unison.
			 */
			dmp->dmr_light( LIGHT_ON, BE_ACCEPT );
			dmp->dmr_light( LIGHT_ON, BE_REJECT );
			dmp->dmr_light( LIGHT_OFF, BE_O_ILLUMINATE );

			/* Include all solids with same tree top */
			FOR_ALL_SOLIDS( sp )  {
				for( j = 0; j <= ipathpos; j++ )  {
					if( sp->s_path[j] !=
							illump->s_path[j] )
						break;
				}
				/* Only accept if top of tree is identical */
				if( j == ipathpos+1 )
					sp->s_iflag = UP;
			}
			state = ST_O_EDIT;
			dmaflag++;
			return;
		}

		/*
		 * Convert DT position to path element select
		 *
		 * The following formula depends heavily on
		 * the fact that s_count will never exceed 7.
		 */
		ipathpos = (y + 2048) * (illump->s_last+1) / 4096;
		ipathpos = illump->s_last - ipathpos;
		dmaflag++;
		return;
	}

	if( !press )
		return;
	/*
	 * When pressed, use the DT for object transformations
	 */
	if( state == ST_S_EDIT && es_edflag > 0 )  switch( es_edflag )  {
		/*
		 *  Solid Edit
		 */
	case SSCALE:
	case PSCALE:
		/* use pen to get a scale factor */
		es_scale = 1.0 + 0.25 *
				((float)(y > 0 ? y : -y) / 2047);
		if ( y <= 0 )
			es_scale = 1.0 / es_scale;
		sedraw = 1;
		return;
	case STRANS:
		/* 
		 * Use pen to change solid's location.
		 * Project solid's V point into view space,
		 * replace X,Y (but NOT Z) components, and
		 * project result back to model space.
		 */
		MAT4X3PNT( pos_view, model2view, es_rec.s.s_values );
		pos_view[X] = x / 2047.0;
		pos_view[Y] = y / 2047.0;
		MAT4X3PNT( es_rec.s.s_values, view2model, pos_view );
		sedraw = 1;
		return;
	case MOVEH:
		/* Use pen to change location of point V+H */
		VADD2( pos_model, &es_rec.s.s_tgc_V, &es_rec.s.s_tgc_H );
		MAT4X3PNT( pos_view, model2view, pos_model );
		pos_view[X] = x / 2047.0;
		pos_view[Y] = y / 2047.0;
		/* Do NOT change pos_view[Z] ! */
		MAT4X3PNT( tr_temp, view2model, pos_view );
		VSUB2( &es_rec.s.s_tgc_H, tr_temp, &es_rec.s.s_tgc_V );
		sedraw = 1;
		return;
	case EARB:
		/* move arb edge, through indicated point */
		tabvec[X] = x / 2047.0;
		tabvec[Y] = y / 2047.0;
		tabvec[Z] = 0;
		MAT4X3PNT( pos_model, view2model, tabvec );
		editarb( pos_model );
		sedraw = 1;
		return;
	default:
		(void)printf("Pen press undefined in this solid edit mode\n");
		return;
	}

	if( state != ST_O_EDIT )  {
		state_err( "Pen Press" );
		return;
	}

	/*
	 *  Object Edit
	 */
	mat_idn( incr_change );
	scale = 1;
	if( movedir & SARROW )  {
		scale = 1.0 + (float)(y > 0 ? y : -y) / (2047);
		if ( y <= 0 )
			scale = 1.0 / scale;

		/*  For this to take effect relative to the view center,
		 *	p' = ( (p - center) * scale ) + center
		 */
		incr_change[15] = 1.0 / scale;
		wrt_view( modelchanges, incr_change, modelchanges );
	}  else if( movedir & (RARROW|UARROW) )  {
		static mat_t oldchanges;	/* temporary matrix */

		/* Vector from object center to cursor */
		MAT4X3PNT( pos_view, model2objview, illump->s_center );
		if( movedir & RARROW )
			pos_view[X] = x / 2047.0;
		if( movedir & UARROW )
			pos_view[Y] = y / 2047.0;

		MAT4X3PNT( pos_model, view2model, pos_view );/* NOT objview */
		MAT4X3PNT( tr_temp, modelchanges, illump->s_center );
		VSUB2( tr_temp, pos_model, tr_temp );
		MAT_DELTAS(incr_change,
			tr_temp[X], tr_temp[Y], tr_temp[Z]);
		mat_copy( oldchanges, modelchanges );
		mat_mul( modelchanges, incr_change, oldchanges );
	}  else  {
		printf("No object edit mode selected;  pen press ignored\n");
		return;
	}
	mat_idn( incr_change );
	new_mats();
}

/*
 *			I L L U M I N A T E
 */
static void
illuminate( y )  {
	register int count;
	register struct solid *sp;

	/*
	 * Divide the tablet into 'ndrawn' VERTICAL zones, and use the
	 * zone number as a sequential position among solids
	 * which are drawn.
	 */
	count = ( (float) y + 2048.0 ) * ndrawn / 4096.0;

	FOR_ALL_SOLIDS( sp )  {
		if( sp->s_flag == UP )
			if( count-- == 0 )  {
				illump = sp;
				illump->s_iflag = UP;
				dmaflag++;
			}  else
				sp->s_iflag = DOWN;
	}
}

/*
 *			B U I L D H R O T
 *
 * This routine builds a Homogeneous rotation matrix, given
 * alpha, beta, and gamma as angles of rotation.
 *
 * NOTE:  Only initialize the rotation 3x3 parts of the 4x4
 * There is important information in dx,dy,dz,s .
 */
void
buildHrot( mat, alpha, beta, ggamma )
register matp_t mat;
double alpha, beta, ggamma;
{
	static float calpha, cbeta, cgamma;
	static float salpha, sbeta, sgamma;

	calpha = cos( alpha );
	cbeta = cos( beta );
	cgamma = cos( ggamma );

	salpha = sin( alpha );
	sbeta = sin( beta );
	sgamma = sin( ggamma );

	/*
	 * compute the new rotation to apply to the previous
	 * viewing rotation.
	 * Alpha is angle of rotation about the X axis, and is done third.
	 * Beta is angle of rotation about the Y axis, and is done second.
	 * Gamma is angle of rotation about Z axis, and is done first.
	 */
#ifdef m_RZ_RY_RX
	/* view = model * RZ * RY * RX (Neuman+Sproul, premultiply) */
	mat[0] = cbeta * cgamma;
	mat[1] = -cbeta * sgamma;
	mat[2] = -sbeta;

	mat[4] = -salpha * sbeta * cgamma + calpha * sgamma;
	mat[5] = salpha * sbeta * sgamma + calpha * cgamma;
	mat[6] = -salpha * cbeta;

	mat[8] = calpha * sbeta * cgamma + salpha * sgamma;
	mat[9] = -calpha * sbeta * sgamma + salpha * cgamma;
	mat[10] = calpha * cbeta;
#endif
	/* This is the correct form for this version of GED */
	/* view = RX * RY * RZ * model (Rodgers, postmultiply) */
	/* Point thumb along axis of rotation.  +Angle as hand closes */
	mat[0] = cbeta * cgamma;
	mat[1] = -cbeta * sgamma;
	mat[2] = sbeta;

	mat[4] = salpha * sbeta * cgamma + calpha * sgamma;
	mat[5] = -salpha * sbeta * sgamma + calpha * cgamma;
	mat[6] = -salpha * cbeta;

	mat[8] = -calpha * sbeta * cgamma + salpha * sgamma;
	mat[9] = calpha * sbeta * sgamma + salpha * cgamma;
	mat[10] = calpha * cbeta;
}


/*
 *  			W R T _ V I E W
 *  
 *  Given a model-space transformation matrix "change",
 *  return a matrix which applies the change with-respect-to
 *  the view center.
 */
wrt_view( out, change, in )
register matp_t out, change, in;
{
	static mat_t t1, t2;

	mat_mul( t1, toViewcenter, in );
	mat_mul( t2, change, t1 );

	/* Build "fromViewcenter" matrix */
	mat_idn( t1 );
	MAT_DELTAS( t1, -toViewcenter[MDX], -toViewcenter[MDY], -toViewcenter[MDZ] );
	mat_mul( out, t1, t2 );
}
