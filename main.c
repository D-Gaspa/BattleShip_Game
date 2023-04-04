#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL_image.h>


#define BOARD_SIZE 10
#define NUM_SHIPS 5
#define CELL_SIZE 32

// Structure for cell states
typedef enum {
    CELL_STATE_EMPTY,
    CELL_STATE_SHIP_LEFT,
    CELL_STATE_SHIP_MIDDLE,
    CELL_STATE_SHIP_RIGHT,
    CELL_STATE_HIT_ENEMY_SHIP,
    CELL_STATE_HIT_OWN_SHIP,
    CELL_STATE_HIT_OCEAN,
    CELL_STATE_MISS
} CellState;

// Structure for the game board
typedef struct {
    int x;
    int y;
    int width;
    int height;
    int occupied;
    int hit;
    CellState state;
    SDL_Rect rect;
} Cell;

typedef struct {
    Cell cells[BOARD_SIZE][BOARD_SIZE];
} GameBoard;

// Structure for the ships
typedef enum {
    CARRIER = 5,
    BATTLESHIP = 4,
    DESTROYER = 3,
    SUBMARINE = 3,
    PATROL_BOAT = 2
} ShipType;

typedef struct {
    ShipType type;
    int size;
    int hit_count;
} Ship;

// Structure for the player
typedef struct {
    GameBoard board;
    Ship ships[NUM_SHIPS];
    int ships_remaining;
} Player;

// Structure for game textures
typedef struct {
    SDL_Texture* ocean;
    SDL_Texture* ship_left;
    SDL_Texture* ship_middle;
    SDL_Texture* ship_right;
    SDL_Texture* hit_enemy_ship;
    SDL_Texture* hit_own_ship;
    SDL_Texture* hit_ocean;
    SDL_Texture* miss;
} GameTextures;

SDL_Texture* load_texture(const char* filename, SDL_Renderer* renderer);

// Functions to load and render textures
GameTextures* load_game_textures(SDL_Renderer* renderer) {
    GameTextures* textures = (GameTextures*)malloc(sizeof(GameTextures));
    if (textures == NULL) {
        printf("Failed to allocate memory for game textures.\n");
        return NULL;
    }//end if

    textures->ocean = load_texture("ocean.png", renderer);
    textures->ship_left = load_texture("ship_left.png", renderer);
    textures->ship_middle = load_texture("ship_middle.png", renderer);
    textures->ship_right = load_texture("ship_right.png", renderer);
    textures->hit_enemy_ship = load_texture("hit_enemy_ship.png", renderer);
    textures->hit_own_ship = load_texture("hit_own_ship.png", renderer);
    textures->hit_ocean = load_texture("hit_ocean.png", renderer);
    textures->miss = load_texture("miss.png", renderer);

    return textures;
}//end load_game_textures

SDL_Texture* load_texture(const char* filename, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(filename);
    if (surface == NULL) {
        printf("Failed to load image: %s. SDL Error: %s\n", filename, IMG_GetError());
        return NULL;
    }//end if

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Failed to create texture from surface. SDL Error: %s\n", SDL_GetError());
    }//end if

    SDL_FreeSurface(surface);
    return texture;
}//end load_texture

void render_cell(SDL_Renderer* renderer, const Cell* cell, const GameTextures* textures) {
    SDL_Texture* texture = NULL;

    switch (cell->state) {
        case CELL_STATE_EMPTY:
            texture = textures->ocean;
            break;
        case CELL_STATE_SHIP_LEFT:
            texture = textures->ship_left;
            break;
        case CELL_STATE_SHIP_MIDDLE:
            texture = textures->ship_middle;
            break;
        case CELL_STATE_SHIP_RIGHT:
            texture = textures->ship_right;
            break;
        case CELL_STATE_HIT_ENEMY_SHIP:
            texture = textures->hit_enemy_ship;
            break;
        case CELL_STATE_HIT_OWN_SHIP:
            texture = textures->hit_own_ship;
            break;
        case CELL_STATE_HIT_OCEAN:
            texture = textures->hit_ocean;
            break;
        case CELL_STATE_MISS:
            texture = textures->miss;
            break;
    }//end switch

    if (texture) {
        SDL_RenderCopy(renderer, texture, NULL, &cell->rect);
    }//end if
}//end render_cell

void initialize_game_board(GameBoard* board) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            Cell* cell = &board->cells[i][j];
            cell->x = i;
            cell->y = j;
            cell->width = CELL_SIZE;
            cell->height = CELL_SIZE;
            cell->occupied = 0;
            cell->hit = 0;
            cell->rect.x = i * CELL_SIZE;
            cell->rect.y = j * CELL_SIZE;
            cell->rect.w = CELL_SIZE;
            cell->rect.h = CELL_SIZE;
        }//end for
    }//end for
}//end initialize_game_board

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return 1;
    }//end if

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return 1;
    }//end if

    // Create an SDL window and renderer
    SDL_Window* window = SDL_CreateWindow("Battleship", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, BOARD_SIZE * CELL_SIZE, BOARD_SIZE * CELL_SIZE, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return 1;
    }//end if

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return 1;
    }//end if

    // Load game textures
    GameTextures* textures = load_game_textures(renderer);
    if (textures == NULL) {
        printf("Failed to load game textures.\n");
        return 1;
    }//end if

    // Initialize game board
    GameBoard board;
    initialize_game_board(&board);

    //Main game loop and other game logic will go here

    // Cleanup
    free(textures);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}//end main