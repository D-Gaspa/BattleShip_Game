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

// Structures for the game board
typedef struct {
    int x;
    int y;
    int width;
    int height;
    bool occupied;
    bool hit;
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
    int x;
    int y;
    int orientation;
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

// Structure for main menu options
typedef enum {
    MAIN_MENU_NEW_GAME,
    MAIN_MENU_LOAD,
    MAIN_MENU_EXIT
} MainMenuOption;

TTF_Font *font = NULL;
int font_size = 24;

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

void initialize_game_board(GameBoard *board) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            Cell *cell = &board->cells[i][j];
            cell->x = i;
            cell->y = j;
            cell->width = CELL_SIZE;
            cell->height = CELL_SIZE;
            cell->occupied = false;
            cell->hit = false;
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

int is_mouse_inside_button(int x, int y, SDL_Rect button_rect) {
    return x >= button_rect.x && x <= button_rect.x + button_rect.w &&
           y >= button_rect.y && y <= button_rect.y + button_rect.h;
}//end is_mouse_inside_button

bool is_position_valid(Player *current_player, int ship_size, int x, int y, int orientation) {
    for (int k = 0; k < ship_size; k++) {
        int cell_x = x + (orientation == 0 ? k : 0);
        int cell_y = y + (orientation == 1 ? k : 0);

        // Check if the cell is inside the grid
        if (cell_x < 0 || cell_x >= BOARD_SIZE || cell_y < 0 || cell_y >= BOARD_SIZE) {
            return false;
        }//end if

        // Check if the cell is already occupied
        if (current_player->board.cells[cell_x][cell_y].occupied == true) {
            return false;
        }//end if
    }//end for
    return true;
}//end is_position_valid

void place_ship(GameBoard *board, Ship *ship, int x, int y, int orientation) {
    ship->x = x;
    ship->y = y;
    ship->orientation = orientation;
    for (int i = 0; i < ship->size; i++) {
        int cell_x = x + (orientation == 0 ? i : 0);
        int cell_y = y + (orientation == 1 ? i : 0);

        Cell *cell = &board->cells[cell_x][cell_y];
        cell->occupied = true;
    }//end for
}//end place_ship

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

// Functions needed for render_placement_ships_left_side
/// \brief Renders a ship part based on the part index and the ship size.
/// \param renderer The renderer to use.
/// \param textures The textures to use.
/// \param ship_rect The rectangle in which the ship part should be rendered.
/// \param part_index The index of the part to be rendered (0 for left, ship_size - 1 for right, and middle parts otherwise).
/// \param ship_size The size of the ship.
void render_ship_part(SDL_Renderer *renderer, GameTextures *textures, SDL_Rect ship_rect, int part_index, int ship_size);

/// \brief Renders a border around the selected ship.
/// \param renderer The renderer to use.
/// \param ship_index The index of the ship to check for selection.
/// \param ship_selected The index of the currently selected ship.
/// \param ships The array of ships.
void render_selected_ship_border(SDL_Renderer *renderer, int ship_index, int ship_selected, Ship ships[]);

/// \brief Renders a hover border around a ship if the mouse is hovering over it and the ship is not placed.
/// \param renderer The renderer to use.
/// \param ship_index The index of the ship to check for hover.
/// \param placed_ships An array of booleans indicating whether a ship has been placed.
/// \param ships The array of ships.
void render_hover_ship_border(SDL_Renderer *renderer, int ship_index, const bool placed_ships[], Ship ships[]);

/// \brief Renders an overlay over a placed ship.
/// \param renderer The renderer to use.
/// \param ship_index The index of the ship to check for placement.
/// \param part_index The index of the ship part.
/// \param placed_ships An array of booleans indicating whether a ship has been placed.
/// \param black_texture A texture with a black color.
void render_placed_ship_overlay(SDL_Renderer *renderer, int ship_index, int part_index, const bool placed_ships[], SDL_Texture *black_texture);


/// \brief Renders the ships that the player can place on the board. This function is used in the placement phase.
/// \param renderer The renderer to use.
/// \param textures The textures to use.
/// \param ships The ships to render.
/// \param placed_ships An array of booleans indicating whether a ship has been placed.
/// \param black_texture A texture with a black color.
/// \param ship_selected The index of the ship that is currently selected.
void render_placement_ships_left_side(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[], SDL_Texture *black_texture, int ship_selected) {
    // Loop through each ship
    for (int i = 0; i < NUM_SHIPS; i++) {
        int ship_size = ships[i].size;

        // Loop through each part of the ship
        for (int j = 0; j < ship_size; j++) {
            SDL_Rect ship_rect = {50 + j * CELL_SIZE, 50 + i * 50, CELL_SIZE, CELL_SIZE};

            // Render the ship part
            render_ship_part(renderer, textures, ship_rect, j, ship_size);

            // Render the selected ship border if the ship is currently selected
            render_selected_ship_border(renderer, i, ship_selected, ships);

            // Render the hover ship border if the mouse is hovering over the ship and it is not placed
            render_hover_ship_border(renderer, i, placed_ships, ships);

            // Render the placed ship overlay if the ship has been placed
            render_placed_ship_overlay(renderer, i, j, placed_ships, black_texture);
        }//end for
    }//end for
}//end render_placement_ships_left_side

void render_ship_part(SDL_Renderer *renderer, GameTextures *textures, SDL_Rect ship_rect, int part_index, int ship_size) {
    SDL_Texture *part_texture = NULL;
    if (part_index == 0) {
        part_texture = textures->ship_left;
    } else if (part_index == ship_size - 1) {
        part_texture = textures->ship_right;
    } else {
        part_texture = textures->ship_middle;
    }//end else
    SDL_RenderCopy(renderer, part_texture, NULL, &ship_rect);
}//end render_ship_part

void render_selected_ship_border(SDL_Renderer *renderer, int ship_index, int ship_selected, Ship ships[]) {
    if (ship_selected == ship_index) {
        SDL_Rect selected_ship_rect = {50, 50 + ship_index * 50, ships[ship_index].size * CELL_SIZE, CELL_SIZE};
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
        SDL_RenderDrawRect(renderer, &selected_ship_rect);
    }//end if
}//end render_selected_ship_border

void render_hover_ship_border(SDL_Renderer *renderer, int ship_index, const bool placed_ships[], Ship ships[]) {
    int x, y;
    SDL_GetMouseState(&x, &y);
    SDL_Rect hover_ship_rect = {50, 50 + ship_index * 50, ships[ship_index].size * CELL_SIZE, CELL_SIZE};
    if (is_mouse_inside_button(x, y, hover_ship_rect) && !placed_ships[ship_index]) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        SDL_RenderDrawRect(renderer, &hover_ship_rect);
    }//end if
}//end render_hover_ship_border

