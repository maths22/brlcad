/*
	Authors:	Paul R. Stay
			Gary S. Moss

			U. S. Army Ballistic Research Laboratory
			Aberdeen Proving Ground
			Maryland 21005-5066
			(301)278-6647 or AV-298-6647
*/
/*
	Originally extracted from SCCS archive:
		SCCS id:	@(#) font.h	2.1
		Modified: 	12/10/86 at 16:04:33	G S M
		Retrieved: 	2/4/87 at 08:53:58
		SCCS archive:	/vld/moss/src/lgt/s.font.h
*/
/*	font.h - Header file for putting fonts up.			*/
#define INCL_FONT
#if defined(sel) || defined(gould) || defined(alliant)
#define BIGENDIAN
#endif
#if defined(BIGENDIAN)
#define SWAB(shrt)	(shrt=(((shrt)>>8) & 0xff) | (((shrt)<<8) & 0xff00))
#define SWABV(shrt)	((((shrt)>>8) & 0xff) | (((shrt)<<8) & 0xff00))
#else
#define	SWAB(shrt)
#define SWABV(shrt)	(shrt)
#endif

/*	vfont.h	4.1	83/05/03 from 4.2 Berkley			*/
/* The structures header and dispatch define the format of a font file.	*/
struct header {
	short		magic;
	unsigned short	size;
	short		maxx;
	short		maxy;
	short		xtend;
}; 
struct dispatch
	{
	unsigned short	addr;
	short		nbytes;
	char		up, down, left, right;
	short		width;
	};
#define FONTDIR		"/vld/moss/src/lgt"	/* Font directory.	*/
#define FONTNAME	"times.r.6"		/* Default font name.	*/
#define FONTNAMESZ	128
#define FONTCOLOR_RED	0.0
#define FONTCOLOR_GRN	0.0
#define FONTCOLOR_BLU	0.0

/* Variables controlling the font itself.				*/
extern FILE *ffdes;		/* Fontfile file descriptor.		*/
extern int offset;		/* Offset to data in the file.		*/
extern struct header hdr;	/* Header for font file.		*/
extern struct dispatch dir[256];/* Directory for character font.	*/
extern int width, height;	/* Width and height of current char.	*/
