CC=gcc
CFLAGS+=-Wall -pedantic -std=c99 -ffast-math -O3
LDLIBS+=-lGL -lGLU -lglut -lpng

3dtree:	read_png.o 3dtree.o

clean:
	-rm *.o 3dtree