void render_placed_ship_overlay(SDL_Renderer *renderer, int ship_index, int part_index, const bool placed_ships[], SDL_Texture *black_texture) {
    if (placed_ships[ship_index]) {
        SDL_Rect placed_ship_rect = {50 + part_index * CELL_SIZE, 50 + ship_index * 50, CELL_SIZE, CELL_SIZE};
        SDL_RenderCopy(renderer, black_texture, NULL, &placed_ship_rect);
    }//end if
}//end render_placed_ship_overlay

// Functions needed for render_placement_orientation_button
/// \brief Sets the button color based on whether the mouse is hovering over the orientation button.
/// \param renderer The renderer to use.
/// \param hover_orientation Indicates whether the mouse is hovering over the orientation button.
void set_button_color(SDL_Renderer *renderer, bool hover_orientation);

/// \brief Renders a shadow for the orientation button.
/// \param renderer The renderer to use.
/// \param orientation_button The SDL_Rect of the orientation button.
void render_button_shadow(SDL_Renderer *renderer, SDL_Rect orientation_button);

/// \brief Renders the orientation text based on the current orientation of the ship (0 for horizontal, 1 for vertical).
/// \param renderer The renderer to use.
/// \param orientation_button The SDL_Rect of the orientation button.
/// \param orientation The current orientation of the ship.
void render_orientation_text(SDL_Renderer *renderer, SDL_Rect orientation_button, int orientation);

