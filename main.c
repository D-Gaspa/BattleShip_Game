// SDL must be told to redefine the main function in order to work with some platforms
#define SDL_MAIN_HANDLED

// Include necessary libraries
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_thread.h>
#include <stdbool.h>

// Define constants for the game
#define BOARD_SIZE 10
#define NUM_SHIPS 5
#define CELL_SIZE 32

// Structure for representing a cell on the game board
typedef struct {
    int x;
    int y;
    int width;
    int height;
    bool occupied;
    bool hit;
} Cell;

// Structure for representing a game board
typedef struct {
    Cell cells[BOARD_SIZE][BOARD_SIZE];
} GameBoard;

// Enum for representing ship types
typedef enum {
    CARRIER = 5,
    BATTLESHIP = 4,
    DESTROYER = 3,
    SUBMARINE = 3,
    PATROL_BOAT = 2
} ShipType;

// Structure for representing a ship
typedef struct {
    ShipType type;
    int size;
    int hit_count;
    int x;
    int y;
    int orientation;
} Ship;

// Structure for representing a player
typedef struct {
    GameBoard board;
    Ship ships[NUM_SHIPS];
    int ships_remaining;
    int is_turn;
} Player;

// Structure for holding game textures
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

// Enum for representing the result of a move
typedef enum {
    MOVE_RESULT_MISS,
    MOVE_RESULT_HIT,
    MOVE_RESULT_SUNK_SHIP
} MoveResult;

// Enum for representing main menu options
typedef enum {
    MAIN_MENU_NEW_GAME,
    MAIN_MENU_LOAD,
    MAIN_MENU_EXIT
} MainMenuOption;

// Global variables for font and font size
TTF_Font *font = NULL;
int font_size = 24;

/// \brief Load an image file as an SDL_Texture.
/// \param filename The path to the image file.
/// \param renderer The SDL_Renderer used to create the texture.
/// \return A pointer to the loaded SDL_Texture, or NULL if loading fails.
SDL_Texture *load_texture(const char *filename, SDL_Renderer *renderer) {
    // Load the image as an SDL_Surface
    SDL_Surface *surface = IMG_Load(filename);
    if (surface == NULL) {
        printf("Failed to load image: %s. SDL Error: %s\n", filename, IMG_GetError());
        return NULL;
    }//end if

    // Convert the SDL_Surface to an SDL_Texture
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Failed to create texture from surface. SDL Error: %s\n", SDL_GetError());
    }//end if

    // Free the SDL_Surface
    SDL_FreeSurface(surface);
    return texture;
}//end load_texture

