#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "fractal_noise.h"
#include "constants.h"
#include "term.h"
#include "canvas.h"

struct winsize ws;
struct termios backup;
struct termios t;
int term_width;
int term_height;
int display_width, display_height;
char* buffer;
int buf_size;
Canvas* canvas;


void die(int i){
    exit(1);
}

void resize(int i){
    ioctl(1, TIOCGWINSZ, &ws);
    term_height = ws.ws_row;
    term_width = ws.ws_col;
    display_width = term_width > MAX_VIEW_WIDTH ?
	MAX_VIEW_WIDTH : term_width;
    display_height = term_height > 
    MAX_VIEW_HEIGHT ? MAX_VIEW_HEIGHT : 
    term_height;
}

int start_term(){
    // Enter the alternate buffer.
    printf("\x1b[?1049H");
    char ch_buffer;
    setvbuf(stdout, &ch_buffer, _IONBF, 1); 
    // Clear the screen.
    printf("\x1b[2J");
    // Hide the cursor.
    printf("\x1b[?25l");
    // Save the initial term settings.
    tcgetattr(1, &backup);
    t = backup;
    // Turn off echo and canonical
    // mode.
    t.c_lflag &= (~ECHO & ~ICANON);
    // Send the new settings to the term
    tcsetattr(1, TCSANOW, &t);
    signal(SIGWINCH, resize);
    resize(0);
    signal(SIGTERM, die);
    signal(SIGINT, die);
    fcntl(2, F_SETFL, fcntl(
		2, F_GETFL) |
	    O_NONBLOCK);
}

void restore(void){
    tcsetattr(1, TCSANOW, &backup);
}

void end_term(){
    // Reset colors
    printf("\x1b[0m");
    // Clear the alternate buffer.
    printf("\x1b[2J");
    // Return to the standard buffer.
    printf("\x1b[?1049L");
    // Show the cursor.
    printf("\x1b[?25h");

    fcntl(2, F_SETFL, fcntl(
		2, F_GETFL) &
	    ~O_NONBLOCK);
    restore();
    fputs("\x1b[1;1H", stdout);
}

char input(){
    char ch; 
    read(1, &ch, 1);
    return ch;
}

int main(){
    start_term();
    int ticks = 0;
    canvas = create_canvas(
    MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
    int noise[MAX_VIEW_AREA];
    fractal_noise(0, 128, MAX_VIEW_WIDTH, 8, 1.0f, 
    noise);
    char* texture = "  ..//##";
    for(int i=0; i<MAX_VIEW_AREA; i++){
        //canvas->cells[i].character = 
        //texture[noise[i] / 8 % 8];
				canvas->cells[i].character = 
					i % 8;
        canvas->cells[i].color = 37;
        canvas->cells[i].bg_color = 40;
    }
    int buf_size = MAX_VIEW_AREA * 10 + 6;
    buffer = malloc(buf_size);
    int quit = 0;
    while(!quit){
        if (input() == 'q') quit = 1;
        for(int i=0; i<MAX_VIEW_AREA; i++){
            canvas->cells[i].character = 
            texture[(noise[i] + ticks) / 8 % 8];
            canvas->cells[i].color = (noise[i] + ticks / 256)
                % 64 / 8 + 30;
            canvas->cells[i].bg_color = 40;
        }
        term_refresh(buffer);
        ticks++;
		usleep(1000000/60);
    }
    end_term();
    return 0;
}

void term_refresh(char* buffer){
		buffer[0] = '\x1b';
		buffer[1] = '[';
		buffer[2] = '1';
		buffer[3] = ';';
		buffer[4] = '1';
		buffer[5] = 'H';
    char* pointer = buffer + 6;
    char next_char;
    for(int y=0; y<term_height; y++){
			for(int x=0; x<term_width; x++){
					next_char = get_canvas_character(canvas, x, y);
					*pointer++ = canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			}
    write(1, buffer, pointer - buffer);
		pointer = buffer;
		}
}
