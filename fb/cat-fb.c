/*
 *			C A T - F B . C
 *
 *  Simulate C/A/T phototypesetter on a framebuffer.
 *  Common usage is troff -t files | cat-fb
 *
 *  Operation is one-pass, top to bottom, so vertical motion is
 *  limited to NLINES scanlines.
 *
 *  Fonts are operating at 200 dpi, giving this correspondence between
 *  screen pixel widths and troff line lengths:
 *	 512	2.56i
 *	1024	5.12i	(4.7i)
 *	1280	6.4i	(6.0i)
 *
 *  Authors -
 *	Ronald B. Natalie
 *	Michael John Muuss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>

#include "fb.h"
#include "vfont-if.h"

#define NFONTS			25	/* # of font+psize combos in cache */
#define SPECIALFONT		3
#define MAXF			4	/* Max fonts mounted in C/A/T -- a constant */

#define LOCAL_RAILMAG		".railmag"
#define GLOBAL_RAILMAG		"/usr/lib/vfont/railmag"

/* The vfonts are scaled for 200 dpi */
#define CONVERT(n)		(n*(200./432.))
#define RECONVERT(n)		(n*(432./200.))


RGBpixel	*scanline;
FBIO		*fbp;

RGBpixel	writing_color = {255, 255, 255};

/* Single-bit wide typesetting buffer.
 * NLINES limits maximum backward vertical motion (eg, in tables).
 */
int	bytes_per_line;		/* # bytes of bitmap per line */
#define NLINES			1024
#define BUFFER_SIZE		(NLINES*1280/8)

char	buffer[BUFFER_SIZE];	/* Big bitmap line buffers  */
char	*buf0p = &buffer[0];	/* Zero origin in circular buffer  */

struct	fontdes {
	int	fnum;
	int	psize;
	struct vfont	*vfp;
} fontdes[NFONTS] = {
	{ -1, 10, VFONT_NULL },
	{ -1, 10, VFONT_NULL },
};

struct point_sizes {
	int	stupid_code;
	int	real_code;
} point_sizes[] = {
	010, 6,
	0, 7,
	01, 8,
	07, 9,
	02, 10,
	03, 11,
	04, 12,
	05, 14,
	0211, 16,
	06, 18,
	0212, 20,
	0213, 22,
	0214, 24,
	0215, 28,
	0216, 36,
	0, 0
};

int	cur_fb_line;		/*  Output absolute scanline number  */

int	cfont = 1;		/* current font index in fontdes[] */
int	new_font_num = -1;
int	fontwanted = 1;
int	new_pt_size = 10;		/* new point size */
int	last_ssize = 02;
int	xpos, ypos;
int	esc, lead, back, verd, mcase, railmag;
double	row, col;		/* position in phototypesetter units */
char	*fontname[MAXF];
char	fnbuf[120];

