// SDL must be told to redefine the main function in order to work with some platforms
#define SDL_MAIN_HANDLED

// Include necessary libraries
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include "pcg_basic.h"
#include <SDL_thread.h>

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

// Struct to store the data of the placement phase buttons
typedef struct ButtonData {
    SDL_Rect exit_button;
    SDL_Rect orientation_button;
    SDL_Rect reset_button;
    SDL_Rect random_button;
    SDL_Rect finish_button;
    bool hover_orientation;
    bool hover_reset;
    bool hover_random;
    bool hover_finish;
} ButtonData;

// Global variables for font and font size
TTF_Font *font = NULL;
int font_size = 24;

// Function prototypes

/// \brief Loads an SDL_Texture from a given file.
///
/// Loads an image file and converts it to an SDL_Texture using the provided SDL_Renderer.
///
/// \param filename A string representing the path to the image file.
/// \param renderer A pointer to an SDL_Renderer.
/// \return SDL_Texture* A pointer to the loaded SDL_Texture or NULL if the operation fails.
SDL_Texture *load_texture(const char *filename, SDL_Renderer *renderer);

/// \brief Loads all game textures.
///
/// Allocates memory for a GameTextures structure and loads individual game textures using the provided SDL_Renderer.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \return GameTextures* A pointer to the loaded GameTextures structure or NULL if the operation fails.
GameTextures *load_game_textures(SDL_Renderer *renderer);

/// \brief Loads an animated background.
///
/// Loads a sequence of image files representing the frames of an animated background.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param filepath A string representing the path to the image files without the frame number and file extension.
/// \param num_frames An int representing the number of frames in the animation.
/// \return SDL_Texture** An array of pointers to the loaded SDL_Textures or NULL if the operation fails.
SDL_Texture **load_animated_background(SDL_Renderer *renderer, const char *filepath, int num_frames);

/// \brief Renders text using the global font variable.
///
/// Renders a given text at the specified position using the global font and the provided SDL_Renderer.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param text A string representing the text to be rendered.
/// \param x The x-coordinate of the text's position.
/// \param y The y-coordinate of the text's position.
/// \return void
void render_text(SDL_Renderer *renderer, const char *text, int x, int y);

/// \brief Renders colored text using the global font variable.
///
/// Renders a given text at the specified position with the specified color using the global font and the provided SDL_Renderer.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param text A string representing the text to be rendered.
/// \param x The x-coordinate of the text's position.
/// \param y The y-coordinate of the text's position.
/// \param r The red component of the text color.
/// \param g The green component of the text color.
/// \param b The blue component of the text color.
/// \return void
void render_colored_text(SDL_Renderer *renderer, const char *text, int x, int y, int r, int g, int b);

/// \brief Determines if the mouse is inside a button.
///
/// Checks if the given mouse coordinates are within the boundaries of an SDL_Rect
/// button.
///
/// \param x The x-coordinate of the mouse.
/// \param y The y-coordinate of the mouse.
/// \param button_rect An SDL_Rect representing the button's position and dimensions.
/// \return int Returns 1 if the mouse is inside the button, 0 otherwise.
int is_mouse_inside_button(int x, int y, SDL_Rect button_rect);

/// \brief Initializes the main menu.
///
/// Loads the animated background textures, sets up the main menu buttons and their
/// positions.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param background_frames A pointer to an array of SDL_Texture pointers for the animated background frames.
/// \param button_rects An array of SDL_Rect structures representing the main menu buttons.
/// \param num_background_frames A pointer to an int to store the number of background frames.
/// \return void
void init_main_menu(SDL_Renderer *renderer, SDL_Texture ***background_frames, SDL_Rect *button_rects, int *num_background_frames);

/// \brief Handles events for the main menu.
///
/// Processes SDL events for the main menu, such as mouse movement and button presses.
///
/// \param event A pointer to an SDL_Event structure.
/// \param button_rects An array of SDL_Rect structures representing the main menu buttons.
/// \param hover_button A pointer to an int that represents the index of the button being hovered.
/// \param selected_option A pointer to a MainMenuOption enum value to store the user's selection.
/// \return int Returns 0 if the user clicks a button, 1 if the main menu should continue running.
int handle_main_menu_events(SDL_Event *event, SDL_Rect *button_rects, int *hover_button, MainMenuOption *selected_option);

