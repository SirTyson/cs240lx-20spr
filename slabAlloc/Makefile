CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -I.

DEPS := $(wildcard ./*.h) 
SOURCES := $(wildcard ./*.c)
OBJ = $(SOURCES:.c=.o)
# the build target executable:
TARGET = slab-alloc

# all: $(TARGET)

# $(TARGET): $(TARGET).c
# 	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

slab-alloc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) -f $(OBJSS) $(TARGET)

