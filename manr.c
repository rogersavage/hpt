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



// Globals related to terminal settings and properties.
struct winsize ws;
struct termios backup;
struct termios t;
extern int term_width, term_height;
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
		return open(ttyname(STDIN_FILENO), O_RDWR | O_SYNC);
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

void addChar(char** pointer, char character){
	(**pointer) = character;
	(*pointer)++;
}

void changeFgBgColor(char** pointer, int fgColor, int bgColor){
	addChar(pointer, '\x1b');
	addChar(pointer, '[');
	addChar(pointer, fgColor / 10 + '0');
	addChar(pointer, fgColor % 10 + '0');
	addChar(pointer, ';');
	addChar(pointer, '4');
	addChar(pointer, bgColor % 10 + '0');
	addChar(pointer, 'm');
}

void changeFgColor(char** pointer, int fgColor){
	addChar(pointer, '\x1b');
	addChar(pointer, '[');
	addChar(pointer, fgColor / 10 + '0');
	addChar(pointer, fgColor % 10 + '0');
	addChar(pointer, 'm');
}

void changeBgColor(char** pointer, int bgColor){
	addChar(pointer, '\x1b');
	addChar(pointer, '[');
	addChar(pointer, bgColor / 10 + '0');
	addChar(pointer, bgColor % 10 + '0');
	addChar(pointer, 'm');
}

void cursorReturn(char** pointer){
	addChar(pointer, '\x1b');
	addChar(pointer, '[');
	addChar(pointer, 'H');
}

void moveToColumn(char** pointer, int x){
	addChar(pointer, '\x1b');
	addChar(pointer, '[');
	if(x >= 100) addChar(pointer, x / 100 + '0');
	if(x >= 10) addChar(pointer, x % 100 / 10 + '0');
	addChar(pointer, x % 10 + '0');
	addChar(pointer, 'G');
}

void updateColor(char** pointer, int* currentFgColor, int* currentBgColor,
	int nextFgColor, int nextBgColor){

	if(*currentFgColor != nextFgColor){
		if(*currentBgColor != nextBgColor){
			changeFgBgColor(pointer, nextFgColor, nextBgColor);
			(*currentFgColor) = nextFgColor;
			(*currentBgColor) = nextBgColor;
		} else{
			changeFgColor(pointer, nextFgColor);
			(*currentFgColor) = nextFgColor;
		}
	} else if(*currentBgColor != nextBgColor){
			changeBgColor(pointer, nextBgColor);
			(*currentBgColor) = nextBgColor;
	}
}

void term_refresh(char* buffer, Canvas* canvas, int tty){
	char* pointer = buffer;
	cursorReturn(&pointer);
	char prev_char = '\0';
	int skip_length = 0;
	int currentFgColor;
	int currentBgColor;

	for(int y = 0; y < term_height; y++){
		currentFgColor = -1;
		currentBgColor = -1;

		for(int x = 0; x < term_width; x++){
			int offset = x + y * MAX_VIEW_WIDTH;
			int nextFgColor = canvas->cells[offset].color;
			int nextBgColor = canvas->cells[offset].bg_color;
			updateColor(&pointer, &currentFgColor, &currentBgColor,
					nextFgColor, nextBgColor);
			char next_char = canvas->cells[x + y * MAX_VIEW_WIDTH].character;
			addChar(&pointer, next_char);
			skip_length = 0;
		}
		skip_length = 0;
		addChar(&pointer, '\n');
	}
	// Cut off that last newline so the screen doesn't scroll
	if(*(pointer-1) == '\n') pointer--;
	write(tty, buffer, pointer - buffer);
}