/// \brief Renders the main menu.
///
/// Renders the animated background, buttons, and button labels for the main menu.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param background_frames An array of SDL_Texture pointers for the animated background frames.
/// \param frame_counter An int representing the current frame of the animated background.
/// \param button_rects An array of SDL_Rect structures representing the main menu buttons.
/// \param hover_button An int representing the index of the button being hovered.
/// \return void
void render_main_menu(SDL_Renderer *renderer, SDL_Texture **background_frames, int frame_counter, SDL_Rect *button_rects, int hover_button);

/// \brief Executes the main menu loop.
///
/// Handles user interaction and rendering for the main menu until the user selects an option or closes the window.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \return MainMenuOption Returns the selected option from the main menu (NEW_GAME, LOAD, or EXIT).
MainMenuOption main_menu(SDL_Renderer *renderer);

/// \brief Initializes the game board.
///
/// Sets up the game board by initializing all cells with their position, size, and initial state.
///
/// \param board A pointer to a GameBoard structure.
/// \return void
void initialize_game_board(GameBoard *board);

/// \brief Determines if a ship position is valid on the game board.
///
/// Checks if the given ship position is within the grid bounds and not overlapping
/// any existing ships on the board.
///
/// \param current_player A pointer to the current Player structure.
/// \param ship_size The size of the ship to be placed.
/// \param x The x-coordinate of the ship's starting position.
/// \param y The y-coordinate of the ship's starting position.
/// \param orientation The ship's orientation (0 for horizontal, 1 for vertical).
/// \return true if the position is valid, false otherwise.
bool is_position_valid(Player *current_player, int ship_size, int x, int y, int orientation);

/// \brief Checks if all ships have been placed on the board.
///
/// Iterates through the placed_ships array and returns true if all ships have been placed.
///
/// \param placed_ships An array of booleans indicating whether each ship has been placed.
/// \return true if all ships have been placed, false otherwise.
bool all_ships_placed(const bool placed_ships[]);

/// \brief Places a ship on the game board.
///
/// Places a ship on the game board at the specified position and orientation, and
/// marks the corresponding cells as occupied.
///
/// \param board A pointer to the GameBoard structure.
/// \param ship A pointer to the Ship structure to be placed.
/// \param x The x-coordinate of the ship's starting position.
/// \param y The y-coordinate of the ship's starting position.
/// \param orientation The ship's orientation (0 for horizontal, 1 for vertical).
/// \return void
void place_ship(GameBoard *board, Ship *ship, int x, int y, int orientation);

/// \brief Places all ships randomly on the board.
///
/// Randomly places all ships on the game board for the current player using the pcg library.
/// ensuring valid positions and orientations. Resets the board and ships before placement.
///
/// \param current_player A pointer to the current Player structure.
/// \param ships An array of Ship structures representing the game ships.
/// \param placed_ships An array of booleans indicating whether each ship has been placed.
/// \param ship_selected A pointer to an int representing the selected ship index.
/// \param orientation_to_reset A pointer to an int representing the orientation to reset.
/// \return void
void place_random_ships(Player *current_player, Ship ships[], bool placed_ships[], int *ship_selected, int *orientation_to_reset);

/// \brief Renders a single part of a ship.
///
/// Renders a single part of a ship (left, middle, or right) based on the part_index and
/// ship size at the specified position.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param textures A pointer to a GameTextures structure holding game textures.
/// \param ship_rect An SDL_Rect representing the position and size of the ship part.
/// \param part_index The index of the part within the ship.
/// \param ship_size The size of the ship.
/// \return void
void render_ship_part(SDL_Renderer *renderer, GameTextures *textures, SDL_Rect ship_rect, int part_index, int ship_size);

/// \brief Renders the border for the currently selected ship.
///
/// Renders a yellow border around the currently selected ship in the ship placement screen.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param ship_index The index of the current ship in the ships array.
/// \param ship_selected The index of the currently selected ship.
/// \param ships An array of Ship structures representing the game ships.
/// \return void
void render_selected_ship_border(SDL_Renderer *renderer, int ship_index, int ship_selected, Ship ships[]);

