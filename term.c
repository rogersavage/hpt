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
#include "term.h"
#include "canvas.h"

#define UTF_BYTE_0 0xE2
#define UTF_BYTE_1 0x96
#define LIGHT_SHADE 0x91
#define SHADE 0x92
#define DARK_SHADE 0x93
#define FULL_BLOCK 0x88
#define PALETTE_SIZE 264

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7

// Globals related to terminal settings and properties.
struct winsize ws;
struct termios backup;
struct termios t;
int term_width, term_height;
int display_width, display_height;

int biggest_buffer, smallest_buffer;
void paint_cell(Canvas* canvas, int x, int y, int index);

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
    int r, g, b;
} Color;

Color palette[264];

void init_palette(Canvas* canvas){

    FILE* log;
    log = fopen("log.txt", "w");
    fseek(log, 0, SEEK_SET);
    fputs("Log File:\n", log);
    fprintf(log, "Here's a %d fprintf\n", 123);

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
		palette[offset].fg = i + 30;;
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

	// Bright Three-Quarter-Shade
	for(int bg=0; bg<8; bg++){
		for(int fg=0; fg<8; fg++){
			palette[offset].fg = fg + 90;
			palette[offset].bg = bg + 40;
			palette[offset++].c = DARK_SHADE;
		}
	}

	// Full Bright
	for(int fg = 0; fg<8; fg++){
		palette[offset].fg = fg + 90;
		palette[offset].bg = fg + 40;
		palette[offset++].c = FULL_BLOCK;
	}

    // Assign RGB values
    for(int i = 0; i<PALETTE_SIZE; i++){
        Color* color = &palette[i];
        color->r = 0;
        color->g = 0;
        color->b = 0;
        int fg_r = 0;
        int fg_g = 0;
        int fg_b = 0;
        int bg_r = 0;
        int bg_g = 0;
        int bg_b = 0;
        if(color->c == ' ' ||
        color->c == FULL_BLOCK){
            int value = 255;
            if(color->c == ' '){
                value = 192;
            }
            switch(color->fg % 10){
            case BLACK:
                if(color->fg / 10 == 9){
                fg_r = 64;
                fg_g = 64;
                fg_b = 64;
                } break;
            case RED: fg_r = value; break;
            case GREEN: fg_g = value; break;
            case YELLOW: fg_r = value;
                fg_g = value; break;
            case BLUE: fg_b = value; break;
            case MAGENTA: fg_r = value;
                fg_b = value; break;
            case CYAN: fg_b = value;
                fg_g = value; break;
            case WHITE: fg_r = value;
                fg_g = value;
                fg_b = value; break;
            }
            color->r = fg_r;
            color->g = fg_g;
            color->b = fg_b;
        }
        if(color->c == LIGHT_SHADE ||
            color->c == SHADE ||
            color->c == DARK_SHADE){

            // Determine FG portion
            int value;
            switch(color->c){
                case LIGHT_SHADE: value = 96; break;
                case SHADE: value = 128; break;
                case DARK_SHADE: value = 192; break;
            }
            if(color->fg / 10 == 3)
                value = value * 3 / 4;

            switch(color->fg % 10){
            case BLACK:
                if(color->fg / 10 == 3) break;
                switch(color->c){
                case LIGHT_SHADE: value = 16; break;
                case SHADE: value = 32; break;
                case DARK_SHADE: value = 48; break;
                }
                color->r = value;
                color->g = value;
                color->b = value; break;
            case RED: fg_r = value; break;
            case GREEN: fg_g = value; break;
            case YELLOW: fg_r = value;
                fg_g = value; break;
            case BLUE: fg_b = value; break;
            case MAGENTA: fg_r = value;
                fg_b = value; break;
            case CYAN: fg_b = value;
                fg_g = value; break;
            case WHITE: fg_r = value;
                fg_g = value;
                fg_b = value; break;
            }
            // Determine BG portion
            switch(color->c){
                case LIGHT_SHADE: value = 129; break;
                case SHADE: value = 86; break;
                case DARK_SHADE: value = 43; break;
            }

            switch(color->bg % 10){
            case RED: bg_r = value; break;
            case GREEN: bg_g = value; break;
            case YELLOW: bg_r = value;
                bg_g = value; break;
            case BLUE: bg_b = value; break;
            case MAGENTA: bg_r = value;
                bg_b = value; break;
            case CYAN: bg_b = value;
                bg_g = value; break;
            case WHITE: fg_r = value;
                bg_g = value;
                bg_b = value; break;
            }
            color->r = fg_r + bg_r;
            color->g = fg_g + bg_g;
            color->b = fg_b + bg_b;
        }
        fprintf(log, "Color %d = %d, %d, %d\n", i,
        color->r, color->g, color->b);
    }

    // Remove duplicates
    int duplicates = 0;
    int x = 0;
    int y = 0;
    for(int i=0; i<PALETTE_SIZE - 1; i++){
        for(int j=i+1; j<PALETTE_SIZE; j++){
            if(
                palette[i].r == palette[j].r &&
                palette[i].g == palette[j].g &&
                palette[i].b == palette[j].b){
                fprintf(log, "Color %d and color %d are both"
                " %d, %d, %d\n", i, j, palette[i].r, palette[i].g, palette[i].b);
                duplicates++;
                paint_cell(canvas, x++, y, i);
                paint_cell(canvas, x++, y, j);
                paint_cell(canvas, x++, y, 0);

                if(x > term_width - 3){
                    x = 0;
                    y++;
                }
            }
        }
    }
    fprintf(log, "Duplicates: %d\n", duplicates);
    fclose(log);
}

