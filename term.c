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
		Canvas* old_canvas = create_canvas(
			MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
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
        term_refresh(buffer, canvas, old_canvas);
        ticks++;
		usleep(1000000/60);
    }
    end_term();
    return 0;
}
/*
void term_refresh_test(char* buffer, Canvas* canvas){
	buffer[0] = '\x1b';
	buffer[1] = '[';
	buffer[2] = 'H';
	char* pointer = buffer + 3;
	for(int i=0; i<term_height; i++){
		ADD('A');
		MOVE(1);
		ADD('B');
		MOVE(1);
		ADD('\n');
	}

	write(1, buffer, pointer - buffer);
}
*/

void term_refresh(char* buffer, Canvas* canvas, Canvas* old_canvas){
	#define ADD(ch) *pointer++ = ch
	#define MOVE(dist) \
		ADD('\x1b'); \
		ADD('['); \
		ADD(dist + '0'); \
		ADD('C'); \
		endline += 3; \
		bottom_right_corner += 3
	#define MOVE_10(dist) \
		ADD('\x1b'); \
		ADD('['); \
		ADD(dist / 10 + '0'); \
		ADD(dist % 10 + '0'); \
		ADD('C')
	#define MOVE_100(dist) \
		ADD('\x1b'); \
		ADD('['); \
		ADD(dist / 100 + '0'); \
		ADD((dist % 100) / 10 + '0'); \
		ADD((dist % 100) % 10 + '0');
	buffer[0] = '\x1b';
	buffer[1] = '[';
	buffer[2] = 'H';
	char* pointer = buffer + 3;
	char* bottom_right_corner = term_width * term_height + pointer;
	char* endline = pointer + term_width;
	int x = 0;
	int y = 0;
	char prev_char = '\0';
	int skip_length = 0;
	for(y = 0; y < term_height; y++){
		for(x = 0; x < term_width; x++){
			char next_char = canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			char old_char = old_canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			if(next_char != old_char){
				if(skip_length > 0){
					if(skip_length < 5 ){
						if(skip_length == 4)
							ADD(canvas->cells[x + y * MAX_VIEW_WIDTH - 4].character);
						if(skip_length >= 3)
							ADD(canvas->cells[x + y * MAX_VIEW_WIDTH - 3].character);
						if(skip_length >= 2)
							ADD(canvas->cells[x + y * MAX_VIEW_WIDTH - 2].character);
							ADD(canvas->cells[x + y * MAX_VIEW_WIDTH - 1].character);
					} else if(skip_length >= 100){
						MOVE_100(skip_length);
					} else if(skip_length >= 10 && skip_length >= 4){
						MOVE_10(skip_length);
					} else {
						MOVE(skip_length);
					}
				}
					skip_length = 0;
					ADD(next_char);
			} else{
				skip_length++;
			}
		}
		skip_length = 0;
		ADD('\n');
	}
	// Cut off that last newline so the screen doesn't scroll
	pointer--;
	write(1, buffer, pointer - buffer);
	// Change this to a buffer flip instead of copying
	Cell* temp = canvas->cells;
	canvas->cells = old_canvas->cells;
	old_canvas->cells = temp;
}
