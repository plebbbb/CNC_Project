default: nfx9000rtu

MODINC := $(shell ./find-modinc)
BINDIR := $(shell ./find-bindir)


ifeq "$(MODINC)" ""
$(error Required files for building components not present.  Install emc2-dev)
endif
include $(MODINC)

ifeq ($(RUN_IN_PLACE),no)
EXTRA_CFLAGS += -I$(EMC2_HOME)/include/emc2
LIBDIR := $(shell ./find-libdir)
ifeq "$(LIBDIR)" ""
$(error LIBDIR not found)
endif
endif

CFLAGS := $(EXTRA_CFLAGS) -URTAPI -U__MODULE__ -DULAPI -Os
CFLAGS += $(shell pkg-config --cflags glib-2.0)
LFLAGS := -Wl,-rpath,$(LIBDIR) -L$(LIBDIR) -llinuxcnchal
LFLAGS += $(shell pkg-config --libs glib-2.0)

include .o/nfx9000rtu.d .o/modbus.d

install: nfx9000rtu
	cp nfx9000rtu $(BINDIR)

nfx9000rtu:.o/nfx9000rtu.o .o/modbus.o
	$(CC) -o $@ $^ $(LFLAGS)

.o/%.o: %.c
	mkdir -p .o
	$(CC) $(CFLAGS) -o $@ -c $<

.o/%.d: %.c
	mkdir -p .o
	$(CC) $(CFLAGS) -MM -MT "$@ $(patsubst %.d,%.o,$@)" $< -o $@.tmp \
			&& mv $@.tmp $@

clean:
	-rm -f nfx9000rtu
	-rm -rf .o

