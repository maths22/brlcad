/*
 *			C M D . C
 *
 * Functions -
 *	f_press		hook for displays with no buttons
 *	f_summary	do directory summary
 *	mged_cmd		Check arg counts, run a command
 *
 *  Authors -
 *	Michael John Muuss
 *	Charles M. Kennedy
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

#include "conf.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/time.h>
#include <time.h>

#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "externs.h"
#include "./ged.h"
#include "./solid.h"
#include "./dm.h"

#ifdef MGED_TCL
#  define XLIB_ILLEGAL_ACCESS	/* necessary on facist SGI 5.0.1 */
#  include "tcl.h"
#  include "tk.h"
#endif

extern void	sync();

#define	MAXARGS		2000	/* Maximum number of args per line */
int	inpara;			/* parameter input from keyboard */

extern int	cmd_glob();

int	f_matpick();
int	mged_cmd();
int	f_sync();

#ifdef MGED_TCL
int	f_gui();

int gui_mode = 0;

Tcl_Interp *interp;
Tk_Window tkwin;
#endif

struct rt_vls history;
struct rt_vls replay_history;
struct timeval lastfinish;
FILE *journalfp;
int firstjournal;

struct funtab {
	char *ft_name;
	char *ft_parms;
	char *ft_comment;
	int (*ft_func)();
	int ft_min;
	int ft_max;
};

