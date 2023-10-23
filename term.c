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
		// Main screenbuffer
    Canvas* canvas = create_canvas(
    MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
		// Alternate screenbuffer, used to skip
		// redrawing cells redunantly
		Canvas* old_canvas = create_canvas(
			MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
		// Fractal noise texture
    int noise[MAX_VIEW_AREA];
    fractal_noise(0, 128, MAX_VIEW_WIDTH, 8, 1.0f, 
    noise);
		// ASCII shading ramp
		// Currently I'm intercepting these and replacing
		// them with unicode extended characters
    char* texture = " ./#";

		// This is the encoded output string that will be
		// assembled and then pushed to stdout once per frame.
		// It needs to be have room for, theoretically
		int max_chars_per_cell = 
			7 // Change both fg and bg color
			+ 2; // In case it's unicode
    int buf_size = 
			MAX_VIEW_AREA * max_chars_per_cell +
			3 + // Signal to reset cursor
			1; // Room for null terminator
    char* buffer = malloc(buf_size);

    int quit = 0;
    while(!quit){
        if (input() == 'q') quit = 1;
				for(int y=0; y<term_height; y++){
					for(int x=0; x<term_width; x++){
						int offset = x + y * MAX_VIEW_WIDTH;
							canvas->cells[offset].character = 
							texture[(noise[offset] + ticks) / 16 % 4];
							canvas->cells[offset].color = 
								((noise[offset] + ticks) / 64) % 8 + 90;
							canvas->cells[offset].bg_color = ticks / 512 % 8 + 40;
					}
				}
        term_refresh(buffer, canvas, old_canvas);
        ticks++;
		usleep(1000000/60);
    }
    end_term();
    return 0;
}

void term_refresh(char* buffer, Canvas* canvas, Canvas* old_canvas){
	#define ADD_UTF(utf_char){ \
		*pointer++ = '\0'; \
		strcat(buffer, utf_char); \
		pointer += 2; \
	}
	#define ADD(ch) \
		if(ch == '.'){ADD_UTF("\u2592");} \
		else if(ch == '/'){ADD_UTF("\u2593");} \
		else if(ch == '#'){ADD_UTF("\u2588");} \
		else *pointer++ = ch
	#define MOVE(dist) \
		ADD('\x1b'); \
		ADD('['); \
		ADD(dist + '0'); \
		ADD('C');
	#define MOVE_10(dist) \
		ADD('\x1b'); \
		ADD('['); \
		ADD(dist / 10 + '0'); \
		ADD(dist % 10 + '0'); \
		ADD('C')
	#define COLOR(color) \
		ADD('\x1b'); \
		ADD('['); \
		ADD(color / 10 + '0'); \
		ADD(color % 10 + '0'); \
		ADD('m')
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
	char prev_char = '\0';
	int skip_length = 0;
	char fg_color = WHITE;
	char bg_color = BLACK;

	for(int y = 0; y < term_height; y++){
		for(int x = 0; x < term_width; x++){
			int offset = x + y * MAX_VIEW_WIDTH;
			char next_fg_color = canvas->cells[offset].color;
			char next_bg_color = canvas->cells[offset].bg_color;
			if(fg_color != next_fg_color){
				if(bg_color != next_bg_color){
					ADD('\x1b');
					ADD('[');
					ADD(next_fg_color / 10 + '0');
					ADD(next_fg_color % 10 + '0');
					ADD(';');
					ADD(next_bg_color / 10 + '0');
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
					fg_color = next_fg_color;
					ADD('\x1b');
					ADD('[');
					ADD(next_fg_color / 10 + '0');
					ADD(next_fg_color % 10 + '0');
					ADD('m');
					bg_color = next_bg_color;
			}

			char next_char = canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			char old_char = old_canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			if(next_char != old_char){
				if(skip_length > 0){
					if(skip_length < 5 ){
						char skip_char;
						int offset = x + y * MAX_VIEW_WIDTH;
						if(skip_length == 4){
							skip_char = canvas->cells[offset - 4].character;
							ADD(skip_char);
						}
						if(skip_length >= 3)
							ADD(canvas->cells[x + y * MAX_VIEW_WIDTH - 3].character);
						if(skip_length >= 2)
							ADD(canvas->cells[x + y * MAX_VIEW_WIDTH - 2].character);
							ADD(canvas->cells[x + y * MAX_VIEW_WIDTH - 1].character);
					} else if(skip_length >= 100){
						MOVE_100(skip_length);
					} else if(skip_length >= 10 && skip_length >= 5){
						MOVE_10(skip_length);
					} else {
						MOVE(skip_length);
					}
				}
					skip_length = 0;					
					if(next_char == '.'){
						ADD('\0');
						strcat(buffer, "\u2592");
						pointer += 2;
					} else if(next_char == '/'){
						ADD('\0');
						strcat(buffer, "\u2593");
						pointer += 2;
					} else if(next_char == '#'){
						ADD('\0');
						strcat(buffer, "\u2588");
						pointer += 2;
					} else{
						ADD(next_char);
					}
			} else{
				skip_length++;
			}
		}
		skip_length = 0;
		ADD('\n');
	}
	// Cut off that last newline so the screen doesn't scroll
	pointer--;

	// Add a null terminator so I can use fputs
	*pointer = '\0';
	// Using fputs instead of a raw write so I can get
	// unicode characters
	fputs(buffer, stdout);
	//write(1, buffer, pointer - buffer);
	
	// Flip buffer
	Cell* temp = canvas->cells;
	canvas->cells = old_canvas->cells;
	old_canvas->cells = temp;
}
