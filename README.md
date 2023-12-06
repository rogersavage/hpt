# Minimal ANSI Renderer
Minimal code for full-screen text "painting" in the terminal with fast  
full-screen refresh.  
  
A simple "make" will compile the demo, a color-cycling fractal noise animation.  
  
Initialize a window struct using createWindow():  
  
	Window* window = createWindow();  
  
Window has a member called "canvas". Draw on window->canvas using these  
functions.  
  
	int getCanvasBgColor(Canvas*, int x, int y);  
	int getCanvasFgColor(Canvas*, int x, int y);  
	char getCanvasCharacter(Canvas*, int x, int y);  
	void setCanvasCharacter(Canvas*, int x, int y, char character);  
	void setCanvasFgColor(Canvas*, int x, int y, int color);  
  
You can also create separate canvases using:  

	Canvas* createCanvas(int width, int height),  

but there is currently no "blit" function.  
  
Note that this is an ANSI-only renderer, and canvas cells can only hold a char.  
  
Because of the way the standard 8+8 colors work, there are three categories  
of color macros. Background colors count as different "colors" internally.  
  
Foreground Colors:  
  
	#define BLACK 30  
	#define RED 31  
	#define GREEN 32  
	#define YELLOW 33  
	#define BLUE 34  
	#define MAGENTA 35  
	#define CYAN 36  
	#define WHITE 37  
  
Bright Foreground Colors:  
  
	#define BR_BLACK 90  
	#define BR_RED 91  
	#define BR_GREEN 92  
	#define BR_YELLOW 93  
	#define BR_BLUE 94  
	#define BR_MAGENTA 95  
	#define BR_CYAN 96  
  
Background Colors:  
  
	#define BG_BLACK 40  
	#define BG_RED 41  
	#define BG_GREEN 42  
	#define BG_YELLOW 43  
	#define BG_BLUE 44  
	#define BG_MAGENTA 45  
	#define BG_CYAN 46  
	#define BG_WHITE 47  
  
Note that this means there is no internal connection between normal and bright  
versions of a given color, and no equivalence between the foreground version  
of a color and the background version of that same color. So, for example,  
setCanvasFgColor(canvas, x, y, BG_BLACK) will produce garbage output.  
  
Poll non-blocking keyboard input using input():  

	char user_input = input();  
  
Render the window onscreen using termRefresh():  

	termRefresh(window);  
  
  
Clean up using endTerm();  
