/*
	gif-fb -- convert a GIF file to (overlaid) frame buffer images

	created:	89/04/26	D A Gwyn

	Typical compilation:	cc -O -I/usr/include/brlcad -o gif2fb \
					gif2fb.c /usr/brlcad/lib/libfb.a
	Add -DNO_VFPRINTF, -DNO_MEMCPY, or -DNO_STRRCHR if vfprintf(),
	memcpy(), or strrchr() are not present in your C library
	(e.g. on 4BSD-based systems).

	This is a full implementation of the (noninteractive) GIF format
	conversion as specified in "A Standard Defining a Mechanism for
	the Storage and Transmission of Raster-Based Graphics Information",
	June 15, 1987 by CompuServe Incorporated.  This spec is far from
	ideal, but it is a standard that has had wide influence in the PC
	arena, and there are a lot of images available in GIF format.  Most
	small computer systems have GIF translators available for them, so
	this program provides a means of getting most PC images into the
	BRL-CAD domain.

	Options:

	-d		"debug": prints information about the images on
			the standard error output

	-f fb_file	outputs to the specified frame buffer file instead
			of the one specified by the FB_FILE environment
			variable (the default frame buffer, if no FB_FILE)

	-F fb_file	same as -f fb_file (BRL-CAD package compatibility)

	-i image#	outputs just the specified image number (starting
			at 1) to the frame buffer, instead of all images

	-o		"overlay": skips the initial clearing of the frame
			buffer to the background color

	gif_file	GIF input file to be translated (standard input if
			no explicit GIF file name is specified)
*/
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
static char	SCCSid[] = "%W% %E%";	/* for "what" utility */
#endif

#define	USAGE	"gif-fb [ -d ] [ -f fb_file ] [ -i image# ] [ -o ] [ gif_file ]"
#define	OPTSTR	"df:F:i:o"

#ifdef BSD	/* BRL-CAD */
#define	NO_VFPRINTF	1
#define	NO_MEMCPY	1
#define	NO_STRRCHR	1
#endif

#include	<signal.h>
#include	<stdio.h>
#include	<string.h>
#if __STDC__
#include	<stdarg.h>
#include	<stdlib.h>
#define	RBMODE	"rb"			/* "b" not really necessary for POSIX */
#else
#ifdef NO_STRRCHR
#define	strrchr( s, c )	rindex( s, c )
#endif
#ifdef NO_MEMCPY
static char *
memcpy( s1, s2, n )			/* not a complete implementation! */
	register char	*s1, *s2;
	int		n;
	{
	register int	m = (n + 7) / 8;	/* wide-path loop counter */

	switch ( n & 7 )		/* same as ( n % 8 ) */
		/* Loop unrolling a la "Duff's device": */
		do	{
	case 0:
			*s1++ = *s2++;
	case 7:
			*s1++ = *s2++;
	case 6:
			*s1++ = *s2++;
	case 5:
			*s1++ = *s2++;
	case 4:
			*s1++ = *s2++;
	case 3:
			*s1++ = *s2++;
	case 2:
			*s1++ = *s2++;
	case 1:
			*s1++ = *s2++;
			}
		while ( --m > 0 );

	return 0;
	}
#else
#include	<memory.h>
#endif
#include	<varargs.h>
#define	RBMODE	"r"
extern void	exit();
extern char	*malloc();
#endif
#ifndef EXIT_SUCCESS
#define	EXIT_SUCCESS	0
#endif
#ifndef EXIT_FAILURE
#define	EXIT_FAILURE	1
#endif
extern char	*optarg;
extern int	getopt(), optind;

#include	<fb.h>			/* BRL CAD package libfb.a interface */

typedef int	bool;
#define	false	0
#define	true	1