char	asctab[128] = {
	'\0',	/*blank*/
	'h',	/*h*/
	't',	/*t*/
	'n',	/*n*/
	'm',	/*m*/
	'l',	/*l*/
	'i',	/*i*/
	'z',	/*z*/
	's',	/*s*/
	'd',	/*d*/
	'b',	/*b*/
	'x',	/*x*/
	'f',	/*f*/
	'j',	/*j*/
	'u',	/*u*/
	'k',	/*k*/
	'\0',	/*blank*/
	'p',	/*p*/
	'\06',	/*_ 3/4 em dash*/
	';',	/*;*/
	'\0',	/*blank*/
	'a',	/*a*/
	'\05',	/*rule*/
	'c',	/*c*/
	'`',	/*` open*/
	'e',	/*e*/
	'\'',	/*' close*/
	'o',	/*o*/
	'\021',	/*1/4*/
	'r',	/*r*/
	'\022',	/*1/2*/
	'v',	/*v*/
	'-',	/*- hyphen*/
	'w',	/*w*/
	'q',	/*q*/
	'/',	/*/*/
	'.',	/*.*/
	'g',	/*g*/
	'\023',	/*3/4*/
	',',	/*,*/
	'&',	/*&*/
	'y',	/*y*/
	'\0',	/*blank*/
	'%',	/*%*/
	'\0',	/*blank*/
	'Q',	/*Q*/
	'T',	/*T*/
	'O',	/*O*/
	'H',	/*H*/
	'N',	/*N*/
	'M',	/*M*/
	'L',	/*L*/
	'R',	/*R*/
	'G',	/*G*/
	'I',	/*I*/
	'P',	/*P*/
	'C',	/*C*/
	'V',	/*V*/
	'E',	/*E*/
	'Z',	/*Z*/
	'D',	/*D*/
	'B',	/*B*/
	'S',	/*S*/
	'Y',	/*Y*/
	'\0',	/*blank*/
	'F',	/*F*/
	'X',	/*X*/
	'A',	/*A*/
	'W',	/*W*/
	'J',	/*J*/
	'U',	/*U*/
	'K',	/*K*/
	'0',	/*0*/
	'1',	/*1*/
	'2',	/*2*/
	'3',	/*3*/
	'4',	/*4*/
	'5',	/*5*/
	'6',	/*6*/
	'7',	/*7*/
	'8',	/*8*/
	'9',	/*9*/
	'*',	/***/
	'\04',	/*minus*/
	'\01',	/*fi*/
	'\02',	/*fl*/
	'\03',	/*ff*/
	'\020',	/* cent sign */
	'\012',	/*ffl*/
	'\011',	/*ffi*/
	'(',	/*(*/
	')',	/*)*/
	'[',	/*[*/
	']',	/*]*/
	'\013',	/* degree */
	'\014',	/* dagger */
	'=',	/*=*/
	'\017',	/* registered */
	':',	/*:*/
	'+',	/*+*/
	'\0',	/*blank*/
	'!',	/*!*/
	'\07',	/* bullet */
	'?',	/*?*/
	'\015',	/*foot mark*/
	'|',	/*|*/
	'\0',	/*blank*/
	'\016',	/* copyright */
	'\010',	/* square */
	'$',	/*$*/
	'\0',
	'\0',
	'"',	/*"*/
	'#',	/*#*/
	'<',	/*<*/
	'>',	/*>*/
	'@',	/*@*/
	'\\',	/*\\*/
	'^',	/*^*/
	'{',	/*{*/
	'}',	/*}*/
	'~'	/*~*/
};