/// \brief Renders the border for a ship when the mouse is hovering over it.
///
/// Renders a green border around a ship in the ship placement screen when the mouse is
/// hovering over it and the ship has not yet been placed.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param ship_index The index of the current ship in the ships array.
/// \param placed_ships An array of booleans indicating whether each ship has been placed.
/// \param ships An array of Ship structures representing the game ships.
/// \return void
void render_hover_ship_border(SDL_Renderer *renderer, int ship_index, const bool placed_ships[], Ship ships[]);

/// \brief Renders an overlay for a placed ship.
///
/// Renders a semi-transparent black overlay over a ship that has been placed on the
/// ship placement screen.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param ship_index The index of the current ship in the ships array.
/// \param part_index The index of the part within the ship.
/// \param placed_ships An array of booleans indicating whether each ship has been placed.
/// \param black_texture An SDL_Texture pointer to the black overlay texture.
/// \return void
void render_placed_ship_overlay(SDL_Renderer *renderer, int ship_index, int part_index, const bool placed_ships[], SDL_Texture *black_texture);

/// \brief Renders the ship icons on the left side of the placement screen.
///
/// Renders the ship icons on the left side of the placement screen, including
/// their hover and selection states, and an overlay for placed ships.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param textures A pointer to a GameTextures structure holding game textures.
/// \param ships An array of Ship structures representing the game ships.
/// \param placed_ships An array of booleans indicating whether each ship has been placed.
/// \param black_texture A pointer to an SDL_Texture representing the black texture.
/// \param ship_selected An int representing the selected ship index.
/// \return void
void render_placement_ships_left_side(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[], SDL_Texture *black_texture, int ship_selected);

/// \brief Sets the button color based on the mouse hover state.
///
/// Sets the button color based on whether the mouse is hovering over the button.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param hover_orientation A bool indicating whether the mouse is hovering over the button.
/// \return void
void set_button_color(SDL_Renderer *renderer, bool hover_orientation);

/// \brief Renders the button shadow.
///
/// Renders the shadow for a button, offset from the button rectangle.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param orientation_button An SDL_Rect representing the button's position and size.
/// \return void
void render_button_shadow(SDL_Renderer *renderer, SDL_Rect orientation_button);

/// \brief Renders the orientation text on the orientation button.
///
/// Renders the orientation text ("Orientation: Horizontal" or "Orientation: Vertical")
/// on the orientation button based on the current orientation value.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param orientation_button An SDL_Rect representing the button's position and size.
/// \param orientation An int representing the current orientation.
/// \return void
void render_orientation_text(SDL_Renderer *renderer, SDL_Rect orientation_button, int orientation);

/// \brief Renders the orientation button during the ship placement phase.
///
/// Renders the button for changing the ship orientation during the ship placement
/// phase, including its hover state, shadow, and orientation text.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param orientation_button An SDL_Rect representing the button's position and size.
/// \param hover_orientation A bool indicating whether the mouse is hovering over the button.
/// \param orientation An int representing the current orientation.
/// \return void
void render_placement_orientation_button(SDL_Renderer *renderer, SDL_Rect orientation_button, int hover_orientation, int orientation);

/// \brief Renders the "Restart the board" button during the ship placement phase.
///
/// Renders the button for restarting the board during the ship placement phase,
/// including its hover state, shadow, and reset text.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param reset_button An SDL_Rect representing the button's position and size.
/// \param hover_reset A bool indicating whether the mouse is hovering over the button.
/// \return void
void render_placement_reset_button(SDL_Renderer *renderer, SDL_Rect reset_button, bool hover_reset);

/// \brief Renders the "Randomize the board" button during the ship placement phase.
///
/// Renders the button to randomize ship positions on the board. Changes the
/// button color based on whether the mouse is hovering over the button.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param random_button An SDL_Rect representing the button's position and size.
/// \param hover_random A bool indicating whether the mouse is hovering over the button.
/// \return void
void render_placement_random_button(SDL_Renderer *renderer, SDL_Rect random_button, bool hover_random);