static char	*arg0;			/* argv[0] for error message */
static bool	clear = true;		/* set iff clear to background wanted */
static bool	debug = false;		/* set for GIF-file debugging info */
static int	image = 0;		/* # of image to display (0 => all) */
static char	*gif_file = NULL;	/* GIF file name */
static FILE	*gfp = NULL;		/* GIF input stream handle */
static char	*fb_file = NULL;	/* frame buffer name */
static FBIO	*fbp = FBIO_NULL;	/* frame buffer handle */
static int	ht;			/* virtual frame buffer height */
static int	width, height;		/* overall "screen" size */
static int	left, top, right, bottom;	/* image boundary */
static bool	M_bit;			/* set iff color map provided */
static bool	I_bit;			/* set iff image interlaced */
static int	cr;			/* # bits of color resolution */
static int	cr_mask;		/* mask to strip all but high cr bits */
static int	g_pixel;		/* global # bits/pixel in image */
static int	pixel;			/* local # bits/pixel in image */
static int	background;		/* color index of screen background */
static int	entries;		/* # of global color map entries */
static RGBpixel	*g_cmap;		/* malloc()ed global color map */
static RGBpixel	*cmap;			/* malloc()ed local color map */
/* NOTE:  Relies on R,G,B order and also on RGBpixel being 3 unsigned chars. */

#define	GIF_EXTENSION	'!'
#define	GIF_IMAGE	','
#define	GIF_TERMINATOR	';'


static char *
Simple( path )
	char		*path;
	{
	register char	*s;		/* -> past last '/' in path */

	return (s = strrchr( path, '/' )) == NULL || *++s == '\0' ? path : s;
	}


static void
VMessage( format, ap )
	char	*format;
	va_list	ap;
	{
	(void)fprintf( stderr, "%s: ", arg0 );
#ifdef NO_VFPRINTF
	(void)fprintf( stderr, format,	/* kludge city */
		       ((int *)ap)[0], ((int *)ap)[1],
		       ((int *)ap)[2], ((int *)ap)[3]
		     );
#else
	(void)vfprintf( stderr, format, ap );
#endif
	(void)putc( '\n', stderr );
	(void)fflush( stderr );
	}


#if __STDC__
static void
Message( char *format, ... )
#else
static void
Message( va_alist )
	va_dcl
#endif
	{
#if !__STDC__
	register char	*format;	/* must be picked up by va_arg() */
#endif
	va_list		ap;

#if __STDC__
	va_start( ap, format );
#else
	va_start( ap );
	format = va_arg( ap, char * );
#endif
	VMessage( format, ap );
	va_end( ap );
	}


#if __STDC__
static void
Fatal( char *format, ... )
#else
static void
Fatal( va_alist )
	va_dcl
#endif
	{
#if !__STDC__
	register char	*format;	/* must be picked up by va_arg() */
#endif
	va_list		ap;

#if __STDC__
	va_start( ap, format );
#else
	va_start( ap );
	format = va_arg( ap, char * );
#endif
	VMessage( format, ap );
	va_end( ap );

	if ( fbp != FBIO_NULL && fb_close( fbp ) == -1 )
		Message( "Error closing frame buffer" );

	exit( EXIT_FAILURE );
	/*NOTREACHED*/
	}


static void
Sig_Catcher( sig )
	int	sig;
	{
	(void)signal( sig, SIG_DFL );

	/* The following is not guaranteed to work, but it's worth a try. */
	Fatal( "Interrupted by signal %d", sig );
	}


static void
Skip()					/* skip over raster data */
	{
	register int	c;

	if ( (c = getc( gfp )) == EOF )
		Fatal( "Error reading code size" );

	while ( (c = getc( gfp )) != 0 )
		if ( c == EOF )
			Fatal( "Error reading block byte count" );
		else
			do
				if ( getc( gfp ) == EOF )
					Fatal( "Error reading data byte" );
			while ( --c > 0 );
	}


/*
	The raster output loop is inverted, because it is simpler to
	let the LZW decompression drive pixel writing than vice versa.
 */

static int	start_row[5] = { 0, 4, 2, 1, 0 };
static int	step[5] = { 8, 8, 4, 2, 1 };
static int	row, col;		/* current pixel coordinates */
static int	pass;			/* current pass */
static int	stop;			/* final pass + 1 */
static RGBpixel	*pixbuf;		/* malloc()ed scan line buffer */


