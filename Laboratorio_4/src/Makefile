OBJS = sdram.o clock.o console.o lcd-spi.o gfx.o

BINARY = sismografo

LDLIBS += -lm

LDSCRIPT = ../stm32f429i-discovery.ld

CFLAGS += --specs=nosys.specs -ffreestanding -nostdlib

include ../../Makefile.include

