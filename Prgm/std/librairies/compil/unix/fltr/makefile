# doit etre present au niveau $(PRGM)/std/librairies/compil/unix/$(LIBFOLDER)
#
include makefile.defs
include ../../../../envir/unix/makefile.unix
ifeq ($(LIBSTANDARD),oui)
	include $(MAKES_STD)/makefile.lib
else
	include $(COMPIL_LIB)/$(LIBFOLDER)/makefile.$(LIBNAME)
endif

../../../../envir/unix/makefile.unix $(MAKES_STD)/makefile.$S $(MAKES_STD)/makefile.lib makefile.defs makefile:
	@echo "----- Inclusion de $@ -----"
