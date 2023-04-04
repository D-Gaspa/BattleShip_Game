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
    CELL_STATE_SHIP_BOTTOM,
    CELL_STATE_SHIP_LEFT,
    CELL_STATE_SHIP_MIDDLE,
    CELL_STATE_SHIP_RIGHT,
    CELL_STATE_SHIP_TOP,
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
    SDL_Texture* ship_top;
    SDL_Texture* ship_left;
    SDL_Texture* ship_middle;
    SDL_Texture* ship_right;
    SDL_Texture* ship_bottom;
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
    textures->ship_bottom = load_texture("Assets/ship_bottom.png", renderer);
    textures->ship_left = load_texture("Assets/ship_left.png", renderer);
    textures->ship_middle = load_texture("Assets/ship_middle.png", renderer);
    textures->ship_right = load_texture("Assets/ship_right.png", renderer);
    textures->ship_top = load_texture("Assets/ship_top.png", renderer);
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
        case CELL_STATE_SHIP_BOTTOM:
            texture = textures->ship_bottom;
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
        case CELL_STATE_SHIP_TOP:
            texture = textures->ship_top;
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

// Helper function to check if the mouse is inside a button
int is_mouse_inside_button(int x, int y, SDL_Rect button_rect) {
    return x >= button_rect.x && x <= button_rect.x + button_rect.w &&
           y >= button_rect.y && y <= button_rect.y + button_rect.h;
}//end is_mouse_inside_button

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
    int hover_button = -1;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            } else if (event.type == SDL_MOUSEMOTION) {
                int x = event.motion.x;
                int y = event.motion.y;

                hover_button = -1;
                for (int i = 0; i < 3; i++) {
                    if (is_mouse_inside_button(x, y, button_rects[i])) {
                        hover_button = i;
                        break;
                    }//end if
                }//end for
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                for (int i = 0; i < 3; i++) {
                    if (is_mouse_inside_button(x, y, button_rects[i])) {
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
            if (hover_button == i) {
                SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255); // Lighter gray
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Original gray
            }//end else
            SDL_RenderFillRect(renderer, &button_rects[i]);

            // Render button shadow
            SDL_Rect shadow_rect = button_rects[i];
            shadow_rect.x += 4;
            shadow_rect.y += 4;
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 128);
            SDL_RenderFillRect(renderer, &shadow_rect);

            // Render button label
            render_text(renderer, button_labels[i], button_rects[i].x + 24, button_rects[i].y + 8);
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

void selecting_screen(SDL_Renderer* renderer, GameTextures* textures) {
    // Initialize variables
    int ship_selected = -1;
    int orientation = 0; // 0 for horizontal, 1 for vertical
    SDL_Rect orientation_button = { 650, 50, 150, 50 }; // x, y, width, height

    // Add ship names
    const char* ship_names[NUM_SHIPS] = {
            "Carrier",
            "Battleship",
            "Destroyer",
            "Submarine",
            "Patrol Boat"
    };

    // Main loop for the selecting_screen
    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                // Check if the user clicked on the orientation button
                if (is_mouse_inside_button(x, y, orientation_button)) {
                    orientation = (orientation + 1) % 2;
                } else {
                    // Check if the user clicked on a ship
                    for (int i = 0; i < NUM_SHIPS; i++) {
                        SDL_Rect ship_rect = {50, 50 + i * 50, 32 * (int) i, 32};
                        if (is_mouse_inside_button(x, y, ship_rect)) {
                            ship_selected = i;
                            break;
                        }//end if
                    }//end for
                }//end else
            }//end else if
        }//end while

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render ships on the left side
        for (int i = 0; i < NUM_SHIPS; i++) {
            int ship_size = (int) i;
            for (int j = 0; j < ship_size; j++) {
                SDL_Rect ship_rect = {50 + j * CELL_SIZE, 50 + i * 50, CELL_SIZE, CELL_SIZE};
                if (j == 0) {
                    SDL_RenderCopy(renderer, textures->ship_left, NULL, &ship_rect);
                } else if (j == ship_size - 1) {
                    SDL_RenderCopy(renderer, textures->ship_right, NULL, &ship_rect);
                } else {
                    SDL_RenderCopy(renderer, textures->ship_middle, NULL, &ship_rect);
                }//end else
            }//end for
            // Render ship names
            render_text(renderer, ship_names[i], 50, 90 + i * 50);
        }//end for

        // Render the orientation button
        if (orientation == 0) {
            render_text(renderer, "Orientation: Horizontal", orientation_button.x, orientation_button.y);
        } else {
            render_text(renderer, "Orientation: Vertical", orientation_button.x, orientation_button.y);
        }//end else

        // Render the grid on the right side
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                SDL_Rect grid_rect = {400 + i * CELL_SIZE, 50 + j * CELL_SIZE, CELL_SIZE, CELL_SIZE};
                SDL_RenderCopy(renderer, textures->ocean, NULL, &grid_rect);
                // Render grid lines
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
                SDL_RenderDrawRect(renderer, &grid_rect);
            }//end for
        }//end for

        // Render the selected ship (if any) at the mouse position
        if (ship_selected != -1) {
            int x, y;
            SDL_GetMouseState(&x, &y);
            int ship_size = (int) ship_selected;
            for (int i = 0; i < ship_size; i++) {
                SDL_Rect ship_rect = {x + i * CELL_SIZE, y, CELL_SIZE, CELL_SIZE};
                if (orientation == 0) {
                    if (i == 0) {
                        SDL_RenderCopy(renderer, textures->ship_left, NULL, &ship_rect);
                    } else if (i == ship_size - 1) {
                        SDL_RenderCopy(renderer, textures->ship_right, NULL, &ship_rect);
                    } else {
                        SDL_RenderCopy(renderer, textures->ship_middle, NULL, &ship_rect);
                    }//end else
                } else {
                    if (i == 0) {
                        SDL_RenderCopy(renderer, textures->ship_top, NULL, &ship_rect);
                    } else if (i == ship_size - 1) {
                        SDL_RenderCopy(renderer, textures->ship_bottom, NULL, &ship_rect);
                    } else {
                        SDL_RenderCopy(renderer, textures->ship_middle, NULL, &ship_rect);
                    }//end else
                }//end else
            }//end for
        }//end if
        // Render the screen
        SDL_RenderPresent(renderer);
    }//end while
}//end selecting_screen

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
        selecting_screen(renderer, textures);
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