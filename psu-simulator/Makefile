ODIR=.
SDIR=../project
HALDIR = ./HAL
HALINCDIR = $(SDIR)/inc

DEASPLAYDIR = $(ODIR)/deasplay
KEYPADDIR = $(ODIR)/keypad
LORENZLIBDIR = $(ODIR)/lorenzlib
MEGNUDIR = $(ODIR)/megnu
I2CDIR = $(ODIR)/i2c
DRIVERDIR = $(SDIR)/driver/adc
APPDIR = $(SDIR)/app

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
DEPS_ += $(KEYPADDIR)
DEPS_ += $(I2CDIR)
DEPS_ += $(DRIVERDIR)
DEPS_ += $(SDIR)

DEPSA = $(foreach dep,$(DEPS_),$(realpath $(dep)))
DEPS = $(addprefix -I, $(DEPSA))

OBJ_ += $(shell find $(DEASPLAYDIR) -name '*.c')
OBJ_ += $(wildcard $(LORENZLIBDIR)/*.c)
OBJ_ += $(wildcard $(HALDIR)/*.c)
OBJ_ += $(wildcard $(MEGNUDIR)/*.c)
OBJ_ += $(wildcard $(KEYPADDIR)/*.c)
OBJ_ += $(wildcard $(I2CDIR)/*.c)
OBJ_ += $(wildcard $(DRIVERDIR)/*.c)
OBJ_ += $(wildcard $(APPDIR)/*.c)
OBJ_ += $(wildcard $(SDIR)/*.c)

# remove the main file for the other project...
OBJ = $(filter-out $(SDIR)/main.c, $(OBJ_))

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
