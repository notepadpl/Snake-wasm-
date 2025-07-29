#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <emscripten.h>
#include <stdlib.h>
#include <time.h>
int game_over_counter=0;
#define GRID_SIZE 20
#define DPAD_SIZE 50
#define DPAD_PADDING 20
#define GAME_WIDTH 480
#define GAME_HEIGHT 480
#define WINDOW_WIDTH (GAME_WIDTH + 3 * DPAD_SIZE + 2 * DPAD_PADDING)
#define WINDOW_HEIGHT GAME_HEIGHT

typedef struct { int x, y; } Point;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

TTF_Font* font = NULL;
SDL_Texture* snake_head_texture = NULL;
SDL_Texture* snake_body_texture = NULL;
SDL_Texture* snake_texture = NULL;
SDL_Texture* food_texture = NULL;

SDL_Rect dpad_up, dpad_down, dpad_left, dpad_right;

Point snake[100];
int snake_length = 5;
int dir_x = 1, dir_y = 0;
Point food;
int score = 0;
int running = 1;
int frame_counter = 0;

SDL_Texture* load_texture(const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) return NULL;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void setup_dpad() {
    int base_x = GAME_WIDTH + DPAD_PADDING;
    int base_y = WINDOW_HEIGHT - 3 * DPAD_SIZE - DPAD_PADDING;

    dpad_up    = (SDL_Rect){base_x + DPAD_SIZE, base_y, DPAD_SIZE, DPAD_SIZE};
    dpad_down  = (SDL_Rect){base_x + DPAD_SIZE, base_y + 2 * DPAD_SIZE, DPAD_SIZE, DPAD_SIZE};
    dpad_left  = (SDL_Rect){base_x, base_y + DPAD_SIZE, DPAD_SIZE, DPAD_SIZE};
    dpad_right = (SDL_Rect){base_x + 2 * DPAD_SIZE, base_y + DPAD_SIZE, DPAD_SIZE, DPAD_SIZE};
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init error: %s\n", TTF_GetError());
        return false;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("IMG_Init error: %s\n", IMG_GetError());
        return false;
    }

    if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer) < 0) {
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return false;
    }

    // Teraz można ładować tekstury
    snake_head_texture = load_texture("assets/snake_yellow_head.png");
    if (!snake_head_texture) {
        printf("Nie udało się załadować snake_yellow_head.png\n");
        return false;
    }

    snake_body_texture = load_texture("assets/snake_yellow_blob.png");
    if (!snake_body_texture) {
        printf("Nie udało się załadować snake_yellow_blob.png\n");
        return false;
    }

    food_texture = load_texture("assets/apple_green.png");
    if (!food_texture) {
        printf("Nie udało się załadować apple_green.png\n");
        return false;
    }

    font = TTF_OpenFont("assets/fast99.ttf", 24);
    if (!font) {
        printf("Nie udało się załadować czcionki: %s\n", TTF_GetError());
        return false;
    }

    srand(time(NULL));
    return true;
}


void spawn_food() {
    int max_x = GAME_WIDTH / GRID_SIZE;
    int max_y = GAME_HEIGHT / GRID_SIZE;

    do {
        food.x = rand() % max_x;
        food.y = rand() % max_y;
        bool on_snake = false;
        for (int i = 0; i < snake_length; ++i) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                on_snake = true;
                break;
            }
        }
        if (!on_snake) break;
    } while (1);
}

void move_snake() {
    for (int i = snake_length - 1; i > 0; --i)
        snake[i] = snake[i - 1];

    snake[0].x += dir_x;
    snake[0].y += dir_y;

    int max_x = GAME_WIDTH / GRID_SIZE;
    int max_y = GAME_HEIGHT / GRID_SIZE;

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
        score++;
        spawn_food();
    }
}

void render_snake() {
    for (int i = 0; i < snake_length; ++i) {
        SDL_Rect dst = {
            snake[i].x * GRID_SIZE,
            snake[i].y * GRID_SIZE,
            GRID_SIZE,
            GRID_SIZE
        };

        if (i == 0)
            SDL_RenderCopy(renderer, snake_head_texture, NULL, &dst);  // Głowa
        else
            SDL_RenderCopy(renderer, snake_body_texture, NULL, &dst);  // Ciało
    }
}
void render_food() {
    SDL_Rect r = {
        food.x * GRID_SIZE,
        food.y * GRID_SIZE,
        GRID_SIZE, GRID_SIZE
    };
    SDL_RenderCopy(renderer, food_texture, NULL, &r);
}

