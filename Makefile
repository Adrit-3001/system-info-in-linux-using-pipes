CC = gcc
CFLAGS = -Wall -Wextra -std=c99

.PHONY: all clean

all: try

try: try.o fun.o
	$(CC) $(CFLAGS) -o $@ $^

try.o: try.c fun.h
	$(CC) $(CFLAGS) -c -o $@ $<

fun.o: fun.c fun.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f try try.o fun.o

help:
	@echo "clean:     Remove auto-generated files"
	@echo "help:      Display this help message"

## List of source files
#SRCS = systemMonitoring.c functions.h
#
## Object files
#OBJS = $(SRCS:.c=.o)
#
## Executable name
#EXEC = systemMonitoring
#
## Main target
#$(EXEC): $(OBJS)
#	$(CC) $(OBJS) -o $(EXEC)
#
## Compile source files into object files
#%.o: %.c
#	$(CC) -c $< -o $@
#
## Clean up
#clean:
#	rm -f $(EXEC) $(OBJS)
