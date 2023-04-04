#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_thread.h>

#define BOARD_SIZE 10
#define NUM_SHIPS 5
#define CELL_SIZE 32

TTF_Font* font = NULL;
int font_size = 24;

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

// Structures for the game board
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

// Structures for the ships
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
    int is_turn;
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

// Structure for move results
typedef enum {
    MOVE_RESULT_MISS,
    MOVE_RESULT_HIT,
    MOVE_RESULT_SUNK_SHIP
} MoveResult;

MoveResult handle_player_move(Player* opponent, int x, int y) {
    Cell* cell = &opponent->board.cells[x][y];

    if (cell->hit) {
        return MOVE_RESULT_MISS;
    }//end if

    cell->hit = 1;
    if (cell->occupied) {
        for (int i = 0; i < NUM_SHIPS; i++) {
            Ship* ship = &opponent->ships[i];
            if (ship->type == cell->state - 1) {
                ship->hit_count++;

                if (ship->hit_count == ship->size) {
                    opponent->ships_remaining--;
                    cell->state = CELL_STATE_HIT_ENEMY_SHIP;
                    return MOVE_RESULT_SUNK_SHIP;
                }//end if
            }//end if
        }//end for

        cell->state = CELL_STATE_HIT_OWN_SHIP;
        return MOVE_RESULT_HIT;
    } else {
        cell->state = CELL_STATE_MISS;
        return MOVE_RESULT_MISS;
    }//end else
}//end handle_player_move

SDL_Texture* load_texture(const char* filename, SDL_Renderer* renderer);

GameTextures* load_game_textures(SDL_Renderer* renderer) {
    GameTextures* textures = (GameTextures*)malloc(sizeof(GameTextures));
    if (textures == NULL) {
        printf("Failed to allocate memory for game textures.\n");
        return NULL;
    }//end if

    textures->ocean = load_texture("Assets/ocean.png", renderer);
    textures->ship_left = load_texture("Assets/ship_left.png", renderer);
    textures->ship_middle = load_texture("Assets/ship_middle.png", renderer);
    textures->ship_right = load_texture("Assets/ship_right.png", renderer);
    textures->hit_enemy_ship = load_texture("Assets/hit_enemy_ship.png", renderer);
    textures->hit_own_ship = load_texture("Assets/hit_own_ship.png", renderer);
    textures->hit_ocean = load_texture("Assets/hit_ocean.png", renderer);
    textures->miss = load_texture("Assets/miss.png", renderer);

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

// Load animated background textures
SDL_Texture** load_animated_background(SDL_Renderer* renderer, const char* filepath, int num_frames) {
    SDL_Texture** textures = (SDL_Texture**)malloc(sizeof(SDL_Texture*) * num_frames);
    for (int i = 0; i < num_frames; i++) {
        char filename[128];
        snprintf(filename, sizeof(filename), "%s%d.png", filepath, i);
        textures[i] = load_texture(filename, renderer);
        if (textures[i] == NULL) {
            printf("Failed to load background frame: %s\n", filename);
            return NULL;
        }//end if
    }//end for
    return textures;
}//end load_animated_background

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

int place_ship_on_board(Player* player, ShipType type, int x, int y, int orientation) {
    int ship_size = (int)type;
    int end_x = x + (orientation == 0 ? ship_size - 1 : 0);
    int end_y = y + (orientation == 1 ? ship_size - 1 : 0);

    // Check if the ship fits within the board
    if (end_x >= BOARD_SIZE || end_y >= BOARD_SIZE) {
        return 0; // Invalid position
    }//end if

    // Check if the ship overlaps with other ships
    for (int i = x; i <= end_x; i++) {
        for (int j = y; j <= end_y; j++) {
            if (player->board.cells[i][j].occupied) {
                return 0; // Invalid position
            }//end if
        }//end for
    }//end for

    // Place the ship on the board
    for (int i = x; i <= end_x; i++) {
        for (int j = y; j <= end_y; j++) {
            player->board.cells[i][j].occupied = 1;
            if (i == x) {
                player->board.cells[i][j].state = CELL_STATE_SHIP_LEFT;
            } else if (i == end_x) {
                player->board.cells[i][j].state = CELL_STATE_SHIP_RIGHT;
            } else {
                player->board.cells[i][j].state = CELL_STATE_SHIP_MIDDLE;
            }//end else
        }//end for
    }//end for

    // Update ship information
    Ship* ship = &player->ships[type];
    ship->type = type;
    ship->size = ship_size;
    ship->hit_count = 0;

    return 1; // Ship placed successfully
}//end place_ship_on_board

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

int handle_events(SDL_Event* event, Player* player1, Player* player2) {
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT) {
            return 0;
        } else if (event->type == SDL_MOUSEBUTTONDOWN) {
            int x = event->button.x / CELL_SIZE;
            int y = event->button.y / CELL_SIZE;

            Player* current_player = player1->is_turn ? player1 : player2;
            Player* opponent = player1->is_turn ? player2 : player1;

            MoveResult result = handle_player_move(opponent, x, y);
            if (result != MOVE_RESULT_MISS) {
                if (opponent->ships_remaining == 0) {
                    printf("Player %d wins!\n", player1->is_turn ? 1 : 2);
                    return 0;
                }//end if
            } else {
                player1->is_turn = !player1->is_turn;
                player2->is_turn = !player2->is_turn;
            }//end else
        }//end else if
    }//end while
    return 1;
}//end handle_events

int input_thread_function(void* data) {
    SDL_Event event;
    int running = 1;
    Player* player1 = ((Player**)data)[0];
    Player* player2 = ((Player**)data)[1];

    while (running) {
        running = handle_events(&event, player1, player2);
    }//end while

    return 0;
}//end input_thread_function