static struct funtab funtab[] = {
"", "", "Primary command Table.",
	0, 0, 0,
"?", "", "summary of available commands",
	f_fhelp,0,MAXARGS,
"%", "", "escape to interactive shell",
	f_comm,1,1,
"3ptarb", "", "makes arb given 3 pts, 2 coord of 4th pt, and thickness",
	f_3ptarb, 1, 27,
"adc", "[<a1|a2|dst|dh|dv|hv|dx|dy|dz|xyz|reset|help> value(s)]",
	"control the angle/distance cursor",
        f_adc, 1, 5,
"ae", "azim elev", "set view using az and elev angles",
	f_aeview, 3, 3,
"analyze", "[arbname]", "analyze faces of ARB",
	f_analyze,1,MAXARGS,
"arb", "name rot fb", "make arb8, rotation + fallback",
	f_arbdef,4,4,
"area", "[endpoint_tolerance]", "calculate presented area of view",
	f_area, 1, 2,
"attach", "[device]", "attach to a display processor, or NU",
	f_attach,1,2,
"B", "<objects>", "clear screen, edit objects",
	f_blast,2,MAXARGS,
"cat", "<objects>", "list attributes (brief)",
	f_cat,2,MAXARGS,
"center", "x y z", "set view center",
	f_center, 4,4,
"color", "low high r g b str", "make color entry",
	f_color, 7, 7,
"comb", "comb_name <operation solid>", "create or extend combination w/booleans",
	f_comb,4,MAXARGS,
#ifdef MGED_TCL
"mconcat", "file [prefix]", "concatenate 'file' onto end of present database.  Run 'dup file' first.",
	f_concat, 2, 3,
#else
"concat", "file [prefix]", "concatenate 'file' onto end of present database.  Run 'dup file' first.",
	f_concat, 2, 3,
#endif
"copyeval", "new_solid path_to_old_solid (seperate path components with spaces, not /)",
	"copy an 'evaluated' path solid",
	f_copyeval, 1, 27,
"cp", "from to", "copy [duplicate] object",
	f_copy,3,3,
"cpi", "from to", "copy cylinder and position at end of original cylinder",
	f_copy_inv,3,3,
"d", "<objects>", "delete list of objects",
	f_delobj,2,MAXARGS,
"debugdir", "", "Print in-memory directory, for debugging",
	f_debugdir, 1, 1,
"debuglib", "[hex_code]", "Show/set debugging bit vector for librt",
	f_debuglib,1,2,
"debugmem", "", "Print librt memory use map",
	f_debugmem, 1, 1,
"debugnmg", "[hex code]", "Show/set debugging bit vector for NMG",
	f_debugnmg,1,2,
"delay", "sec usec", "Delay for the specified amount of time",
	f_delay,3,3,
"dm", "set var=val", "Do display-manager specific command",
	f_dm, 2, MAXARGS,
"dup", "file [prefix]", "check for dup names in 'file'",
	f_dup, 2, 3,
"E", "<objects>", "evaluated edit of objects",
	f_evedit,2,MAXARGS,
"e", "<objects>", "edit objects",
	f_edit,2,MAXARGS,
"echo", "[text]", "echo arguments back",
	f_echo, 1, MAXARGS,
"edcodes", "object(s)", "edit region ident codes",
	f_edcodes, 2, MAXARGS,
"edcolor", "", "text edit color table",
	f_edcolor, 1, 1,
"edcomb", "combname Regionflag regionid air los [GIFTmater]", "edit combination record info",
	f_edcomb,6,7,
"edgedir", "[delta_x delta_y delta_z]|[rot fb]", "define direction of ARB edge being moved",
	f_edgedir, 3, 4,
"ev",	"[-w] [-n] [-P#] [-T] <objects>", "evaluate objects via NMG tessellation",
	f_ev, 2, MAXARGS,
"eqn", "A B C", "planar equation coefficients",
	f_eqn, 4, 4,
"extrude", "#### distance", "extrude dist from face",
	f_extrude,3,3,
"facedef", "####", "define new face for an arb",
	f_facedef, 2, MAXARGS,
"facetize", "[-t] [-P#] new_obj old_obj(s)", "convert objects to faceted NMG objects at current tol",
	f_facetize, 3, MAXARGS,
"find", "<objects>", "find all references to objects",
	f_find, 1, MAXARGS,
"fix", "", "fix display after hardware error",
	f_fix,1,1,
"fracture", "NMGsolid [prefix]", "fracture an NMG solid into many NMG solids, each containing one face\n",
	f_fracture, 2, 3,
"g", "groupname <objects>", "group objects",
	f_group,3,MAXARGS,
#ifdef MGED_TCL
"gui",	"", "Bring up a Tcl/Tk Graphical User Interface",
	f_gui, 1, 1,
#endif
#ifdef HIDELINE
"H", "plotfile [step_size %epsilon]", "produce hidden-line unix-plot",
	f_hideline,2,4,
#endif
"help", "[commands]", "give usage message for given commands",
	f_help,0,MAXARGS,
"history", "[-delays]", "describe command history",
	f_history, 1, 4,
"i", "obj combination [operation]", "add instance of obj to comb",
	f_instance,3,4,
"idents", "file object(s)", "make ascii summary of region idents",
	f_tables, 3, MAXARGS,
"ill", "name", "illuminate object",
	f_ill,2,2,
"in", "[-f] [-s] parameters...", "keyboard entry of solids.  -f for no drawing, -s to enter solid edit",
	f_in, 1, MAXARGS,
"inside", "", "finds inside solid per specified thicknesses",
	f_inside, 1, MAXARGS,
"item", "region item [air]", "change item # or air code",
	f_itemair,3,4,
"joint", "command [options]", "articualtion/animation commands",
	f_joint, 1, MAXARGS,
"journal", "fileName", "record all commands and timings to journal",
	f_journal, 1, 2,
"keep", "keep_file object(s)", "save named objects in specified file",
	f_keep, 3, MAXARGS,
"keypoint", "[x y z | reset]", "set/see center of editing transformations",
	f_keypoint,1,4,
"kill", "[-f] <objects>", "delete object[s] from file",
	f_kill,2,MAXARGS,
"killall", "<objects>", "kill object[s] and all references",
	f_killall, 2, MAXARGS,
"killtree", "<object>", "kill complete tree[s] - BE CAREFUL",
	f_killtree, 2, MAXARGS,
"knob", "id [val]", "emulate knob twist",
	f_knob,2,3,
"l", "<objects>", "list attributes (verbose)",
	f_list,2,MAXARGS,
"labelvert", "object[s]", "label vertices of wireframes of objects",
	f_labelvert, 2, MAXARGS,
"listeval", "", "lists 'evaluated' path solids",
	f_pathsum, 1, MAXARGS,
"ls", "", "table of contents",
	dir_print,1,MAXARGS,
"M", "1|0 xpos ypos", "handle a mouse event",
	f_mouse, 4,4,
"make", "name <arb8|sph|ellg|tor|tgc|rpc|rhc|epa|ehy|eto|part|grip|half|nmg>", "create a primitive",
	f_make,3,3,
"mater", "comb [material]", "assign/delete material to combination",
	f_mater,2,3,
"matpick", "# or a/b", "select arc which has matrix to be edited, in O_PATH state",
	f_matpick, 2,2,
"memprint", "", "print memory maps",
	f_memprint, 1, 1,
"mirface", "#### axis", "mirror an ARB face",
	f_mirface,3,3,
"mirror", "old new axis", "Arb mirror ??",
	f_mirror,4,4,
"mv", "old new", "rename object",
	f_name,3,3,
"mvall", "oldname newname", "rename object everywhere",
	f_mvall, 3, 3,
"nirt", "", "trace a single ray from current view",
	f_nirt,1,MAXARGS,
"opendb", "database.g", "Close current .g file, and open new .g file",
	f_opendb, 2, 2,
"orientation", "x y z w", "Set view direction from quaternion",
	f_orientation, 5, 5,
"orot", "xdeg ydeg zdeg", "rotate object being edited",
	f_rot_obj, 4, 4,
"overlay", "file.plot [name]", "Read UNIX-Plot as named overlay",
	f_overlay, 2, 3,
"p", "dx [dy dz]", "set parameters",
	f_param,2,4,
"paths", "pattern", "lists all paths matching input path",
	f_pathsum, 1, MAXARGS,
"permute", "tuple", "permute vertices of an ARB",
	f_permute,2,2,
"plot", "[-float] [-zclip] [-2d] [-grid] [out_file] [|filter]", "make UNIX-plot of view",
	f_plot, 2, MAXARGS,
"polybinout", "file", "store vlist polygons into polygon file (experimental)",
	f_polybinout, 2, 2,
"pov", "args", "experimental:  set point-of-view",
	f_pov, 3+4+1, MAXARGS,
"prcolor", "", "print color&material table",
	f_prcolor, 1, 1,
"prefix", "new_prefix object(s)", "prefix each occurrence of object name(s)",
	f_prefix, 3, MAXARGS,
"preview", "[-v] [-d sec_delay] rt_script_file", "preview new style RT animation script",
	f_preview, 2, MAXARGS,
"press", "button_label", "emulate button press",
	f_press,2,MAXARGS,
"push", "object[s]", "pushes object's path transformations to solids",
	f_push, 2, MAXARGS,
"q", "", "quit",
	f_quit,1,1,
"quit", "", "quit",
	f_quit,1,1,
"qorot", "x y z dx dy dz theta", "rotate object being edited about specified vector",
	f_qorot, 8, 8,
"qvrot", "dx dy dz theta", "set view from direction vector and twist angle",
	f_qvrot, 5, 5,
"r", "region <operation solid>", "create or extend a Region combination",
	f_region,4,MAXARGS,
"red", "object", "edit a group or region using a text editor",
	f_red, 2, 2,
"refresh", "", "send new control list",
	f_refresh, 1,1,
"regdebug", "", "toggle register print",
	f_regdebug, 1,2,
"regdef", "item [air] [los] [GIFTmaterial]", "change next region default codes",
	f_regdef, 2, 5,
"regions", "file object(s)", "make ascii summary of regions",
	f_tables, 3, MAXARGS,
"release", "", "release current display processor [attach NU]",
	f_release,1,1,
"rfarb", "", "makes arb given point, 2 coord of 3 pts, rot, fb, thickness",
	f_rfarb, 1, 27,
"rm", "comb <members>", "remove members from comb",
	f_rm,3,MAXARGS,
"rmats", "file", "load views from file (experimental)",
	f_rmats,2,MAXARGS,
"rotobj", "xdeg ydeg zdeg", "rotate object being edited",
	f_rot_obj, 4, 4,
"rrt", "prog [options]", "invoke prog with view",
	f_rrt,2,MAXARGS,
"rt", "[options]", "do raytrace of view",
	f_rt,1,MAXARGS,
"rtcheck", "[options]", "check for overlaps in current view",
	f_rtcheck,1,MAXARGS,
"savekey", "file [time]", "save keyframe in file (experimental)",
	f_savekey,2,MAXARGS,
"saveview", "file [args]", "save view in file for RT",
	f_saveview,2,MAXARGS,
#ifdef MGED_TCL
"mscale", "factor", "scale object by factor",
	f_sc_obj,2,2,
#else
"scale", "factor", "scale object by factor",
	f_sc_obj,2,2,
#endif
"sed", "solid", "solid-edit named solid",
	f_sed,2,2,
#ifdef MGED_TCL
"mset",	"[var=opt]", "assign/display mged variables",
	f_set,1,2,
#else
"set",	"[var=opt]", "assign/display mged variables",
	f_set,1,2,
#endif
"shader", "comb material [arg(s)]", "assign materials (like 'mater')",
	f_shader, 3,MAXARGS,
"size", "size", "set view size",
	f_view, 2,2,
"solids", "file object(s)", "make ascii summary of solid parameters",
	f_tables, 3, MAXARGS,
#ifndef MGED_TCL
"source", "file/pipe", "read and process file/pipe of commands",
	f_source, 2,MAXARGS,
#endif
"status", "", "get view status",
	f_status, 1,1,
"summary", "[s r g]", "count/list solid/reg/groups",
	f_summary,1,2,
"sync",	"",	"forces UNIX sync",
	f_sync, 1, 1,
"t", "", "table of contents",
	dir_print,1,MAXARGS,
"tab", "object[s]", "tabulates objects as stored in database",
	f_tabobj, 2, MAXARGS,
"ted", "", "text edit a solid's parameters",
	f_tedit,1,1,
"title", "string", "change the title",
	f_title,1,MAXARGS,
"tol", "[abs #]|[rel #]", "show/set absolute or relative tolerance for tessellation",
	f_tol, 1, 3,
"tops", "", "find all top level objects",
	f_tops,1,1,
"track", "<parameters>", "adds tracks to database",
	f_amtrack, 1, 27,
"translate", "x y z", "trans object to x,y, z",
	f_tr_obj,4,4,
"tree",	"object(s)", "print out a tree of all members of an object",
	f_tree, 2, MAXARGS,
"units", "[mm|cm|m|in|ft|...]", "change units",
	f_units,1,2,
"vrmgr", "host {master|slave|overview}", "link with Virtual Reality manager",
	f_vrmgr, 3, MAXARGS,
"vrot", "xdeg ydeg zdeg", "rotate viewpoint",
	f_vrot,4,4,
"vrot_center", "v|m x y z", "set center point of viewpoint rotation, in model or view coords",
	f_vrot_center, 5, 5,
"whichid", "ident(s)", "lists all regions with given ident code",
	f_which_id, 2, MAXARGS,
"x", "lvl", "print solid table & vector list",
	f_debug, 1,2,
"Z", "", "zap all objects off screen",
	f_zap,1,1,
"zoom", "scale_factor", "zoom view in or out",
	f_zoom, 2,2,
0, 0, 0,
	0, 0, 0,
};