char spectab[128] = {
	'\0',	/*blank*/
	'w',	/*psi*/
	'h',	/*theta*/
	'm',	/*nu*/
	'l',	/*mu*/
	'k',	/*lambda*/
	'i',	/*iota*/
	'f',	/*zeta*/
	'r',	/*sigma*/
	'd',	/*delta*/
	'b',	/*beta*/
	'n',	/*xi*/
	'g',	/*eta*/
	'u',	/*phi*/
	't',	/*upsilon*/
	'j',	/*kappa*/
	'\0',	/*blank*/
	'p',	/*pi*/
	'@',	/*at-sign*/
	'7',	/*down arrow*/
	'\0',	/*blank*/
	'a',	/*alpha*/
	'|',	/*or*/
	'v',	/*chi*/
	'"',	/*"*/
	'e',	/*epsilon*/
	'=',	/*=*/
	'o',	/*omicron*/
	'4',	/*left arrow*/
	'q',	/*rho*/
	'6',	/*up arrow*/
	's',	/*tau*/
	'_',	/*underrule*/
	'\\',	/*\*/
	'W',	/*Psi*/
	'\07',	/*bell system sign*/
	'\001',	/*infinity*/
	'c',	/*gamma*/
	'\002',	/*improper superset*/
	'\003',	/*proportional to*/
	'\004',	/*right hand*/
	'x',	/*omega*/
	'\0',	/*blank*/
	'(',	/*gradient*/
	'\0',	/*blank*/
	'U',	/*Phi*/
	'H',	/*Theta*/
	'X',	/*Omega*/
	'\005',	/*cup (union)*/
	'\006',	/*root en*/
	'\014',	/*terminal sigma*/
	'K',	/*Lambda*/
	'-',	/*minus*/
	'C',	/*Gamma*/
	'\015',	/*integral sign*/
	'P',	/*Pi*/
	'\032',	/*subset of*/
	'\033',	/*superset of*/
	'2',	/*approximates*/
	'y',	/*partial derivative*/
	'D',	/*Delta*/
	'\013',	/*square root*/
	'R',	/*Sigma*/
	'1',	/*approx =*/
	'\0',	/*blank*/
	'>',	/*>*/
	'N',	/*Xi*/
	'<',	/*<*/
	'\016',	/*slash (longer)*/
	'\034',	/*cap (intersection)*/
	'T',	/*Upsilon*/
	'\035',	/*not*/
	'\023',	/*right ceiling (rt of ")*/
	'\024',	/*left top (of big curly)*/
	'\017',	/*bold vertical*/
	'\030',	/*left center of big curly bracket*/
	'\025',	/*left bottom*/
	'\026',	/*right top*/
	'\031',	/*right center of big curly bracket*/
	'\027',	/*right bot*/
	'\021',	/*right floor (rb of ")*/
	'\020',	/*left floor (left bot of big sq bract)*/
	'\022',	/*left ceiling (lt of ")*/
	'*',	/*multiply*/
	'/',	/*divide*/
	'\010',	/*plus-minus*/
	'\011',	/*<=*/
	'\012',	/*>=*/
	'0',	/*identically equal*/
	'3',	/*not equal*/
	'{',	/*{*/
	'}',	/*}*/
	'\'',	/*' acute accent*/
	'\`',	/*` grave accent*/
	'^',	/*^*/
	'#',	/*sharp*/
	'\036',	/*left hand*/
	'\037',	/*member of*/
	'~',	/*~*/
	'z',	/*empty set*/
	'\0',	/*blank*/
	'Y',	/*dbl dagger*/
	'Z',	/*box rule*/
	'9',	/*asterisk*/
	'[',	/*improper subset*/
	']',	/*circle*/
	'\0',	/*blank*/
	'+',	/*eqn plus*/
	'5',	/*right arrow*/
	'8'	/*section mark*/
};

extern char	*malloc();

extern int	getopt();
extern char	*optarg;
extern int	optind;


static char	*framebuffer = NULL;

static int	scr_width = 0;		/* use device default size */
static int	scr_height = 0;
static int	clear = 0;		/* clear screen before writing */
static int	overlay = 0;		/* overlay FROM STDIN rather than fb */
static int	debug = 0;

static char usage[] = "\
Usage: cat-fb [-h -c -O] [-F framebuffer] [-C r/g/b]\n\
	[-S squarescrsize] [-W scr_width] [-N scr_height] [troff_files]\n";

