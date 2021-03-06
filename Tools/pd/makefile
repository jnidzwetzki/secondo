# This file is part of the PD system
# Copyright (C) 1998 Ralf Hartmut Gueting, Fachbereich Informatik, FernUniversitaet Hagen

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# File: makefile
#
# 05/03 Markus Spiekermann. Makefile revised to make it Windows/Unix
#    compatible. Automatic enumeration of pd files is now done by a
#    lex generated scanner, this is a replacement for the perl solution.
#
# 10/03 Markus Spiekermann. Target install introduced.
#
# 05/04 Markus Spiekermann. Changes in documentation and script files.
# 12/06 Markus Spiekermann. More generic rules introduced.

# Uncomment next line to switch on debug mode
#OPTIONS = -g

# The first line is used when compiling with FLEX (2.5.4a) and BISON (1.35)
# The second line when compiling with LEX and YACC
LINKLIBS =  -lfl 

# The first line is used when compiling with FLEX and BISON
# The second line when compiling with LEX and YACC
LEX = flex
#LEX = lex

# The first line is used when compiling with FLEX and BISON
# The second line when compiling with LEX and YACC
YACC = bison --yacc
#YACC = yacc

# specify your C-compiler
ifeq ($(platform),android)
CC := $(HOME)/toolchain-standalone/bin/arm-linux-androideabi-gcc
else
CC := gcc
endif

DOCU_FILES = PD1 PDNestedText.h PDNestedText.c \
	     PD3 PDLex.l PDTokens.h PDLexTest.c PDParserDS.c PDParser.y \
	     PD6 pd.header PDMaketex.c \
	     PD7 pdtabs.c linebreaks.c \
	     PD8 pdview \
	     PD8.2 pdshow \
	     PD8.3 pd2tex \
	     PD8.4 pd2dvi \
	     PD8.5 pd2ps \
             PD8.6 pd2pdf \
	     PD9 makefile \
	     PDRefs

DOCU_HTML_FILES = HTML1 PDParserHTML.y PDMakeHTML.c \
		  HTML4 pd2html \
		  HTML5 makefile \
		  PDRefsHTML

OBJECTS = PDNestedText.o \
	  PDLib.o 

APPS = pdtabs \
	  linebreaks \
	  enumerate \
	  maketex \
          linecheck \
	  tabcheck \
	  filterpd \
	  makehtml \
	  makeascii

DOCU = docu docuhtml

SCRIPTS = pd2ascii pd2html pd2tex pd2dvi pd2ps pd2pdf pdshow pdview checkpd pdpreview

TEMPORARYS = PDLex.c enumerate.c linecheck.c tabcheck.c y.tab.c


.PHONY all:
all: $(OBJECTS) $(APPS) $(DOCU)
	@chmod ugo+x $(SCRIPTS)

maketex: PDMaketex.c PDParser.y PDLex.c PDParserDS.c $(OBJECTS) 
	$(YACC) PDParser.y
	$(CC) $(OPTIONS) -o $@ $< y.tab.c $(OBJECTS) $(LINKLIBS)

makehtml: PDMakeHTML.c PDParserHTML.y PDLex.c PDParserDS.c $(OBJECTS)
	$(YACC) PDParserHTML.y
	$(CC) $(OPTIONS) -o $@ $< y.tab.c $(OBJECTS) $(LINKLIBS)

makeascii: PDMakeASCII.c PDParserASCII.y PDLex.c PDParserDS.c $(OBJECTS)
	$(YACC) PDParserASCII.y
	$(CC) $(OPTIONS) -o $@ $< y.tab.c $(OBJECTS) $(LINKLIBS)

pdtabs: pdtabs.c
	$(CC) -o $@ $< 

linebreaks: linebreaks.c
	$(CC) -o $@ $< 


# some dependencies
PDNestedText.o: PDNestedText.h

PDLib.o: PDLib.h


# some generic translation rules

%.c: %.l
	$(LEX) -o$@ $<

%.o: %.c
	$(CC) -c -g $<

%: %.c
	$(CC) -o $@ $< $(LINKLIBS)


docu: $(DOCU_FILES)
	cat $^ > $@

docuhtml: $(DOCU_HTML_FILES)
	cat $^ > $@

.PHONY dist:
dist: pd.tar.gz

pd.tar.gz:
	cvs export -r$(tag) pd
	tar -czf  pd.tgz pd/*
	rm -r pd

INST_DIR = $(prefix)/pd
EPS_FILES = $(shell find ./Figures -name "*.eps")
.PHONY install:
install: $(OBJECTS) $(SCRIPTS)
	install -d $(INST_DIR)
	install -m744 -d $(INST_DIR)/Figures
	install $(OBJECTS) $(INST_DIR)
	install -m444 $(DOCU) $(INST_DIR)
	install -m444 $(EPS_FILES) $(INST_DIR)/Figures
	install $(SCRIPTS) $(INST_DIR)
	install pd.header $(INST_DIR)

.PHONY clean:
clean:
	rm -f $(OBJECTS) $(APPS) $(DOCU) $(TEMPORARYS) *.exe 

