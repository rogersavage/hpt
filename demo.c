
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

int term_width, term_height;

void animate_fractal_noise(Canvas* canvas, int* noise, int ticks){
    for(int y=0; y<term_height; y++){
        for(int x=0; x<term_width; x++){
					int offset = x + y * MAX_VIEW_WIDTH;
					int sample = noise[offset] + ticks;
					int fg_color = sample / 8 % 10 + 30;
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
	int tty = open(ttyname(STDIN_FILENO), O_RDWR | O_SYNC);
	start_term();
	int ticks = 0;
	Canvas* canvas = create_canvas(
	MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
	Canvas* backbuffer = create_canvas(
	MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
	int noise[MAX_VIEW_AREA];
	fractal_noise(rand() % 128, 128, MAX_VIEW_WIDTH, 8, 1.0f, 
	noise);
	int max_chars_per_cell = 
			7 // Change both fg and bg color
			+ 3; // In case it's unicode
	int buf_size = 
			MAX_VIEW_AREA * max_chars_per_cell +
			3 + // Signal to reset cursor
			1; // Room for null terminator
	char* buffer = malloc(buf_size);
	int quit = 0;
	char* message = "Press 'q' to quit";
	while(!quit){
			char user_input = input();
			if (user_input == 'q') quit = 1;
			animate_fractal_noise(canvas, noise, ticks);
			draw_text(canvas, term_width / 2 - strlen(message) / 2,
					term_height / 2, message);
			term_refresh(buffer, canvas, backbuffer, tty);
			ticks++;
	usleep(1000000/60);
	}
	end_term();
	return 0;
}