static void
PutPixel( value )
	register int	value;
	{
	if ( pass == stop )
		Fatal( "Too much raster data for image size" );

	if ( value > entries )
		Fatal( "Decoded color index %d exceeds color map size", value );

	pixbuf[col - left][RED] = cmap[value][RED];	/* stuff pixel */
	pixbuf[col - left][GRN] = cmap[value][GRN];
	pixbuf[col - left][BLU] = cmap[value][BLU];

	if ( ++col == right )
		{
		/*
		   Note that BRL-CAD frame buffers are upside-down.
		   The following produces a right-side-up image at
		   the bottom of the available frame buffer.
		 */

		if ( fb_write( fbp, left, ht - row, pixbuf, right - left ) == -1
		   )
			Fatal( "Error writing scan line to frame buffer" );

		col = left;

		if ( (row += step[pass]) >= bottom
		  && ++pass < stop
		   )
			row = start_row[pass];
		}
	}


/*
	Limpel-Ziv-Welch decompression, based on "A Technique for
	High-Performance Data Compression" by Terry A. Welch in IEEE
	Computer, June 1984, pp. 8-19.

	GIF format usurps the first two "compression codes" for use
	as "reset" and "end of information".  The initial "code size"
	varies from 2 through (incremented) pixel, a maximum of 8.
	The LZW code data starts out 1 bit wider than the GIF "code
	size", and grows occasionally, up to 12 bits per code "chunk".
	LZW codes from 0 through (clear_code-1) are raw colormap
	index values, while those from clear_code on up are indices
	into the string-chaining table.

	This is my own implementation, using recursion instead of an
	explicit stack to expand the strings.
 */

static int	code_size;		/* initial LZW chunk size, in bits */
static int	chunk_size;		/* current LZW chunk size, in bits */
static int	chunk_mask;		/* bit mask for extracting chunks */
static int	compress_code;		/* first compression code value */
static int	k;			/* extension character */
static struct
	{
	int		pfx;		/* prefix string's table index */
	int		ext;		/* extension value */
	}	table[1 << 12];		/* big enough for 12-bit codes */
/* Unlike the example in Welch's paper, our table contains no atomic values. */

static int	bytecnt;		/* # of bytes remaining in block */
static int	rem_bits;		/* data bits left over from last call */
static int	bits_on_hand;		/* # of bits left over from last call */


static int
GetCode()
	{
	register int	next_val;

	while ( bits_on_hand < chunk_size )
		{
		/* Read 8 more bits from the GIF file. */

		while ( bytecnt == 0 )
			{
			/* Start new data block. */

			if ( (bytecnt = getc( gfp )) == EOF )
				Fatal( "%s at start of new LZW data block",
				       feof( gfp ) ? "EOF" : "Error"
				     );

			if ( bytecnt == 0 )
				Message( "Warning: 0-byte data block" );
			/* Should this abort the image?  GIF spec is unclear. */
			}

		if ( (next_val = getc( gfp )) == EOF )
			Fatal( "%s while reading LZW data block",
			       feof( gfp ) ? "EOF" : "Error"
			     );

		--bytecnt;		/* used up another byte of input */
		rem_bits |= next_val << bits_on_hand;
		bits_on_hand += 8;
		}

	/* Now have enough bits to extract the next LZW code. */

	next_val = rem_bits & chunk_mask;

	/* Prepare for next entry. */

	rem_bits >>= chunk_size;
	bits_on_hand -= chunk_size;

	return next_val;
	}


/* WARNING:  This recursion could get pretty deep (4093 nested calls)! */
static void
Expand( c )
	register int	c;		/* LZW code */
	{
	if ( c < compress_code )	/* "atomic", i.e. raw color index */
		PutPixel( k = c );	/* first atom in string */
	else	{			/* "molecular"; follow chain */
		Expand( table[c].pfx );
		PutPixel( table[c].ext );
		}
	}


