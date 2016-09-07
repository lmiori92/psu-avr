IDIR =../project
CC=gcc $(sdl-config --static-libs) -lm
CFLAGS=-I$(IDIR) -g
LDFLAGS= $(sdl-config --static-libs) -lm

ODIR=.
SDIR=../project
LDIR =../lib
HALDIR = ./HAL

LIBS=-lm -lncurses -lpthread

_DEPS = psu.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = lib.o remote.o display.o psu.o keypad.o pid.o menu.o settings.o
OBJ = $(patsubst %,$(SDIR)/%,$(_OBJ))

#special case for the simulator
OBJ += main.o display_config.o $(HALDIR)/display_hal_ncurses.o $(HALDIR)/uart.o $(HALDIR)/time.o $(HALDIR)/system.o $(HALDIR)/pwm.o $(HALDIR)/adc.o $(HALDIR)/encoder.o $(HALDIR)/eeprom.o

all: simulator

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) ${LDFLAGS}

simulator: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS) ${LDFLAGS}

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~  
	rm -f $(SDIR)/*.o
	rm -f $(HALDIR)/*.o
