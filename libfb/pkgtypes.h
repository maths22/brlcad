/*
 * Types of packages.  Put here because I didn't know where it should go.
 *   pkg.h perhaps or should that be generic?
 */

#define	MSG_FBOPEN	1
#define	MSG_FBCLOSE	2
#define	MSG_FBCLEAR	3
#define	MSG_FBREAD	4
#define	MSG_FBWRITE	5
#define	MSG_FBCURSOR	6
#define	MSG_FBWINDOW	7
#define	MSG_FBZOOM	8
#define	MSG_FBGETSIZE	9
#define	MSG_FBSETSIZE	10
#define	MSG_FBSETBACKG	11
#define	MSG_FBRMAP	12
#define	MSG_FBWMAP	13
#define	MSG_FBIOINIT	14
#define	MSG_FBWPIXEL	15

#define	MSG_DATA	20
#define	MSG_RETURN	21
#define	MSG_CLOSE	22
#define	MSG_ERROR	23

#define	MSG_PAGEIN	30
#define	MSG_PAGEOUT	31

#define	MSG_NORETURN	100
