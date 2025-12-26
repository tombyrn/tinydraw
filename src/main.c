#include "./header.h"

int last_frame_time = 0;
int is_running = 0;
int mouse_x, mouse_y;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

SDL_Texture* plus_texture = NULL;
SDL_Texture* minus_texture = NULL;
SDL_Texture* download_texture = NULL;
SDL_Texture* trash_texture = NULL;


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


struct color chosen_color;

struct button increase_rows;
struct button decrease_rows;

struct button increase_cols;
struct button decrease_cols;

struct button save_img;
struct button clear_canvas;

int load_svg(char* filename, SDL_Texture** texture) {
	SDL_RWops* rwops = SDL_RWFromFile(filename, "rb");
	if(rwops == NULL) {
		fprintf(stderr, "Error reading from file \"%s\"\n", filename);
		return 0;
	}
	
	SDL_Surface* svg = IMG_LoadSVG_RW(rwops);
	if(svg == NULL) {
		fprintf(stderr, "Error parsing file \"%s\"\n", filename);
		return 0;
	}
	*texture = SDL_CreateTextureFromSurface(renderer, svg);
	
	SDL_RWclose(rwops);
	SDL_FreeSurface(svg);
	return 1;
}

int initialize_window(void) {
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "Error initializing SDL\n");
		return 0;
	}

	if(IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		fprintf(stderr, "Error initializing SDL_Image\n");
		return 0;
	}

	window = SDL_CreateWindow(
		"tinydraw",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		0
	);

	if(window == NULL) {
		fprintf(stderr, "Error creating SDL Window.\n");
		return 0;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if(!renderer) {
		fprintf(stderr, "Error creating SDL Renderer.\n");
		return 0;
	}
	if(load_svg("./plus.svg", &plus_texture) == 0) return 0;
	if(load_svg("./minus.svg", &minus_texture) == 0) return 0;
	if(load_svg("./download.svg", &download_texture) == 0) return 0;
	if(load_svg("./trash.svg", &trash_texture) == 0) return 0;
	
	return 1;
}

void setup() {
	// setup canvas
	canvas.rows = 50;
	canvas.cols = 50;
	canvas.pixel_size = 12;
	canvas.is_drawing = false;

	// initialze pixel grid and set all white
	canvas.grid = calloc(canvas.rows, sizeof(struct pixel*));
	for(int i = 0; i < canvas.rows; i++) {
		canvas.grid[i] = calloc(canvas.cols, sizeof(struct pixel));
		for (int j = 0; j < canvas.cols; j++) {
			SET_COLOR(canvas.grid[i][j].c, 255, 255, 255);
			canvas.grid[i][j].c.a = SDL_ALPHA_OPAQUE;
			SET_RECT(canvas.grid[i][j].rect, j * canvas.pixel_size, i * canvas.pixel_size, canvas.pixel_size, canvas.pixel_size);
		}
	}

	// set drawing color
	SET_COLOR(chosen_color, 0, 0, 0);
	chosen_color.a = SDL_ALPHA_OPAQUE;

	// create drawing canvas
	canvas.texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		canvas.cols * canvas.pixel_size,
		canvas.rows * canvas.pixel_size
	);

	if(!canvas.texture) {
		fprintf(stderr, "Failed to create canvas texture: %s\n", SDL_GetError());
		SDL_Quit();
	}

	// setup rectangle that texture will be drawn to 
	SET_RECT(canvas.rect, (SCREEN_WIDTH - CANVAS_RECT_SIZE)/2, (SCREEN_HEIGHT - CANVAS_RECT_SIZE)/2, CANVAS_RECT_SIZE, CANVAS_RECT_SIZE);

	// setup buttons
	SET_RECT(decrease_rows.rect, SCREEN_WIDTH - 83, 25, 25, 25);
	decrease_rows.clicked = false;

	SET_RECT(increase_rows.rect, SCREEN_WIDTH - 41, 25, 25, 25);
	increase_rows.clicked = false;

	SET_RECT(decrease_cols.rect, SCREEN_WIDTH - 83, 60, 25, 25);
	decrease_cols.clicked = false;
	
	SET_RECT(increase_cols.rect, SCREEN_WIDTH - 41, 60, 25, 25);
	increase_cols.clicked = false;
	
	SET_RECT(save_img.rect, SCREEN_WIDTH - 83, SCREEN_HEIGHT - 50, 25, 25);
	save_img.clicked = false;

	SET_RECT(clear_canvas.rect, 25, SCREEN_HEIGHT - 50, 25, 25);
	clear_canvas.clicked = false;

	// setup palette
	SET_RECT(palette.container, 25, (SCREEN_HEIGHT - CANVAS_RECT_SIZE)/2, 50, 20*25+10);

	palette.num_colors = 50;
	palette.colors = calloc(palette.num_colors, sizeof(struct swatch));
	int swatch_w = 20;
	int swatch_h = 20;
	int m = 0;

	// setup color swatches within palette
	for(int i = 0; i < palette.num_colors; i++) {
		palette.colors[i].c.a = SDL_ALPHA_OPAQUE;
		SET_COLOR(palette.colors[i].c, RAND_COLOR, RAND_COLOR, RAND_COLOR);

		if(i % 2 == 0) {
			SET_RECT(palette.colors[i].b.rect, palette.container.x + 5, palette.container.y + 5 + (swatch_h * m), 20, 20);
		}
		else {
			SET_RECT(palette.colors[i].b.rect, palette.container.x + 5 + swatch_w + 1, palette.container.y + 5 + (swatch_h * m), 20, 20);
		}
		if(i % 2 == 1) m++;
	}

	// setup more colors here iyw all others are random
	SET_COLOR(palette.colors[0].c, 255, 255, 255);
	SET_COLOR(palette.colors[1].c, 0, 0, 0);
	SET_COLOR(palette.colors[2].c, 255, 0, 0);
	SET_COLOR(palette.colors[3].c, 0, 255, 0);
	SET_COLOR(palette.colors[4].c, 0, 0, 255);
	SET_COLOR(palette.colors[5].c, 0, 255, 255);

	// fill the texture with white pixels
	SDL_SetRenderTarget(renderer, canvas.texture);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, NULL);
}

