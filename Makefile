# By Brice Arnould <unbrice@vleu.net>
# Copyright (C) 2011 Gandi SAS.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