/// \brief Renders the orientation button that the player can use to change the orientation of the ship that is currently selected.
/// \param renderer The renderer to use.
/// \param orientation_button The SDL_Rect of the orientation button.
/// \param hover_orientation Indicates whether the mouse is hovering over the orientation button.
/// \param orientation The current orientation of the ship (0 for horizontal, 1 for vertical).
void render_placement_orientation_button(SDL_Renderer *renderer, SDL_Rect orientation_button, int hover_orientation, int orientation) {
    // Set button color based on mouse hover
    set_button_color(renderer, hover_orientation);

    // Render button background
    SDL_RenderFillRect(renderer, &orientation_button);

    // Render button shadow
    render_button_shadow(renderer, orientation_button);

    // Render orientation text
    render_orientation_text(renderer, orientation_button, orientation);
}//end render_placement_orientation_button

void set_button_color(SDL_Renderer *renderer, bool hover_orientation) {
    if (hover_orientation) {
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255); // Lighter gray
    } else {
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Original gray
    }//end else
}//end set_button_color

void render_button_shadow(SDL_Renderer *renderer, SDL_Rect orientation_button) {
    SDL_Rect shadow_rect = orientation_button;
    shadow_rect.x += 4;
    shadow_rect.y += 4;
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 128);
    SDL_RenderFillRect(renderer, &shadow_rect);
}//end render_button_shadow

void render_orientation_text(SDL_Renderer *renderer, SDL_Rect orientation_button, int orientation) {
    if (orientation == 0) {
        render_text(renderer, "Orientation: Horizontal", orientation_button.x + 24, orientation_button.y + 8);
    } else {
        render_text(renderer, "Orientation: Vertical", orientation_button.x + 24, orientation_button.y + 8);
    }//end else
}//end render_orientation_text

// Functions needed for render_placement_grid_ships
/// \brief Renders the grid background during the placement phase.
/// \param renderer The renderer to use.
/// \param textures The textures to use.
/// \param ship_selected The index of the ship that is currently selected.
void render_grid_background(SDL_Renderer *renderer, GameTextures *textures, int ship_selected);

/// \brief Renders the ship hover effect during the placement phase.
/// \param renderer The renderer to use.
/// \param textures The textures to use.
/// \param ships The ships to render.
/// \param ship_selected The index of the ship that is currently selected.
/// \param orientation The current orientation of the ship (0 for horizontal, 1 for vertical).
/// \param grid_mouse_x The x-coordinate of the grid cell the mouse is hovering over.
/// \param grid_mouse_y The y-coordinate of the grid cell the mouse is hovering over.
/// \param valid_position Indicates whether the ship can be placed at the current position.
void render_ship_hover(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], int ship_selected, int orientation, int grid_mouse_x, int grid_mouse_y, bool valid_position);

/// \brief Renders the placed ships during the placement phase.
/// \param renderer The renderer to use.
/// \param textures The textures to use.
/// \param ships The ships to render.
/// \param placed_ships An array of booleans indicating whether a ship has been placed.
void render_placed_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[]);

/// \brief Renders a ship segment during the placement phase.
/// \param renderer The renderer to use.
/// \param textures The textures to use.
/// \param ship The ship to render.
/// \param segment The segment of the ship to render.
/// \param orientation The orientation of the ship (0 for horizontal, 1 for vertical).
/// \param ship_rect The SDL_Rect representing the position and dimensions of the ship segment.
void render_ship_segment(SDL_Renderer *renderer, GameTextures *textures, Ship ship, int segment, int orientation, SDL_Rect *ship_rect);

/// \brief Renders the ship border during the placement phase.
/// \param renderer The renderer to use.
/// \param ship_size The size of the ship.
/// \param segment The segment of the ship.
/// \param orientation The orientation of the ship (0 for horizontal, 1 for vertical).
/// \param ship_rect The SDL_Rect representing the position and dimensions of the ship segment.
void render_ship_border(SDL_Renderer *renderer, int ship_size, int segment, int orientation, SDL_Rect *ship_rect);

