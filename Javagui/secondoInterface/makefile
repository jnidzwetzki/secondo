
include ../makefile.inc

.PHONY: all

all: 
	$(MAKE) -C lib all
	$(MAKE) -C tools all
	$(MAKE) -C sj all


.PHONY: doc # creates the documentation  via $(JAVADOC)
doc: removedoc 
	@make -s all
	mkdir $(DOCPATH)
	$(JAVADOC)  -d $(DOCPATH) -sourcepath .  -classpath .  $(shell find -type d -not -name "*CVS" -not -name tmp -not -iname "[0-9]*" -not -path "lib*"  -not -ipath "*/[0-9]*"  -printf '%P\n' | sed "s#/#.#g" )   




.PHONY:SecondoInterface.jar
SecondoInterface.jar: all 
	rm -rf exporttmp
	mkdir exporttmp
	@echo "extract third party jar files"
	@cd exporttmp;find ../lib -name "*jar" -exec jar -xf {} \; ; cd ..
	@find -maxdepth 1 -not -name . -not -name exporttmp -exec cp -r {} exporttmp \;
	@find exporttmp -type d -name CVS -exec rm -rf {} \; 2>/dev/null || echo ""
	@find exporttmp -name "*java" -exec rm -f {} \;
	@find exporttmp -name "makefile" -exec rm -f {} \;
	@find exporttmp -name ".cvsignore" -exec rm -f {} \;
	@find exporttmp -type f -name "SecondoInterface.jar" -exec rm -f {} \;
	@rm -rf exporttmp/lib
	@echo "Building jar archive"
	@(cd exporttmp; jar -cf  ../SecondoInterface.jar .)	
	rm -rf exporttmp






.PHONY: clean

clean: removedoc
	$(MAKE) -C sj clean
	$(MAKE) -C tools clean
	$(MAKE) -C lib clean
	rm -f SecondoInterface.jar


.PHONY: removedoc
removedoc:
	rm -rf $(DOCPATH)
