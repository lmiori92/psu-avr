ODIR=.
SDIR=../project
HALDIR = ./HAL
HALINCDIR = $(SDIR)/HAL/inc
DEASPLAYDIR = $(SDIR)/deasplay
KEYPADDIR = $(SDIR)/keypad
LORENZLIBDIR = $(SDIR)/lorenzlib
MEGNUDIR = $(SDIR)/megnu

LIBS=-lm -lncurses -lpthread

#_DEPS = psu.h
#DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

#DEPS += $(wildcard $(HALDIR)/*.h)

#DEPS += $(wildcard $(LORENZLIBDIR)/*.h)
#DEPS += $(wildcard $(HALINCDIR)/*.h)
#DEPS += $(wildcard $(DEASPLAYDIR)/*.h)
#DEPS += $(wildcard $(MEGNUDIR)/*.h)
#DEPS += $(wildcard $(SDIR)/*.h)

DEPS_ += $(LORENZLIBDIR)
DEPS_ += $(HALINCDIR)
DEPS_ += $(DEASPLAYDIR)
DEPS_ += $(MEGNUDIR)
DEPS_ += $(SDIR)

DEPSA = $(foreach dep,$(DEPS_),$(realpath $(dep)))
DEPS = $(addprefix -I, $(DEPSA))

OBJ += $(wildcard $(LORENZLIBDIR)/*.c)
OBJ += $(wildcard $(HALDIR)/*.c)
OBJ += $(wildcard $(DEASPLAYDIR)/*.c)
OBJ += $(wildcard $(MEGNUDIR)/*.c)
OBJ += $(wildcard $(SDIR)/*.c)

OBJ += $(ODIR)/main.c

#_OBJ = $(LORENZLIBDIR)/lib.o remote.o display.o psu.o keypad.o pid.o menu.o settings.o
#OBJ = $(patsubst %,$(SDIR)/%,$(_OBJ))
OBJECTS = $(OBJ:.c=.o)

#special case for the simulator
#OBJ += main.o display_config.o $(HALDIR)/display_hal_ncurses.o $(HALDIR)/uart.o $(HALDIR)/time.o $(HALDIR)/system.o $(HALDIR)/pwm.o $(HALDIR)/adc.o $(HALDIR)/encoder.o $(HALDIR)/eeprom.o

CC=gcc -lm
CFLAGS=$(DEPS) -g
LDFLAGS= -lm $(LIBS)
EXECUTABLE=psu-simulator

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

#simulator: $(OBJ)
#	gcc -o $@ $^ $(CFLAGS) $(LIBS) ${LDFLAGS}

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core 
	rm -f $(SDIR)/*.o
	rm -f $(HALDIR)/*.o	
	rm -f $(HALINCDIR)/*.o
	rm -f $(DEASPLAYDIR)/*.o
	rm -f $(KEYPADDIR)/*.o
	rm -f $(KEYPADDIR)/*.o
	rm -f $(LORENZLIBDIR)/*.o
	rm -f $(MEGNUDIR)/*.o
