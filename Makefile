CC = gcc
CFLAGS = -Wall -O2

all: prefetch_control

prefetch_control: prefetch_control.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f prefetch_control

.PHONY: all clean 