/// \brief Renders the grid with ships and the ship that is currently selected (if any). This function is used in the placement phase.
/// \param renderer The renderer to use.
/// \param textures The textures to use.
/// \param ships The ships to render.
/// \param placed_ships An array of booleans indicating whether a ship has been placed.
/// \param ship_selected The index of the ship that is currently selected.
/// \param orientation The current orientation of the ship (0 for horizontal, 1 for vertical).
/// \param grid_mouse_x The x-coordinate of the grid cell the mouse is hovering over.
/// \param grid_mouse_y The y-coordinate of the grid cell the mouse is hovering over.
/// \param valid_position Indicates whether the ship can be placed at the current position.
void render_placement_grid_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[], int ship_selected, int orientation, int grid_mouse_x, int grid_mouse_y, bool valid_position) {
    // Render grid background
    render_grid_background(renderer, textures, ship_selected);

    // Render ship hover
    render_ship_hover(renderer, textures, ships, ship_selected, orientation, grid_mouse_x, grid_mouse_y, valid_position);

    // Render placed ships
    render_placed_ships(renderer, textures, ships, placed_ships);
}//end render_placement_grid_ships

void render_grid_background(SDL_Renderer *renderer, GameTextures *textures, int ship_selected) {
    SDL_Texture *background_texture = ship_selected >= 0 ? textures->ocean_selection_mode : textures->ocean;

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            SDL_Rect grid_rect = {400 + i * CELL_SIZE, 50 + j * CELL_SIZE, CELL_SIZE, CELL_SIZE};
            SDL_RenderCopy(renderer, background_texture, NULL, &grid_rect);
        }//end for
    }//end for
}//end render_grid_background

void render_ship_hover(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], int ship_selected, int orientation, int grid_mouse_x, int grid_mouse_y, bool valid_position) {
    if (ship_selected >= 0) {
        int ship_size = ships[ship_selected].size;

        // Set the color for the ship border based on valid_position
        if (valid_position) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
        }//end else

        for (int k = 0; k < ship_size; k++) {
            int cell_x = grid_mouse_x + (orientation == 0 ? k : 0);
            int cell_y = grid_mouse_y + (orientation == 1 ? k : 0);

            // Make sure the cell is within the grid
            if (cell_x >= 0 && cell_x < BOARD_SIZE && cell_y >= 0 && cell_y < BOARD_SIZE) {
                SDL_Rect ship_rect = {400 + cell_x * CELL_SIZE, 50 + cell_y * CELL_SIZE, CELL_SIZE, CELL_SIZE};

                // Render the ship segment based on orientation and position
                render_ship_segment(renderer, textures, ships[ship_selected], k, orientation, &ship_rect);

                // Render the ship border
                render_ship_border(renderer, ship_size, k, orientation, &ship_rect);
            }//end if
        }//end for
    }//end if
}//end render_ship_hover

void render_ship_border(SDL_Renderer *renderer, int ship_size, int segment, int orientation, SDL_Rect *ship_rect) {
    if (orientation == 1) {
        if (segment == 0) {
            SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y, ship_rect->x + ship_rect->w, ship_rect->y); // Top border
        } else if (segment == ship_size - 1) {
            SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y + ship_rect->h - 1, ship_rect->x + ship_rect->w, ship_rect->y + ship_rect->h - 1); // Bottom border
        }//end else if
        SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y, ship_rect->x, ship_rect->y + ship_rect->h); // Left border
        SDL_RenderDrawLine(renderer, ship_rect->x + ship_rect->w - 1, ship_rect->y, ship_rect->x + ship_rect->w - 1, ship_rect->y + ship_rect->h); // Right border
    } else {
        if (segment == 0) {
            SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y, ship_rect->x, ship_rect->y + ship_rect->h); // Left border
        } else if (segment == ship_size - 1) {
            SDL_RenderDrawLine(renderer, ship_rect->x + ship_rect->w - 1, ship_rect->y, ship_rect->x + ship_rect->w - 1, ship_rect->y + ship_rect->h); // Right border
        } //end else if
        SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y, ship_rect->x + ship_rect->w, ship_rect->y); // Top border
        SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y + ship_rect->h - 1, ship_rect->x + ship_rect->w, ship_rect->y + ship_rect->h - 1); // Bottom border
    }//end if
}//end render_ship_border

