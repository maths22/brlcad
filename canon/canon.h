#define	IPU_LUN_SCANNER		0x0
#define	IPU_LUN_FILM_SCANNER	0x1
#define	IPU_LUN_ANALOG_INPUT	0x2
#define	IPU_LUN_PRINTER		0x3
#define	IPU_LUN_ANALOG_OUTPUT	0x4

#define IPU_RGB_FILE	2
#define IPU_BITMAP_FILE	0
#define IPU_PALETTE_FILE 3

#define IPU_UNITS_INCH	'\0'
#define IPU_UNITS_MM	'\1'

#define IPU_AUTOSCALE	0
#define IPU_AUTOSCALE_IND	1
#define IPU_MAG_FACTOR	2
#define IPU_RESOLUTION	3

#define IPU_GAMMA_STANDARD	0
#define IPU_GAMMA_RGB		1
#define IPU_GAMMA_CG		2

#define IPU_UPPER_CASSETTE	0
#define IPU_LOWER_CASSETTE	1
#define IPU_MANUAL_FEED		128

#define	IPU_MAX_FILES	17	/* 16 image/palette + 1 bit-mapped */

union ipu_prsc_param {
	char	c[4];
	int	i;
	short	s[2];
};


extern int	ipu_debug;
#ifdef __sgi
#include <dslib.h>
extern int	ipu_not_ready(struct dsreq *dsp);
extern char	*ipu_inquire(struct dsreq *dsp);
extern int	ipu_remote(struct dsreq *dsp);
extern void	ipu_create_file(struct dsreq *dsp,u_char id,u_char type,int width,int height,char clear);
extern void	ipu_delete_file(struct dsreq *dsp, u_char id);
extern u_char	*ipu_get_image(struct dsreq *dsp,char id,int sx,int sy,int w,int h);
extern void	ipu_put_image(struct dsreq *dsp,char id,int w,int h,u_char *img);
extern void	ipu_print_config(struct dsreq *dsp,char units,int divisor,u_char conv,u_char mosaic,u_char gamma,int tray);
extern void	ipu_print_file(struct dsreq *dsp,char id,int copies,int wait,int sx,int sy,int sw,int sh,union ipu_prsc_param *param);
extern char	*ipu_list_files(struct dsreq *dsp);
extern int	ipu_stop(struct dsreq *dsp,int halt);
extern void	ipu_scan_file(struct dsreq *dsp,char id,char wait,int sx,int sy,int w,int h,union ipu_prsc_param *param);
extern void	ipu_scan_config(struct dsreq *dsp,char units,int divisor,char conv,char field,short rotation);
#endif
#if __stdc__
extern int	parse_args(int ac, char *av[]);
extern void	usage(char *s);
#else
extern int	parse_args();
extern void	usage();
#endif
extern char *progname;
extern char *scsi_device;
extern char gamma;
extern char tray;
extern char conv;
extern char clear;
extern int width;
extern int height;
extern int zoom;
extern int scr_width;
extern int scr_height;
extern int scr_xoff;
extern int scr_yoff;
extern int copies;
extern int autosize;
extern int units;
extern int divisor;
extern int mosaic;
extern union ipu_prsc_param param;
extern char *arg_v[];
extern int arg_c;
extern char *print_queue;
