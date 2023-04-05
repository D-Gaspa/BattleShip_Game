#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_thread.h>
#include <stdbool.h>

#define BOARD_SIZE 10
#define NUM_SHIPS 5
#define CELL_SIZE 32

TTF_Font *font = NULL;
int font_size = 24;

// Structure for cell states
typedef enum {
    CELL_STATE_EMPTY,
    CELL_STATE_EMPTY_SELECTION_MODE,
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
    SDL_Texture *ocean;
    SDL_Texture *ocean_selection_mode;
    SDL_Texture *ship_top;
    SDL_Texture *ship_left;
    SDL_Texture *ship_middle;
    SDL_Texture *ship_right;
    SDL_Texture *ship_bottom;
    SDL_Texture *hit_enemy_ship;
    SDL_Texture *hit_own_ship;
    SDL_Texture *hit_ocean;
    SDL_Texture *miss;
} GameTextures;

// Structure for move results
typedef enum {
    MOVE_RESULT_MISS,
    MOVE_RESULT_HIT,
    MOVE_RESULT_SUNK_SHIP
} MoveResult;

SDL_Texture *load_texture(const char *filename, SDL_Renderer *renderer);

GameTextures *load_game_textures(SDL_Renderer *renderer) {
    GameTextures *textures = (GameTextures *) malloc(sizeof(GameTextures));
    if (textures == NULL) {
        printf("Failed to allocate memory for game textures.\n");
        return NULL;
    }//end if

    textures->ocean = load_texture("Assets/ocean.png", renderer);
    textures->ocean_selection_mode = load_texture("Assets/ocean_selection_mode.png", renderer);
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

SDL_Texture *load_texture(const char *filename, SDL_Renderer *renderer) {
    SDL_Surface *surface = IMG_Load(filename);
    if (surface == NULL) {
        printf("Failed to load image: %s. SDL Error: %s\n", filename, IMG_GetError());
        return NULL;
    }//end if

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Failed to create texture from surface. SDL Error: %s\n", SDL_GetError());
    }//end if

    SDL_FreeSurface(surface);
    return texture;
}//end load_texture