get_args( argc, argv )
register char **argv;
{
	register int c;

	while ( (c = getopt( argc, argv, "dhcOF:S:W:N:C:" )) != EOF )  {
		switch( c )  {
		case 'd':
			debug = 1;
			break;
		case 'O':
			overlay = 1;
			break;
		case 'h':
			/* high-res */
			scr_height = scr_width = 1024;
			break;
		case 'c':
			clear = 1;
			break;
		case 'F':
			framebuffer = optarg;
			break;
		case 'S':
			scr_height = scr_width = atoi(optarg);
			break;
		case 'W':
			scr_width = atoi(optarg);
			break;
		case 'N':
			scr_height = atoi(optarg);
			break;
		case 'C':
			{
				register char *cp = optarg;
				register unsigned char *conp = writing_color;

				/* premature null => atoi gives zeros */
				for( c=0; c < 3; c++ )  {
					*conp++ = atoi(cp);
					while( *cp && *cp++ != '/' ) ;
				}
			}
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( optind >= argc )  {
		/* No file name args */
		if( isatty(fileno(stdin)) )
			return(0);
		/* mainline will actually do the processing */
	}
	return(1);		/* OK */
}

main(argc, argv) 
	int argc;
	char *argv[];
{
	char	*cp;

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	if( (fbp = fb_open( framebuffer, scr_width, scr_height )) == FBIO_NULL )
		exit(1);
	scr_width = fb_getwidth(fbp);
	scr_height = fb_getheight(fbp);
	if( (cp = malloc(scr_width*sizeof(RGBpixel))) == (char *)0 )
		exit(42);
	scanline = (RGBpixel *)cp;
	bytes_per_line = (scr_width+7)/8;

	cur_fb_line = scr_height-1;	/* start at top of screen */

	readrailmag();

	if( optind >= argc )  {
		/* Process one TROFF file from stdin */
		if( overlay )  {
			fprintf(stderr,"cat-fb: -O ignored, stdin used for C/A/T code\n");
			overlay = 0;
		}
		ofile(stdin);
	} else {
		for( ; optind < argc; optind++ )  {
			register FILE *fp;
			if( (fp = fopen(argv[optind], "r")) == NULL )  {
				perror(argv[optind]);
				continue;
			}
			ofile(fp);
			fclose(fp);
		}
	}
	slop_lines(NLINES);		/* Flush bitmap buffer */
	fb_close(fbp);
	exit(0);
}

readrailmag()
{
	register int i;
	register char *cp;
	register int rmfd;
	char c;

	if ((rmfd = open(LOCAL_RAILMAG, 0)) < 0)
		if ((rmfd = open(GLOBAL_RAILMAG, 0)) < 0) {
			/* Defaults */
			fontname[0] = "R";
			fontname[1] = "I";
			fontname[2] = "B";
			fontname[3] = "S";
			return;
		}
	cp = fnbuf;
	for (i = 0; i < 4; i++) {
		fontname[i] = cp;
		while (read(rmfd, &c, 1) == 1 && c != '\n')
			*cp++ = c;
		*cp++ = '\0';
	}
	close(rmfd);
}

ofile(fp)
register FILE	*fp;
{
	register int c;
	double scol;
	static int initialized;

	while ((c = getc(fp)) != EOF) {
		if (!c)
			continue;
		if (c & 0200) {
			esc += (~c) & 0177;
			continue;
		}
		if (esc) {
			if (back)
				esc = -esc;
			col += esc;
			ypos = CONVERT(col);
			esc = 0;
		}
		if ((c & 0377) < 0100)	/*  Purely for efficiency  */
			goto normal_char;
		switch (c) {

		case 0100:
			if (initialized)
				goto out;
			initialized = 1;
/**			row = 25;	/** original value **/
			row = -108;	/* ignore 1/4 C/A/T inch header */
			xpos = CONVERT(row);
			for (c = 0; c < BUFFER_SIZE; c++)
				buffer[c] = 0;
			col = 0;
			esc = 0;
			lead = 0;
			ypos = 0;
			verd = 0;
			back = 0;
			mcase = 0;
			railmag = 0;
			if (loadfont(railmag, fontdes[cfont].psize) < 0)
				fprintf(stderr, "Can't load inital font\n");
			break;

		case 0101:	/* lower rail */
			crail(railmag &= ~01);
			break;

		case 0102:	/* upper rail */
			crail(railmag |= 01);
			break;

		case 0103:	/* upper mag */
			crail(railmag |= 02);
			break;

		case 0104:	/* lower mag */
			crail(railmag &= ~02);
			break;

		case 0105:	/* lower case */
			mcase = 0;
			break;

		case 0106:	/* upper case */
			mcase = 0100;
			break;

		case 0107:	/* escape forward */
			back = 0;
			break;

		case 0110:	/* escape backwards */
			back = 1;
			break;

		case 0111:	/* stop */
			break;

		case 0112:	/* lead forward */
			verd = 0;
			break;

		case 0113:	/* undefined */
			break;

		case 0114:	/* lead backward */
			verd = 1;
			break;

		case 0115:	/* undefined */
		case 0116:
		case 0117:
			break;

		default:
			if ((c & 0340) == 0140)	/* leading */ {
				lead = (~c) & 037;
				if (verd)
					lead = -lead;
				row += lead*3;	/*  Lead is 3 units  */
				c = CONVERT(row);
				while (c >= NLINES) {
					slop_lines(15);
					c = CONVERT(row);
				}
				xpos = c;
				if(debug)fprintf(stderr,"v=%d (%f)\n", xpos, row);
				continue;
			}
			if ((c & 0360) == 0120)	/* size change */ {
				loadfont(railmag, findsize(c & 017));
				continue;
			}
			if (c & 0300)
				continue;

normal_char:
			c = (c & 077) | mcase;
			outc(c);
		}
	}
out:
	return;
}

findsize(code)
	register int code;
{
	register struct point_sizes *psp;

	psp = point_sizes;
	while (psp->real_code != 0) {
		if ((psp->stupid_code & 017) == code)
			break;
		psp++;
	}
	code = 0;
	if (!(last_ssize & 0200) && (psp->stupid_code & 0200))
		code = -55;
	else if ((last_ssize & 0200) && !(psp->stupid_code & 0200))
		code = 55;
	if (back)
		code = -code;
	esc += code;
	last_ssize = psp->stupid_code;
	return (psp->real_code);
}

crail(nrail)
	register int nrail;
{
	register int psize;

	psize = fontdes[cfont].psize;
	if (fontwanted)
		psize = new_pt_size;
	loadfont(nrail, psize);
}


/* Queue up a request to change to a new font */
loadfont(fnum, size)
register int fnum;
register int size;
{
	register int i;
	char cbuf[80];

	fontwanted = 0;
	if (fnum < 0 || fnum >= MAXF) {
		fprintf(stderr, "Internal error: illegal font\n");
		return(-1);
	}
	if (fnum == fontdes[cfont].fnum && size == fontdes[cfont].psize)
		return(0);
	for (i = 0; i < NFONTS; i++)
		if (fontdes[i].fnum == fnum && fontdes[i].psize == size) {
			cfont = i;
			return (0);
		}
	new_font_num = fnum;
	new_pt_size = size;
	fontwanted++;
	return (0);
}


readinfont()
{
	register struct vfont	*vfp;
	register int fnum, size, font;
	char cbuf[BUFSIZ];

	fnum = new_font_num;
	size = new_pt_size;
	sprintf(cbuf, "%s.%d", fontname[fnum], size);

	if( (vfp = vfont_get( cbuf )) == VFONT_NULL )  {
		fontwanted = 0;
		return (-1);
	}

	cfont = relfont();

	fontdes[cfont].fnum = fnum;
	fontdes[cfont].psize = fontdes[cfont].psize = size;
	fontdes[cfont].vfp = vfp;
	if(debug) fprintf(stderr,"slot %d = %s\n", cfont, cbuf );

	fontwanted = 0;
	new_font_num = new_pt_size = -1;
	return (0);
}

int lastloaded	= -1;

relfont()
{
	register int newfont;

	/* First, see if a font table slot happens to be availible */
	for( newfont = 0; newfont < NFONTS; newfont++ )  {
		if( fontdes[newfont].psize > 0 )
			continue;
		return(newfont);
	}

	/* Reuse an existing slot, with Ron's strange heuristic */

	newfont = lastloaded;
	/*
	 * optimization for special font.  since we think that usually
	 * there is only one character at a time from any special math
	 * font, make it the candidate for removal.
	 */
	if (fontdes[cfont].fnum != SPECIALFONT || fontdes[cfont].vfp==VFONT_NULL)
		if (++newfont>=NFONTS)
			newfont = 0;
	lastloaded = newfont;
	if( fontdes[newfont].vfp != VFONT_NULL )
		vfont_free(fontdes[newfont].vfp);
		
	fontdes[newfont].vfp = VFONT_NULL;
	fontdes[newfont].fnum = -1;
	fontdes[newfont].psize = -1;
	return (newfont);
}

outc(code)
	int code;
{
	char c;				/* character to print */
	register struct vfont_dispatch *vdp;	/* ptr to character font record */
	register unsigned char *addr;	/* addr of font data */
	int llen;			/* length of each font line */
	int nlines;			/* number of font lines */
	register unsigned char *scanp;	/* ptr to output buffer */
	int scanp_inc;			/* increment to start of next buffer */
	int offset;			/* bit offset to start of font data */
	int i;				/* loop counter */
	register int count;		/* font data ptr */
	register unsigned fontdata;	/* font data temporary */
	register int off8;		/* offset + 8 */

	if (fontwanted)
		readinfont();
	if (railmag == SPECIALFONT) {
		if ((c = spectab[code]) < 0)
			return(0);
	} else if ((c = asctab[code]) < 0)
		return(0);
	vdp = &fontdes[cfont].vfp->vf_dispatch[c];

	if (vdp->vd_nbytes <= 0 )
		return(0);

	/* xpos is vertical (typ. called Y), ypos is horizontal (typ. X)
	 * like a strip-chart recorder */
	if(debug)fprintf(stderr,"%c h=%d v=%d  l=%d r=%d  u=%d d=%d  w=%d\n",
		c, ypos, xpos, vdp->vd_left, vdp->vd_right,
		vdp->vd_up, vdp->vd_down, vdp->vd_width);
	addr = &fontdes[cfont].vfp->vf_bits[vdp->vd_addr];
	llen = (vdp->vd_left + vdp->vd_right+7)/8;
	nlines = vdp->vd_up + vdp->vd_down;
	if (xpos+vdp->vd_down >= NLINES)
		slop_lines(xpos + vdp->vd_down - NLINES + 1);
	scanp = ( (xpos-vdp->vd_up-1)*bytes_per_line+
		(ypos-vdp->vd_left)/8 ) + buf0p;
	if (scanp < &buffer[0])
		scanp += BUFFER_SIZE;
	scanp_inc = bytes_per_line-llen;
	offset = (ypos-vdp->vd_left)&07;
	off8 = 8-offset;
	for (i = 0; i < nlines; i++) {
		if (scanp >= &buffer[BUFFER_SIZE])
			scanp -= BUFFER_SIZE;
		count = llen;
		/* This buffer goes left-to-right [0], [1], ... */
		if (scanp + count <= &buffer[BUFFER_SIZE])
			do {
				fontdata = *addr++;
				*scanp |= (fontdata >> offset);
				scanp++;
				*scanp |= (fontdata << off8);
				count--;
			} while (count > 0);
		scanp += scanp_inc+count;
		addr += count;
	}
	return (1);
}

slop_lines(nlines)
	int nlines;
{
	register int i, rlines;
	
  	/* cur_fb_line += nlines;  */
	rlines = (&buffer[BUFFER_SIZE] - buf0p) / bytes_per_line;
	if (rlines < nlines) {
		if (writelines(rlines, buf0p) < 0)
			exit(1);
		bzero(buf0p, rlines * bytes_per_line);
		buf0p = buffer;
		nlines -= rlines;
		xpos -= rlines;
		row -= RECONVERT(rlines);
	}
	if (writelines(nlines, buf0p) < 0)
		exit(1);
	bzero(buf0p, bytes_per_line * nlines);
	buf0p += bytes_per_line * nlines;
	if (buf0p >= &buffer[BUFFER_SIZE])
		buf0p -= BUFFER_SIZE;
	xpos -= nlines;
	row -= RECONVERT(nlines);
}

/*
 *  Overlay framebuffer with indicated lines of bitmap.
 */
writelines(nlines, buf)
	int	nlines;		/*  Number of scan lines to put out.  */
	register char	*buf;	/*  Pointer to buffer location.  */
{
	register RGBpixel *pp;
	register int	bit;
	register int	bufval;
	int	lpos;
	int	l;

	for(l = 0; l < nlines; l++)  {
		if(cur_fb_line < 0 )  {
			/* Ran off bottom of screen */
			fb_close(fbp);
			exit(0);
		}
		if( clear )
			bzero( (char *)scanline, scr_width*3 );
		else if( overlay )  {
			if( fread( (char *)scanline, scr_width*3, 1, stdin ) != 1 )  {
				clear = 1;
				overlay = 0;
			}
		} else
			fb_read( fbp, 0, cur_fb_line, scanline, scr_width );
		pp = scanline;
		for( lpos = 0; lpos < bytes_per_line; lpos++)  {
			if( (bufval = *buf) == 0 )  {
				pp += 8;
				buf++;
				continue;
			}
			for(bit = 0x80; bit; bit >>= 1)  {
				if(bufval & bit)  {
					COPYRGB( *pp, writing_color );
				}
				pp++;
			}
			buf++;
		}
		fb_write( fbp, 0, cur_fb_line, scanline, scr_width );
		cur_fb_line--;
	}
	return(0);
}