void render_score() {
    SDL_Color white = {255, 255, 255, 255};
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Score: %d", score);

    SDL_Surface* surface = TTF_RenderText_Solid(font, buffer, white);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    int base_x = GAME_WIDTH + DPAD_PADDING;
    int base_y = WINDOW_HEIGHT - 3 * DPAD_SIZE - DPAD_PADDING;

    SDL_Rect dst = {base_x + DPAD_SIZE, base_y - surface->h - 10, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void render_game_over() {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* surface1 = TTF_RenderText_Solid(font, "Game Over", white);
    SDL_Surface* surface2 = NULL;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Score: %d", score);
    surface2 = TTF_RenderText_Solid(font, buffer, white);

    if (!surface1 || !surface2) return;

    SDL_Texture* texture1 = SDL_CreateTextureFromSurface(renderer, surface1);
    SDL_Texture* texture2 = SDL_CreateTextureFromSurface(renderer, surface2);

    int center_x = WINDOW_WIDTH / 2;

    SDL_Rect dst1 = {center_x - surface1->w / 2, WINDOW_HEIGHT / 2 - 50, surface1->w, surface1->h};
    SDL_Rect dst2 = {center_x - surface2->w / 2, WINDOW_HEIGHT / 2 + 10, surface2->w, surface2->h};

    SDL_RenderCopy(renderer, texture1, NULL, &dst1);
    SDL_RenderCopy(renderer, texture2, NULL, &dst2);

    SDL_DestroyTexture(texture1);
    SDL_DestroyTexture(texture2);
    SDL_FreeSurface(surface1);
    SDL_FreeSurface(surface2);
}

void render_dpad() {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &dpad_up);
    SDL_RenderFillRect(renderer, &dpad_down);
    SDL_RenderFillRect(renderer, &dpad_left);
    SDL_RenderFillRect(renderer, &dpad_right);
}

void quit() {
    if (snake_head_texture) SDL_DestroyTexture(snake_head_texture);
if (snake_body_texture) SDL_DestroyTexture(snake_body_texture);
    
    if (snake_texture) SDL_DestroyTexture(snake_texture);
    if (food_texture) SDL_DestroyTexture(food_texture);
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}
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
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
    int x = e.button.x;
    int y = e.button.y;

    if (SDL_PointInRect(&(SDL_Point){x, y}, &dpad_up)) {
        if (dir_y != 1) { dir_x = 0; dir_y = -1; }
    } else if (SDL_PointInRect(&(SDL_Point){x, y}, &dpad_down)) {
        if (dir_y != -1) { dir_x = 0; dir_y = 1; }
    } else if (SDL_PointInRect(&(SDL_Point){x, y}, &dpad_left)) {
        if (dir_x != 1) { dir_x = -1; dir_y = 0; }
    } else if (SDL_PointInRect(&(SDL_Point){x, y}, &dpad_right)) {
        if (dir_x != -1) { dir_x = 1; dir_y = 0; }
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
    render_score();
render_dpad();
    SDL_RenderPresent(renderer);

    if (!running) {
        emscripten_cancel_main_loop();
        quit();
    }
}
void loop2() {
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
            int x = e.button.x, y = e.button.y;
            SDL_Point p = {x, y};
            if (SDL_PointInRect(&p, &dpad_up))    if (dir_y != 1)  { dir_x = 0; dir_y = -1; }
            if (SDL_PointInRect(&p, &dpad_down))  if (dir_y != -1) { dir_x = 0; dir_y = 1; }
            if (SDL_PointInRect(&p, &dpad_left))  if (dir_x != 1)  { dir_x = -1; dir_y = 0; }
            if (SDL_PointInRect(&p, &dpad_right)) if (dir_x != -1) { dir_x = 1; dir_y = 0; }
        }
    }

    if (!running) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    render_game_over();
    SDL_RenderPresent(renderer);

    if (++game_over_counter >= 120) { // 2 sekundy przy 60 FPS
        emscripten_cancel_main_loop();
    }
    return;
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
