
include ../../../envir/unix/makefile.unix

unix: dumpc vt kikela
	@echo ".............. Logiciels dumpc, vt et kikela prets ............."

include $(COMPIL_LIB)/utils/makefile.names

DIAGSLIB = -L$(LIBS) -l$(UTILSNAME)

# ***** Repertoires propres ***************************************************

DIAGS = $(SOURCE_DBG)
DIAGSRELS = $(RELS)/diags
DIAGSDIR = $(BIN_ACQ)/bin

INCLUDES = -I$(UTILS)

# ***** Interface utilisateur ************************************************

dumpc: $(DIAGSDIR)/dumpc
	$(RELOAD)

$(DIAGSDIR)/dumpc: $(DIAGSRELS) $(DIAGSRELS)/dumpc.$O $(UTILSLIB)
	$(LD) $(LFLAGS) $G $(EXECUTABLE) $(DIAGSRELS)/dumpc.$O $(DIAGSLIB) $(LIBSTD)
	@echo ".................... Executable $@ pret ...................."

$(DIAGSRELS)/dumpc.$O: $(DIAGS)/dumpc.c
	$(CC) $(BINAIRE) $(INCLUDES) $(CFLAGS) $D -DVT100 $(DIAGS)/dumpc.c

vt: $(DIAGSDIR)/vt
	$(RELOAD)

$(DIAGSDIR)/vt: $(DIAGSRELS) $(DIAGSRELS)/vt.$O $(UTILSLIB)
	$(LD) $(LFLAGS) $G $(EXECUTABLE) $(DIAGSRELS)/vt.$O $(DIAGSLIB) $(LIBSTD) 
	@echo ".................... Executable $@ pret ...................."

$(DIAGSRELS)/vt.$O: $(DIAGS)/vt.c
	$(CC) $(BINAIRE) $(INCLUDES) $(CFLAGS) $D $(DIAGS)/vt.c

kikela: $(DIAGSDIR)/kikela
	$(RELOAD)

$(DIAGSDIR)/kikela: $(DIAGSRELS) $(DIAGSRELS)/kikela.$O $(UTILSLIB)
	$(LD) $(LFLAGS) $G $(EXECUTABLE) $(DIAGSRELS)/kikela.$O $(DIAGSLIB) $(LIBSTD) 
	@echo ".................... Executable $@ pret ...................."

$(DIAGSRELS)/kikela.$O: $(DIAGS)/kikela.c
	$(CC) $(BINAIRE) $(INCLUDES) $(CFLAGS) $D $(DIAGS)/kikela.c

$(DIAGSRELS): $(RELS)
	mkdir -pv $(DIAGSRELS)

clean: 
	rm -vf $(DIAGSDIR)/dumpc $(DIAGSRELS)/dumpc.o $(DIAGSDIR)/vt $(DIAGSRELS)/vt.o $(DIAGSDIR)/kikela $(DIAGSRELS)/kikela.o