/*
 *	H I S T O R Y _ R E C O R D
 *
 *	Stores the given command with start and finish times in the
 *	  history vls'es.
 */

void
history_record( cmdp, start, finish )
struct rt_vls *cmdp;
struct timeval *start, *finish;
{
	static int done = 0;
	struct rt_vls timing;

	rt_vls_vlscat( &history, cmdp );

	rt_vls_init( &timing );
	if( done != 0 ) {
		if( lastfinish.tv_usec > start->tv_usec ) {
			rt_vls_printf( &timing, "delay %ld %08ld\n",
			    start->tv_sec - lastfinish.tv_sec - 1,
			    start->tv_usec - lastfinish.tv_usec + 1000000L );
		} else {
			rt_vls_printf( &timing, "delay %ld %08ld\n",
				start->tv_sec - lastfinish.tv_sec,
				start->tv_usec - lastfinish.tv_usec );
		}
	}		

	/* As long as this isn't our first command to record after setting
           up the journal (which would be "journal", which we don't want
	   recorded!)... */

	if( journalfp != NULL && !firstjournal ) {
		rt_vls_fwrite( journalfp, &timing );
		rt_vls_fwrite( journalfp, cmdp );
	}

	rt_vls_vlscat( &replay_history, &timing );
	rt_vls_vlscat( &replay_history, cmdp );

	lastfinish.tv_sec = finish->tv_sec;
	lastfinish.tv_usec = finish->tv_usec;
	done = 1;
	firstjournal = 0;

