/*
	Authors:	Gary S. Moss
			Jeff Hanes	(math consultation)

			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
/*
	Originally extracted from SCCS archive:
		SCCS id:	@(#) extern.h	2.2
		Modified: 	1/30/87 at 17:22:09	G S M
		Retrieved: 	2/4/87 at 08:53:22
		SCCS archive:	/vld/moss/src/lgt/s.extern.h
*/
#if ! defined(FAST)
#include "machine.h"
#endif

#if ! defined(INCL_FB)
#include "fb.h"
#endif

/* Functions.								*/
#if defined( BSD )
extern char		*tmpnam(), *gets(), *strtok();
#endif
#if defined( BSD ) || defined( sgi )
extern int		(*norml_sig)(), (*abort_sig)();
extern int		abort_RT();
#else
extern void		(*norml_sig)(), (*abort_sig)();
extern void		abort_RT();
#endif
extern char		*getenv();
extern char		*malloc();
extern char		*sbrk();
extern char		*strncpy();

extern fastf_t		pow_Of_2();

extern int		do_IR_Model(), do_IR_Backgr();
extern int		pars_Argv();
extern int		get_Answer();
extern int		fb_Setup();
extern int		texture_Val();

extern void		append_Octp();
extern void		close_Output_Device();
extern void		cons_Vector();
extern void		delete_OcList();
extern void		display_Temps();
extern void		do_line();
extern void		exit();
extern void		exit_Neatly();
extern void		free();
extern void		fb_Zoom_Window();
extern void		grid_Rotate();
extern void		loc_Perror();
extern void		perror();
extern void		prnt_Event();
extern void		prnt_IR_Status();
extern void		prnt_Lgt_Status();
extern void		prnt_Menu();
extern void		prnt_Octree();
extern void		prnt_Pixel();
extern void		prnt_Prompt();
extern void		prnt_Scroll();
extern void		prnt_Status();
extern void		prnt_Timer();
extern void		prnt_Title();
extern void		prnt_Trie();
extern void		prnt_Usage();
extern void		prnt3vec();
extern void		render_Model();
extern void		ring_Bell();
extern void		rt_log();
extern void		user_Interaction();

/* Variables.								*/
extern FBIO		*fbiop;

extern RGBpixel		bgpixel;
extern RGBpixel		*ir_table;

extern char		*CS, *DL;
extern char		*beginptr;
extern char		*ged_file;

extern char		err_file[];
extern char		fb_file[];
extern char		ir_file[];
extern char		ir_db_file[];
extern char		lgt_db_file[];
extern char		mat_db_file[];
extern char		script_file[];
extern char		texture_file[];
extern char		title[];
extern char		timer[];
extern char		version[];

extern fastf_t		bg_coefs[];
extern fastf_t		cell_sz;
extern fastf_t		degtorad;
extern fastf_t		grid_dist;
extern fastf_t		grid_hor[];
extern fastf_t		grid_ver[];
extern fastf_t		grid_loc[];
extern fastf_t		grid_scale;
extern fastf_t		grid_roll;
extern fastf_t		modl_cntr[];
extern fastf_t		modl_radius;
extern fastf_t		x_grid_offset, y_grid_offset;
extern fastf_t		rel_perspective;
extern fastf_t		sample_sz;
extern fastf_t		view2model[];
extern fastf_t		view_size;
extern int		LI, CO;
extern int		anti_aliasing;
extern int		aperture_sz;
extern int		background[];
extern int		co;
extern int		debug;
extern int		errno;
extern int		fatal_error;
extern int		fb_mapping;
extern int		fb_width;
extern int		fb_ulen, fb_vlen;
extern int		grid_dist_flag;
extern int		grid_sz;
extern int		grid_x_org, grid_y_org;
extern int		grid_x_fin, grid_y_fin;
extern int		grid_x_cur, grid_y_cur;
extern int		icon_mapping;
extern int		ir_offset;
extern int		ir_min, ir_max;
extern int		ir_paint;
extern int		ir_paint_flag;
extern int		ir_mapx, ir_mapy;
extern int		ir_noise;
extern int		ir_mapping;
extern int		lgt_db_size;
extern int		li;
extern int		max_bounce;
extern int		nprocessors;
#ifdef PARALLEL
extern int		npsw;
#endif
extern int		pix_buffered;
extern int		save_view_flag;
extern int		tracking_cursor;
extern int		tty;
extern int		user_interrupt;
extern int		x_fb_origin, y_fb_origin;
extern int		zoom;

extern unsigned char	arrowcursor[];
extern unsigned char	sweeportrack[];
extern unsigned char	arrowcursor[];
extern unsigned char	target1[];

#ifdef PARALLEL
extern struct resource	resource[];
#endif
extern struct rt_i	*rt_ip;

#ifdef sgi
extern int	win_active;
extern long	defpup(), qtest();
#define WIN_L	(1023-511-4)
#define WIN_R	(1023-4)
#define WIN_B	4
#define WIN_T	(511+4)
#define	SGI_XCVT( v_ ) (((v_) - xwin) / (fbiop->if_width/grid_sz))
#define SGI_YCVT( v_ ) (((v_) - ywin) / (fbiop->if_width/grid_sz))
#else
#define	SGI_XCVT( v_ ) (v_)
#define SGI_YCVT( v_ ) (v_)
#endif