/// \brief Renders the "Finish placing ships" button during the ship placement phase.
///
/// Renders the button to finish placing ships on the board. Changes the button
/// color based on whether the mouse is hovering over the button and whether all ships
/// have been placed.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param finish_button An SDL_Rect representing the button's position and size.
/// \param hover_finish A bool indicating whether the mouse is hovering over the button.
/// \param placed_ships An array of booleans indicating whether each ship has been placed.
/// \param black_texture A pointer to an SDL_Texture representing the black texture.
/// \return void
void render_placement_finish_button(SDL_Renderer *renderer, SDL_Rect finish_button, bool hover_finish, const bool placed_ships[], SDL_Texture *black_texture);

/// \brief Renders the buttons during the ship placement phase.
///
/// Renders the buttons for changing orientation, resetting the board, randomizing
/// the board, and finishing the ship placement phase.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param button_data A pointer to a ButtonData structure holding button information.
/// \param placed_ships An array of booleans indicating whether each ship has been placed.
/// \param orientation An int representing the current orientation.
/// \param black_texture A pointer to an SDL_Texture representing the black texture.
/// \return void
void render_placement_buttons(SDL_Renderer *renderer ,ButtonData *button_data, const bool placed_ships[], int orientation, SDL_Texture *black_texture);

/// \brief Renders the grid background during the ship placement phase.
///
/// Renders the background for the grid during the ship placement phase. Chooses
/// the appropriate background texture based on whether a ship is selected.
///
/// \param renderer A pointer to an SDL_Renderer.
/// \param textures A pointer to a GameTextures structure holding game textures.
/// \param ship_selected An int representing the selected ship index.
/// \return void
void render_grid_background(SDL_Renderer *renderer, GameTextures *textures, int ship_selected);

/// \brief Renders the ship hover during the placement phase.
///
/// This function renders the ship that the user is currently hovering over the grid during the placement phase.
/// The ship's border color is set to either white or red based on whether the current position is valid.
///
/// \param renderer A pointer to an SDL_Renderer structure that represents the rendering context.
/// \param textures A pointer to a GameTextures structure that holds game-related textures.
/// \param ships An array of Ship structures representing the game's ships.
/// \param ship_selected The index of the selected ship.
/// \param orientation The orientation of the ship (0 for horizontal, 1 for vertical).
/// \param grid_mouse_x The x coordinate of the mouse on the game grid.
/// \param grid_mouse_y The y coordinate of the mouse on the game grid.
/// \param valid_position A boolean indicating whether the current ship placement position is valid.
/// \return void
void render_ship_hover(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], int ship_selected, int orientation, int grid_mouse_x, int grid_mouse_y, bool valid_position);

/// \brief Renders the placed ships on the game grid.
///
/// This function renders the ships that have been placed on the game grid during the placement phase.
///
/// \param renderer A pointer to an SDL_Renderer structure that represents the rendering context.
/// \param textures A pointer to a GameTextures structure that holds game-related textures.
/// \param ships An array of Ship structures representing the game's ships.
/// \param placed_ships An array of boolean values representing whether each ship has been placed on the grid.
/// \return void
void render_placed_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[]);

/// \brief Renders a single segment of a ship.
///
/// This function renders a specific segment of a ship based on the provided orientation and segment index.
///
/// \param renderer A pointer to an SDL_Renderer structure that represents the rendering context.
/// \param textures A pointer to a GameTextures structure that holds game-related textures.
/// \param ship A Ship structure representing the current ship.
/// \param segment The index of the current ship segment.
/// \param orientation The orientation of the ship (0 for horizontal, 1 for vertical).
/// \param ship_rect A pointer to an SDL_Rect structure that represents the ship's rectangle on the screen.
/// \return void
void render_ship_segment(SDL_Renderer *renderer, GameTextures *textures, Ship ship, int segment, int orientation, SDL_Rect *ship_rect);

/// \brief Renders the border of the ship during the placement phase.
///
/// This function draws the border of the ship segments during the placement phase.
///
/// \param renderer A pointer to an SDL_Renderer structure that represents the rendering context.
/// \param ship_size The size of the ship.
/// \param segment The index of the current ship segment.
/// \param orientation The orientation of the ship (0 for horizontal, 1 for vertical).
/// \param ship_rect A pointer to an SDL_Rect structure that represents the ship's rectangle on the screen.
/// \return void
void render_ship_border(SDL_Renderer *renderer, int ship_size, int segment, int orientation, SDL_Rect *ship_rect);

