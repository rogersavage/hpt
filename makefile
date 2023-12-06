demo : \
demo.o \
manr.o \
fractal_noise.o \
window.o \
canvas.o
	gcc manr.o \
		demo.o \
		fractal_noise.o \
		canvas.o \
		window.o \
		-o demo -g

demo.o : demo.c
	gcc -c demo.c
manr.o : manr.c
	gcc -c manr.c
fractal_noise.o : fractal_noise.c
	gcc -c fractal_noise.c
canvas.o : canvas.c
	gcc -c canvas.c
window.o : window.c
	gcc -c window.c
	
