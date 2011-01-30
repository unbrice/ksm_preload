CC	= gcc
CFLAGS += -std=c99 -Wall -Wextra -Wcast-align -Wpointer-arith	\
	  -Wcast-align -Wno-sign-compare -Wconversion
#CFLAGS += -O0 -g -ggdb3 -Wbad-function-cast -DDEBUG # -pg
CFLAGS += -DNDEBUG -O2

.PHONY: all clean dist

all:    ksm-preload.so

dist:
	rm -f *.o
	rm -f callgrind.out.*

clean:  dist
	rm -f ksm-preload.so
	find -name '*.~' -exec rm {} \;

ksm-preload.so: ksm_preload.c Makefile
	$(CC) $(CFLAGS) --shared -fPIC -ldl $< -o $@