/// \brief Renders the game grid and ships during the placement phase.
///
/// This function renders the game grid, the ship hover, and the placed ships during the placement phase.
///
/// \param renderer A pointer to an SDL_Renderer structure that represents the rendering context.
/// \param textures A pointer to a GameTextures structure that holds game-related textures.
/// \param ships An array of Ship structures representing the game's ships.
/// \param placed_ships An array of boolean values representing whether each ship has been placed on the grid.
/// \param ship_selected The index of the selected ship.
/// \param orientation The orientation of the ship (0 for horizontal, 1 for vertical).
/// \param grid_mouse_x The x coordinate of the mouse on the game grid.
/// \param grid_mouse_y The y coordinate of the mouse on the game grid.
/// \param valid_position A boolean indicating whether the current ship placement position is valid.
/// \return void
void render_placement_grid_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[], int ship_selected, int orientation, int grid_mouse_x, int grid_mouse_y, bool valid_position);

/// \brief Resets a game board's occupied cells to false.
///
/// This function clears the occupied cells of a game board, setting them all to false.
///
/// \param board A pointer to the GameBoard structure representing the game board to reset.
/// \return void
void reset_game_board(GameBoard *board);

/// \brief Resets the ship placement phase variables.
///
/// Resets the variables related to the ship placement phase, such as ships,
/// placed_ships, ship_selected, orientation, and the game board.
///
/// \param ships An array of Ship structures representing the game ships.
/// \param placed_ships An array of booleans indicating whether each ship has been placed.
/// \param ship_selected A pointer to an int representing the selected ship index.
/// \param orientation A pointer to an int representing the current orientation.
/// \param board A pointer to a GameBoard structure representing the game board.
/// \return void
void reset_placement_phase(Ship *ships, bool *placed_ships, int *ship_selected, int *orientation, GameBoard *board);

/// \brief Handles events during the ship placement phase of a battleship game.
///
/// This function processes SDL events during the ship placement phase, such as user clicks and mouse movements.
/// It updates the game state and user interface based on these events, including placing ships on the board, changing ship orientation,
/// and interacting with various buttons.
///
/// \param event A pointer to an SDL_Event structure representing the current event.
/// \param running A pointer to a boolean that indicates if the game is running or not.
/// \param ship_selected A pointer to an integer that holds the index of the currently selected ship.
/// \param placed_ships A pointer to a boolean array representing if a ship has been placed on the board or not.
/// \param ships A pointer to an array of Ship structures representing the ships in the game.
/// \param current_player A pointer to a Player structure representing the current player.
/// \param grid_mouse_x An integer representing the x-coordinate of the mouse on the grid.
/// \param grid_mouse_y An integer representing the y-coordinate of the mouse on the grid.
/// \param valid_position A boolean representing whether the current position is valid for placing a ship.
/// \param orientation A pointer to an integer representing the current orientation of the selected ship.
/// \param button_data A pointer to a ButtonData structure containing data for all buttons in the placement phase.
/// \return void
void handle_placement_phase_event(SDL_Event *event, bool *running, int *ship_selected, bool *placed_ships, Ship *ships, Player *current_player, int grid_mouse_x, int grid_mouse_y, bool valid_position, int *orientation, ButtonData *button_data);

/// \brief Displays and handles the ship placement phase screen for a battleship game.
///
/// This function sets up the ship placement screen, rendering the game board, ships, and buttons, as well as handling
/// user interaction during the placement phase. It utilizes other functions to handle SDL events and render various
/// elements on the screen.
///
/// \param renderer A pointer to an SDL_Renderer structure that represents the rendering context.
/// \param textures A pointer to a GameTextures structure that holds game-related textures.
/// \param current_player A pointer to a Player structure representing the current player.
///
/// \return void
void placement_phase_screen(SDL_Renderer *renderer, GameTextures *textures, Player *current_player);

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
                                  800, 600, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            return -1;
        }//end if
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
            printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
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
        placement_phase_screen(renderer, textures, &player1);

        SDL_DestroyRenderer(renderer); // Destroy the renderer for player1
        SDL_DestroyWindow(window);     // Destroy the window for player1

        // Create a new window and renderer for player2
        window = SDL_CreateWindow("Battleship - Player 2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  800, 600, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
            return -1;
        }//end if

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
            printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
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

