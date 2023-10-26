#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <time.h>

#include "fractal_noise.h"
#include "constants.h"
#include "term.h"
#include "canvas.h"

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

int biggest_buffer, smallest_buffer;

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

typedef struct Color{
	int fg;
	int bg;
	char c;
} Color;

Color palette[200];

void init_palette(){
	int offset = 0;
	for(int i=0; i<200; i++){
		palette[i].fg = 91;
		palette[i].bg = 46;
		palette[i].c = '?';
	}

	// Dim Quarter-Shade
	for(int bg=0; bg<8; bg++){
		for(int fg=0; fg<8; fg++){
			if(fg == bg) continue;
			palette[offset].fg = fg + 30;
			palette[offset].bg = bg + 40;
			palette[offset++].c = LIGHT_SHADE; 
		}
	}

	// Dim Half-Shade
	int j=1;
	for(int bg=0; bg<8; bg++){
		for(int fg=j++; fg<8; fg++){
			palette[offset].fg = fg + 30;
			palette[offset].bg = bg + 40;
			palette[offset++].c = SHADE;
		}
	}

	// Dim
	for(int i=0; i<8; i++){
		palette[offset].fg = 0;
		palette[offset].bg = i + 40;
		palette[offset++].c = ' ';
	}

	// Bright Quarter-Shade
	for(int bg=0; bg<8; bg++){
		for(int fg=0; fg<8; fg++){
			palette[offset].fg = fg + 90;
			palette[offset].bg = bg + 40;
			palette[offset++].c = LIGHT_SHADE;
		}
	}

	// Bright Half-Shade
	j=0;
	for(int bg=0; bg<8; bg++){
		for(int fg=j++; fg<8; fg++){
			palette[offset].fg = fg + 90;
			palette[offset].bg = bg + 40;
			palette[offset++].c = SHADE; 
		}
	}


	// Full Bright
	for(int fg = 0; fg<8; fg++){
		palette[offset].fg = fg + 90;
		palette[offset].bg = fg + 40;
		palette[offset++].c = FULL_BLOCK;
	}
}

void paint_cell(Canvas* canvas, int x, int y, int index){
	int offset = x + y * MAX_VIEW_WIDTH;
	canvas->cells[offset].color = palette[index].fg;
	canvas->cells[offset].bg_color = palette[index].bg;
	canvas->cells[offset].character = palette[index].c;
}


int main(){
	FILE* tty = fopen(ttyname(STDIN_FILENO), "w");

	srand(time(NULL));
	init_palette();
	biggest_buffer = 0;
	smallest_buffer = MAX_VIEW_AREA;
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
    fractal_noise(rand() % 128, 128, MAX_VIEW_WIDTH, 8, 1.0f, 
    noise);

		// This is a gradient ramp, with dithering symbols
		// of increasing intensity
    char texture[] = {
			' ',
			/*
			LIGHT_SHADE, 
			LIGHT_SHADE, 
			SHADE, 
			SHADE, 
			DARK_SHADE, 
			DARK_SHADE, 
			FULL_BLOCK,
			*/
			'.',
			'/',
			'#'
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
				char user_input = input();
        if (user_input == 'q') quit = 1;
				for(int y=0; y<term_height; y++){
					for(int x=0; x<term_width; x++){
						int offset = x + y * MAX_VIEW_WIDTH;
						/*
							canvas->cells[offset].character = 
							texture[(noise[offset] + ticks) / 8 % 4];
							int color = 
								((noise[offset] + ticks) / 16) % 16;
							int brightness = 30;
							if(color % 2 == 0){
								brightness = 90;
							}
							color /= 2;
							color += brightness;
							canvas->cells[offset].color = color;
							canvas->cells[offset].bg_color = ticks / 128 % 8 + 100;
							*/
						paint_cell(canvas, x, y,
								(noise[offset] + ticks) / 8 % 200);
					}
				}
				/*
				for(int j=1; j<16; j++){
				for(int i=0; i<term_width - 1; i++){
					paint_cell(canvas, i, j, i);
				}
				}

				for(int j=16; j<32; j++){
				for(int i=term_width; i<200; i++){
					paint_cell(canvas, i - term_width, j, i);
				}
				}
				*/

        term_refresh(buffer, canvas, old_canvas, tty);
        ticks++;
		usleep(1000000/60);
    }
		fflush(tty);
    end_term();
		printf("Term dimensions: %d x %d\n", term_width, term_height);
		printf("Biggest buffer: %d\n", biggest_buffer);
		printf("Smallest buffer: %d\n", smallest_buffer);
    return 0;
}


void term_refresh(char* buffer, Canvas* canvas, Canvas* old_canvas,
		FILE* tty){
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
		else if(ch == (char)FULL_BLOCK){ADD_UTF(FULL_BLOCK);} \
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
	int fg_color;
	int bg_color;

	for(int y = 0; y < term_height; y++){
		fg_color = -1;
		bg_color = -1;
		for(int x = 0; x <= term_width; x++){
			int offset = x + y * MAX_VIEW_WIDTH;
			int next_fg_color = canvas->cells[offset].color;
			int next_bg_color = canvas->cells[offset].bg_color;
			
			int old_fg= old_canvas->cells[offset].color;
			int old_bg = old_canvas->cells[offset].bg_color;

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
			//int offset = x + y + MAX_VIEW_WIDTH;
			char next_char = canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			char old_char = old_canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			if(next_char == old_char &&
					next_fg_color == old_fg &&
					next_bg_color == old_bg) skip_length++;
			else{
				if(skip_length > 0){
					if(skip_length < 4 ){
						char skip_char;
						int offset = x + y * MAX_VIEW_WIDTH;
						if(skip_length == 3){
							skip_char = canvas->cells[offset - 3].character;
							ADD(skip_char);
						}
						if(skip_length >= 2){
							skip_char = canvas->cells[offset - 2].character;
							ADD(skip_char);
						}
						skip_char = canvas->cells[offset - 1].character;
						ADD(skip_char);
					} else if(skip_length >= 10 && skip_length >= 4){
						MOVE_10(skip_length);
					} else if(skip_length >= 100){
						MOVE_100(skip_length);
					} else {
						MOVE(skip_length);
					}
				}
				skip_length = 0;					
				ADD(next_char);
			}
		}
		if(skip_length > 0) {
			ADD('\n');
			skip_length = 0;
		}
	}
	// Cut off that last newline so the screen doesn't scroll
	pointer--;
	fwrite(buffer, 1, pointer - buffer, tty);
	if(smallest_buffer > pointer - buffer) smallest_buffer = pointer - buffer;
	if(biggest_buffer < pointer - buffer) biggest_buffer = pointer - buffer;
	
	// Flip buffer
	Cell* temp = canvas->cells;
	canvas->cells = old_canvas->cells;
	old_canvas->cells = temp;
}