// set button struct clicked attribute to true
void check_button_click(struct button* b) {
	if(mouse_x > b->rect.x && mouse_x < b->rect.x + b->rect.w &&
	   mouse_y > b->rect.y && mouse_y < b->rect.y + b->rect.h)
		b->clicked = true;
}

void resize_canvas_grid(int n_rows, int n_cols) {
	// allocate new grid
	struct pixel** n_grid = calloc(n_rows, sizeof(struct pixel*));
	for(int i = 0; i < n_rows; i++) {
		n_grid[i] = calloc(n_cols, sizeof(struct pixel));
		for(int j = 0; j < n_cols; j++) {
			// copy from old grid when possible
			if(i < canvas.rows && j < canvas.cols)
				n_grid[i][j] = canvas.grid[i][j];
			else {
				n_grid[i][j].c.a = SDL_ALPHA_OPAQUE;
				SET_COLOR(n_grid[i][j].c, 255, 255, 255);
				SET_RECT(n_grid[i][j].rect, j * canvas.pixel_size, i * canvas.pixel_size, canvas.pixel_size, canvas.pixel_size);
			}
		}
	}

	// free old grid
	for(int i = 0; i < canvas.rows; i++) 
		free(canvas.grid[i]);
	free(canvas.grid);

	// update canvas struct
	canvas.grid = n_grid;
	canvas.rows = n_rows;
	canvas.cols = n_cols;
	
	// recreate texture with new grid
	SDL_DestroyTexture(canvas.texture);
	canvas.texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		canvas.cols * canvas.pixel_size,
		canvas.rows * canvas.pixel_size
	);
}

void save_canvas() {
	// save current render target to reset to
    SDL_Texture* target = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, canvas.texture);

    int width, height;
    SDL_QueryTexture(canvas.texture, NULL, NULL, &width, &height);

    // create full-res surface
    SDL_Surface* full = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	// render texture to surface
    SDL_RenderReadPixels(renderer, NULL, full->format->format,
                         full->pixels, full->pitch);

    // create downscaled surface
    int new_w = width  / canvas.pixel_size;
    int new_h = height / canvas.pixel_size;

    SDL_Surface* descaled = SDL_CreateRGBSurface(0, new_w, new_h,
                                             32, 0, 0, 0, 0);

    // descale texture to save as true pixel size
    for(int y = 0; y < new_h; y++) {
        for(int x = 0; x < new_w; x++) {

            int src_x = x * canvas.pixel_size;
            int src_y = y * canvas.pixel_size;

            Uint8* src_row = (Uint8*)full->pixels + src_y * full->pitch;
            Uint32* src_px = (Uint32*)(src_row + src_x * 4);

            Uint8* dst_row = (Uint8*)descaled->pixels + y * descaled->pitch;
            Uint32* dst_px = (Uint32*)(dst_row + x * 4);

            *dst_px = *src_px;
        }
    }

    // save downscaled image
    IMG_SavePNG(descaled, SAVED_IMAGE_PATH);

    SDL_FreeSurface(full);
    SDL_FreeSurface(descaled);

	// reset rendering target
    SDL_SetRenderTarget(renderer, target);
}


void process_input() {
	SDL_Event event;
	SDL_GetMouseState(&mouse_x, &mouse_y);

	while(SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				is_running = 0;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					is_running = 0;
				break;
			case SDL_MOUSEBUTTONDOWN:
				// start drawing, actual bounds-checking is done in update
				canvas.is_drawing = true;

				// check if buttons were clicked
				check_button_click(&increase_rows);
				check_button_click(&decrease_rows);
				check_button_click(&increase_cols);
				check_button_click(&decrease_cols);

				for(int i = 0; i < palette.num_colors; i++) {
					check_button_click(&palette.colors[i].b);
				}

				check_button_click(&save_img);
				check_button_click(&clear_canvas);

				break;
			case SDL_MOUSEBUTTONUP:
				canvas.is_drawing = false;
				break;
		}
	}
}