/// \brief Load all game textures and store them in a GameTextures structure.
/// \param renderer The SDL_Renderer used to create the textures.
/// \return A pointer to the loaded GameTextures, or NULL if loading fails.
GameTextures *load_game_textures(SDL_Renderer *renderer) {
    // Allocate memory for the GameTextures structure
    GameTextures *textures = (GameTextures *) malloc(sizeof(GameTextures));
    if (textures == NULL) {
        printf("Failed to allocate memory for game textures.\n");
        return NULL;
    }//end if

    // Load individual game textures
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

/// \brief Load an animated background by loading multiple image frames as SDL_Textures.
/// \param renderer The SDL_Renderer used to create the textures.
/// \param filepath The base file path of the image sequence, without the frame number or file extension.
/// \param num_frames The number of frames in the animation.
/// \return An array of pointers to the loaded SDL_Texture frames, or NULL if loading fails.
SDL_Texture **load_animated_background(SDL_Renderer *renderer, const char *filepath, int num_frames) {
    // Allocate memory for the array of SDL_Texture pointers
    SDL_Texture **textures = (SDL_Texture **) malloc(sizeof(SDL_Texture *) * num_frames);

    // Load each frame of the animation
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

/// \brief Renders text at the specified position on the screen.
/// \param renderer The SDL_Renderer used to render the text.
/// \param text The text string to render.
/// \param x The x-coordinate of the top-left corner of the text.
/// \param y The y-coordinate of the top-left corner of the text.
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

/// \brief Checks if the mouse is inside the specified button.
/// \param x The x-coordinate of the mouse cursor.
/// \param y The y-coordinate of the mouse cursor.
/// \param button_rect The rectangle representing the button's position and size.
/// \return 1 if the mouse is inside the button, 0 otherwise.
int is_mouse_inside_button(int x, int y, SDL_Rect button_rect) {
    return x >= button_rect.x && x <= button_rect.x + button_rect.w &&
           y >= button_rect.y && y <= button_rect.y + button_rect.h;
}//end is_mouse_inside_button

/// \brief Initialize main menu by loading the animated background textures and creating the buttons.
/// \param renderer The SDL_Renderer used to create the textures.
/// \param background_frames A pointer to the array of background frames.
/// \param button_rects An array of SDL_Rects representing the positions of the buttons.
/// \param num_background_frames A pointer to the number of background frames.
void init_main_menu(SDL_Renderer *renderer, SDL_Texture ***background_frames, SDL_Rect *button_rects, int *num_background_frames) {
    // Load animated background textures
    *num_background_frames = 10;
    *background_frames = load_animated_background(renderer, "Assets/Backgrounds/frame_",*num_background_frames);
    if (*background_frames == NULL) {
        printf("Failed to load animated background.\n");
        return;
    }//end if

    // Create buttons and positions for the main menu
    for (int i = 0; i < 3; i++) {
        button_rects[i].x = (BOARD_SIZE * CELL_SIZE) / 2 - 75;
        button_rects[i].y = 150 + i * 60;
        button_rects[i].w = 150;
        button_rects[i].h = 40;
    }//end for
}//end init_main_menu

/// \brief Handle main menu events, such as mouse motion and button clicks.
/// \param event Pointer to the SDL_Event structure.
/// \param button_rects An array of SDL_Rects representing the positions of the buttons.
/// \param hover_button Pointer to the integer representing the button the mouse is hovering over.
/// \param selected_option Pointer to the MainMenuOption representing the selected menu option.
/// \return 1 if the program should continue running, 0 if the program should exit.
int handle_main_menu_events(SDL_Event *event, SDL_Rect *button_rects, int *hover_button, MainMenuOption *selected_option) {
    int running = 1;

    // Handle SDL_QUIT event (e.g., user closes the window)
    if (event->type == SDL_QUIT) {
        running = 0;
    }//end if

    // Handle mouse movement
    else if (event->type == SDL_MOUSEMOTION) {
        int x = event->motion.x;
        int y = event->motion.y;

        // Check if the mouse is hovering over any buttons
        *hover_button = -1;
        for (int i = 0; i < 3; i++) {
            if (is_mouse_inside_button(x, y, button_rects[i])) {
                *hover_button = i;
                break;
            }//end if
        }//end for
    }//end if

    // Handle mouse button press
    else if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;

        // Check if any buttons were clicked
        for (int i = 0; i < 3; i++) {
            if (is_mouse_inside_button(x, y, button_rects[i])) {
                *selected_option = (MainMenuOption)i;
                return 0;
            }//end if
        }//end for
    }//end if

    return running;
}//end handle_main_menu_events

/// \brief Render the main menu scene, including the animated background, buttons, and button labels.
/// \param renderer The SDL_Renderer used to render the menu.
/// \param background_frames An array of SDL_Textures representing the frames of the animated background.
/// \param frame_counter The current frame of the animated background.
/// \param button_rects An array of SDL_Rects representing the positions of the buttons.
/// \param hover_button The index of the button the mouse is hovering over.
void render_main_menu(SDL_Renderer *renderer, SDL_Texture **background_frames, int frame_counter, SDL_Rect *button_rects, int hover_button) {
    // Render the animated background
    SDL_RenderCopy(renderer, background_frames[frame_counter], NULL, NULL);

    // Button labels
    const char *button_labels[] = {"New Game", "Load", "Exit"};

    // Render buttons
    for (int i = 0; i < 3; i++) {
        // Set button background color based on whether the mouse is hovering over the button
        if (hover_button == i) {
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255); // Lighter gray for hover
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Original gray
        }//end if

        // Render button background
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
}//end render_main_menu

/// \brief Renders the main menu.
/// \param renderer The SDL_Renderer used to render the menu.
/// \return the selected menu option.
MainMenuOption main_menu(SDL_Renderer *renderer) {
    MainMenuOption selected_option = MAIN_MENU_EXIT;

    // Initialize main menu
    int num_background_frames;
    SDL_Texture **background_frames;
    SDL_Rect button_rects[3];
    init_main_menu(renderer, &background_frames, button_rects, &num_background_frames);

    int running = 1;
    int frame_counter = 0;
    int hover_button = -1;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            running = handle_main_menu_events(&event, button_rects, &hover_button, &selected_option);
            if (running == 0) {
                break;
            }//end if
        }//end while

        // Render the main menu
        frame_counter = (frame_counter + 1) % num_background_frames;
        render_main_menu(renderer, background_frames, frame_counter, button_rects, hover_button);

        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / 60); // Limit frame rate to 60 FPS
    }//end while

    // Free resources
    for (int i = 0; i < num_background_frames; i++) {
        SDL_DestroyTexture(background_frames[i]);
    }//end for
    free(background_frames);

    return selected_option;
}//end main_menu