// Function definitions

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

void render_colored_text(SDL_Renderer *renderer, const char *text, int x, int y, int r, int g, int b) {
    // Set the text color
    SDL_Color color = {r, g, b, 255};

    // Create the surface and texture
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    // Set the text position
    SDL_Rect text_rect = {x, y, surface->w, surface->h};

    // Render the texture
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);

    // Free resources
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}//end render_colored_text

int is_mouse_inside_button(int x, int y, SDL_Rect button_rect) {
    return x >= button_rect.x && x <= button_rect.x + button_rect.w &&
           y >= button_rect.y && y <= button_rect.y + button_rect.h;
}//end is_mouse_inside_button

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

bool all_ships_placed(const bool placed_ships[]) {
    // Iterate through all ships
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (!placed_ships[i]) {
            return false;
        }//end if
    }//end for
    return true;
}//end all_ships_placed

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

void place_random_ships(Player *current_player, Ship ships[], bool placed_ships[], int *ship_selected, int *orientation_to_reset) {
    // Reset the board and ships
    reset_placement_phase(ships, placed_ships, ship_selected, orientation_to_reset, &current_player->board);

    // Loop until all ships are placed
    for (int i = 0; i < NUM_SHIPS; i++) {

        int x, y, orientation;
        bool valid;
        do {
            // Generate random coordinates and orientation
            x = (int)pcg32_boundedrand(BOARD_SIZE);
            y = (int)pcg32_boundedrand(BOARD_SIZE);
            orientation = (int)pcg32_boundedrand(2);

            // Check if the position is valid
            valid = is_position_valid(current_player, ships[i].size, x, y, orientation);
        } while (!valid);

        // Place the ship and mark it as placed
        place_ship(&current_player->board, &ships[i], x, y, orientation);
        placed_ships[i] = true;
    }//end for
}//end place_random_ships

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
        render_text(renderer, "Orientation: Horizontal", orientation_button.x + 24, orientation_button.y + 10);
    } else {
        render_text(renderer, "Orientation: Vertical", orientation_button.x + 24, orientation_button.y + 10);
    }//end else
}//end render_orientation_text

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

void render_placement_reset_button(SDL_Renderer *renderer, SDL_Rect reset_button, bool hover_reset) {
    // Set button color based on mouse hover
    set_button_color(renderer, hover_reset);

    // Render button background
    SDL_RenderFillRect(renderer, &reset_button);

    // Render button shadow
    render_button_shadow(renderer, reset_button);

    // Render reset text
    render_text(renderer, "Restart the board", reset_button.x + 24, reset_button.y + 10);
}//end render_placement_reset_button

void render_placement_random_button(SDL_Renderer *renderer, SDL_Rect random_button, bool hover_random) {
    // Set button color based on mouse hover
    set_button_color(renderer, hover_random);

    // Render button background
    SDL_RenderFillRect(renderer, &random_button);

    // Render button shadow
    render_button_shadow(renderer, random_button);

    // Render random text
    render_text(renderer, "Randomize the board", random_button.x + 24, random_button.y + 10);
}//end render_placement_random_button

void render_placement_finish_button(SDL_Renderer *renderer, SDL_Rect finish_button, bool hover_finish, const bool placed_ships[], SDL_Texture *black_texture) {
    // Set button color based on mouse hover
    set_button_color(renderer, hover_finish);

    // Render button background
    SDL_RenderFillRect(renderer, &finish_button);

    // Render button shadow
    render_button_shadow(renderer, finish_button);

    // Check if all ships have been placed
    if (all_ships_placed(placed_ships)) {
        // Render finish text
        render_text(renderer, "Finish placing ships", finish_button.x + 24, finish_button.y + 10);
    } else {
        // Render black texture
        SDL_RenderCopy(renderer, black_texture, NULL, &finish_button);

        // Render finish text with a different color
        render_colored_text(renderer, "Finish placing ships", finish_button.x + 24, finish_button.y + 10, 255, 255, 255);
    }//end else
}//end render_placement_finish_button