void render_text(SDL_Renderer* renderer, const char* text, int x, int y) {
    // Set the text color
    SDL_Color color = {0, 0, 0, 255}; // Black

    // Create a surface containing the rendered text
    SDL_Surface* text_surface = TTF_RenderText_Solid(font, text, color);
    if (text_surface == NULL) {
        printf("TTF_RenderText_Solid: %s\n", TTF_GetError());
        exit(2);
    }//end if

    // Convert the surface to a texture
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (text_texture == NULL) {
        printf("SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
        exit(2);
    }//end if

    // Set the position and size of the text
    SDL_Rect text_rect;
    text_rect.x = x;
    text_rect.y = y;
    text_rect.w = text_surface->w;
    text_rect.h = text_surface->h;

    // Render the text texture
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);

    // Free resources
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}//end render_text

// Create a main menu
typedef enum {
    MAIN_MENU_NEW_GAME,
    MAIN_MENU_LOAD,
    MAIN_MENU_EXIT
} MainMenuOption;

MainMenuOption main_menu(SDL_Renderer* renderer) {
    MainMenuOption selected_option = MAIN_MENU_EXIT;

    // Load animated background textures
    const int num_background_frames = 10;
    SDL_Texture** background_frames = load_animated_background(renderer, "Assets/Backgrounds/frame_", num_background_frames);
    if (background_frames == NULL) {
        printf("Failed to load animated background.\n");
        return MAIN_MENU_EXIT;
    }//end if

    // Create buttons and positions for the main menu
    const char* button_labels[] = {"New Game", "Load", "Exit"};
    SDL_Rect button_rects[3];
    for (int i = 0; i < 3; i++) {
        button_rects[i].x = (BOARD_SIZE * CELL_SIZE) / 2 - 75;
        button_rects[i].y = 150 + i * 60;
        button_rects[i].w = 150;
        button_rects[i].h = 40;
    }//end for

    int running = 1;
    int frame_counter = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                for (int i = 0; i < 3; i++) {
                    if (x >= button_rects[i].x && x <= button_rects[i].x + button_rects[i].w &&
                        y >= button_rects[i].y && y <= button_rects[i].y + button_rects[i].h) {
                        selected_option = (MainMenuOption)i;
                        running = 0;
                        break;
                    } else {
                        selected_option = MAIN_MENU_EXIT;
                    }//end else
                }//end for
            }//end else if
        }//end while

        // Render the animated background
        SDL_RenderCopy(renderer, background_frames[frame_counter], NULL, NULL);
        frame_counter = (frame_counter + 1) % num_background_frames;

        for (int i = 0; i < 3; i++) {
            // Render button background
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderFillRect(renderer, &button_rects[i]);

            // Render button shadow
            SDL_Rect shadow_rect = button_rects[i];
            shadow_rect.x += 4;
            shadow_rect.y += 4;
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 128);
            SDL_RenderFillRect(renderer, &shadow_rect);

            // Render button label
            render_text(renderer, button_labels[i], button_rects[i].x + 45, button_rects[i].y + 8);
        }//end for

        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / 60); // Limit frame rate to 60 FPS
    }//end while

    // Free resources
    // Free background frames
    for (int i = 0; i < num_background_frames; i++) {
        SDL_DestroyTexture(background_frames[i]);
    }//end for
    free(background_frames);

    return selected_option;
}//end main_menu

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return -1;
    }//end if

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return -1;
    }//end if

    // Create an SDL window and renderer
    SDL_Window* window = SDL_CreateWindow("Battleship", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, BOARD_SIZE * CELL_SIZE, BOARD_SIZE * CELL_SIZE, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return -1;
    }//end if

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return -1;
    }//end if

    // Load game textures
    GameTextures* textures = load_game_textures(renderer);
    if (textures == NULL) {
        printf("Failed to load game textures.\n");
        return -1;
    }//end if

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(2);
    }//end if

    font = TTF_OpenFont("Assets/Fonts/cambria.ttc", font_size);
    if (font == NULL) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        exit(2);
    }//end if

    // Create main menu
    MainMenuOption menu_option = main_menu(renderer);
    if (menu_option == MAIN_MENU_EXIT) {
        // Exit the game
        free(textures);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        TTF_CloseFont(font);
        TTF_Quit();
        return 0;
    } else if (menu_option == MAIN_MENU_LOAD) {
        // Load a saved game (not yet implemented)
    } else if (menu_option == MAIN_MENU_NEW_GAME) {
        // Start a new game
    }//end else if

    // Initialize game board
    GameBoard board;
    initialize_game_board(&board);

    // Initialize players
    Player player1;
    Player player2;
    initialize_game_board(&player1.board);
    initialize_game_board(&player2.board);
    player1.is_turn = 1;
    player2.is_turn = 0;
    Player* players[] = { &player1, &player2 };

    // Create input thread
    SDL_Thread* input_thread = SDL_CreateThread(input_thread_function, "InputThread", (void*)players);
    if (input_thread == NULL) {
        printf("Failed to create input thread. SDL Error: %s\n", SDL_GetError());
        return -1;
    }//end if

    int running = 1;
    SDL_Event event;

    // Main game loop
    while (running) {
        running = handle_events(&event, &player1, &player2);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render the board
        Player* current_player = player1.is_turn ? &player1 : &player2;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                render_cell(renderer, &current_player->board.cells[i][j], textures);
            }//end for
        }//end for

        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / 60); // Limit frame rate to 60 FPS
    }//end while

    // Wait for input thread to finish
    int thread_return_value;
    SDL_WaitThread(input_thread, &thread_return_value);

    // Cleanup and exit
    free(textures);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    TTF_CloseFont(font);
    TTF_Quit();
    return 0;
}//end main