#ifndef WINDOW_H
#define WINDOW_H

#include "canvas.h"

typedef struct Window{
	int tty;
	Canvas* canvas;
	char* buffer;
} Window;

Window* createWindow();
void freeWindow();

#endif
