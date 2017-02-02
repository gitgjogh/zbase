LIBSIMDIRS = libsim
LIBZBASEDIRS = libzbase
INCLUDES = $(LIBSIMDIRS):$(LIBZBASEDIRS)
LIBSIM = $(LIBSIMDIRS)/libsim.a
LIBZBASE = $(LIBZBASEDIRS)/libzbase.a

CC = gcc
CFLAGS = -c -O3
LIBS = -lm

TMPDIR = mk.tmp
BINSRCS = ztest.c
BINOBJS = $(BINSRCS:%.c=$(TMPDIR)/%.o)
OUTBIN = ztest


.PHONY: all
all : libzbase $(OUTBIN) 


.PHONY: clean
clean: 
	@echo; echo "cleaning ..."
	rm -rf $(TMPDIR)
	rm -rf $(OUTBIN)
	cd $(LIBZBASEDIRS); make clean
	
$(OUTBIN): $(BINOBJS) $(LIBSIM) $(LIBZBASE) | libzbase
	@echo; echo "[LD] linking ..."
	cc -I$(LIBSIMDIRS) -I$(LIBZBASEDIRS) -o $@ $^ 

$(BINOBJS): $(LIBZBASE) Makefile
$(BINOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -I$(LIBZBASEDIRS) -o $@ $<

.PHONY: libzbase
libzbase $(LIBZBASE):
	cd $(LIBZBASEDIRS); make

$(TMPDIR):
	mkdir $(TMPDIR)
