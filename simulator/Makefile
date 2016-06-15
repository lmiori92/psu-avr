IDIR =../project
CC=gcc
CFLAGS=-I$(IDIR) -g

ODIR=.
SDIR=../project
LDIR =../lib
HALDIR = ./HAL

LIBS=-lm -lncurses -lpthread

_DEPS = psu.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = lib.o remote.o display.o psu.o keypad.o
OBJ = $(patsubst %,$(SDIR)/%,$(_OBJ))

#special case for the simulator
OBJ += main.o display_config.o $(HALDIR)/display_hal_ncurses.o $(HALDIR)/uart.o $(HALDIR)/time.o $(HALDIR)/system.o $(HALDIR)/pwm.o $(HALDIR)/adc.o $(HALDIR)/encoder.o

all: simulator

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

simulator: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~  
	rm -f $(SDIR)/*.o