void render_placement_buttons(SDL_Renderer *renderer ,ButtonData *button_data, const bool placed_ships[], int orientation, SDL_Texture *black_texture) {
    // Render the orientation button
    render_placement_orientation_button(renderer, button_data->orientation_button, button_data->hover_orientation, orientation);

    // Render reset button
    render_placement_reset_button(renderer, button_data->reset_button, button_data->hover_reset);

    // Render random button
    render_placement_random_button(renderer, button_data->random_button, button_data->hover_random);

    // Render finish button
    render_placement_finish_button(renderer, button_data->finish_button, button_data->hover_finish, placed_ships, black_texture);
}//end render_placement_buttons

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

void render_placement_grid_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[], int ship_selected, int orientation, int grid_mouse_x, int grid_mouse_y, bool valid_position) {
    // Render grid background
    render_grid_background(renderer, textures, ship_selected);

    // Render ship hover
    render_ship_hover(renderer, textures, ships, ship_selected, orientation, grid_mouse_x, grid_mouse_y, valid_position);

    // Render placed ships
    render_placed_ships(renderer, textures, ships, placed_ships);
}//end render_placement_grid_ships

void reset_game_board(GameBoard *board) {
    // Reset all cells to be unoccupied
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board->cells[i][j].occupied = false;
        }//end for
    }//end for
}//end reset_game_board

void reset_placement_phase(Ship *ships, bool *placed_ships, int *ship_selected, int *orientation, GameBoard *board) {
    // Reset ship_selected and orientation
    *ship_selected = -1;
    *orientation = 0;

    // Reset ships and placed_ships arrays
    for (int i = 0; i < NUM_SHIPS; i++) {
        placed_ships[i] = false;
        ships[i].hit_count = 0;
        ships[i].x = 0;
        ships[i].y = 0;
    }//end for

    // Reset the game board
    reset_game_board(board);
}//end reset_placement_phase