void render_placed_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[]) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (placed_ships[i]) {
            for (int j = 0; j < ships[i].size; j++) {
                int cell_x = ships[i].x + (ships[i].orientation == 0 ? j : 0);
                int cell_y = ships[i].y + (ships[i].orientation == 1 ? j : 0);
                SDL_Rect ship_rect = {400 + cell_x * CELL_SIZE, 50 + cell_y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
                render_ship_segment(renderer, textures, ships[i], j, ships[i].orientation, &ship_rect);
            }//end for
        }//end if
    }//end for
}//end render_placed_ships

void render_ship_segment(SDL_Renderer *renderer, GameTextures *textures, Ship ship, int segment, int orientation, SDL_Rect *ship_rect) {
    if (orientation == 1) {
        if (segment == 0) {
            SDL_RenderCopy(renderer, textures->ship_top, NULL, ship_rect);
        } else if (segment == ship.size - 1) {
            SDL_RenderCopy(renderer, textures->ship_bottom, NULL, ship_rect);
        } else {
            SDL_RenderCopyEx(renderer, textures->ship_middle, NULL, ship_rect, 90, NULL, SDL_FLIP_NONE);
        }//end else
    } else {
        if (segment == 0) {
            SDL_RenderCopy(renderer, textures->ship_left, NULL, ship_rect);
        } else if (segment == ship.size - 1) {
            SDL_RenderCopy(renderer, textures->ship_right, NULL, ship_rect);
        } else {
            SDL_RenderCopy(renderer, textures->ship_middle, NULL, ship_rect);
        }//end else
    }//end else
}//end render_ship_segment

/// \brief Helper function for handling events during the placement phase
/// \param event The event to handle
/// \param running Pointer to the running variable
/// \param ship_selected Pointer to the selected ship
/// \param placed_ships Array of booleans indicating if a ship has been placed
/// \param ships Array of ships
/// \param orientation Pointer to the orientation of the selected ship
/// \param hover_orientation Pointer to the orientation of the ship hover
/// \param current_player Pointer to the current player
/// \param grid_mouse_x The x coordinate of the mouse on the grid
/// \param grid_mouse_y The y coordinate of the mouse on the grid
/// \param valid_position Boolean indicating if the ship is in a valid position
/// \param orientation_button The orientation button
/// \param exit_button The exit button
void handle_placement_phase_event(SDL_Event *event, bool *running, int *ship_selected, bool *placed_ships, Ship *ships, int *orientation, int *hover_orientation, Player *current_player, int grid_mouse_x, int grid_mouse_y, bool valid_position, SDL_Rect orientation_button, SDL_Rect exit_button) {
    if (event->type == SDL_QUIT) {
        *running = 0;
    } else if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;

        // Check if the user clicked on the exit button
        if (is_mouse_inside_button(x, y, exit_button)) {
            *running = 0;
        }//end if

        // Check if the user clicked on the orientation button
        if (is_mouse_inside_button(x, y, orientation_button)) {
            *orientation = (*orientation + 1) % 2;
        } else {
            // Check if the user clicked on a ship
            for (int i = 0; i < NUM_SHIPS; i++) {
                if (placed_ships[i]) {
                    continue;
                }//end if

                int ship_size = ships[i].size;
                for (int j = 0; j < ship_size; j++) {
                    SDL_Rect ship_rect = {50 + j * CELL_SIZE, 50 + i * 50, CELL_SIZE, CELL_SIZE};
                    if (is_mouse_inside_button(x, y, ship_rect)) {
                        *ship_selected = i;
                        break;
                    }//end if
                }//end for
            }//end for
        }//end else
    } else if (event->type == SDL_MOUSEMOTION) {
        int x = event->motion.x;
        int y = event->motion.y;

        // Check if the user is hovering over the orientation button
        if (is_mouse_inside_button(x, y, orientation_button)) {
            *hover_orientation = 1;
        } else {
            *hover_orientation = 0;
        }//end else
    }//end else if

    // Check if the user clicked on the grid
    if (grid_mouse_x >= 0 && grid_mouse_x < BOARD_SIZE && grid_mouse_y >= 0 && grid_mouse_y < BOARD_SIZE) {
        if (valid_position && event->type == SDL_MOUSEBUTTONDOWN && *ship_selected >= 0 && !placed_ships[*ship_selected]) {
            place_ship(&current_player->board, &ships[*ship_selected], grid_mouse_x, grid_mouse_y, *orientation);
            placed_ships[*ship_selected] = true; // Set the ship as placed
            *ship_selected = -1;

            // Update player's ships
            current_player->ships[*ship_selected] = ships[*ship_selected];
            current_player->ships_remaining++;
        }//end if
    }//end if
}//end handle_placement_phase_event

