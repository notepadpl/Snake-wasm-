#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <emscripten.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL_ttf.h>
#define GRID_SIZE 20
#define DPAD_SIZE 50
#define DPAD_PADDING 20

#define GAME_WIDTH 480
#define GAME_HEIGHT 480

#define WINDOW_WIDTH (GAME_WIDTH + 3 * DPAD_SIZE + 2 * DPAD_PADDING)
#define WINDOW_HEIGHT GAME_HEIGHT
int score = 0;
TTF_Font* font = NULL;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

SDL_Rect dpad_up, dpad_down, dpad_left, dpad_right;

typedef struct {
    int x, y;
} Point;

Point snake[100];
int snake_length = 5;
int dir_x = 1, dir_y = 0;
Point food;

int running = 1;
int frame_counter = 0;

void setup_dpad() {
    int base_x = GAME_WIDTH + DPAD_PADDING;
    int base_y = WINDOW_HEIGHT - 3 * DPAD_SIZE - DPAD_PADDING;

    dpad_up    = (SDL_Rect){base_x + DPAD_SIZE, base_y, DPAD_SIZE, DPAD_SIZE};
    dpad_down  = (SDL_Rect){base_x + DPAD_SIZE, base_y + 2 * DPAD_SIZE, DPAD_SIZE, DPAD_SIZE};
    dpad_left  = (SDL_Rect){base_x, base_y + DPAD_SIZE, DPAD_SIZE, DPAD_SIZE};
    dpad_right = (SDL_Rect){base_x + 2 * DPAD_SIZE, base_y + DPAD_SIZE, DPAD_SIZE, DPAD_SIZE};
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;

    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    if (!window || !renderer) return false;

    srand(time(NULL));
    if (TTF_Init() < 0) return false;
font = TTF_OpenFont("assets/fast99.ttf", 24); // pamiętaj dodać font.ttf do projektu
if (!font) return false;
    return true;
}

void spawn_food() {
    int max_x = GAME_WIDTH / GRID_SIZE;
    int max_y = GAME_HEIGHT / GRID_SIZE;

    bool valid = false;
    while (!valid) {
        valid = true;
        food.x = rand() % max_x;
        food.y = rand() % max_y;

        for (int i = 0; i < snake_length; ++i) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                valid = false;
                break;
            }
        }
    }
}

void move_snake() {
    for (int i = snake_length - 1; i > 0; --i)
        snake[i] = snake[i - 1];

    snake[0].x += dir_x;
    snake[0].y += dir_y;

    int max_x = GAME_WIDTH / GRID_SIZE;
    int max_y = GAME_HEIGHT / GRID_SIZE;
// kolizja z samym sobą
for (int i = 1; i < snake_length; ++i) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
        running = 0;
        return;
    }
}
    if (snake[0].x < 0) snake[0].x = max_x - 1;
    else if (snake[0].x >= max_x) snake[0].x = 0;

    if (snake[0].y < 0) snake[0].y = max_y - 1;
    else if (snake[0].y >= max_y) snake[0].y = 0;

    if (snake[0].x == food.x && snake[0].y == food.y) {
        if (snake_length < 100) snake_length++;
        spawn_food();
        score++;
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
void render_score() {
    SDL_Color white = {255, 255, 255, 255};
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Score: %d", score);

    SDL_Surface* surface = TTF_RenderText_Solid(font, buffer, white);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;

    SDL_Rect dst = {300, 10, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
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

void render_dpad() {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &dpad_up);
    SDL_RenderFillRect(renderer, &dpad_down);
    SDL_RenderFillRect(renderer, &dpad_left);
    SDL_RenderFillRect(renderer, &dpad_right);
}

void quit() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (font) TTF_CloseFont(font);
TTF_Quit();
}

void loop() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = 0;
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:    if (dir_y != 1)  { dir_x = 0; dir_y = -1; } break;
                case SDLK_DOWN:  if (dir_y != -1) { dir_x = 0; dir_y = 1; }  break;
                case SDLK_LEFT:  if (dir_x != 1)  { dir_x = -1; dir_y = 0; } break;
                case SDLK_RIGHT: if (dir_x != -1) { dir_x = 1; dir_y = 0; }  break;
            }
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            int x = e.button.x;
            int y = e.button.y;
            SDL_Point p = {x, y};

            if (SDL_PointInRect(&p, &dpad_up))    { if (dir_y != 1)  { dir_x = 0; dir_y = -1; } }
            else if (SDL_PointInRect(&p, &dpad_down))  { if (dir_y != -1) { dir_x = 0; dir_y = 1; } }
            else if (SDL_PointInRect(&p, &dpad_left))  { if (dir_x != 1)  { dir_x = -1; dir_y = 0; } }
            else if (SDL_PointInRect(&p, &dpad_right)) { if (dir_x != -1) { dir_x = 1; dir_y = 0; } }
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
    render_score();
    render_dpad();

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

    setup_dpad();
    spawn_food();

    emscripten_set_main_loop(loop, -1, 1);
    return 0;
}