/// \brief Initializes a game board with empty cells.
/// \param board The GameBoard to initialize.
void initialize_game_board(GameBoard *board) {
    // Iterate through all cells of the board
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

/// \brief Determines if a ship can be placed at the specified position on the board.
/// \param current_player The player to check the position for.
/// \param ship_size The size of the ship to place.
/// \param x The x-coordinate of the top-left cell of the ship.
/// \param y The y-coordinate of the top-left cell of the ship.
/// \param orientation The orientation of the ship (0 for horizontal, 1 for vertical).
/// \return true if the position is valid, false otherwise.
bool is_position_valid(Player *current_player, int ship_size, int x, int y, int orientation) {
    // Iterate through all cells of the ship
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

/// \brief Places a ship on the game board.
/// \param board The GameBoard on which the ship will be placed.
/// \param ship The Ship to place on the board.
/// \param x The x-coordinate of the top-left cell of the ship.
/// \param y The y-coordinate of the top-left cell of the ship.
/// \param orientation The orientation of the ship (0 for horizontal, 1 for vertical).
void place_ship(GameBoard *board, Ship *ship, int x, int y, int orientation) {
    ship->x = x;
    ship->y = y;
    ship->orientation = orientation;

    // Iterate through all cells of the ship
    for (int i = 0; i < ship->size; i++) {
        int cell_x = x + (orientation == 0 ? i : 0);
        int cell_y = y + (orientation == 1 ? i : 0);

        Cell *cell = &board->cells[cell_x][cell_y];
        cell->occupied = true;
    }//end for
}//end place_ship

// Functions needed for render_placement_ships_left_side
/// \brief Renders a ship part (left, middle, or right) at the given position based on the part index and the ship size.
/// \param renderer The renderer to use.
/// \param textures The textures to use.
/// \param ship_rect The rectangle in which the ship part should be rendered.
/// \param part_index The index of the part to be rendered (0 for left, ship_size - 1 for right, and middle parts otherwise).
/// \param ship_size The size of the ship.
void render_ship_part(SDL_Renderer *renderer, GameTextures *textures, SDL_Rect ship_rect, int part_index, int ship_size);

/// \brief Renders a border around the selected ship to indicate that it is selected.
/// \param renderer The renderer to use.
/// \param ship_index The index of the ship to check for selection.
/// \param ship_selected The index of the currently selected ship.
/// \param ships The array of ships.
void render_selected_ship_border(SDL_Renderer *renderer, int ship_index, int ship_selected, Ship ships[]);

/// \brief Render a border around the ship that the mouse is hovering over (if it is not placed yet)
/// \param renderer The renderer to use.
/// \param ship_index The index of the ship to check for hover.
/// \param placed_ships An array of booleans indicating whether a ship has been placed.
/// \param ships The array of ships.
void render_hover_ship_border(SDL_Renderer *renderer, int ship_index, const bool placed_ships[], Ship ships[]);

/// \brief Render a black overlay on top of the placed ship to indicate that it is placed
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

    // Determine which part of the ship to render
    if (part_index == 0) {
        part_texture = textures->ship_left;
    } else if (part_index == ship_size - 1) {
        part_texture = textures->ship_right;
    } else {
        part_texture = textures->ship_middle;
    }//end else

    // Render the ship part
    SDL_RenderCopy(renderer, part_texture, NULL, &ship_rect);
}//end render_ship_part

void render_selected_ship_border(SDL_Renderer *renderer, int ship_index, int ship_selected, Ship ships[]) {
    // Check if the current ship is selected
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

    // Check if the mouse is hovering over the ship, and it has not been placed yet
    if (is_mouse_inside_button(x, y, hover_ship_rect) && !placed_ships[ship_index]) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        SDL_RenderDrawRect(renderer, &hover_ship_rect);
    }//end if
}//end render_hover_ship_border

void render_placed_ship_overlay(SDL_Renderer *renderer, int ship_index, int part_index, const bool placed_ships[], SDL_Texture *black_texture) {
    // Check if the current ship has been placed
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

/// \brief Renders a shadow under the orientation button to give it a 3D effect.
/// \param renderer The renderer to use.
/// \param orientation_button The SDL_Rect of the orientation button.
void render_button_shadow(SDL_Renderer *renderer, SDL_Rect orientation_button);

/// \brief Renders the orientation text (either "Horizontal" or "Vertical") inside the orientation button based on the current orientation of the ship (0 for horizontal, 1 for vertical).
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
    // Change the color based on the hover state
    if (hover_orientation) {
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255); // Lighter gray
    } else {
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Original gray
    }//end else
}//end set_button_color

