CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = tte-c

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) -lm

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

test: $(TARGET)
	gcc $(CFLAGS) -I. tests/test_tte.c src/color.o src/terminal.o src/utils.o src/effects.o -o tests/test_tte -lm
	./tests/test_tte

.SUFFIXES: .c .o