#!/bin/sh
#			V E R I F Y . S H
#
# This shell script is used to verify the configuration of the compile-time
# environment for the BRL CAD Package.
# It is run automatically by ./setup.sh, but when hunting problems
# it can be useful all by itself.
#
#  $Header$

# Ensure that all subordinate scripts run with the Bourne shell,
# rather than the user's shell
SHELL=/bin/sh
export SHELL

# At this point, we expect machinetype.sh to have been installed.
eval `machinetype.sh -v`
BRLCAD_ROOT=$BASEDIR
export BRLCAD_ROOT

BINDIR=$BASEDIR/bin

############################################################################
#
# Path & Directory sanity check
# Make sure that BINDIR is in the current user's search path
# For this purpose, specifically exclude "dot" from the check.
#
# Unlike the code in setup.sh, this version only checks, it
# does not create/modify anything.
#
############################################################################
echo verify.sh: Verifying that ${BINDIR} is in your search path.
PATH_ELEMENTS=`echo $PATH | sed 's/^://
				s/:://g
				s/:$//
				s/:\\.:/:/g
				s/:/ /g'`

not_found=1		# Assume cmd not found
for PREFIX in ${PATH_ELEMENTS}
do
	if test ${PREFIX} = ${BINDIR}
	then
		# This was -x, but older BSD systems don't do -x.
		if test -d ${PREFIX}
		then
			# all is well
			not_found=0
			break
		fi
		echo "$0 WARNING:  ${PREFIX} is in the search path, but is not a directory."
		exit 1
	fi

	# Make sure that there are no conflicting files earlier in path.
	if test -f ${PREFIX}/machinetype.sh -o -f ${PREFIX}/cake
	then
		echo " "
		echo "$0 ERROR: Different version of BRL-CAD detected in ${PREFIX},"
		echo " which is earlier in your search path than ${BINDIR}."
		echo " Please place ${BINDIR} earlier in your PATH,"
		echo " or change the setting of BRLCAD_ROOT"
		exit 2
	fi
done
if test ${not_found} -ne 0
then
	echo "$0 ERROR:  ${BINDIR} (BINDIR) is not in your Shell search path!"
	echo "$0 ERROR:  BRL-CAD setup can not proceed until this has been fixed."
	echo "$0 ERROR:  Consult installation directions for more information,"
	echo "$0 ERROR:  file: install.doc, section INSTALLATION DIRECTORIES."
	exit 3		# Die
fi


############################################################################
#
#  Ensure that machinetype.sh and Cakefile.defs are set up the same way.
#  This is mostly a double-check on people porting to new machines.
#
#  Rather than invoke CPP here trying to guess exactly how CAKE
#  is invoking it, just run CAKE on a trivial Cakefile and compare
#  the results.
#
############################################################################
echo verify.sh: Comparing machinetype.sh and Cakefile.defs

if test ! -f Cakefile.defs
then
	echo "$0: No Cakefile.defs, please run this script in your BRL-CAD source directory."
	exit 2
fi

IN_FILE=/tmp/verify_in$$
OUT_FILE=/tmp/verify_out$$
trap '/bin/rm -f ${IN_FILE} ${OUT_FILE}; exit 1' 1 2 3 15	# Clean up temp file

rm -f ${OUT_FILE}

cat > ${IN_FILE} << EOF
#line 1 "$0"
#include "Cakefile.defs"

#ifdef BSD
#undef BSD	/* So that C_UNIXTYPE can have a value of BSD, not "43", etc. */
${OUT_FILE}:
	echo C_MACHINE=MTYPE ';' > ${OUT_FILE}
	echo C_UNIXTYPE=\'BSD\' ';' >> ${OUT_FILE}
	echo C_HAS_TCP=HAS_TCP >> ${OUT_FILE}
#else
#undef SYSV
${OUT_FILE}:
	echo C_MACHINE=MTYPE ';' > ${OUT_FILE}
	echo C_UNIXTYPE=\'SYSV\' ';' >> ${OUT_FILE}
	echo C_HAS_TCP=HAS_TCP >> ${OUT_FILE}
#endif
EOF

# Run the file through CAKE.
## echo cake -f ${IN_FILE} ${OUT_FILE}
cake -f ${IN_FILE} ${OUT_FILE} > /dev/null
if test $? -ne 0
then
	/bin/rm -f ${IN_FILE} ${OUT_FILE}
	echo "$0: cake returned error code"
	exit 4
fi

if test ! -f ${OUT_FILE}
then
	/bin/rm -f ${IN_FILE} ${OUT_FILE}
	echo "$0: cake run failed to create expected output file"
	exit 5
fi

# Source the output file as a shell script, to set C_* variables here.
## cat ${OUT_FILE}
. ${OUT_FILE}

# See if things match up
if test "x${MACHINE}" != "x${C_MACHINE}" -o \
	"x${UNIXTYPE}" != "x${C_UNIXTYPE}" -o \
	"x${HAS_TCP}" != "x${C_HAS_TCP}"
then
	echo "$0 ERROR:  Mis-match between machinetype.sh and Cakefile.defs"
	echo "$0 ERROR:  machinetype.sh claims ${MACHINE} ${UNIXTYPE} ${HAS_TCP}"
	echo "$0 ERROR:  Cakefile.defs claims ${C_MACHINE} ${C_UNIXTYPE} ${C_HAS_TCP}"
	echo "It may be useful to examine ${IN_FILE} and ${OUT_FILE}"
	exit 6		# Die
fi

echo " OK. PATH, cake, machinetype.sh, and Cakefile.defs are set properly."
/bin/rm -f ${IN_FILE} ${OUT_FILE}
exit 0
