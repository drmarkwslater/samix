include ../../../../std/envir/unix/makefile.unix

ifeq ($(wm),x11)
#	Interactive Processing and Analysis under unIX
	XQ = ipaix
	CLAGS += -DForIpaix

else ifeq ($(wm),gl)
#	Graphical Interactive xxxx xxxx xxxx
	XQ = gigas
	CLAGS += -DForGigas

else ifeq ($(wm),wx)
#	wxWidgets Interactive Graphical Operation
	XQ = wigo
	CLAGS += -DForTangoWXF

else
#	Traitement et Analyse Nouvelle GeneratiOn
	XQ = tango
	CLAGS += -DForTango

endif
CFLAGS += -DCOMPIL_DIR=$(COMPIL_EDW)/tango -DCOMPIL_DIR=$(SOURCE_EDW)/tango -DAPPLI_NAME=$(XQ) -DAPPLI_DIR=$(BIN_ACQ) -DAPPLI_FILE=$(XQ)

$(FOLDER): $(XQ)
	@echo "                    _____________________"
	@echo "                   |                     |"
	@echo "                   | Logiciel $(XQ) pret |"
	@echo "                   |_____________________|"
#	@echo ".................... Logiciel $(XQ) pret ...................."

include $(COMPIL_LIB)/utils/makefile.defs
UTILS := $(SOURCE_LIB)/$(LIBFOLDER)
UTILSNAME := $(LIBNAME)
UTILSLIB := $(LIBS)/lib$(LIBNAME).a
UTILSMAKE := $(COMPIL_LIB)/$(LIBFOLDER)
$(UTILSLIB):
	cd $(UTILSMAKE) && make

include $(COMPIL_LIB)/opium4/makefile.defs
OPIUM := $(SOURCE_LIB)/$(LIBFOLDER)
OPIUMNAME := $(LIBNAME)
OPIUMLIB := $(LIBS)/lib$(LIBNAME).a
OPIUMMAKE := $(COMPIL_LIB)/$(LIBFOLDER)
$(OPIUMLIB):
	cd $(OPIUMMAKE) && make

GRAFLISTE = $(OPIUMLIB)

ifeq ($(wm),gl)
	include $(COMPIL_LIB)/GLFW/makefile.defs
	GLFWNAME := $(LIBNAME)
	GLFWLIB := $(LIBS)/lib$(LIBNAME).a
	GLFWMAKE := $(COMPIL_LIB)/$(LIBFOLDER)
	$(GLFWLIB):
		cd $(GLFWMAKE) && make

	include $(COMPIL_LIB)/GLUT/makefile.defs
	GLUTNAME := $(LIBNAME)
	GLUTLIB := $(LIBS)/lib$(LIBNAME).a
	GLUTMAKE := $(COMPIL_LIB)/$(LIBFOLDER)
	$(GLUTLIB):
		cd $(GLUTMAKE) && make

	GRAFLISTE += $(GLFWLIB) $(GLUTLIB)
endif

include $(COMPIL_LIB)/fltr/makefile.defs
FLTR := $(SOURCE_LIB)/$(LIBFOLDER)
FLTRNAME := $(LIBNAME)
FLTRLIB := $(LIBS)/lib$(LIBNAME).a
FLTRMAKE := $(COMPIL_LIB)/$(LIBFOLDER)
$(FLTRLIB):
	cd $(FLTRMAKE) && make

#	include $(COMPIL_LIB)/FFT/makefile.defs
#	FFT := $(SOURCE_LIB)/$(LIBFOLDER)
#	FFTNAME := $(LIBNAME)
#	FFTLIB := $(LIBS)/lib$(LIBNAME).a
#	FFTMAKE := $(COMPIL_LIB)/$(LIBFOLDER)
#	$(FFTLIB):
#		cd $(FFTMAKE) && make

include $(COMPIL_LIB)/tcp/makefile.defs
TCP := $(SOURCE_LIB)/$(LIBFOLDER)
TCPNAME := $(LIBNAME)
TCPLIB := $(LIBS)/lib$(LIBNAME).a
TCPMAKE := $(COMPIL_LIB)/$(LIBFOLDER)
$(TCPLIB):
	cd $(TCPMAKE) && make

include $(COMPIL_LIB)/db/makefile.defs
DB := $(SOURCE_LIB)/$(LIBFOLDER)
DBNAME := $(LIBNAME)
DBLIB := $(LIBS)/lib$(LIBNAME).a
DBMAKE := $(COMPIL_LIB)/$(LIBFOLDER)
$(DBLIB):
	cd $(DBMAKE) && make

#	include $(COMPIL_EDW)/PCI/makefile.PCI
# supprime de SAMBALIBS: -l$(PCINAME)
# supprime de $(BIN)/samba:  $(PCILIB)

