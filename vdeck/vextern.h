/*
	SCCS id:	@(#) vextern.h	2.4
	Last edit: 	12/20/85 at 19:03:47
	Retrieved: 	6/16/86 at 20:29:50
	SCCS archive:	/vld/src/vdeck/s.vextern.h

	Author:		Gary S. Moss
			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
#include <math.h>
#include <setjmp.h>

				/* Machine.h use to be here
				 * and seemed to break everything 
				 * for vdeck since it wanted 
				 * to use only floats so here is
				 * a quick fix to make things work
				 * since we should really not be 
			 	 * supporting this anyways.
				 */

typedef float	fastf_t;	/* double|float, "Fastest" float type */
#define LOCAL	static		/* static|auto, for serial|parallel cpu */
#define FAST	LOCAL		/* LOCAL|register, for fastest floats */
typedef long	bitv_t;		/* largest integer type */
#define BITV_SHIFT	5	/* log2( bits_wide(bitv_t) ) */

#include "vmath.h"
#include "db.h"

/* Special characters.							*/
#define	LF		"\n"
#define BLANKS	"                                                                          "

/* Colors.								*/
#define RED	'1'
#define GREEN	'2'
#define YELLOW	'3'
#define BLUE	'4'
#define MAGENTA	'5'
#define CYAN	'6'
#define WHITE	'7'

/* Command line options.						*/
#define DECK		'd'
#define ERASE		'e'
#define INSERT		'i'
#define LIST		'l'
#define MENU		'?'
#define NUMBER		'n'
#define QUIT		'q'
#define REMOVE		'r'
#define RETURN		'\0'
#define SHELL		'!'
#define SORT_TOC	's'
#define TOC		't'
#define UNKNOWN		default

/* Prompts.								*/
#define CMD_PROMPT	"\ncommand( ? for menu )>> "
#define LST_PROMPT	"\nSOLIDS LIST( ? for menu )> "
#define PROMPT		"vdeck> "

/* Size limits.								*/
#define MAXLN	80	/* max length of input line */
#define MAXRR	100	/* max regions to remember */
#define MAXSOL	4000	/* max solids in description */
#define NDIR	9000	/* max objects in input */
#define MAXPATH	32	/* max level of hierarchy */
#define MAXARG	20	/* max arguments on command line */
#define ARGSZ	32	/* max length of command line argument */

/* Standard flag settings.						*/
#define UP	0
#define DOWN	1
#define QUIET	0
#define NOISY	1
#define UPP     1
#define DWN     0
#define YES	1
#define NO	0
#define ON	1
#define OFF	0

/* Output vector fields.						*/
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

/* For solid parameter manipulation.					*/
#define SV0	&(rec->s.s_values[0])
#define	SV1	&(rec->s.s_values[3])
#define SV2     &(rec->s.s_values[6])
#define SV3	&(rec->s.s_values[9])
#define SV4     &(rec->s.s_values[12])
#define SV5     &(rec->s.s_values[15])
#define SV6     &(rec->s.s_values[18])
#define SV7     &(rec->s.s_values[21])

/* Dot product of vector a and vector b.				*/
#define MAX( a, b )		if( (a) < (b) )		a = b
#define MIN( a, b )		if( (a) > (b) )		a = b
#define MINMAX( min, max, val )		MIN( min, val );\
				else	MAX( max, val )

/* Object directory structure.						*/
typedef struct directory {
	char	*d_namep;	/* pointer to name string */
	long	d_addr;		/* disk address in object file */
} Directory;
#define	DIR_NULL	((Directory *)NULL)

/* Region names to find comgeom numbers for.				*/
struct findrr {
	char rr_name[16];    /* name to find comgeom number for */
	long rr_pos;         /* position in regfd to add the comgeom # */
};

/* Identification structure for final discrimination of solids.		*/
struct deck_ident {
	short	i_index;
	char	i_name[16];
	mat_t	i_mat;    /* homogeneous transformation matrix */
};

extern double	unit_conversion;
extern int	debug;
extern char	*usage[], *cmd[];
extern mat_t	xform, notrans, identity;

typedef union record Record;
extern Record record;
extern struct findrr findrr[];

extern char	dir_names[], *dir_last;

extern int	discr[];

extern char		*strcpy(), *strcat(), *mktemp();
extern char		*emalloc();
extern long		lseek();
extern void		abort_sig(), quit();
extern Directory	*lookup(), *diradd();
extern void		toc(), list_toc();
extern void		prompt();
extern void		exit(), free(), perror();

extern char	*toc_list[];
extern char	*curr_list[];
extern int	curr_ct;
extern char	*arg_list[];
extern int	arg_ct;
extern char	*tmp_list[];
extern int	tmp_ct;

extern int	objfd;		extern char	*objfile;
extern int	solfd;		extern char	st_file[];
extern int	regfd;		extern char	rt_file[];
extern int	ridfd;		extern char	id_file[];
extern int	idfd, rd_idfd;	extern char	disc_file[];
extern int	rrfd, rd_rrfd;	extern char	reg_file[];

extern int	ndir, nns, nnr, numrr;

extern int			regflag, orflag, delsol, delreg;
extern int			isave;
extern char			buff[],	name[];
extern char			operate;
extern long			savsol;
extern struct deck_ident	d_ident, idbuf;

extern jmp_buf		env;
#define EPSILON		0.0001
#define CONV_EPSILON	0.01

#ifdef BSD
#define strchr(a,b)	index(a,b)
#define strrchr(a,b)	rindex(a,b)
extern char *index(), *rindex();
#endif