	rt_vls_free( &timing );
}		


/*
 *	F _ J O U R N A L
 *
 *	Opens the journal file, so each command and the time since the previous
 *	  one will be recorded.  Or, if called with no arguments, closes the
 *	  journal file.
 */

int
f_journal( argc, argv )
int argc;
char **argv;
{
	if( argc < 2 ) {
		if( journalfp != NULL )
			fclose( journalfp );
		journalfp = NULL;
		return CMD_OK;
	} else {
		if( journalfp != NULL ) {
			rt_log("First shut off journaling with \"journal\" (no args)\n");
			return CMD_BAD;
		} else {
			journalfp = fopen(argv[1], "a+");
			if( journalfp == NULL ) {
				rt_log( "Error opening %s for appending\n", argv[1] );
				return CMD_BAD;
			}
			firstjournal = 1;
		}
	}

	return CMD_OK;
}


/*
 *	F _ D E L A Y
 *
 * 	Uses select to delay for the specified amount of seconds and 
 *	  microseconds.
 */

int
f_delay( argc, argv )
int argc;
char **argv;
{
	struct timeval tv;

	tv.tv_sec = atoi( argv[1] );
	tv.tv_usec = atoi( argv[2] );
	select( 0, NULL, NULL, NULL, &tv );

	return CMD_OK;
}


/*
 *	F _ H I S T O R Y
 *
 *	Prints out the command history, either to rt_log or to a file.
 */

int
f_history( argc, argv )
int argc;
char **argv;
{
	FILE *fp;
	struct rt_vls *which_history;

	fp = NULL;
	which_history = &history;

	while( argc >= 2 ) {
		if( strcmp(argv[1], "-delays") == 0 ) {
			if( which_history == &replay_history ) {
				rt_log( "history: -delays option given more than once\n" );
				return CMD_BAD;
			}
			which_history = &replay_history;
		} else if( strcmp(argv[1], "-outfile") == 0 ) {
			if( fp != NULL ) {
				rt_log( "history: -outfile option given more than once\n" );
				return CMD_BAD;
			} else if( argc < 3 || strcmp(argv[2], "-delays") == 0 ) {
				rt_log( "history: I need a file name\n" );
				return CMD_BAD;
			} else {
				fp = fopen( argv[2], "a+" );
				if( fp == NULL ) {
					rt_log( "history: error opening file" );
					return CMD_BAD;
				}
				--argc;
				++argv;
			}
		} else {
			rt_log( "Invalid option %s\n", argv[1] );
		}
		--argc;
		++argv;
	}

	if( fp == NULL ) {
		rt_log( rt_vls_addr(which_history) );
	} else {
		rt_vls_fwrite( fp, which_history );
		fclose( fp );
	}

	return CMD_OK;
}


#ifdef MGED_TCL

/*
 *	T C L _ A P P I N I T
 *
 *	Called by the Tcl/Tk libraries for initialization.
 *	Unncessary in our case; cmd_setup does all the work.
 */


int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
	return TCL_OK;
}


/*			C M D _ W R A P P E R
 *
 */

int
cmd_wrapper(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	switch (mged_cmd(argc, argv, funtab)) {
	case CMD_OK:
		interp->result = "MGED_Ok";
		return TCL_OK;
	case CMD_BAD:
		interp->result = "MGED_Error";
		return TCL_ERROR;
	case CMD_MORE:
		interp->result = "MGED_More";
		return TCL_ERROR;
	default:
		interp->result = "MGED_Unknown";
		return TCL_ERROR;
	}
}

void
mged_tk_hook()
{
	while( Tk_DoOneEvent( TK_DONT_WAIT | TK_ALL_EVENTS ) )  /*NIL*/;
}

void
mged_tk_idle(non_blocking)
int	non_blocking;
{
	int	flags;

	flags = TK_DONT_WAIT | TK_ALL_EVENTS;

	while( Tk_DoOneEvent( flags ) )  {
		/* NIL */	;
	}
}



/*
 *	G U I _ O U T P U T
 *
 *	Used as a hook for rt_log output.  Redirects output to the Tcl/Tk
 *		MGED Interaction window.
 */

int
gui_output( str )
char *str;
{
	char buf[10240];
	char *old;

	sprintf(buf, ".i.f.text insert insert \"%s\"", str);
	Tcl_Eval(interp, buf);
	Tcl_Eval(interp, ".i.f.text yview -pickplace insert");
	Tcl_Eval(interp, "set printend [.i.f.text index insert]");

	return strlen(str);
}



/*
 *	G U I _ C M D L I N E
 *
 *	Called from the Tcl/Tk section to process commands from
 *	the MGED Interaction window.
 */

