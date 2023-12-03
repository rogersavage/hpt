manr : \
manr.o \
fractal_noise.o \
canvas.o
	gcc manr.o \
		fractal_noise.o \
		canvas.o \
		-o manr -g
manr.o : manr.c
	gcc -c manr.c
fractal_noise.o : fractal_noise.c
	gcc -c fractal_noise.c
canvas.o : canvas.c
	gcc -c canvas.c
	
