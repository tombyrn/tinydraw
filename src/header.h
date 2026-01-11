#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT  650

#define CANVAS_RECT_SIZE 600

#define CANVAS_WIDTH (canvas.pixel_size * canvas.cols)
#define CANVAS_HEIGHT (canvas.pixel_size * canvas.rows)

#define SAVED_IMAGE_PATH "./image.png"

#define FPS 60
#define MS_PER_FRAME (1000 / FPS)

#define RAND_COLOR (rand() % 255)

#define SET_RECT(rect, _x, _y, _w, _h)\
            (rect).x = _x;\
            (rect).y = _y;\
            (rect).w = _w;\
            (rect).h = _h;
#define SET_COLOR(c, _r, _g, _b)\
            (c).r = _r;\
            (c).g = _g;\
            (c).b = _b;



struct color {
	int r, g, b, a;
};

struct button {
	SDL_Rect rect;
	bool clicked;
};

struct pixel {
	struct color c;
	SDL_Rect rect;
};

struct canvas {
	int rows, cols;
	int pixel_size;
	bool is_drawing;
	struct pixel** grid;

	SDL_Rect rect;
	SDL_Texture* texture;
} canvas;

struct swatch {
	struct button b;
	struct color c;
};

struct palette {
	struct swatch* colors;
	SDL_Rect container;
	int num_colors;
} palette;