int
gui_cmdline( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	static struct rt_vls argstr, hargstr;
	static int done = 0;
	struct timeval start, finish;
	int result;

	if( done == 0 ) {
		rt_vls_init( &argstr );
		rt_vls_init( &hargstr );
		done = 1;
		rt_vls_strcpy( &argstr, "" );
		rt_vls_strcpy( &hargstr, "" );
	}

	rt_vls_trunc( &hargstr, 0 );

	if( rt_vls_strlen(&argstr) > 0 ) {
		/* Remove newline */
		rt_vls_trunc( &argstr, rt_vls_strlen(&argstr)-1 );
		rt_vls_strcat( &argstr, " " );
	}

	rt_vls_strcat( &argstr, argv[1] );

	gettimeofday( &start, NULL );
	result = Tcl_Eval(interp, rt_vls_addr(&argstr));
	gettimeofday( &finish, NULL );
	
	switch( result ) {
	case TCL_OK:
		if( strcmp(interp->result, "MGED_Ok") != 0 )
			rt_log( interp->result );
			    /* If the command was more than just \n, record. */
		if( rt_vls_strlen(&argstr) > 1 )
			history_record( &argstr, &start, &finish );
		rt_vls_trunc( &argstr, 0 );
		pr_prompt();
		return TCL_OK;
	case TCL_ERROR:
		if( strcmp(interp->result, "MGED_Error") == 0 ) {
			rt_vls_printf(&hargstr, "# %s", rt_vls_addr(&argstr));
			history_record( &hargstr, &start, &finish );
			rt_vls_trunc( &argstr, 0 );
			pr_prompt();
			return TCL_OK;
		} else if( strcmp(interp->result, "MGED_More") == 0 ) {
			return TCL_OK;
		} else {
			char *tmp;
			tmp = (char *)malloc( strlen(interp->result)+1 );
			strcpy( tmp, interp->result );
			rt_log( "%s\n", tmp );
			rt_vls_printf(&hargstr, "# %s", rt_vls_addr(&argstr));
			history_record( &hargstr, &start, &finish );
			rt_vls_trunc( &argstr, 0 );
			pr_prompt();
			interp->result = tmp;
			interp->freeProc = (void (*)())free;
			return TCL_ERROR;
		}
	default:
		interp->result = "MGED: Unknown Error.";
		return TCL_ERROR;
	}
}

/*
 *	C M D _ P R E V
 */

int
cmd_prev( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	return TCL_OK;
}

/*
 *	C M D _ N E X T
 */

int
cmd_next( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	return TCL_OK;
}


/*
 *	G E T K N O B
 *
 *	Procedure called by the Tcl/Tk interface code to find the values
 *	of the knobs/sliders.
 */

int
getknob( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	char *cmd;

	if( argc < 2 ) {
		interp->result = "getknob: need a knob name";
		return TCL_ERROR;
	}

	cmd = argv[1];
	switch( cmd[0] ) {
	case 'x':
		if( mged_variables.rateknobs )
			sprintf(interp->result, "%lf", rate_rotate[X]);
		else
			sprintf(interp->result, "%lf", absolute_rotate[X]);
		return TCL_OK;
	case 'y':
		if( mged_variables.rateknobs )
			sprintf(interp->result, "%lf", rate_rotate[Y]);
		else
			sprintf(interp->result, "%lf", absolute_rotate[Y]);
		return TCL_OK;
	case 'z':
		if( mged_variables.rateknobs )
			sprintf(interp->result, "%lf", rate_rotate[Z]);
		else	
			sprintf(interp->result, "%lf", absolute_rotate[Z]);
		return TCL_OK;
	case 'X':
		if( mged_variables.rateknobs )
			sprintf(interp->result, "%lf", rate_slew[X]);
		else
			sprintf(interp->result, "0");
		return TCL_OK;
	case 'Y':
		if( mged_variables.rateknobs )
			sprintf(interp->result, "%lf", rate_slew[Y]);
		else
			sprintf(interp->result, "0");
		return TCL_OK;
	case 'Z':
		if( mged_variables.rateknobs )
			sprintf(interp->result, "%lf", rate_slew[Z]);
		else
			sprintf(interp->result, "0");
		return TCL_OK;
	case 'S':
		if( mged_variables.rateknobs )
			sprintf(interp->result, "%lf", rate_zoom);
		else
			sprintf(interp->result, "0");
		return TCL_OK;
	default:
		interp->result = "getknob: invalid knob name";
		return TCL_ERROR;
	}
}

/*
 *	G U I _ K N O B
 *
 *	Replaces the regular knob function.
 *	All this one does is call the regular one knob function, then update
 *	  the Tk sliders.
 */

int
gui_knob( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	int result;

	/* First, call regular "knob" function and proceed if ok. */
	if((result=cmd_wrapper( clientData, interp, argc, argv )) == TCL_OK) { 
		struct rt_vls tclcmd;

		rt_vls_init( &tclcmd );
		if( strcmp(argv[1], "zero") == 0 ) {
			rt_vls_strcpy( &tclcmd, "sliders_zero" );
		} else {
			rt_vls_printf( &tclcmd, 
    "global sliders; if { $sliders(exist) } then { .sliders.f.k%s set %d }",
				argv[1], (int) (2048.0*atof(argv[2])) );
		}

		result = Tcl_Eval( interp, rt_vls_addr(&tclcmd) );
		rt_vls_free( &tclcmd );
	}

	return result;
}


/*
 *	F _ G U I
 *
 *	Puts the necessary hooks in to interface with the MGED Interaction
 *		window.
 */

