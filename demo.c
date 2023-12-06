
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>

#include "fractal_noise.h"
#include "constants.h"
#include "manr.h"
#include "canvas.h"
#include "minunit.h"
#include "window.h"

int term_width, term_height;

void animate_fractal_noise(Canvas* canvas, int* noise, int ticks){
    for(int y=0; y<term_height; y++){
        for(int x=0; x<term_width; x++){
					int offset = x + y * MAX_VIEW_WIDTH;
					int sample = noise[offset] + ticks;
					int fg_color = sample / 32 % 10 + 30;
					char* texture = " ./#";
					set_canvas_character(canvas, x, y, texture[sample / 16 % 4]);
					set_canvas_color(canvas, x, y, fg_color);
        }
    }
}

void draw_text(Canvas* canvas, int x, int y, char* text){
	for(int i=0; i<strlen(text); i++){
		set_canvas_character(canvas, x + i, y, text[i]);
		set_canvas_color(canvas, x + i, y, WHITE);
	}
}

int main(){
	int noise[MAX_VIEW_AREA];
	fractal_noise(rand() % 128, 128, MAX_VIEW_WIDTH, 8, 1.0f, noise);
	int quit = 0;
	int ticks = 0;
	Window* window = createWindow();
	char* message = "Press 'q' to quit";
	while(!quit){
			char user_input = input();
			if (user_input == 'q') quit = 1;
			animate_fractal_noise(window->canvas, noise, ticks);
			draw_text(window->canvas, term_width / 2 - strlen(message) / 2,
					term_height / 2, message);
			termRefresh(window);
			ticks++;
	usleep(1000000/60);
	}
	endTerm(window);
	return 0;
}