TANGOLIBS = -L$(LIBS) -l$(OPIUMNAME) -l$(TCPNAME)
ifeq ($(OS),Darwin)
	ifeq ($(wm),gl)
		TANGOLIBS += -l$(GLFWNAME) -l$(GLUTNAME) -L/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/ -lGL
	endif
	TANGOLIBS += -lcurl
endif
ifeq ($(OS),Linux)
	ifeq ($(wm),gl)
		TANGOLIBS += -l$(GLFWNAME) -l$(GLUTNAME) -lGL -lXrandr -lXxf86vm -ldl -lpthread
	endif
endif
TANGOLIBS += -l$(DBNAME) -l$(UTILSNAME) -l$(FLTRNAME) -lX11 -lm


# ***** Repertoires propres ***************************************************

TANGO = $(SOURCE_EDW)/tango
SALSA = $(SOURCE_EDW)/salsa
TANGODEFS = $(wildcard $(TANGO)/*.h) $(wildcard $(TANGO)/EDW/*.h) $(wildcard $(SALSA)/*.h)
TANGOBIN = $(RELS)/tango
SAMBABIN = $(RELS)/samba

INCLUDES = -I$(SOURCE_EDW) -I$(TANGO) -I$(TANGO)/EDW -I$(SALSA) -I$(FLTR) -I$(SOURCE_LIB) -I$(OPIUM) -I$(TCP) -I$(DB) -I$(UTILS) -I/usr/include

# ***** Listes ***************************************************************

TANGOCODE = $(notdir $(wildcard $(TANGO)/*.c))
TANGOOBJS = $(TANGOCODE:.c=.o)
TANGORELS := $(foreach fichier,$(TANGOOBJS),$(TANGOBIN)/$(fichier))

TGEDWCODE = $(notdir $(wildcard $(TANGO)/EDW/*.c))
TGEDWOBJS = $(TGEDWCODE:.c=.o)
TGEDWRELS := $(foreach fichier,$(TGEDWOBJS),$(TANGOBIN)/$(fichier))

SALSACODE = $(notdir $(wildcard $(SALSA)/*.c))
SALSAOBJS = $(SALSACODE:.c=.o)
SALSARELS := $(foreach fichier,$(SALSAOBJS),$(SAMBABIN)/$(fichier))

# ***** Interface utilisateur ************************************************

$(XQ): $(BIN_ACQ)/$(XQ)
	@$(PRGM)/std/envir/unix/version_counter $(BIN_ACQ) $(XQ) $(TANGO)
	$(RELOAD)

$(BIN_ACQ)/$(XQ): $(TANGOBIN) $(SAMBABIN) $(TANGORELS) $(TGEDWRELS) $(SALSARELS) $(LIBS) $(UTILSLIB) $(GRAFLISTE) $(TCPLIB) $(DBLIB) $(FLTRLIB)
#	@echo "ar=" && which ar
#	@echo "ld=" && which ld
	@echo "===== Edition de liens pour $@"
	$(LD) $(LFLAGS) $G $(EXECUTABLE) $(TANGORELS) $(TGEDWRELS) $(SALSARELS) $(TANGOLIBS) $(LIBSTD)
	@cp -R $(COMPIL_EDW)/tango/Info/ $(BIN_ACQ)/TangoInfo
	@echo "====================== Executable $(XQ) cree ====================="
	@echo "| Systeme: $(OS)/$(wm) (compilateur: $(CC))"

$(TANGORELS): $(TANGO)/$(notdir $(@:.o=.c)) $(TANGODEFS)
	@echo "===== Compile $(TANGO)/$(notdir $(@:.o=.c))"
	$(CC) $(CFLAGS) $(INCLUDES) -DTANGO $(BINAIRE) $(TANGO)/$(notdir $(@:.o=.c))

$(TGEDWRELS): $(TANGO)/EDW/$(notdir $(@:.o=.c)) $(TANGODEFS)
	@echo "===== Compile $(TANGO)/EDW/$(notdir $(@:.o=.c))"
	$(CC) $(CFLAGS) $(INCLUDES) -DTANGO $(BINAIRE) $(TANGO)/EDW/$(notdir $(@:.o=.c))

$(SALSARELS): $(SALSA)/$(notdir $(@:.o=.c)) $(SAMBADEFS)
	@echo "===== Compile $(SALSA)/$(notdir $(@:.o=.c))"
	$(CC) $(CFLAGS) $(INCLUDES) -DTANGO $(BINAIRE) $(SALSA)/$(notdir $(@:.o=.c))

$(SAMBABIN): $(RELS)
	rm -f $(SAMBABIN)/defauts.o
	mkdir -pv $(SAMBABIN)

$(TANGOBIN): $(RELS)
	mkdir -pv $(TANGOBIN)

$(BIN_ACQ)/TangoInfo: $(BIN_ACQ)
	mkdir -pv $(BIN_ACQ)/TangoInfo

clean:
	rm -vf $(TANGOOBJS) $(SALSAOBJS)