void handle_placement_phase_event(SDL_Event *event, bool *running, int *ship_selected, bool *placed_ships, Ship *ships, Player *current_player, int grid_mouse_x, int grid_mouse_y, bool valid_position, int *orientation, ButtonData *button_data) {    // Handle SDL_QUIT event (e.g., user closes the window)
    // Handle SDL_QUIT event (e.g., user closes the window)
    if (event->type == SDL_QUIT) {
        *running = 0;
    }//end if

    // Handle mouse button press event
    else if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;

        // Check if the user clicked on the exit button
        if (is_mouse_inside_button(x, y, button_data->exit_button)) {
            *running = 0;
        }//end if

        // Check if the user clicked on the orientation button
        if (is_mouse_inside_button(x, y, button_data->orientation_button)) {
            *orientation = (*orientation + 1) % 2;
        } else {
            // Check if the user clicked on a ship
            for (int i = 0; i < NUM_SHIPS; i++) {
                if (placed_ships[i]) {
                    continue;
                }//end if

                int ship_size = ships[i].size;

                // Loop through each segment of the ship
                for (int j = 0; j < ship_size; j++) {
                    SDL_Rect ship_rect = {50 + j * CELL_SIZE, 50 + i * 50, CELL_SIZE, CELL_SIZE};
                    if (is_mouse_inside_button(x, y, ship_rect)) {
                        *ship_selected = i;
                        break;
                    }//end if
                }//end for
            }//end for
        }//end else

        // Check if the user clicked on the reset button
        if (is_mouse_inside_button(x, y, button_data->reset_button)) {
            reset_placement_phase(ships, placed_ships, ship_selected, orientation, &current_player->board);
        }//end if

        // Check if the user clicked on the "Random Board" button
        if (is_mouse_inside_button(x, y, button_data->random_button)) {
            place_random_ships(current_player, ships, placed_ships, ship_selected, orientation);
        }//end if

        // Check if the user clicked on the "Finish placement phase" button
        if (is_mouse_inside_button(x, y, button_data->finish_button) && all_ships_placed(placed_ships)) {
            *running = 0;
        }//end if
    }//end else if

    // Handle mouse motion event
    else if (event->type == SDL_MOUSEMOTION) {
        int x = event->motion.x;
        int y = event->motion.y;

        // Check if the user is hovering over the orientation button
        if (is_mouse_inside_button(x, y, button_data->orientation_button)) {
            button_data->hover_orientation = 1;
        } else {
            button_data->hover_orientation = 0;
        }//end else

        // Check if the user is hovering over the reset button
        if (is_mouse_inside_button(x, y, button_data->reset_button)) {
            button_data->hover_reset = 1;
        } else {
            button_data->hover_reset = 0;
        }//end else

        if (is_mouse_inside_button(x, y, button_data->random_button)) {
            button_data->hover_random = 1;
        } else {
            button_data->hover_random = 0;
        }//end else

        // Check if the user is hovering over the "Finish placement phase" button
        if (is_mouse_inside_button(x, y, button_data->finish_button) && all_ships_placed(placed_ships)) {
            button_data->hover_finish = 1;
        } else {
            button_data->hover_finish = 0;
        }//end else
    }//end else if

    // Check if the user clicked on the grid
    if (grid_mouse_x >= 0 && grid_mouse_x < BOARD_SIZE && grid_mouse_y >= 0 && grid_mouse_y < BOARD_SIZE) {
        if (valid_position && event->type == SDL_MOUSEBUTTONDOWN && *ship_selected >= 0 && !placed_ships[*ship_selected]) {
            place_ship(&current_player->board, &ships[*ship_selected], grid_mouse_x, grid_mouse_y, *orientation);
            placed_ships[*ship_selected] = true; // Set the ship as placed

            // Update player's ships
            current_player->ships[*ship_selected] = ships[*ship_selected];
            current_player->ships_remaining++;

            *ship_selected = -1;
        }//end if
    }//end if
}//end handle_placement_phase_event

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
    int hover_reset = 0;
    int hover_random = 0;
    int hover_finish = 0;

    initialize_game_board(&current_player->board);

    // Initialize buttons
    SDL_Rect orientation_button = {50, 300, 275, 50}; // x, y, width, height
    SDL_Rect exit_button = {0, 0, 100, 50};
    SDL_Rect reset_button = {50, 400, 275, 50};
    SDL_Rect random_button = {50, 500, 275, 50};
    SDL_Rect finish_button = {425, 400, 275, 50};

    // Initialize the struct
    ButtonData button_data = {
            exit_button,
            orientation_button,
            reset_button,
            random_button,
            finish_button,
            hover_orientation,
            hover_reset,
            hover_random,
            hover_finish
    };

    // Initialize ships
    Ship ships[NUM_SHIPS] = {
            {CARRIER,     5, 0, 0, 0},
            {BATTLESHIP,  4, 0, 0, 0},
            {DESTROYER,   3, 0, 0, 0},
            {SUBMARINE,   3, 0, 0, 0},
            {PATROL_BOAT, 2, 0, 0, 0}
    };
    bool placed_ships[NUM_SHIPS] = {false};

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

        // Handle events
        while (SDL_PollEvent(&event)) {
            handle_placement_phase_event(&event, &running, &ship_selected, placed_ships, ships, current_player, grid_mouse_x, grid_mouse_y, valid_position, &orientation, &button_data);
        }//end while

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render screen background
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);

        // Render the ships on the left side of the screen
        render_placement_ships_left_side(renderer, textures, ships, placed_ships, black_texture, ship_selected);

        // Render the grid and the ships on the grid (if any).
        render_placement_grid_ships(renderer, textures, ships, placed_ships, ship_selected, orientation, grid_mouse_x, grid_mouse_y, valid_position);

        // Render exit option
        render_text(renderer, "Exit", exit_button.x + 25, exit_button.y + 10);

        // Render the buttons
        render_placement_buttons(renderer, &button_data, placed_ships, orientation, black_texture);

        // Render the screen
        SDL_RenderPresent(renderer);
    }//end while

    // Destroy textures
    SDL_DestroyTexture(background_texture);
    SDL_FreeSurface(black_surface);
    SDL_DestroyTexture(black_texture);
}//end placement_phase_screen
