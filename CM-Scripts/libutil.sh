#!/bin/bash
#
# Jan 2005, M. Spiekermann. This is a small library
# of functions useful for several shell scripts.

if [ -z "$BASH" ]; then
  printf "%s\n" "Error: You need a bash shell to run this script!"
  exit 1
fi

# Useful date and time information 
date_TimeStamp=$(date "+%y%m%d-%H%M%S")
date_ymd=${date_TimeStamp%-*}
date_HMS=${date_TimeStamp#*-}

# printSep $1
#
# $1 message
#
# print a separator with a number and a message
declare -i stepCtr=1
function printSep() {

  printf "\n%s\n" "Step ${stepCtr}: ${1}"
  printf "%s\n" "------------------------------------"
  let stepCtr++
}

# checkCmd $1
#
# $1 command
#
# execute a command. In case of an error display the
# returncode
declare -i rc=0
function checkCmd() {

  eval ${1}  # useful if $1 contains quotes or variables
  let rc=$?  # save returncode

  if [ $rc -ne 0 ]; then
    printf "\n Failure! Command {${1}} returned with value ${rc} \n"
  fi
}

# sendMail $1 $2 $3 [$4]
#
# $1 subject
# $2 recipients
# $3 body
# $4 attached file
#
# Sends a mail (with a given attachment) to the list of
# recipients.
function sendMail() {

  if [ "${4}" != "" ]; then
    attachment="-a ${4}"
  fi

  mail -s"$1" ${attachment} "$2" <<-EOFM
$3
EOFM

}

#showGPL
#
function showGPL() {

  printf "%s\n"   "Copyright (C) 2004, University in Hagen,"
  printf "%s\n"   "Department of Computer Science,"
  printf "%s\n\n" "Database Systems for New Applications."
  printf "%s\n"   "This is free software; see the source for copying conditions."
  printf "%s\n"   "There is NO warranty; not even for MERCHANTABILITY or FITNESS"
  printf "%s\n"   "FOR A PARTICULAR PURPOSE."
}

TEMP="/tmp"
if [ ! -d $TEMP ]; then
  printf "%s\n" "creating directory $TEMP"
fi 

buildDir=${SECONDO_BUILD_DIR}

####################################################################################
#
# Test functions
#
####################################################################################

if [ "$1" == "test" ]; then  

for msg in "hallo" "dies" "ist" "ein" "test"
do
  printSep $msg
done 

checkCmd "echo 'hallo' > test.txt 2>&1"
checkCmd "dfhsjhdfg > test.txt 2>&1"

XmailBody="This is a generated message!  

  Users who comitted to CVS yesterday:
  $recipients

  You will find the output of make in the attached file.
  Please fix the problem as soon as possible."

sendMail "Test Mail!" "spieker root" "$XmailBody" "test.txt"

fi
