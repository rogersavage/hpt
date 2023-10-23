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

// ANSI 8+8 colors
// I've tried this different ways,
// I figure since I can't guarantee bright backgrounds are
// available, there's no use in breaking up the colors into two
// digits. (I.e. since my TTY can't do BRIGHT_BG + GREEN).
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
#define BR_WHITE 97

#define BG_BLACK 40
#define BG_RED 41
#define BG_GREEN 42
#define BG_YELLOW 43
#define BG_BLUE 44
#define BG_MAGENTA 45
#define BG_CYAN 46
#define BG_WHITE 47

// I'm using extended UTF-8 symbols that require a three-byte
// sequence. The first two bytes are always the same,
// and for the third I use these defines to make them readable
#define UTF_BYTE_0 0xE2
#define UTF_BYTE_1 0x96

#define LIGHT_SHADE 0x91
#define SHADE 0x92
#define DARK_SHADE 0x93
#define FULL_BLOCK 0x88 

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

int main(){
    start_term();
    int ticks = 0;

		// Main screenbuffer
    Canvas* canvas = create_canvas(
    MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);

		// Alternate screenbuffer, used to skip
		// redrawing cells redunantly.
		// This is by far the most important performance
		// aspect in this code.
		Canvas* old_canvas = create_canvas(
			MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);

		// Fractal noise texture
    int noise[MAX_VIEW_AREA];
    fractal_noise(0, 128, MAX_VIEW_WIDTH, 8, 1.0f, 
    noise);

		// This is a gradient ramp, with dithering symbols
		// of increasing intensity
    char texture[] = {
			' ',
			LIGHT_SHADE, 
			SHADE, 
			DARK_SHADE, 
			//FULL_BLOCK
		};

		// This is the encoded output string that will be
		// assembled and then pushed to stdout once per frame.
		// It needs to be have room for, theoretically
		// a color change sequence for every single cell,
		// as well as two extra characters in case it's
		// a unicode symbol.
		int max_chars_per_cell = 
			7 // Change both fg and bg color
			+ 3; // In case it's unicode
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
	// None of this uses C std string functions. Instead I build the
	// output buffer by inserting the new character and incrementing
	// the insertion pointer.
	#define ADD_UTF(utf_char){ \
		*pointer++ = UTF_BYTE_0; \
		*pointer++ = UTF_BYTE_1; \
		*pointer++ = utf_char; \
	}
	#define ADD(ch) \
		if(ch == (char)LIGHT_SHADE){ADD_UTF(LIGHT_SHADE);} \
		else if(ch == (char)SHADE){ADD_UTF(SHADE);} \
		else if(ch == (char)DARK_SHADE){ADD_UTF(DARK_SHADE);} \
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
			
			// Check if either or both the fg/bg color needs to be changed
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

			// Compare next character to character already onscreen,
			// To see if we can just skip it.
			char next_char = canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			char old_char = old_canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			if(next_char != old_char){
				if(skip_length > 0){
					// If the length of our skip would be shorter than the number
					// of characters added by the required escape sequence,
					// this would result in a gain in buffer consumption instead
					// of a reduction, so we have to check.
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
					} else if(skip_length >= 10 && skip_length >= 5){
						MOVE_10(skip_length);
					// Since this move can only get us to the rightmost column,
					// and it does not wrap around, a maximum skip length of 999
					// will always be adequate.
					// I could come back later and see about a multi-line skip
					// routine that would calculate the new C;R cursor pos and use
					// a different escape sequence.
					} else if(skip_length >= 100){
						MOVE_100(skip_length);
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
	
	// Flip buffer
	Cell* temp = canvas->cells;
	canvas->cells = old_canvas->cells;
	old_canvas->cells = temp;
}
