TOPDIR = ../..
SRCDIRS = $(TOPDIR)/src
INCDIRS = $(TOPDIR)/inc
INCLUDES = $(INCDIRS)
VPATH = $(TOPDIR):$(SRCDIRS):$(INCLUDES) 

CC = gcc
CFLAGS = -c -O3 -I$(INCLUDES)
CFLAGS += -DZLIST_TEST 
CFLAGS += -DZLOG_TEST 
CFLAGS += -DZOPT_TEST 
CFLAGS += -DZQUEUE_TEST 
CFLAGS += -DZSTRQ_TEST 
CFLAGS += -DZHASH_TEST
LIBS = -lm

TMPDIR = mk.tmp
LIBSRCS = zhash.c zlist.c zopt.c zqueue.c zstrq.c sim_log.c sim_utils.c sim_opt.c
LIBOBJS = $(LIBSRCS:%.c=$(TMPDIR)/%.o)
OUTLIB = libzbase.a

BINSRCS = ztest.c 
BINOBJS = $(BINSRCS:%.c=$(TMPDIR)/%.o)
OUTBIN = ztest

.PHONY: all
all : $(OUTLIB) $(OUTBIN)

.PHONY: clean
clean: 
	@echo; echo "cleaning ..."
	rm -rf $(TMPDIR)
	rm -rf $(OUTLIB)
	rm -rf $(OUTBIN)

$(OUTLIB): $(LIBOBJS)
	@echo; echo "[AR] linking ..."
	rm -f $@
	$(AR) -crs $@ $^
	
$(OUTBIN): $(BINOBJS) $(OUTLIB)
	@echo; echo "[LD] linking"
	$(CC) -o $@ $^ -I../../ -I$(INCLUDES)
    
$(LIBOBJS) $(BINOBJS): $(INCDIRS)/*.h Makefile | $(TMPDIR)

$(LIBOBJS): $(TMPDIR)/%.o:%.c
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -o $@ $<

$(BINOBJS): $(TMPDIR)/%.o:%.c
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -o $@ $<

$(TMPDIR):
	mkdir $(TMPDIR)
