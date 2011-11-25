#!/bin/sh

#
# ps2png - convert inputfile to pnggray
#
# Author:  Christoph Dalitz, Hochschule Niederrhein
# Version: 1.3, 2006.08.21
#

# config options
USEIMAGEMAGICK4ROTATION="no"  # set to "yes" for use of convert
GS=gs
DEVICE=pnggray

# command line arguments
DPI=100
OUFILE=""
INFILE=""
EPSCROP=""
ROTATE="yes"
PGMNAME="`basename $0`"
USAGEMSG="Summary:
    $PGMNAME converts a PS file into a PNG bitmap image
\nUsage:
    $PGMNAME [-dpi <dpi>] [-o <pngfile>] [-eps] <psfile>
\nOptions:
    -dpi <dpi>  dots per inch resolution in bitmap output
                (default: $DPI dpi)
    -eps        take care of eps bounding box
    -norot      suppress rotation of landscape images
    -o <fname>  set name of output file
                (default: iput file prefix plus \".png\")"

while [ $# -gt 0 ]
do
	case "$1" in
		-\?)   echo -e "$USAGEMSG"; exit 0;;
		-dpi)  DPI=$2; shift;;
		-o)    OUTFILE=$2; shift;;
		-eps)  EPSCROP="-dEPSCrop";;
		-norot) ROTATE="no";;
		*)     INFILE="$1";;
	esac
	shift
done
if [ -z "$INFILE" -o ! -r "$INFILE" ]
then
	echo -e "$USAGEMSG" 1>&2
	exit 1
fi
if [ -z "$OUTFILE" ]
then
	OUTFILE=`echo $INFILE | sed 's/\.[^.]*$//'`.png
fi

# do conversion
$GS -r$DPI -dSAFER -q -sDEVICE=$DEVICE -sOutputFile=$OUTFILE \
	-dBATCH -dNOPAUSE $EPSCROP $INFILE

# try to detect landscape mode and rotate
if [ $ROTATE = 'yes' ]
then
	if grep -q -e '%%Orientation:.*[Ll]andscape' $INFILE
	then
		echo "Landscape orientation detected. Rotate image."
		echo "If this is wrong, use option -norot"
		if [ $USEIMAGEMAGICK4ROTATION = 'yes' ]
		then
			mv $OUTFILE tmp.png
			convert -rotate 90 tmp.png $OUTFILE
			rm tmp.png
		else
			python <<EOF
from gamera.core import *
init_gamera()
image=load_image("$OUTFILE")
image = image.rotate(90,0)
image.save_PNG("$OUTFILE")
EOF
		fi
	fi
fi