// Load animated background textures
SDL_Texture **load_animated_background(SDL_Renderer *renderer, const char *filepath, int num_frames) {
    SDL_Texture **textures = (SDL_Texture **) malloc(sizeof(SDL_Texture *) * num_frames);
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

void render_cell(SDL_Renderer *renderer, const Cell *cell, const GameTextures *textures) {
    SDL_Texture *texture = NULL;

    switch (cell->state) {
        case CELL_STATE_EMPTY:
            texture = textures->ocean;
            break;
        case CELL_STATE_EMPTY_SELECTION_MODE:
            texture = textures->ocean_selection_mode;
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

void initialize_game_board(GameBoard *board) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            Cell *cell = &board->cells[i][j];
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

void render_text(SDL_Renderer *renderer, const char *text, int x, int y) {
    // Set the text color
    SDL_Color color = {0, 0, 0, 255}; // Black

    // Create a surface containing the rendered text
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, color);
    if (text_surface == NULL) {
        printf("TTF_RenderText_Solid: %s\n", TTF_GetError());
        exit(2);
    }//end if

    // Convert the surface to a texture
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
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

MainMenuOption main_menu(SDL_Renderer *renderer) {
    MainMenuOption selected_option = MAIN_MENU_EXIT;

    // Load animated background textures
    const int num_background_frames = 10;
    SDL_Texture **background_frames = load_animated_background(renderer, "Assets/Backgrounds/frame_",
                                                               num_background_frames);
    if (background_frames == NULL) {
        printf("Failed to load animated background.\n");
        return MAIN_MENU_EXIT;
    }//end if

    // Create buttons and positions for the main menu
    const char *button_labels[] = {"New Game", "Load", "Exit"};
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
                        selected_option = (MainMenuOption) i;
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

void selecting_screen(SDL_Renderer *renderer, GameTextures *textures, Player *current_player) {
    // Load background texture
    SDL_Texture *background_texture = IMG_LoadTexture(renderer, "Assets/selecting_screen_background.jpg");

    // Initialize variables
    int ship_selected = -1;
    int orientation = 0; // 0 for horizontal, 1 for vertical
    int hover_orientation = 0;
    initialize_game_board(&current_player->board);

    SDL_Rect orientation_button = {50, 300, 275, 50}; // x, y, width, height
    SDL_Rect exit_button = {0, 0, 100, 50};

    Ship ships[NUM_SHIPS] = {
            {CARRIER,     CARRIER,     0},
            {BATTLESHIP,  BATTLESHIP,  0},
            {DESTROYER,   DESTROYER,   0},
            {SUBMARINE,   SUBMARINE,   0},
            {PATROL_BOAT, PATROL_BOAT, 0}
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

                // Check if the user clicked on the exit button
                if (is_mouse_inside_button(x, y, exit_button)) {
                    running = 0;
                }//end if

                // Check if the user clicked on the orientation button
                if (is_mouse_inside_button(x, y, orientation_button)) {
                    orientation = (orientation + 1) % 2;
                } else {
                    // Check if the user clicked on a ship
                    for (int i = 0; i < NUM_SHIPS; i++) {
                        int ship_size = ships[i].size;
                        for (int j = 0; j < ship_size; j++) {
                            SDL_Rect ship_rect = {50 + j * CELL_SIZE, 50 + i * 50, CELL_SIZE, CELL_SIZE};
                            if (is_mouse_inside_button(x, y, ship_rect)) {
                                ship_selected = i;
                                break;
                            }//end if
                        }//end for
                    }//end for
                }//end else
            } else if (event.type == SDL_MOUSEMOTION) {
                int x = event.motion.x;
                int y = event.motion.y;

                // Check if the user is hovering over the orientation button
                if (is_mouse_inside_button(x, y, orientation_button)) {
                    hover_orientation = 1;
                } else {
                    hover_orientation = 0;
                }//end else
            }//end else if
        }//end while

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render screen background
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);

        // Render ships on the left side
        for (int i = 0; i < NUM_SHIPS; i++) {
            int ship_size = ships[i].size;
            for (int j = 0; j < ship_size; j++) {
                SDL_Rect ship_rect = {50 + j * CELL_SIZE, 50 + i * 50, CELL_SIZE, CELL_SIZE};
                if (j == 0) {
                    SDL_RenderCopy(renderer, textures->ship_left, NULL, &ship_rect);
                } else if (j == ship_size - 1) {
                    SDL_RenderCopy(renderer, textures->ship_right, NULL, &ship_rect);
                } else {
                    SDL_RenderCopy(renderer, textures->ship_middle, NULL, &ship_rect);
                }//end else
                if (ship_selected == i) {
                    SDL_Rect selected_ship_rect = {50, 50 + i * 50, ships[i].size * CELL_SIZE, CELL_SIZE};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
                    SDL_RenderDrawRect(renderer, &selected_ship_rect);
                }//end if
                // Add green border when hovering over a ship
                int x, y;
                SDL_GetMouseState(&x, &y);
                SDL_Rect hover_ship_rect = {50, 50 + i * 50, ships[i].size * CELL_SIZE, CELL_SIZE};
                if (is_mouse_inside_button(x, y, hover_ship_rect)) {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
                    SDL_RenderDrawRect(renderer, &hover_ship_rect);
                }//end if
            }//end for
        }//end for

        // Render the orientation button and background
        if (hover_orientation) {
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255); // Lighter gray
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Original gray
        }//end else
        SDL_RenderFillRect(renderer, &orientation_button);

        // Render button shadow
        SDL_Rect shadow_rect = orientation_button;
        shadow_rect.x += 4;
        shadow_rect.y += 4;
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 128);
        SDL_RenderFillRect(renderer, &shadow_rect);

        if (orientation == 0) {
            render_text(renderer, "Orientation: Horizontal", orientation_button.x + 24, orientation_button.y + 8);
        } else {
            render_text(renderer, "Orientation: Vertical", orientation_button.x + 24, orientation_button.y + 8);
        }//end else

        // Render the grid on the right side
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        int grid_mouse_x = (mouse_x - 400) / CELL_SIZE;
        int grid_mouse_y = (mouse_y - 50) / CELL_SIZE;

        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                SDL_Rect grid_rect = {400 + i * CELL_SIZE, 50 + j * CELL_SIZE, CELL_SIZE, CELL_SIZE};

                // Render grid with ocean_selection_mode texture if a ship is selected
                if (ship_selected >= 0) {
                    SDL_RenderCopy(renderer, textures->ocean_selection_mode, NULL, &grid_rect);
                } else {
                    SDL_RenderCopy(renderer, textures->ocean, NULL, &grid_rect);
                }//end else

                // Render grid lines
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
                SDL_RenderDrawRect(renderer, &grid_rect);

                // Render ship on the grip and white border when hovering over cells with a ship selected taking into account the orientation
                if (ship_selected >= 0) {
                    int ship_size = ships[ship_selected].size;
                    bool is_hovering = false;

                    for (int k = 0; k < ship_size; k++) {
                        int cell_x = grid_mouse_x + (orientation == 0 ? k : 0);
                        int cell_y = grid_mouse_y + (orientation == 1 ? k : 0);

                        if (cell_x == i && cell_y == j) {
                            is_hovering = true;
                            break;
                        }//end if
                    }//end for
                    if (is_hovering) {
                        for (int k = 0; k < ship_size; k++) {
                            int cell_x = grid_mouse_x + (orientation == 0 ? k : 0);
                            int cell_y = grid_mouse_y + (orientation == 1 ? k : 0);

                            // Rotate the ship if the orientation is vertical 90 degrees
                            if (orientation == 1) {
                                SDL_Rect ship_rect = {400 + cell_x * CELL_SIZE, 50 + cell_y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
                                if (k == 0) {
                                    SDL_RenderCopy(renderer, textures->ship_top, NULL, &ship_rect);
                                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y, ship_rect.x + ship_rect.w, ship_rect.y); // Top border
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y, ship_rect.x, ship_rect.y + ship_rect.h); // Left border
                                    SDL_RenderDrawLine(renderer, ship_rect.x + ship_rect.w, ship_rect.y, ship_rect.x + ship_rect.w, ship_rect.y + ship_rect.h); // Right border
                                } else if (k == ship_size - 1) {
                                    SDL_RenderCopy(renderer, textures->ship_bottom, NULL, &ship_rect);
                                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y + ship_rect.h, ship_rect.x + ship_rect.w, ship_rect.y + ship_rect.h); // Bottom border
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y, ship_rect.x, ship_rect.y + ship_rect.h); // Left border
                                    SDL_RenderDrawLine(renderer, ship_rect.x + ship_rect.w, ship_rect.y, ship_rect.x + ship_rect.w, ship_rect.y + ship_rect.h); // Right border
                                } else {
                                    SDL_RenderCopyEx(renderer, textures->ship_middle, NULL, &ship_rect, 90, NULL, SDL_FLIP_NONE);
                                    // Draw left and right borders without the top and bottom borders
                                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y, ship_rect.x, ship_rect.y + ship_rect.h); // Left border
                                    SDL_RenderDrawLine(renderer, ship_rect.x + ship_rect.w, ship_rect.y, ship_rect.x + ship_rect.w, ship_rect.y + ship_rect.h); // Right border
                                }//end else
                            } else {
                                SDL_Rect ship_rect = {400 + cell_x * CELL_SIZE, 50 + cell_y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
                                if (k == 0) {
                                    SDL_RenderCopy(renderer, textures->ship_left, NULL, &ship_rect);
                                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y, ship_rect.x + ship_rect.w, ship_rect.y); // Top border
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y, ship_rect.x, ship_rect.y + ship_rect.h); // Left border
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y + ship_rect.h, ship_rect.x + ship_rect.w, ship_rect.y + ship_rect.h); // Bottom border
                                } else if (k == ship_size - 1) {
                                    SDL_RenderCopy(renderer, textures->ship_right, NULL, &ship_rect);
                                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y, ship_rect.x + ship_rect.w, ship_rect.y); // Top border
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y + ship_rect.h, ship_rect.x + ship_rect.w, ship_rect.y + ship_rect.h); // Bottom border
                                    SDL_RenderDrawLine(renderer, ship_rect.x + ship_rect.w, ship_rect.y, ship_rect.x + ship_rect.w, ship_rect.y + ship_rect.h); // Right border
                                } else {
                                    SDL_RenderCopy(renderer, textures->ship_middle, NULL, &ship_rect);
                                    // Draw top and bottom borders without the left and right borders
                                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y, ship_rect.x + ship_rect.w, ship_rect.y); // Top border
                                    SDL_RenderDrawLine(renderer, ship_rect.x, ship_rect.y + ship_rect.h, ship_rect.x + ship_rect.w, ship_rect.y + ship_rect.h); // Bottom border
                                }//end else
                            }//end else
                        }//end for
                    }//end if
                }//end if
            }//end for
        }//end for
        // Render the exit button
        render_text(renderer, "Exit", exit_button.x + 25, exit_button.y + 10);

        // Render the screen
        SDL_RenderPresent(renderer);
    }//end while

    // Destroy textures
    SDL_DestroyTexture(background_texture);
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
    SDL_Window *window = SDL_CreateWindow("Battleship", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          BOARD_SIZE * CELL_SIZE, BOARD_SIZE * CELL_SIZE, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return -1;
    }//end if

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return -1;
    }//end if

    // Load game textures
    GameTextures *textures = load_game_textures(renderer);
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

    // Initialize players
    Player player1;
    Player player2;
    player1.is_turn = 1;
    player2.is_turn = 0;

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
        SDL_DestroyRenderer(renderer); // Destroy the renderer for the main menu
        SDL_DestroyWindow(window);     // Destroy the window for the main menu

        window = SDL_CreateWindow("Battleship - Player 1", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  800, 500, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            return -1;
        }//end if
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        // Load game textures
        textures = load_game_textures(renderer);

        if (textures == NULL) {
            printf("Failed to load game textures.\n");
            return -1;
        }//end if
        if (renderer == NULL) {
            printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
            return -1;
        }//end if
        selecting_screen(renderer, textures, &player1);

        SDL_DestroyRenderer(renderer); // Destroy the renderer for player1
        SDL_DestroyWindow(window);     // Destroy the window for player1

        // Create a new window and renderer for player2
        window = SDL_CreateWindow("Battleship - Player 2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  800, 500, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            return -1;
        }//end if

        // Load game textures
        textures = load_game_textures(renderer);

        if (textures == NULL) {
            printf("Failed to load game textures.\n");
            return -1;
        }//end if
        if (renderer == NULL) {
            printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
            return -1;
        }//end if

        selecting_screen(renderer, textures, &player2);
    }//end else if

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