/*			P I X C O L O R S
 *
 *	Count the number of different pixel values in a PIX format image.
 *	If the "-v" option is selected, list each unique pixel value 
 *	to the standard output.
 *
 *	Author(s)
 *	Lee A. Butler	butler@stsci.edu
 *
 *	Options
 *	v	list colors
 */
#include <stdio.h>
#include "externs.h"

/* declarations to support use of getopt() system call */
char *options = "v";
char verbose = 0;
extern char *optarg;
extern int optind, opterr, getopt();
char *progname = "(noname)";

#define PIXELS 1024
unsigned char pixbuf[BUFSIZ*3];

/* This grotesque array provides 1 bit for each of the 2^24 possible pixel
 * values.
 * The "NOBASE" comment below allows compilation on the Gould 9000 series
 * of computers.
 */
/*NOBASE*/
unsigned char vals[1L << (24-3)];

/*
 *	D O I T --- Main function of program
 */
void doit()
{
	unsigned long pixel, count;
	int bytes;
	register int mask, i;
	register unsigned long k;


	count = 0;
	while ((bytes=fread(pixbuf, 3, PIXELS, stdin)) > 0) {
		for (i=(bytes-1)*3 ; i >= 0 ; i -= 3) {
			pixel = pixbuf[i] + 
				(pixbuf[i+1] << 8) +
				(pixbuf[i+2] << 16);

			if ( ! ( vals[k=(pixel >> 3)] &
			    (mask=(1 << (pixel & 0x07))) ) ) {
				vals[k] |= (unsigned char)mask;
				++count;
			}
		}
	}
	(void) printf("%u\n", count);
	if (verbose)
		for (i=0 ; i < 1<<24 ; ++i)
			if ( (vals[i>>3] & (1<<(i & 0x07))) )
				(void) printf("%3d %3d %3d\n",
					i & 0x0ff,
					(i >> 8) & 0x0ff,
					(i >> 16) & 0x0ff);
}

void usage()
{
	(void) fprintf(stderr, "Usage: %s [ -v ] < PIXfile\n", progname);
	exit(1);
}

/*
 *	M A I N
 *
 *	Perform miscelaneous tasks such as argument parsing and
 *	I/O setup and then call "doit" to perform the task at hand
 */
int main(ac,av)
int ac;
char *av[];
{
	int  c, isatty();

	progname = *av;
	if (isatty(fileno(stdin))) usage();
	
	/* Get # of options & turn all the option flags off
	 */

	/* Turn off getopt's error messages */
	opterr = 0;

	/* get all the option flags from the command line
	 */
	while ((c=getopt(ac,av,options)) != EOF) {
		if ( c == 'v' ) verbose = ! verbose;
		else usage();
	}

	if (optind < ac) usage();

	doit();
	return(0);
}
