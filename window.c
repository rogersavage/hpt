
#include <stdlib.h>

#include "window.h"
#include "manr.h"
#include "constants.h"

Window* createWindow(){
	Window* newWindow = malloc(sizeof(Window));
	newWindow->tty = start_term();
	newWindow->canvas = createCanvas(MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
	int max_chars_per_cell = 
			7 // Change both fg and bg color
			+ 3; // In case it's unicode
	int buf_size = 
			MAX_VIEW_AREA * max_chars_per_cell +
			3 + // Signal to reset cursor
			1; // Room for null terminator
	newWindow->buffer = malloc(buf_size);
	return newWindow;
}

void freeWindow(Window* window){
	free(window->buffer);
	freeCanvas(window->canvas);
	free(window);
}