int
f_gui( argc, argv ) 
int argc;
char **argv;
{
	rt_log("Please direct your attention to the mged interaction window.\n");
	rt_add_hook( gui_output );
	Tcl_CreateCommand(interp, "cmdline", gui_cmdline, (ClientData)NULL,
			  (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(interp, "getknob", getknob, (ClientData)NULL,
			  (Tcl_CmdDeleteProc *)NULL);
	Tcl_CreateCommand(interp, "knob", gui_knob, (ClientData)NULL,
			  (Tcl_CmdDeleteProc *)NULL);
	gui_mode = 1;
	return CMD_OK;
}

#endif


/* 			C M D _ S E T U P
 *
#ifdef MGED_TCL
 * Sets up the TCL interpreter.
#endif
 */ 

void
cmd_setup(interactive)
int	interactive;
{
#ifdef MGED_TCL
        register struct funtab *ftp;
	char	buf[1024];
#endif

	rt_vls_init( &history );
	rt_vls_strcpy( &history, "" );

	rt_vls_init( &replay_history );
	rt_vls_strcpy( &replay_history, "" );

	journalfp = NULL;

#ifdef MGED_TCL

	interp = Tcl_CreateInterp();

	Tcl_CreateCommand(interp, "exit", cmd_wrapper, (ClientData)NULL,
			(Tcl_CmdDeleteProc *)NULL);

	Tcl_SetVar(interp, "tcl_interactive",
		interactive ? "1" : "0", TCL_GLOBAL_ONLY);

	/* This runs the init.tcl script */
	if( Tcl_Init(interp) == TCL_ERROR )
	rt_log("Tcl_Init error %s\n", interp->result);

	/* Screen name should be same as attachment */
	/* This binds in Tk commands.  Do AFTER binding MGED commands */
	tkwin = Tk_CreateMainWindow(interp, NULL, "TkMGED", "tkMGED");
	if (tkwin == NULL)
	rt_log("Error creating Tk window: %s\n", interp->result);

	if (tkwin != NULL) {
		/* XXX HACK! */
		extern void (*extrapoll_hook)();	/* ged.c */
		extern int  extrapoll_fd;		/* ged.c */

		extrapoll_hook = mged_tk_hook;
		extrapoll_fd = Tk_Display(tkwin)->fd;

		Tk_GeometryRequest(tkwin, 200, 200);
#if 0
		Tk_MakeWindowExist(tkwin);
		Tk_MapWindow(tkwin);
#endif
		/* This runs the tk.tcl script */
		if( Tk_Init(interp) == TCL_ERROR )
		rt_log("Tk_init error %s\n", interp->result);
	}

	/* Finally, add in all the MGED commands, if they don't conflict */
        for (ftp = funtab+1; ftp->ft_name != NULL; ftp++)  {
        	sprintf(buf, "info commands %s", ftp->ft_name);
        	if( Tcl_Eval(interp, buf) != TCL_OK ||
		    interp->result[0] != '\0' )  {
	        	rt_log("WARNING:  '%s' name collision (%s)\n", ftp->ft_name, interp->result);
        	}
		Tcl_CreateCommand(interp, ftp->ft_name, cmd_wrapper, 
			(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
        }

	/* Handle any delayed events which result */
	while( Tk_DoOneEvent( TK_DONT_WAIT | TK_ALL_EVENTS ) )  /*NIL*/;

	/* Link to some internal variables */
	Tcl_LinkVar(interp, "mged_center_x", (char *)&toViewcenter[MDX], TCL_LINK_DOUBLE );
	Tcl_LinkVar(interp, "mged_center_y", (char *)&toViewcenter[MDY], TCL_LINK_DOUBLE );
	Tcl_LinkVar(interp, "mged_center_z", (char *)&toViewcenter[MDZ], TCL_LINK_DOUBLE );
#endif
}

/* wrapper for sync() */

int
f_sync(argc, argv)
int argc;
char **argv;
{
	sync();
	return CMD_OK;
}

/*
 *			C M D L I N E
 *
 *  This routine is called to process a vls full of commands.
 *  Each command is newline terminated.
 *  The input string will not be altered in any way.
 *
 *  Returns -
 *	!0	when a prompt needs to be printed.
 *	 0	no prompt needed.
 */

int
cmdline(vp)
struct rt_vls	*vp;
{
	int	i;
	int	need_prompt = 0;
	int	len;
	register char	*cp;
	char		*ep;
	char		*end;
	struct rt_vls	cmd;
	struct rt_vls	cmd_buf;
	struct rt_vls	str;
	int  	result;
	int	argc;
	char 	*argv[MAXARGS+2];
	struct timeval start, finish;

	struct rt_vls	hadd;

	RT_VLS_CHECK(vp);

	if( (len = rt_vls_strlen( vp )) <= 0 )  return 0;
		
	cp = rt_vls_addr( vp );
	end = cp + len;

	rt_vls_init( &cmd );
	rt_vls_init( &cmd_buf );
	rt_vls_init( &str );
	rt_vls_init( &hadd );
	rt_vls_strcpy( &hadd, "" );

	while( cp < end )  {
		ep = strchr( cp, '\n' );
		if( ep == NULL )  break;

		/* Copy one cmd, incl newline.  Null terminate */
		rt_vls_strncpy( &cmd, cp, ep-cp+1 );
		/* parse_line insists on it ending with newline&null */
#ifndef MGED_TCL		
		rt_vls_strcpy( &cmd_buf, rt_vls_addr(&cmd) );
		i = parse_line( rt_vls_addr(&cmd_buf), &argc, argv );
#endif
		while (1) {
			int done;

			done = 0;
#ifdef MGED_TCL
			gettimeofday( &start, NULL );
			result = Tcl_Eval(interp, rt_vls_addr(&cmd));
			gettimeofday( &finish, NULL );

			switch( result ) {
			case TCL_OK:
	/* If it's TCL's OK, then print out the associated return value. */
				if( strcmp(interp->result, "MGED_Ok") != 0
				    && strlen(interp->result) > 0 ) {
					/* Some TCL value to display */
					rt_log("%s\n", interp->result);
				}
				rt_vls_strcpy( &hadd, rt_vls_addr(&cmd) );
				done = 1;
				break;
			case TCL_ERROR:
	/* If it's an MGED error, don't print out an error message. */
				if(strcmp(interp->result, "MGED_Error") == 0){
					rt_vls_printf( &hadd, "# %s",
						rt_vls_addr(&cmd) );
					done = 1;
					break;
				} 

	/* If it's a TCL error, print out the associated error message. */
				if( strcmp(interp->result, "MGED_More") != 0 ){
					rt_vls_printf( &hadd, "# %s",
						rt_vls_addr(&cmd) );
					rt_log("%s\n", interp->result);
					done = 1;
					break;
				}

	/* Fall through to here iff it's MGED_More. */
				done = 0;
				break;

			}
#else
			gettimeofday( &start, NULL );
			result = mged_cmd(argc, argv, funtab);
			gettimeofday( &finish, NULL );
			switch( result ) {
			case CMD_OK:
				rt_vls_strcpy( &hadd, rt_vls_addr(&cmd) );
				done = 1;
				break;
			case CMD_BAD:
				rt_vls_printf( &hadd, "# %s",
					rt_vls_addr(&cmd) );
				done = 1;
				break;
			case CMD_MORE:
				done = 0;
				break;
			}
#endif

	/* Record into history and return if we're all done. */

			if( done ) {
					/* Record non-newline commands */
				if( rt_vls_strlen(&hadd) > 1 )
				    history_record( &hadd, &start, &finish );
				break;
			}

			/* If we get here, it means the command failed due
			   to insufficient arguments.  In this case, grab some
			   more from stdin and call the command again. */

			rt_vls_gets( &str, stdin );

			/* Remove newline */
			rt_vls_trunc( &cmd, rt_vls_strlen(&cmd)-1 );

			rt_vls_strcat( &cmd, " " );
			rt_vls_vlscatzap( &cmd, &str );
			rt_vls_strcat( &cmd, "\n" );
#ifndef MGED_TCL
			rt_vls_strcpy( &cmd_buf, rt_vls_addr(&cmd) );
			i = parse_line( rt_vls_addr(&cmd_buf), &argc, argv );
#endif
			rt_vls_free( &str );
		}
#ifndef MGED_TCL
		if( i < 0 )  continue;	/* some kind of error */
#endif
		need_prompt = 1;

		cp = ep+1;
	}

	rt_vls_free( &cmd );
	rt_vls_free( &cmd_buf );
	rt_vls_free( &str );
	rt_vls_free( &hadd );
	return need_prompt;
}

/*
 *			P A R S E _ L I N E
 *
 * Parse commandline into argument vector
 * Returns nonzero value if input is to be ignored
 * Returns less than zero if there is no input to read.
 */

int
parse_line(line, argcp, argv)
char	*line;
int	*argcp;
char   **argv;
{
	register char *lp;
	register char *lp1;

	(*argcp) = 0;
	lp = &line[0];

	/* Delete leading white space */
	while( (*lp == ' ') || (*lp == '\t'))
		lp++;

	argv[0] = lp;

	if( *lp == '\n' )
		return(1);		/* NOP */

	if( *lp == '#' )
		return 1;		/* NOP -- a comment line */

	/* Handle "!" shell escape char so the shell can parse the line */
	if( *lp == '!' )  {
		(void)system( ++lp);
		rt_log("!\n");
		return(1);		/* Don't process command line! */
	}

	/*  Starting with the first non-whitespace, search ahead for the
	 *  first whitespace (or newline) at the end of each command
	 *  element and turn it into a null.  Then while there is more
	 *  turn it into nulls.  Once the next string is spotted (or
	 *  the of the command line) glob it if necessary and prepare
	 *  for the next command element.
	 */
	for( ; *lp != '\0'; lp++ )  {
		if((*lp == ' ') || (*lp == '\t') || (*lp == '\n'))  {
			*lp = '\0';
			lp1 = lp + 1;
			if((*lp1 == ' ') || (*lp1 == '\t') || (*lp1 == '\n'))
				continue;
			/* If not cmd [0], check for regular exp */
			if( *argcp > 0 )
				(void)cmd_glob(argcp, argv, MAXARGS);
			if( (*argcp)++ >= MAXARGS )  {
				rt_log("More than %d arguments, excess flushed\n", MAXARGS);
				argv[MAXARGS] = (char *)0;
				return(0);
			}
			argv[*argcp] = lp1;
		}
		/* Finally, a non-space char */
	}
	/* Null terminate pointer array */
	argv[*argcp] = (char *)0;
	return(0);
}

/*
 *			M G E D _ C M D
 *
 *  Check a table for the command, check for the correct
 *  minimum and maximum number of arguments, and pass control
 *  to the proper function.  If the number of arguments is
 *  incorrect, print out a short help message.
 */
int
mged_cmd( argc, argv, functions )
int	argc;
char	**argv;
struct funtab *functions;
{
	register struct funtab *ftp;

	if( argc == 0 )  {
		rt_log("no command entered, type '%s?' for help\n",
		    functions->ft_name);
		return CMD_BAD;
	}

	for( ftp = functions+1; ftp->ft_name ; ftp++ )  {
		if( strcmp( ftp->ft_name, argv[0] ) != 0 )
			continue;
		/* We have a match */
		if( (ftp->ft_min <= argc) &&
		    (argc <= ftp->ft_max) )  {
			/* Input has the right number of args.
		    	 * Call function listed in table, with
		    	 * main(argc, argv) style args
		    	 */
			switch (ftp->ft_func(argc, argv)) {
			case CMD_OK:
				return CMD_OK;
			case CMD_BAD:
				return CMD_BAD;
			case CMD_MORE:
				return CMD_MORE;
			default:
				rt_log("mged_cmd(): Invalid return from %s\n",
					ftp->ft_name);
				return CMD_BAD;
			}
		}
		rt_log("Usage: %s%s %s\n\t(%s)\n",functions->ft_name,
		    ftp->ft_name, ftp->ft_parms, ftp->ft_comment);
		return CMD_BAD;
	}
	rt_log("%s%s: no such command, type '%s?' for help\n",
	    functions->ft_name, argv[0], functions->ft_name);
	return CMD_BAD;
}

/* Let the user temporarily escape from the editor */
/* Format: %	*/

int
f_comm( argc, argv )
int	argc;
char	**argv;
{

	register int pid, rpid;
	int retcode;

	(void)signal( SIGINT, SIG_IGN );
	if ( ( pid = fork()) == 0 )  {
		(void)signal( SIGINT, SIG_DFL );
		(void)execl("/bin/sh","-",(char *)NULL);
		perror("/bin/sh");
		mged_finish( 11 );
	}
	while ((rpid = wait(&retcode)) != pid && rpid != -1)
		;
	(void)signal(SIGINT, cur_sigint);
	rt_log("!\n");

	return CMD_OK;  /* ? */
}

/* Quit and exit gracefully */
/* Format: q	*/

int
f_quit( argc, argv )
int	argc;
char	**argv;
{
	if( state != ST_VIEW )
		button( BE_REJECT );
	quit();			/* Exiting time */
	/* NOTREACHED */
}

/*
 *			H E L P C O M M
 *
 *  Common code for help commands
 */

static int
helpcomm( argc, argv, functions)
int	argc;
char	**argv;
struct funtab *functions;
{
	register struct funtab *ftp;
	register int	i, bad;

	bad = 0;
	
	/* Help command(s) */
	for( i=1; i<argc; i++ )  {
		for( ftp = functions+1; ftp->ft_name; ftp++ )  {
			if( strcmp( ftp->ft_name, argv[i] ) != 0 )
				continue;
			rt_log("Usage: %s%s %s\n\t(%s)\n", functions->ft_name,
			    ftp->ft_name, ftp->ft_parms, ftp->ft_comment);
			break;
		}
		if( !ftp->ft_name ) {
			rt_log("%s%s: no such command, type '%s?' for help\n",
			    functions->ft_name, argv[i], functions->ft_name);
			bad = 1;
		}
	}

	return bad ? CMD_BAD : CMD_OK;
}

/*
 *			F _ H E L P
 *
 *  Print a help message, two lines for each command.
 *  Or, help with the indicated commands.
 */
int f_help2();

int
f_help( argc, argv )
int	argc;
char	**argv;
{
#if 0
	/* There needs to be a better way to trigger this */
	if( argc <= 1 )  {
		/* User typed just "help" */
		system("Mosaic http://ftp.arl.mil/ftp/brl-cad/html/mged &");
	}
#endif
	return f_help2(argc, argv, &funtab[0]);
}

int
f_help2(argc, argv, functions)
int argc;
char **argv;
struct funtab *functions;
{
	register struct funtab *ftp;

	if( argc <= 1 )  {
		rt_log("The following commands are available:\n");
		for( ftp = functions+1; ftp->ft_name; ftp++ )  {
			rt_log("%s%s %s\n\t(%s)\n", functions->ft_name,
			    ftp->ft_name, ftp->ft_parms, ftp->ft_comment);
		}
		return CMD_OK;
	}
	return helpcomm( argc, argv, functions );
}

/*
 *			F _ F H E L P
 *
 *  Print a fast help message;  just tabulate the commands available.
 *  Or, help with the indicated commands.
 */
int f_fhelp2();

int
f_fhelp( argc, argv )
int	argc;
char	**argv;
{
	return f_fhelp2(argc, argv, &funtab[0]);
}

int
f_fhelp2( argc, argv, functions)
int	argc;
char	**argv;
struct funtab *functions;
{
	register struct funtab *ftp;

	if( argc <= 1 )  {
		rt_log("The following %scommands are available:\n",
		    functions->ft_name);
		for( ftp = functions+1; ftp->ft_name; ftp++ )  {
			col_item(ftp->ft_name);
		}
		col_eol();
		return CMD_OK;
	}
	return helpcomm( argc, argv, functions );
}

/* Hook for displays with no buttons */
int
f_press( argc, argv )
int	argc;
char	**argv;
{
	register int i;

	for( i = 1; i < argc; i++ )
		press( argv[i] );

	return CMD_OK;
}

int
f_summary( argc, argv )
int	argc;
char	**argv;
{
	register char *cp;
	int flags = 0;
	int bad;

	bad = 0;
	if( argc <= 1 )  {
		dir_summary(0);
		return CMD_OK;
	}
	cp = argv[1];
	while( *cp )  switch( *cp++ )  {
		case 's':
			flags |= DIR_SOLID;
			break;
		case 'r':
			flags |= DIR_REGION;
			break;
		case 'g':
			flags |= DIR_COMB;
			break;
		default:
			rt_log("summary:  S R or G are only valid parmaters\n");
			bad = 1;
			break;
	}
	dir_summary(flags);
	return bad ? CMD_BAD : CMD_OK;
}

/*
 *			S O U R C E _ F I L E
 *
 */

void
mged_source_file(fp)
register FILE	*fp;
{
	struct rt_vls	str;
	int		len;

	rt_vls_init(&str);

	while( (len = rt_vls_gets( &str, fp )) >= 0 )  {
		rt_vls_strcat( &str, "\n" );
		if( len > 0 )  (void)cmdline( &str );
		rt_vls_trunc( &str, 0 );
	}

	rt_vls_free(&str);
}


/*
 *	F _ E C H O
 *
 */

int
f_echo( argc, argv )
int	argc;
char	*argv[];
{
	register int i;

	for( i=1; i < argc; i++ )  {
		rt_log( i==1 ? "%s" : " %s", argv[i] );
	}
	rt_log("\n");

	return CMD_OK;
}