static void
LZW()
	{
	register int	c;		/* input LZW code, also input byte */
	register int	w;		/* prefix code */
	register int	next_code;	/* next available table index */
	register int	max_code;	/* limit at which LZW chunk must grow */
	int		eoi_code;	/* end of LZW stream */
	int		clear_code;	/* table reset code */

	if ( (code_size = getc( gfp )) == EOF )
		Fatal( "Error reading code size" );

	if ( code_size < pixel )
		Message( "Warning: initial code size smaller than colormap" );

	if ( code_size > pixel && !(pixel == 1 && code_size == 2) )
		Message( "Warning: initial code size greater than colormap" );
	/* This case will probably eventually trigger Fatal() in PutPixel(). */

	/* Initialize GetCode() parameters. */

	bytecnt = 0;			/* need a new data block */
	bits_on_hand = 0;		/* there was no "last call" */

	/* Initialize LZW algorithm parameters. */

	clear_code = 1 << code_size;
	eoi_code = clear_code + 1;
	compress_code = clear_code + 2;

	if ( (chunk_size = code_size + 1) > 12 )	/* LZW chunk size */
		Fatal( "GIF spec's LZW code size limit (12) violated" );

	max_code = 1 << chunk_size;	/* LZW chunk will grow at this point */
	chunk_mask = max_code - 1;
	next_code = compress_code;	/* empty chain-code table */
	w = -1;				/* we use -1 for "nil" */

	while ( (c = GetCode()) != eoi_code )
		if ( c == clear_code )
			{
			/* Reinitialize LZW parameters. */

			chunk_size = code_size + 1;
			max_code = 1 << chunk_size;	/* growth trigger */
			chunk_mask = max_code - 1;
			next_code = compress_code;	/* empty code table */
			w = -1;		/* we use -1 for "nil" */
			}
		else	{
			if ( c > next_code )
				Fatal( "LZW code too large" );

			if ( c == next_code )
				{	/* KwKwK special case */
				if ( w < 0 )	/* w supposedly previous code */
					Fatal( "initial LZW KwKwK code??" );

				Expand( w );	/* sets `k' */
				PutPixel( k );
				}
			else		/* normal case */
				Expand( c );	/* sets `k' */

			if ( w >= 0 && next_code < 1 << 12 )
				{
				table[next_code].pfx = w;
				table[next_code].ext = k;

				if ( ++next_code == max_code
				  && chunk_size < 12
				   )	{
					++chunk_size;
					max_code <<= 1;
					chunk_mask = max_code - 1;
					}
				}

			w = c;
			}

	/* EOI code encountered. */

	if ( bytecnt > 0 )
		{
		Message( "Warning: unused raster data present" );

		do
			if ( (c == getc( gfp )) == EOF )
				Fatal( "Error reading extra raster data" );
		while ( --bytecnt > 0 );
		}

	/* Strange data format in the GIF spec! */

	if ( (c = getc( gfp )) != 0 )
		Fatal( "Zero byte count missing" );
	}


static void
Rasters()				/* process (convert) raster data */
	{
	/* Initialize inverted-loop parameters. */

	pass = I_bit ? 0 : 4;		/* current pass */
	stop = I_bit ? 4 : 5;		/* final pass + 1 */

	row = top + start_row[pass];	/* next pixel row */
	col = left;			/* next pixel column */

	/* Process rasters in possibly interlaced order. */

	LZW();				/* uncompress LZW data & write pixels */
	}


