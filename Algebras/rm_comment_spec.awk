#!/usr/bin/awk -f
#
# specs.awk
# Created in 06.12.2002 by Victor Teixeira de Almeida
# This shell script will read the result of the ordered (without duplicates) concatenation of the specs files for all algebras. 
# It will be called by the makefile of the Secondo System. If this script finds an operator that have more than one specification, 
# it will return a WARNING containing the important information for the makefile.
#
$1 != "#" { print }

