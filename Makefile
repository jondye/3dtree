all:	source.c
	gcc -ansi -pedantic -Wall -ffast-math -O3 source.c -lGL -lGLU -lglut
	./a.out

debug:	source.c
	gcc -ansi -pedantic -Wall -g source.c -lGL -lGLU -lglut
	xxgdb ./a.out