/// \brief Handles the placement phase screen, where the player can place ships on the game board.
/// \param renderer The SDL_Renderer to use.
/// \param textures The game textures to use.
/// \param current_player The current player whose ships will be placed on the game board.
void placement_phase_screen(SDL_Renderer *renderer, GameTextures *textures, Player *current_player) {
    // Load background texture
    SDL_Texture *background_texture = IMG_LoadTexture(renderer, "Assets/selecting_screen_background.jpg");

    // Create black surface
    SDL_Surface *black_surface = SDL_CreateRGBSurface(0, CELL_SIZE, CELL_SIZE, 32, 0, 0, 0, 0);
    SDL_SetSurfaceAlphaMod(black_surface, 128); // Set the alpha value to 128 (50% opacity)
    SDL_FillRect(black_surface, NULL, SDL_MapRGB(black_surface->format, 0, 0, 0));
    SDL_Texture *black_texture = SDL_CreateTextureFromSurface(renderer, black_surface);
    SDL_SetTextureBlendMode(black_texture, SDL_BLENDMODE_BLEND);

    // Initialize variables
    int ship_selected = -1;
    int orientation = 0; // 0 for horizontal, 1 for vertical
    int hover_orientation = 0;
    initialize_game_board(&current_player->board);

    SDL_Rect orientation_button = {50, 300, 275, 50}; // x, y, width, height
    SDL_Rect exit_button = {0, 0, 100, 50};

    Ship ships[NUM_SHIPS] = {
            {CARRIER,     5, 0, 0, 0},
            {BATTLESHIP,  4, 0, 0, 0},
            {DESTROYER,   3, 0, 0, 0},
            {SUBMARINE,   3, 0, 0, 0},
            {PATROL_BOAT, 2, 0, 0, 0}
    };
    bool placed_ships[NUM_SHIPS] = {false, false, false, false, false};

    // Main loop for the placement_phase_screen
    bool running = 1;
    while (running) {
        SDL_Event event;

        // Get the current mouse position on the grid
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        int grid_mouse_x = (mouse_x - 400) / CELL_SIZE;
        int grid_mouse_y = (mouse_y - 50) / CELL_SIZE;
        bool valid_position = is_position_valid(current_player, ships[ship_selected].size, grid_mouse_x, grid_mouse_y, orientation);

        while (SDL_PollEvent(&event)) {
            handle_placement_phase_event(&event, &running, &ship_selected, placed_ships, ships, &orientation, &hover_orientation, current_player, grid_mouse_x, grid_mouse_y, valid_position, orientation_button, exit_button);
        }//end while

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render screen background
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);

        // Render the ships on the left side of the screen
        render_placement_ships_left_side(renderer, textures, ships, placed_ships, black_texture, ship_selected);

        // Render the orientation button
        render_placement_orientation_button(renderer, orientation_button, hover_orientation, orientation);

        // Render the grid and the ships on the grid (if any).
        render_placement_grid_ships(renderer, textures, ships, placed_ships, ship_selected, orientation, grid_mouse_x, grid_mouse_y, valid_position);

        // Render the exit button
        render_text(renderer, "Exit", exit_button.x + 25, exit_button.y + 10);

        // Render the screen
        SDL_RenderPresent(renderer);
    }//end while

    // Destroy textures
    SDL_DestroyTexture(background_texture);
    SDL_FreeSurface(black_surface);
    SDL_DestroyTexture(black_texture);
}//end placement_phase_screen

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
    player1.ships_remaining = NUM_SHIPS;
    player2.ships_remaining = NUM_SHIPS;

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
        placement_phase_screen(renderer, textures, &player1);

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

        placement_phase_screen(renderer, textures, &player2);
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