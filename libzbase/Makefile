LIBSIMDIRS = ../libsim
LIBZBASEDIRS = 
INCLUDES = $(LIBSIMDIRS):$(LIBZBASEDIRS)
LIBSIM = $(LIBSIMDIRS)/libsim.a

CC = gcc
CFLAGS = -c -O3
LIBS = -lm

TMPDIR = mk.tmp
LIBZBASESRCS = zhash.c zlist.c zqueue.c zstrq.c
LIBZBASEOBJS = $(LIBZBASESRCS:%.c=$(TMPDIR)/%.o)
LIBZBASE = libzbase.a


.PHONY: all
all : libsim $(LIBZBASE)

.PHONY: clean
clean: 
	@echo; echo "cleaning ..."
	rm -rf $(TMPDIR)
	rm -rf $(LIBZBASE)
	cd $(LIBSIMDIRS); make clean

$(LIBZBASE): $(LIBZBASEOBJS) | libsim
	@echo; echo "[AR] linking '$@' ..."
	rm -f $@
	$(AR) -crs $@ $^

$(LIBZBASEOBJS): $(LIBSIM) *.h Makefile
$(LIBZBASEOBJS): $(TMPDIR)/%.o:%.c | $(TMPDIR)
	@echo; echo "[CC] compiling: $< "
	$(CC) $(CFLAGS) -I$(LIBSIMDIRS) -I$(LIBZBASEDIRS) -l$(LIBSIM) -o $@ $<

.PHONY: libsim
libsim $(LIBSIM):
	cd $(LIBSIMDIRS); make

$(TMPDIR):
	mkdir $(TMPDIR)
