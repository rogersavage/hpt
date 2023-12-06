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

Canvas* createCanvas(int width, int height);
void freeCanvas(Canvas* canvas);

int getCanvasBgColor(Canvas*, int x, int y);
int getCanvasFgColor(Canvas*, int x, int y);
char getCanvasCharacter(Canvas*, int x, int y);
void setCanvasCharacter(Canvas*, int x, int y, char character);
void setCanvasFgColor(Canvas*, int x, int y, int color);

#endif
