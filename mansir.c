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
#include "mansir.h"
#include "canvas.h"

#define BLACK 30
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37

#define BR_BLACK 90
#define BR_RED 91
#define BR_GREEN 92
#define BR_YELLOW 93
#define BR_BLUE 94
#define BR_MAGENTA 95
#define BR_CYAN 96

#define BG_BLACK 40
#define BG_RED 41
#define BG_GREEN 42
#define BG_YELLOW 43
#define BG_BLUE 44
#define BG_MAGENTA 45
#define BG_CYAN 46
#define BG_WHITE 47


// Globals related to terminal settings and properties.
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

void term_refresh(char* buffer, Canvas* canvas, Canvas* backbuffer,
int tty){
	#define ADD(ch) \
		*pointer++ = ch
	// Move cursor to 1,1
	buffer[0] = '\x1b';
	buffer[1] = '[';
	buffer[2] = 'H';
	char* pointer = buffer + 3;
	char prev_char = '\0';
	int skip_length = 0;
	int fg_color;
	int bg_color;

	for(int y = 0; y < term_height; y++){
		fg_color = -1;
		bg_color = -1;

		for(int x = 0; x < term_width; x++){
			int offset = x + y * MAX_VIEW_WIDTH;
			int next_fg_color = canvas->cells[offset].color;
			int next_bg_color = canvas->cells[offset].bg_color;
			int old_fg = backbuffer->cells[offset].color;
			int old_bg = backbuffer->cells[offset].bg_color;

			// Check if either or both the fg/bg color needs to be changed
			if(fg_color != next_fg_color){
				if(bg_color != next_bg_color){
					ADD('\x1b');
					ADD('[');
					ADD(next_fg_color / 10 + '0');
					ADD(next_fg_color % 10 + '0');
					ADD(';');
					ADD('4');
					ADD(next_bg_color % 10 + '0');
					ADD('m');
					fg_color = next_fg_color;
					bg_color = next_bg_color;
				} else{
					ADD('\x1b');
					ADD('[');
					ADD(next_fg_color / 10 + '0');
					ADD(next_fg_color % 10 + '0');
					ADD('m');
					fg_color = next_fg_color;
				}
			} else if(bg_color != next_bg_color){
					ADD('\x1b');
					ADD('[');
					ADD('4');
					ADD(next_bg_color % 10 + '0');
					ADD('m');
					bg_color = next_bg_color;
			}

			// Compare next character to character already onscreen,
			// To see if we can just skip it.
			char next_char = canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			char old_char = backbuffer->cells[x + y * MAX_VIEW_WIDTH].character;
			
			if(next_char == old_char &&
                next_fg_color == old_fg &&
                next_bg_color == old_bg){
			    skip_length++;
			}
			if(skip_length > 0){
				if(skip_length < 6){
					for(int i=0; i<skip_length - 1; i++){
						ADD(canvas->cells[offset - 1].character);
					}
				} else{
					ADD('\x1b');
					ADD('[');
					if(x > 99) ADD((x) / 100 + '0');
					if(x > 9) ADD((x) % 100 / 10 + '0');
					ADD((x) % 10 + '0');
					ADD('G');
				}
			}
			ADD(next_char);
			skip_length = 0;
		}
		if(skip_length > 0) ADD('\n');
	}
	// Cut off that last newline so the screen doesn't scroll
	if(*(pointer-1) == '\n') pointer--;
	//fwrite(buffer, 1, pointer - buffer, tty);
	write(tty, buffer, pointer - buffer);
	
	// Flip buffer
	Cell* temp = canvas->cells;
	canvas->cells = backbuffer->cells;
	backbuffer->cells = temp;
}
