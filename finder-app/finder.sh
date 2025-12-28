#!/bin/sh

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
        FILESDIR=$1
        SEARCHSTR=$2

	if [ ! -d "$FILESDIR" ]
	then
		echo "ERROR: The folder $FILESDIR do not exists"
		exit 1
	fi
else
	## If we have 
	echo "Script usage"
	echo ""
	echo "$0 <FILESDIR> <SEARCHDIR>"
	exit 1
fi

## Now we are ready to search for the string
FILECOUNT=`find $FILESDIR -type f | wc -l`
SEARCHCOUNT=`grep -rI "$SEARCHSTR" $FILESDIR | wc -l`
MESSAGE="The number of files are $FILECOUNT and the number of matching lines are $SEARCHCOUNT"

echo $MESSAGE