void update() {
	int time_to_wait = MS_PER_FRAME - (SDL_GetTicks() - last_frame_time);
	if(time_to_wait > 0 && time_to_wait <= MS_PER_FRAME)
		SDL_Delay(time_to_wait);

	// draw to canvas if neededs
	if(canvas.is_drawing) {
		// map mouse position to canvas texture coordinates
		int rel_x = mouse_x - canvas.rect.x;
		int rel_y = mouse_y - canvas.rect.y;

		// only draw if mouse is inside the canvas rect
		if(rel_x >= 0 && rel_x < canvas.rect.w && rel_y >= 0 && rel_y < canvas.rect.h) {
			// scale mouse coords to texture pixel coords
			float scale_x = (float)(canvas.cols * canvas.pixel_size) / (float)canvas.rect.w;
			float scale_y = (float)(canvas.rows * canvas.pixel_size) / (float)canvas.rect.h;

			int tex_x = (int)(rel_x * scale_x);
			int tex_y = (int)(rel_y * scale_y);

			int row = tex_y / canvas.pixel_size;
			int col = tex_x / canvas.pixel_size;

			if(row >= 0 && row < canvas.rows && col >= 0 && col < canvas.cols) {
				struct pixel* clicked_pixel = &canvas.grid[row][col];
				clicked_pixel->c = chosen_color;
			}
			
		}
	}
		
	// handle button clicks
	if(increase_rows.clicked) {
		resize_canvas_grid(canvas.rows+1, canvas.cols);
		increase_rows.clicked = false;
	}
	if(canvas.rows > 1 && decrease_rows.clicked) {
		resize_canvas_grid(canvas.rows-1, canvas.cols);
		decrease_rows.clicked = false;
	}
	if(increase_cols.clicked) {
		resize_canvas_grid(canvas.rows, canvas.cols+1);
		increase_cols.clicked = false;
	}
	if(canvas.cols > 1 && decrease_cols.clicked) {
		resize_canvas_grid(canvas.rows, canvas.cols-1);
		decrease_cols.clicked = false;
	}
	if(save_img.clicked) {
		save_canvas();
		save_img.clicked = false;
	}

	// handle new color chosen
	for(int i = 0; i < palette.num_colors; i++) {
		if(palette.colors[i].b.clicked) {
			chosen_color = palette.colors[i].c;
			palette.colors[i].b.clicked = false;
		}
	}

	if(clear_canvas.clicked) {
		for(int i = 0; i < canvas.rows; i++) {
			for(int j = 0; j < canvas.cols; j++) {
				SET_COLOR(canvas.grid[i][j].c, 255, 255, 255);
			}
		}
		clear_canvas.clicked = false;
	}

	last_frame_time = SDL_GetTicks();
}


void render() {

	// render pixels to texture
	SDL_SetRenderTarget(renderer, canvas.texture);
	for(int i = 0; i < canvas.rows; i++) {
		for(int j = 0; j < canvas.cols; j++) {
			struct pixel* p = &canvas.grid[i][j];
			SDL_SetRenderDrawColor(renderer, p->c.r, p->c.g, p->c.b, p->c.a);
			SDL_RenderFillRect(renderer, &p->rect);

		}
	}
	SDL_SetRenderTarget(renderer, NULL);

	// render the canvas texture to the screen (stretches/scales to window)
	SDL_RenderCopy(renderer, canvas.texture, NULL, &canvas.rect);

	// render buttons
	SDL_SetRenderDrawColor(renderer, 211, 211, 211, 255);
	SDL_RenderCopy(renderer, minus_texture, NULL, &decrease_rows.rect);
	SDL_RenderCopy(renderer, plus_texture, NULL, &increase_rows.rect);
	SDL_RenderCopy(renderer, minus_texture, NULL, &decrease_cols.rect);
	SDL_RenderCopy(renderer, plus_texture, NULL, &increase_cols.rect);

	SDL_RenderCopy(renderer, download_texture, NULL, &save_img.rect);
	SDL_RenderCopy(renderer, trash_texture, NULL, &clear_canvas.rect);

	// render palette
	SDL_RenderDrawRect(renderer, &palette.container);
	for(int i = 0; i < palette.num_colors; i++) {
		SDL_SetRenderDrawColor(renderer, palette.colors[i].c.r, palette.colors[i].c.g, palette.colors[i].c.b, palette.colors[i].c.a);
		SDL_RenderFillRect(renderer, &palette.colors[i].b.rect);
	}

	SDL_RenderPresent(renderer);
}

void destroy_window() {
	for(int i = 0; i < canvas.rows; i++)
		free(canvas.grid[i]);
	free(canvas.grid);

	if(plus_texture) SDL_DestroyTexture(plus_texture);
	if(minus_texture) SDL_DestroyTexture(minus_texture);
	if(canvas.texture) SDL_DestroyTexture(canvas.texture);
	if(renderer) SDL_DestroyRenderer(renderer);
	if(window) SDL_DestroyWindow(window);
	SDL_Quit();
}

int main() {
	srand(time(NULL));
	is_running = initialize_window();
	setup();

	while (is_running) {
		process_input();
		update();
		render();
	}

	destroy_window();
	return 0;
}