void render_button_shadow(SDL_Renderer *renderer, SDL_Rect orientation_button) {
    // Create a new rectangle for the shadow, offset from the button
    SDL_Rect shadow_rect = orientation_button;
    shadow_rect.x += 4;
    shadow_rect.y += 4;

    // Set the color for the shadow and render it
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 128);
    SDL_RenderFillRect(renderer, &shadow_rect);
}//end render_button_shadow

void render_orientation_text(SDL_Renderer *renderer, SDL_Rect orientation_button, int orientation) {
    // Render the appropriate text based on the orientation value
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
    // Choose the background texture based on whether a ship is selected
    SDL_Texture *background_texture = ship_selected >= 0 ? textures->ocean_selection_mode : textures->ocean;

    // Render the grid background by looping through all cells
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            SDL_Rect grid_rect = {400 + i * CELL_SIZE, 50 + j * CELL_SIZE, CELL_SIZE, CELL_SIZE};
            SDL_RenderCopy(renderer, background_texture, NULL, &grid_rect);
        }//end for
    }//end for
}//end render_grid_background

void render_ship_hover(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], int ship_selected, int orientation, int grid_mouse_x, int grid_mouse_y, bool valid_position) {
    // Check if a ship is selected
    if (ship_selected >= 0) {
        int ship_size = ships[ship_selected].size;

        // Set the color for the ship border based on valid_position
        if (valid_position) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
        }//end else

        // Loop through each segment of the ship
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
    // Vertical orientation
    if (orientation == 1) {

        // Draw top and bottom borders for the first and last segments
        if (segment == 0) {
            SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y, ship_rect->x + ship_rect->w, ship_rect->y); // Top border
        } else if (segment == ship_size - 1) {
            SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y + ship_rect->h - 1, ship_rect->x + ship_rect->w, ship_rect->y + ship_rect->h - 1); // Bottom border
        }//end else if

        // Draw left and right borders for all segments
        SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y, ship_rect->x, ship_rect->y + ship_rect->h); // Left border
        SDL_RenderDrawLine(renderer, ship_rect->x + ship_rect->w - 1, ship_rect->y, ship_rect->x + ship_rect->w - 1, ship_rect->y + ship_rect->h); // Right border
    } else { // Horizontal orientation

        // Draw left and right borders for the first and last segments
        if (segment == 0) {
            SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y, ship_rect->x, ship_rect->y + ship_rect->h); // Left border
        } else if (segment == ship_size - 1) {
            SDL_RenderDrawLine(renderer, ship_rect->x + ship_rect->w - 1, ship_rect->y, ship_rect->x + ship_rect->w - 1, ship_rect->y + ship_rect->h); // Right border
        }//end else if

        // Draw top and bottom borders for all segments
        SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y, ship_rect->x + ship_rect->w, ship_rect->y); // Top border
        SDL_RenderDrawLine(renderer, ship_rect->x, ship_rect->y + ship_rect->h - 1, ship_rect->x + ship_rect->w, ship_rect->y + ship_rect->h - 1); // Bottom border
    }//end if
}//end render_ship_border

void render_placed_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[]) {
    // Loop through all ships
    for (int i = 0; i < NUM_SHIPS; i++) {
        // Render the ship if it is placed
        if (placed_ships[i]) {
            // Loop through each segment of the ship
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
    // Vertical orientation
    if (orientation == 1) {

        // Render the top, bottom, and middle segments based on the segment index
        if (segment == 0) {
            SDL_RenderCopy(renderer, textures->ship_top, NULL, ship_rect);
        } else if (segment == ship.size - 1) {
            SDL_RenderCopy(renderer, textures->ship_bottom, NULL, ship_rect);
        } else {
            SDL_RenderCopyEx(renderer, textures->ship_middle, NULL, ship_rect, 90, NULL, SDL_FLIP_NONE);
        }//end else
    } else { // Horizontal orientation

        // Render the left, right, and middle segments based on the segment index
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
    // Handle SDL_QUIT event (e.g., user closes the window)
    if (event->type == SDL_QUIT) {
        *running = 0;
    }//end if

    // Handle mouse button press event
    else if (event->type == SDL_MOUSEBUTTONDOWN) {
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
    }//end else if

    // Handle mouse motion event
    else if (event->type == SDL_MOUSEMOTION) {
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