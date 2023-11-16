mansir : \
mansir.o \
fractal_noise.o \
canvas.o
	gcc mansir.o \
		fractal_noise.o \
		canvas.o \
		-o mansir -g
mansir.o : mansir.c
	gcc -c mansir.c
fractal_noise.o : fractal_noise.c
	gcc -c fractal_noise.c
canvas.o : canvas.c
	gcc -c canvas.c
	
