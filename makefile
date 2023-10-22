term : \
term.o \
fractal_noise.o \
canvas.o
	gcc term.o \
		fractal_noise.o \
		canvas.o \
		-o term -g
term.o : term.c
	gcc -c term.c
fractal_noise.o : fractal_noise.c
	gcc -c fractal_noise.c
canvas.o : canvas.c
	gcc -c canvas.c
	
