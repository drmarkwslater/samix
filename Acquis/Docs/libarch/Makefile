SRCE = sources
RELS = rels

EXCUSES = -Wno-format-security -Wno-int-to-pointer-cast
# -Wno-invalid-source-encoding -Wno-tautological-compare -fcolor-diagnostics -fgnu89-inline
COMPILE = gcc $(EXCUSES) -c -DCMDE_UNIX -DUNIX -D$(OSTYPE) -I$(SRCE) -o $@

LIBLIST = $(RELS)/evtread.o $(RELS)/relecture.o $(RELS)/defauts.o $(RELS)/claps.o $(RELS)/decode.o $(RELS)/dateheure.o

all: lib example

lib: $(LIBLIST)
	ar -rv libarch.a $(LIBLIST)

example: $(RELS)/example.o libarch.a
	cc -o example $(RELS)/example.o -L. -larch
	./example

clean: 
	rm -vf example libarch.a $(RELS)/*

libarch.a: $(LIBLIST)
	ar -rv libarch.a $(LIBLIST)

$(RELS)/evtread.o: $(SRCE)/evtread.c $(SRCE)/evtread.h
	mkdir -pv $(RELS)
	$(COMPILE) $(SRCE)/evtread.c

$(RELS)/relecture.o: $(SRCE)/relecture.c
	mkdir -pv $(RELS)
	$(COMPILE) $(SRCE)/relecture.c

$(RELS)/defauts.o: $(SRCE)/defauts.c
	mkdir -pv $(RELS)
	$(COMPILE) $(SRCE)/defauts.c

$(RELS)/claps.o: $(SRCE)/claps.c $(SRCE)/claps.h
	mkdir -pv $(RELS)
	$(COMPILE) $(SRCE)/claps.c

$(RELS)/dateheure.o: $(SRCE)/dateheure.c $(SRCE)/dateheure.h
	mkdir -pv $(RELS)
	$(COMPILE) $(SRCE)/dateheure.c

$(RELS)/decode.o: $(SRCE)/decode.c $(SRCE)/decode.h
	mkdir -pv $(RELS)
	$(COMPILE) $(SRCE)/decode.c

$(RELS)/example.o: $(SRCE)/example.c
	mkdir -pv $(RELS)
	$(COMPILE) $(SRCE)/example.c