void paint_cell(Canvas* canvas, int x, int y, int index){
	int offset = x + y * MAX_VIEW_WIDTH;
	canvas->cells[offset].color = palette[index].fg;
	canvas->cells[offset].bg_color = palette[index].bg;
	canvas->cells[offset].character = palette[index].c;
}

void animate_fractal_noise(Canvas* canvas, int* noise, int ticks){
    for(int y=0; y<term_height; y++){
        for(int x=0; x<term_width; x++){
            int offset = x + y * MAX_VIEW_WIDTH;
            paint_cell(canvas, x, y,
                    (noise[offset] + ticks) / 12 % 264);
        }
    }
}

void draw_color_bars(Canvas* canvas){
    int y = 0;
    int x = 0;
    for(int i=0; i<264; i++){
        paint_cell(canvas, i + x, y, i);
        if(i > 0 && i % term_width == 0){
            y += 1;
            x -= term_width + 1;
        }
    }
}

int main(){
	int tty = open(ttyname(STDIN_FILENO), O_RDWR | O_SYNC);
	srand(time(NULL));
	biggest_buffer = 0;
	smallest_buffer = MAX_VIEW_AREA;
    start_term();
    int ticks = 0;
    Canvas* canvas = create_canvas(
    MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
    Canvas* old_canvas = create_canvas(
    MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
	init_palette(canvas);
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
    while(!quit){
        char user_input = input();
        if (user_input == 'q') quit = 1;
        //animate_fractal_noise(canvas, noise, ticks);
        //draw_color_bars(canvas);
        term_refresh(buffer, canvas, old_canvas, tty);
        ticks++;
		usleep(1000000/60);
    }
    end_term();
    printf("Term dimensions: %d x %d\n", term_width, term_height);
    printf("Biggest buffer: %d\n", biggest_buffer);
    printf("Smallest buffer: %d\n", smallest_buffer);
    return 0;
}


void term_refresh(char* buffer, Canvas* canvas, Canvas* old_canvas,
int tty){
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
		char temp[12];
        // Make sure "Bright" doesn't carry over to next line
        ADD('\x1b');
        ADD('[');
        ADD('m');

		for(int x = 0; x < term_width; x++){
			int offset = x + y * MAX_VIEW_WIDTH;
			int next_fg_color = canvas->cells[offset].color;
			int next_bg_color = canvas->cells[offset].bg_color;
			int old_fg = old_canvas->cells[offset].color;
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
			char next_char = canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			char old_char = old_canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			
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
	if(smallest_buffer > pointer - buffer) smallest_buffer = pointer - buffer;
	if(biggest_buffer < pointer - buffer) biggest_buffer = pointer - buffer;
	
	// Flip buffer
	Cell* temp = canvas->cells;
	canvas->cells = old_canvas->cells;
	old_canvas->cells = temp;
}
