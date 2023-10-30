hpt: \
hpt.o \
fractal_noise.o \
canvas.o
	gcc hpt.o \
		fractal_noise.o \
		canvas.o \
		-o hpt -g
hpt.o : hpt.c
	gcc -c hpt.c
fractal_noise.o : fractal_noise.c
	gcc -c fractal_noise.c
canvas.o : canvas.c
	gcc -c canvas.c
	
