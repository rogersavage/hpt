#ifndef CANVAS_H
#define CANVAS_H

typedef struct cell_struct{
    int color;
    int bg_color;
    int character;
} Cell;

typedef struct canvas_struct{
    int width, height;
    Cell* cells;
} Canvas;

Canvas* create_canvas(int width, int height);
int get_canvas_bg_color(Canvas*, int x, int y);
int get_canvas_color(Canvas*, int x, int y);
char get_canvas_character(Canvas*, int x, int y);
void set_canvas_character(Canvas*, int x, int y,
char character);
void set_canvas_color(Canvas*, int x, int y,
int color);

#endif
