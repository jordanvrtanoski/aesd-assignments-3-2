#!/bin/bash

#***********************************************************************#
#
# (C) 2025 Jordan Vrtanoski
# This is a shell script developed for the assigment
#
#***********************************************************************#

## Cheeck that we have 2 arguments supplid
if [ $# -eq 2 ]
then
	## Capture the arguments in a variable
        WRITEFILE=$1
        WRITESTR=$2
	DIRNAME=`dirname "$WRITEFILE"`

	if [ ! -d "$DIRNAME" ]
	then
		mkdir -p $DIRNAME
	fi
else
	## If we have 
	echo "Script usage"
	echo ""
	echo "$0 <FILESDIR> <SEARCHDIR>"
	exit 1
fi

## Now we are ready to write the string

echo $WRITESTR > $WRITEFILE

if [ "$?" -ne 0 ]; then
	echo "ERROR: Error writing the file"
	exit 1
fi

