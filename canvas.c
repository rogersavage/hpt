#include "canvas.h"
#include <stdlib.h>
#include "constants.h"

int get_canvas_bg_color(Canvas* canvas, 
int x, int y){
    return canvas->cells[x + y * MAX_VIEW_WIDTH].
    bg_color;
}

int get_canvas_color(Canvas* canvas, 
int x, int y){
    return canvas->cells[x + y * MAX_VIEW_WIDTH].
    color;
}

char get_canvas_character(Canvas* canvas,
int x, int y){
    return canvas->cells[x + y * MAX_VIEW_WIDTH].
    character;
}

void set_canvas_color(Canvas* canvas,
int x, int y, int color){
    canvas->cells[x + y * canvas->width].
    color = color;
}
void set_canvas_character(Canvas* canvas,
int x, int y, char character){
    canvas->cells[x + y * canvas->width].
    character = character;
}

Canvas* create_canvas(int width, int height){
    Canvas* new_canvas = malloc(sizeof(Canvas));
    new_canvas->width = width;
    new_canvas->height = height;
    new_canvas->cells = malloc(sizeof(Cell)
    * width * height);
    for(int i=0; i<width * height; i++){
        new_canvas->cells[i].character = ' ';
        new_canvas->cells[i].color = 37;
        new_canvas->cells[i].bg_color = 40;
    }
    return new_canvas;
}

void freeCanvas(Canvas* canvas){
	free(canvas->cells);
	free(canvas);
}
