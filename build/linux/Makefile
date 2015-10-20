TOPDIR = ../..
LIBSIMDIRS = $(TOPDIR)/libsim
LIBZBASEDIRS = $(TOPDIR)/libzbase
INCLUDES = $(LIBSIMDIRS):$(LIBZBASEDIRS)
VPATH = $(TOPDIR):$(LIBSIMDIRS):$(LIBZBASEDIRS)

CC = gcc
CFLAGS = -c -O3
LIBS = -lm

TMPDIR = mk.tmp
LIBSIMSRCS = sim_log.c sim_utils.c sim_opt.c
LIBSIMOBJS = $(LIBSIMSRCS:%.c=$(TMPDIR)/%.o)
LIBSIM = libsim.a

LIBZBASESRCS = zhash.c zlist.c zqueue.c zstrq.c
LIBZBASEOBJS = $(LIBZBASESRCS:%.c=$(TMPDIR)/%.o)
LIBZBASE = libzbase.a

BINSRCS = ztest.c 
BINOBJS = $(BINSRCS:%.c=$(TMPDIR)/%.o)
OUTBIN = ztest


.PHONY: all
all : $(LIBSIM) $(LIBZBASE) $(OUTBIN) 


.PHONY: clean
clean: 
	@echo; echo "cleaning ..."
	rm -rf $(TMPDIR)
	rm -rf $(LIBSIM)
	rm -rf $(LIBZBASE)
	rm -rf $(OUTBIN)

$(LIBSIM): $(LIBSIMOBJS) 
	@echo; echo "[AR] linking '$@' ..."
	rm -f $@
	$(AR) -crs $@ $^
    
$(LIBZBASE): $(LIBZBASEOBJS)
	@echo; echo "[AR] linking '$@' ..."
	rm -f $@
	$(AR) -crs $@ $^
	
$(OUTBIN): $(BINOBJS) $(LIBSIM) $(LIBZBASE)
	@echo; echo "[LD] linking ..."
	cc -I$(LIBSIMDIRS) -I$(LIBZBASEDIRS) -o $@ $^

$(LIBSIMOBJS): $(LIBSIMDIRS)/*.h Makefile
$(LIBSIMOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -o $@ $<
    
$(LIBZBASEOBJS): $(LIBSIMDIRS)/*.h $(LIBZBASEDIRS)/*.h Makefile
$(LIBZBASEOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -I$(LIBZBASEDIRS) -o $@ $<

$(BINOBJS): $(LIBSIMDIRS)/*.h $(LIBZBASEDIRS)/*.h Makefile
$(BINOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -I$(LIBZBASEDIRS) -o $@ $<

$(TMPDIR):
	mkdir $(TMPDIR)
