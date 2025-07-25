#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <emscripten.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define GRID_SIZE 20

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;

    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    if (window == NULL || renderer == NULL) return false;

    srand(time(NULL));
    return true;
}

int running = 1;

typedef struct {
    int x, y;
} Point;

Point snake[100];
int snake_length = 5;
int dir_x = 1, dir_y = 0;

Point food;

void spawn_food() {
    int max_x = WINDOW_WIDTH / GRID_SIZE;
    int max_y = WINDOW_HEIGHT / GRID_SIZE;

    bool valid = false;
    while (!valid) {
        valid = true;
        food.x = rand() % max_x;
        food.y = rand() % max_y;

        // upewnij się, że jedzenie nie pojawi się na wężu
        for (int i = 0; i < snake_length; ++i) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                valid = false;
                break;
            }
        }
    }
}

void quit() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void move_snake() {
    for (int i = snake_length - 1; i > 0; --i)
        snake[i] = snake[i - 1];

    snake[0].x += dir_x;
    snake[0].y += dir_y;

    int max_x = WINDOW_WIDTH / GRID_SIZE;
    int max_y = WINDOW_HEIGHT / GRID_SIZE;

    // wrap-around
    if (snake[0].x < 0) snake[0].x = max_x - 1;
    else if (snake[0].x >= max_x) snake[0].x = 0;

    if (snake[0].y < 0) snake[0].y = max_y - 1;
    else if (snake[0].y >= max_y) snake[0].y = 0;

    // sprawdzanie kolizji z jedzeniem
    if (snake[0].x == food.x && snake[0].y == food.y) {
        if (snake_length < 100) snake_length++;
        spawn_food();
    }
}

void render_snake() {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (int i = 0; i < snake_length; ++i) {
        SDL_Rect r = {
            snake[i].x * GRID_SIZE,
            snake[i].y * GRID_SIZE,
            GRID_SIZE, GRID_SIZE
        };
        SDL_RenderFillRect(renderer, &r);
    }
}

void render_food() {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect r = {
        food.x * GRID_SIZE,
        food.y * GRID_SIZE,
        GRID_SIZE, GRID_SIZE
    };
    SDL_RenderFillRect(renderer, &r);
}

int frame_counter = 0;

void loop() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = 0;
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP: if (dir_y != 1) { dir_x = 0; dir_y = -1; } break;
                case SDLK_DOWN: if (dir_y != -1) { dir_x = 0; dir_y = 1; } break;
                case SDLK_LEFT: if (dir_x != 1) { dir_x = -1; dir_y = 0; } break;
                case SDLK_RIGHT: if (dir_x != -1) { dir_x = 1; dir_y = 0; } break;
            }
        }
    }

    if (++frame_counter >= 10) {
        move_snake();
        frame_counter = 0;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    render_snake();
    render_food();

    SDL_RenderPresent(renderer);

    if (!running) {
        emscripten_cancel_main_loop();
        quit();
    }
}

int main() {
    if (!init()) return 1;

    for (int i = 0; i < snake_length; ++i) {
        snake[i].x = 5 - i;
        snake[i].y = 5;
    }

    spawn_food();

    emscripten_set_main_loop(loop, -1, 1);
    return 0;
}
