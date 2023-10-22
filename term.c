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
int term_width, term_height;
int display_width, display_height;

void die(int i){
    exit(1);
}

void resize(int i){
    ioctl(1, TIOCGWINSZ, &ws);
    term_height = ws.ws_row;
    term_width = ws.ws_col;
    display_width = term_width > MAX_VIEW_WIDTH ? MAX_VIEW_WIDTH : term_width;
    display_height = term_height > MAX_VIEW_HEIGHT ? MAX_VIEW_HEIGHT : 
    term_height;
}

int start_term(){
    // Enter the alternate buffer.
    printf("\x1b[?1049H");
		// Turn off stdout buffering
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
		// Make input non-blocking
		fcntl(2, F_SETFL, fcntl(2, F_GETFL) | O_NONBLOCK);
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
    Canvas* canvas = create_canvas(
    MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
    int noise[MAX_VIEW_AREA];
    fractal_noise(0, 128, MAX_VIEW_WIDTH, 8, 1.0f, 
    noise);
    char* texture = " ./#";
    int buf_size = MAX_VIEW_AREA * 10 + 6;
    char* buffer = malloc(buf_size);
    int quit = 0;
    while(!quit){
        if (input() == 'q') quit = 1;
				for(int y=0; y<term_height; y++){
					for(int x=0; x<term_width; x++){
						int offset = x + y * MAX_VIEW_WIDTH;
							canvas->cells[offset].character = 
							texture[(noise[offset] + ticks) / 16 % 4];
					}
				}
        term_refresh(buffer, canvas);
        ticks++;
		usleep(1000000/60);
    }
    end_term();
    return 0;
}

void term_refresh(char* buffer, Canvas* canvas){
	buffer[0] = '\x1b';
	buffer[1] = '[';
	buffer[2] = 'H';
	char* pointer = buffer + 3;
	char* bottom_right_corner = term_width * term_height + pointer;
	char* endline = pointer + term_width;
	int x = 0;
	int y = 0;
	while(pointer < bottom_right_corner){
		while(pointer < endline){
			*pointer++ = canvas->cells[x++ + y * MAX_VIEW_WIDTH].character;
		}
		endline += term_width;
		y++;
		x = 0;
	}
	write(1, buffer, pointer - buffer);
}