int
main( argc, argv )
	int	argc;
	char	*argv[];
	{
	/* Plant signal catcher. */
	{
	static int	getsigs[] =	/* signals to catch */
		{
		SIGHUP,			/* hangup */
		SIGINT,			/* interrupt */
		SIGQUIT,		/* quit */
		SIGPIPE,		/* write on a broken pipe */
		SIGTERM,		/* software termination signal */
		0
		};
	register int	i;

	for ( i = 0; getsigs[i] != 0; ++i )
		if ( signal( getsigs[i], SIG_IGN ) != SIG_IGN )
			(void)signal( getsigs[i], Sig_Catcher );
	}

	/* Process arguments. */

	arg0 = Simple( argv[0] );	/* save for possible error message */

	{
		register int	c;
		register bool	errors = false;

		while ( (c = getopt( argc, argv, OPTSTR )) != EOF )
			switch( c )
				{
			default:	/* just in case */
			case '?':	/* invalid option */
				errors = true;
				break;

			case 'd':	/* -d */
				debug = true;
				break;

			case 'f':	/* -f fb_file */
			case 'F':	/* -F fb_file */
				fb_file = optarg;
				break;

			case 'i':	/* -i image# */
				image = atoi( optarg );
				break;

			case 'o':	/* -o */
				clear = false;
				break;
				}

		if ( errors )
			Fatal( "Usage: %s", USAGE );
	}

	if ( optind < argc )		/* gif_file */
		{
		if ( optind < argc - 1 )
			{
			Message( "Usage: %s", USAGE );
			Fatal( "Can't handle multiple GIF files" );
			}

		if ( (gfp = fopen( gif_file = argv[optind], RBMODE )) == NULL )
			Fatal( "Couldn't open GIF file \"%s\"", gif_file );
		}
	else
		gfp = stdin;

	/* Process GIF signature. */

	{
		char	sig[6];		/* GIF signature (assume 8-bit bytes) */

		if ( fread( sig, 1, 6, gfp ) != 6 )
			Fatal( "Error reading GIF signature" );

		/* In theory, the signature should be mapped to ASCII here. */

		if ( strncmp( sig, "GIF", 3 ) != 0 )
			/* We could scan until "GIF" is seen, but why bother. */
			Fatal( "File does not start with \"GIF\"" );

		if ( strncmp( &sig[3], "87a", 3 ) != 0 )
			Message(
			     "GIF version \"%3.3s\" not known, \"87a\" assumed",
				 &sig[3]
			       );
	}

	/* Process screen descriptor. */

	{
		unsigned char	desc[7];	/* packed screen descriptor */

		if ( fread( desc, 1, 7, gfp ) != 7 )
			Fatal( "Error reading screen descriptor" );

		width = desc[1] << 8 | desc[0];
		height = desc[3] << 8 | desc[2];
		M_bit = (desc[4] & 0x80) != 0;
		cr = (desc[4] >> 4 & 0x07) + 1;
		g_pixel = (desc[4] & 0x07) + 1;
		background = desc[5];

		if ( debug )
			{
			Message( "screen %dx%d", width, height );

			if ( M_bit )
				Message( "global color map provided" );

			Message( "%d bits of color resolution", cr );
			Message( "%d default bits per pixel", g_pixel );
			Message( "background color index %d", background );
			}

		if ( desc[5] & 0x08 != 0x00 )
			Message( "Screen descriptor byte 6 bit 3 unknown" );

		if ( desc[6] != 0x00 )
			Message( "Screen descriptor byte 7 = %2.2x unknown",
				 desc[6]
			       );
	}

	/* Process global color map. */

	if ( (g_cmap = (RGBpixel *)malloc( 256 * sizeof(RGBpixel) )) == NULL
	  || (cmap = (RGBpixel *)malloc( 256 * sizeof(RGBpixel) )) == NULL
	   )
		Fatal( "Insufficient memory for color maps" );

	entries = 1 << g_pixel;

	if ( M_bit )
		{
		register int	i;

		/* Read in global color map. */

		if ( debug )
			Message( "global color map has %d entries", entries );

		if ( fread( g_cmap, 3, entries, gfp ) != entries )
			Fatal( "Error reading global color map" );

		/* Mask off low-order "noise" bits found in some GIF files. */

		cr_mask = ~0 << 8 - cr;

		for ( i = 0; i < entries; ++i )
			{
			g_cmap[i][RED] &= cr_mask;
			g_cmap[i][GRN] &= cr_mask;
			g_cmap[i][BLU] &= cr_mask;
			}
		}
	else	{
		register int	i;

		/* Set up default linear grey scale global color map.
		   GIF specs for this case are utterly nonsensical. */

		if ( debug )
			Message( "default global color map has %d grey values",
				 entries
			       );

		for ( i = 0; i < entries; ++i )
			g_cmap[i][RED] =
			g_cmap[i][GRN] =
			g_cmap[i][BLU] = i * 256.0 / entries + 0.5;
		}

	/* Open frame buffer for unbuffered output. */

	if ( (pixbuf = (RGBpixel *)malloc( width * sizeof(RGBpixel) )) == NULL )
		Fatal( "Insufficient memory for scan line buffer" );

	if ( (fbp = fb_open( fb_file, width, height )) == FBIO_NULL )
		Fatal( "Couldn't open frame buffer" );

	{
		register int	wt = fb_getwidth( fbp );

		ht = fb_getheight( fbp );

		if ( wt < width || ht < height )
			Fatal( "Frame buffer too small (%dx%d); %dx%d needed",
			       wt, ht, width, height
			     );

		ht = height - 1;	/* for later use as (ht - row) */
	}

	if ( fb_wmap( fbp, (ColorMap *)NULL ) == -1 )
		Fatal( "Error setting up linear color map" );

	/* Fill frame buffer with background color. */

	if ( clear && fb_clear( fbp, g_cmap[background] ) == -1 )
		Fatal( "Error clearing frame buffer to background" );

	/* Convert images.  GIF spec says no pauses between them. */

	for ( ; ; )
		{
		register int	c;

		if ( (c = getc( gfp )) == EOF )
			Fatal( "Missing GIF terminator" );

		switch( c )
			{
		default:
			Message( "Warning: unknown separator 0x%2.2x", c );
			continue;	/* so says the GIF spec */

		case GIF_TERMINATOR:	/* GIF terminator */
    terminate:
			/* GIF spec suggests pause and wait for go-ahead here,
			   also "screen clear", but they're impractical. */

			if ( fb_close( fbp ) == -1 )
				{
				fbp = FBIO_NULL;	/* avoid second try */
				Fatal( "Error closing frame buffer" );
				}

			fbp = FBIO_NULL;

			if ( image > 0 )
				Fatal( "Specified image not found" );

			exit( EXIT_SUCCESS );

		case GIF_EXTENSION:	/* GIF extension block introducer */
			{
			register int	i;

			if ( (i = getc( gfp )) == EOF )
				Fatal( "Error reading extension function code"
				     );

			Message( "Extension function code %d unknown", i );

			while ( (i = getc( gfp )) != 0 )
				{
				if ( i == EOF )
					Fatal(
				      "Error reading extension block byte count"
					     );

				do
					if ( getc( gfp ) == EOF )
						Fatal(
						 "Error reading extension block"
						     );
				while ( --i > 0 );
				}
			}
			break;

		case GIF_IMAGE:		/* image separator */
			{
				unsigned char	desc[9];  /* image descriptor */

				if ( fread( desc, 1, 9, gfp ) != 9 )
					Fatal( "Error reading image descriptor"
					     );

				left = desc[1] << 8 | desc[0];
				top = desc[3] << 8 | desc[2];
				right = desc[5] << 8 | desc[4];
				bottom = desc[7] << 8 | desc[6];
				M_bit = (desc[8] & 0x80) != 0;
				I_bit = (desc[8] & 0x40) != 0;

				pixel = M_bit ? (desc[8] & 0x07) + 1 : g_pixel;

				if ( debug )
					{
					Message( "image (%d,%d,%d,%d)",
						 left, top, right, bottom
					       );

					if ( M_bit )
						{
						Message(
						      "local color map provided"
						       );
						Message( "%d bits per pixel",
							 pixel
						       );
						}

					Message( I_bit ? "interlaced"
						       : "sequential"
					       );
					}

				if ( left < 0 || right > width || left >= right
				  || top < 0 || bottom > height || top >= bottom
				   )
					Fatal( "Absurd image (%d,%d,%d,%d)",
					       left, top, right, bottom
					     );
			}

			/* Process local color map. */

			entries = 1 << pixel;

			if ( M_bit )
				{
				register int	i;

				/* Read in local color map. */

				if ( debug )
					Message(
					       "local color map has %d entries",
						 entries
					       );

				if ( fread( cmap, 3, entries, gfp ) != entries )
					Fatal( "Error reading local color map"
					     );

				/* Mask off low-order "noise" bits. */

				for ( i = 0; i < entries; ++i )
					{
					cmap[i][RED] &= cr_mask;
					cmap[i][GRN] &= cr_mask;
					cmap[i][BLU] &= cr_mask;
					}
				}
			else	{
				register int	i;

				/* Use default global color map. */

				if ( debug )
					Message( "global color map used" );

				(void)memcpy( cmap, g_cmap, 3 * entries );
				}

			/* `image' is 0 if all images are to be displayed;
			   otherwise it is a down-counter to the image wanted */
			if ( image <= 1 )
				{
				Rasters();	/* process the raster data */

				if ( image != 0 )
					goto terminate;	/* that's all, folks */
				}
			else	{
				--image;	/* desperately seeking Susan? */
				Skip();	/* skip over the raster data */
				}

			break;
			}
		}
	/* [not reached] */
	}
