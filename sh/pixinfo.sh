#!/bin/sh
#			P I X I N F O . S H
#
# Given the name of an image file, extract the base name,
# the suffix, and the dimensions.
#
#  e.g.:
#	data-w123-n456.pix
#	data-123x456.pix
#	stuff.rle
#
#  The output of this script is one line, suitable for use in a
#  Bourne-shell accent-grave form, like this:
#
#	eval `pixinfo.sh $FILE`		# Sets BASE, SUFFIX, WIDTH, HEIGHT
#
#  -Mike Muuss, BRL, 11-Sept-92.
#
#  $Header$

if test "$1" = ""
then
	echo "Usage: pixinfo.sh filename"
	echo "	gives /bin/sh definitions for BASE SUFFIX WIDTH HEIGHT"
	exit 1
fi

FILE=$1

eval `basename $FILE|sed -e 's/\\(.*\\)\\.\\(.*\\)/\\1;SUFFIX=\\2;/' -e 's/^/LHS=/' `

# First, see if size is encoded in the name.
BASE=
case "$SUFFIX" in
pix|bw)
	eval `echo $LHS|sed \
-e 's/\\(.*\\)-\\(.*\\)x\\(.*\\)/;BASE=\\1;WIDTH=\\2;HEIGHT=\\3;/'   \
-e 's/\\(.*\\)-w\\(.*\\)-n\\(.*\\)/;BASE=\\1;WIDTH=\\2;HEIGHT=\\3;/' \
-e 's/^/GOOP=/' `
	# if BASE is not set, then the file name is not of this form,
	# and GOOP will have the full string in it.
	;;
esac

# if that was enough, print and quit, else work some more.
if test "$BASE" != ""
then
	echo "BASE=$BASE; SUFFIX=$SUFFIX; WIDTH=$WIDTH; HEIGHT=$HEIGHT"
	exit 0
fi

# Second, see if some program can tell us more about this file,
# usually giving -w -n style info.
BASE=$LHS
case "$SUFFIX" in
pix|bw)
	# XXX Need to run a new autosize program on these guys.
	# fall through
	ARGS="-w0 -n0"
	SUFFIX=unknown_pix
	;;
jpg|jpeg)
	ARGS=`jpeg-fb -h $FILE` ;;
rle)
	ARGS=`rle-pix -H $FILE` ;;
gif)
	ARGS=`gif2fb -h $FILE 2>&1 ` ;;
*)
	ARGS="-w0 -n0"
	SUFFIX=unknown ;;
esac

set -- `getopt hs:S:w:n:W:N: $ARGS`
while :
do
	case $1 in
	-h)
		WIDTH=1024; HEIGHT=1024;;
	-s|-S)
		WIDTH=$2; HEIGHT=$2; shift;;
	-w|-W)
		WIDTH=$2; shift;;
	-n|-N)
		HEIGHT=$2; shift;;
	--)
		break;
	esac
	shift
done

echo "BASE=$BASE; SUFFIX=$SUFFIX; WIDTH=$WIDTH; HEIGHT=$HEIGHT"
exit 0
