// SDL must be told to redefine the main function in order to work with some platforms
#define SDL_MAIN_HANDLED

// Include necessary libraries
#include <SDL.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include "pcg_basic.h"
#include <SDL_thread.h>

// Define constants for the game
#define NUM_SHIPS 5
#define CELL_SIZE 32
#define BOARD_SIZE 10

// Structure for representing a cell on the game board
typedef struct {
    int x;
    int y;
    int width;
    int height;
    int ship_index;
    bool occupied;
    bool hit;
} Cell;

// Structure for representing a game board
typedef struct {
    Cell cells[BOARD_SIZE][BOARD_SIZE];
} GameBoard;

// Structure for representing a ship
typedef struct {
    int size;
    int hit_count;
    int x;
    int y;
    int orientation;
} Ship;

// Structure for representing a player
typedef struct {
    int remaining_ships;
    bool is_turn;
    bool has_shot;
    bool is_human;
    bool can_shoot;
    bool placed_ships[NUM_SHIPS];
    GameBoard board;
    Ship ships[NUM_SHIPS];
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

// Enum for representing main menu options
typedef enum {
    MAIN_MENU_NEW_GAME_PVP,
    MAIN_MENU_NEW_GAME_PVC,
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

// Enum for representing the current state of the AI
typedef enum {
    SEARCH,
    TARGET,
    DESTROY
} AI_State;

// Global variables for font and font size
TTF_Font *font = NULL;
int font_size = 24;

// Function prototypes

/// \brief Save the current game state to a file.
///
/// This function saves the current game state, including the two players and the current turn,
/// to a binary file named "saved_game.dat". It returns true if the save operation is successful,
/// and false otherwise.
///
/// \param player1 Pointer to the first player's data.
/// \param player2 Pointer to the second player's data.
/// \param current_turn Integer representing the current turn (1 or 2).
/// \param ai_state Pointer to an AI_State representing the current state of the AI.
/// \return true if the game state is saved successfully, false otherwise.
bool save_game(Player *player1, Player *player2, int current_turn, AI_State *ai_state);

/// \brief Load the game state from a file.
///
/// This function loads the game state from a binary file named "saved_game.dat", including the two players
/// and the current turn. It returns true if the load operation is successful, and false otherwise.
///
/// \param player1 Pointer to the first player's data.
/// \param player2 Pointer to the second player's data.
/// \param current_turn Pointer to an integer that will store the loaded current turn value (1 or 2).
/// \param ai_state Pointer to an AI_State that will store the loaded AI state.
/// \return true if the game state is loaded successfully, false otherwise.
bool load_game(Player *player1, Player *player2, int *current_turn, AI_State *ai_state);

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

/// \brief Initializes ships for a player.
///
/// This function initializes the ships for a player by setting the size and initial values
/// for each ship. The hit_count is set to 0, and the position and orientation are set to
/// invalid values (-1 and 0, respectively) as the ships have not been placed on the board yet.
///
/// \param player Pointer to the Player struct representing the player whose ships are being initialized.
void initialize_ships(Player *player);

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

/// \brief Checks if all ships have been placed.
///
/// This function checks if all the ships have been placed on the board by iterating
/// through the placed_ships array. If all the ships have been placed, the function
/// returns true; otherwise, it returns false.
///
/// \param placed_ships Array containing the placement status of each ship.
/// \return true if all ships have been placed, false otherwise.
bool all_ships_placed(const bool placed_ships[]);

/// \brief Places a ship on the game board.
///
/// This function places a ship on the game board at the specified position and orientation.
/// The cells that the ship occupies are marked as occupied and updated with the necessary
/// information, such as coordinates and hit status.
///
/// \param board Pointer to the GameBoard struct representing the game board.
/// \param ship Pointer to the Ship struct representing the ship to place.
/// \param x Integer representing the x-coordinate of the ship's position on the board.
/// \param y Integer representing the y-coordinate of the ship's position on the board.
/// \param orientation Integer representing the orientation of the ship (0 for horizontal, 1 for vertical).
/// \param ship_index Integer representing the index of the ship in the ships array.
void place_ship(GameBoard *board, Ship *ship, int x, int y, int orientation, int ship_index);

/// \brief Places ships randomly on a player's board.
///
/// This function generates random positions and orientations for each ship in the game,
/// and places them on the player's board. It ensures that the randomly generated positions
/// and orientations are valid before placing the ships.
///
/// \param current_player Pointer to the Player struct representing the player whose board the ships are being placed on.
/// \param ships Pointer to the array of Ship structs representing the ships to place.
/// \param placed_ships Pointer to the array of booleans indicating whether each ship has been placed.
/// \param ship_selected Pointer to an integer representing the index of the currently selected ship (-1 if none).
/// \param orientation_to_reset Pointer to an integer representing the orientation to reset after placing ships.
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
/// \param hover_state A bool indicating whether the mouse is hovering over the button.
/// \return void
void set_button_color(SDL_Renderer *renderer, bool hover_state);

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
/// \param board_x The x coordinate of the game grid.
/// \param board_y The y coordinate of the game grid.
/// \return void
void render_placed_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[], int board_x, int board_y);

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

/// \brief Render the invalid position border on the game board.
///
/// This function draws a red border around the game board when an invalid position is clicked.
///
/// \param renderer The SDL_Renderer used for rendering.
/// \return void
void render_invalid_position_border(SDL_Renderer *renderer);

/// \brief Removes a ship from the game board.
///
/// This function removes a ship from the game board by setting the 'occupied' status
/// of the cells it occupied to false. The ship's position and orientation are used
/// to determine which cells to update.
///
/// \param board Pointer to the GameBoard struct representing the game board.
/// \param ship Pointer to the Ship struct representing the ship to remove.
/// \param x Integer representing the x-coordinate of the ship's position on the board.
/// \param y Integer representing the y-coordinate of the ship's position on the board.
/// \param orientation Integer representing the orientation of the ship (0 for horizontal, 1 for vertical).
void remove_ship_from_board(GameBoard *board, Ship *ship, int x, int y, int orientation);

/// \brief Finds the index of the ship located at a specific position on the player's board.
///
/// This function iterates through the player's ships to find the one occupying the given
/// coordinates on the board. If a ship is found at the specified position, its index is
/// returned; otherwise, the function returns -1.
///
/// \param player Pointer to the Player struct representing the player whose ships are being searched.
/// \param x Integer representing the x-coordinate of the position to search.
/// \param y Integer representing the y-coordinate of the position to search.
/// \return The index of the ship found at the given position, or -1 if no ship is found.
int find_ship_at_position(Player *player, int x, int y);

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

/// \brief Handles mouse button down events during the ship placement phase.
///
/// This function processes mouse button down events during the ship placement phase of the game.
/// It is responsible for handling interactions with buttons, selecting and placing ships on the grid,
/// and updating the game state accordingly.
///
/// \param event The SDL_Event representing the mouse button down event.
/// \param running Pointer to a boolean indicating whether the placement phase is still running.
/// \param current_player Pointer to the Player struct representing the current player.
/// \param ship_selected Pointer to an integer representing the currently selected ship.
/// \param placed_ships Pointer to a boolean array indicating which ships have been placed.
/// \param orientation Pointer to an integer representing the orientation of the selected ship.
/// \param button_data Pointer to a ButtonData struct containing information about the game buttons.
/// \param invalid_click Pointer to a boolean indicating whether the last click was invalid (e.g., on an invalid grid position).
/// \param grid_mouse_x Integer representing the grid x-coordinate of the mouse cursor.
/// \param grid_mouse_y Integer representing the grid y-coordinate of the mouse cursor.
/// \param valid_position Pointer to a boolean indicating whether the current position is valid for placing the selected ship.
/// \param ships Pointer to an array of Ship structs representing the available ships.
void handle_placement_mouse_button_down(SDL_Event *event, bool *running, Player *current_player, int *ship_selected, bool *placed_ships, int *orientation, ButtonData *button_data, bool *invalid_click, int grid_mouse_x, int grid_mouse_y, const bool *valid_position, Ship *ships);

/// \brief Handles mouse motion events during the ship placement phase.
///
/// This function processes mouse motion events during the ship placement phase of the game.
/// It is responsible for updating the hover state of buttons as the mouse cursor moves over them.
///
/// \param event The SDL_Event representing the mouse motion event.
/// \param button_data Pointer to a ButtonData struct containing information about the game buttons.
/// \param placed_ships Pointer to a boolean array indicating which ships have been placed.
void handle_placement_mouse_motion(SDL_Event *event, ButtonData *button_data, bool *placed_ships);


/// \brief Handle events during the placement phase of the game.
///
/// This function handles various events like mouse clicks, mouse movements, and SDL_QUIT events
/// during the placement phase of the game. It also checks whether a ship has been selected,
/// whether the user clicks on a valid position, and updates the game state accordingly.
///
/// \param event The SDL_Event to be processed.
/// \param running A pointer to a boolean flag indicating whether the placement phase is still running or not.
/// \param ship_selected A pointer to an integer representing the index of the selected ship.
/// \param placed_ships A boolean array indicating whether each ship has been placed on the board or not.
/// \param ships An array of Ship objects representing the available ships.
/// \param current_player A pointer to the current Player object.
/// \param grid_mouse_x The x-coordinate of the mouse position on the grid.
/// \param grid_mouse_y The y-coordinate of the mouse position on the grid.
/// \param valid_position A pointer to a boolean flag indicating whether the current ship placement position is valid.
/// \param orientation A pointer to an integer representing the ship's orientation (0 for horizontal, 1 for vertical).
/// \param invalid_click A pointer to a boolean flag indicating whether an invalid position has been clicked or not.
/// \param button_data A pointer to a ButtonData structure containing information about the buttons.
/// \return void
void handle_placement_phase_event(SDL_Event *event, bool *running, int *ship_selected, bool *placed_ships, Ship *ships, Player *current_player, int grid_mouse_x, int grid_mouse_y, const bool *valid_position, int *orientation, bool *invalid_click, ButtonData *button_data);

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

/// \brief Places ships for the computer player.
///
/// Randomly places ships for the computer player and initializes the corresponding values.
/// This function is designed for a human player versus computer game mode, where the computer's
/// ship placement should be hidden from the human player.
///
/// \param computer A pointer to the computer player's Player struct.
/// \return void
void placement_phase_computer(Player *computer);

/// \brief Render the player's board.
///
/// This function renders the player's board with the placed ships and hit/miss cells.
/// The board is drawn on the provided renderer using the provided textures at the specified
/// board_x and board_y coordinates.
///
/// \param renderer The SDL_Renderer to draw on.
/// \param textures A pointer to the GameTextures structure containing necessary textures.
/// \param player A pointer to the Player structure containing the board and ships data.
/// \param board_x The x-coordinate for the top-left corner of the player's board.
/// \param board_y The y-coordinate for the top-left corner of the player's board.
/// \return void
void render_player_board(SDL_Renderer *renderer, GameTextures *textures, Player *player, int board_x, int board_y);

/// \brief Render the opponent's board.
///
/// This function renders the opponent's board with the hit/miss cells.
/// The board is drawn on the provided renderer using the provided textures at the specified
/// board_x and board_y coordinates.
///
/// \param renderer The SDL_Renderer to draw on.
/// \param textures A pointer to the GameTextures structure containing necessary textures.
/// \param opponent A pointer to the Player structure containing the opponent's board data.
/// \param board_x The x-coordinate for the top-left corner of the opponent's board.
/// \param board_y The y-coordinate for the top-left corner of the opponent's board.
/// \return void
void render_opponent_board(SDL_Renderer *renderer, GameTextures *textures, Player *opponent, int board_x, int board_y);

/// \brief Render both player's and opponent's boards.
///
/// This function renders both the current player's and the opponent's boards side by side.
/// It uses the provided renderer and textures to draw the boards.
///
/// \param renderer The SDL_Renderer to draw on.
/// \param textures A pointer to the GameTextures structure containing necessary textures.
/// \param current_player A pointer to the Player structure containing the current player's board and ships data.
/// \param opponent A pointer to the Player structure containing the opponent's board data.
/// \return void
void render_game_boards(SDL_Renderer *renderer, GameTextures *textures, Player *current_player, Player *opponent);

/// \brief Render the hover effect on the game board.
///
/// This function renders the hover effect for a specified cell and its corresponding x and y-axis cells on the game board.
///
/// \param renderer The SDL_Renderer to draw on.
/// \param white_texture The SDL_Texture to be used for the hover effect.
/// \param cell_x The x-coordinate of the specified cell.
/// \param cell_y The y-coordinate of the specified cell.
/// \param board_x The x-coordinate of the game board.
/// \param board_y The y-coordinate of the game board.
/// \return void
void render_game_hover_effect(SDL_Renderer *renderer, SDL_Texture *white_texture, int cell_x, int cell_y, int board_x, int board_y);

/// \brief Render the "Finish turn" button.
///
/// This function renders the "Finish turn" button with a hover effect and text.
///
/// \param renderer The SDL_Renderer to draw on.
/// \param finish_turn_button An SDL_Rect containing the position and dimensions of the "Finish turn" button.
/// \param hover_finish_turn A boolean representing whether the mouse is hovering over the "Finish turn" button.
/// \return void
void render_finish_turn_button(SDL_Renderer *renderer, SDL_Rect finish_turn_button, bool hover_finish_turn);

/// \brief Render the remaining ships count for both players.
///
/// This function renders the text displaying the remaining ships count for both the current player and the opponent.
///
/// \param renderer The SDL_Renderer to draw on.
/// \param current_player A pointer to the Player structure containing the current player's remaining ships data.
/// \param opponent A pointer to the Player structure containing the opponent's remaining ships data.
/// \return void
void render_remaining_ships_text(SDL_Renderer *renderer, Player *current_player, Player *opponent);

/// \brief Update the hit count of a ship and check if it is sunk.
///
/// This function updates the hit count of a ship and checks if the ship is sunk.
/// If the ship is sunk, it decrements the player's remaining ships count.
///
/// \param player A pointer to the Player structure containing the player's ships data.
/// \param ship_index The index of the ship in the player's ships array.
/// \return true if the ship is sunk, false otherwise.
bool update_hit_count(Player *player, int ship_index);

/// \brief Update the window title with the current player number.
///
/// This function updates the window title to display the current player number.
///
/// \param window The SDL_Window whose title needs to be updated.
/// \param current_player_num The current player number.
/// \return void
void update_window_title(SDL_Window *window, int current_player_num);

/// \brief Display the winner message on the screen.
///
/// This function displays the winner message, indicating which player has won the game.
///
/// \param renderer The SDL_Renderer to draw on.
/// \param winner_player_num The player number of the winner.
/// \return void
void show_winner_message(SDL_Renderer *renderer, int winner_player_num);

/// \brief Handle mouse button down events during the game.
///
/// This function handles mouse button down events while the game is being played, such as shooting at the opponent's board.
///
/// \param renderer The SDL_Renderer to draw on.
/// \param textures A pointer to the GameTextures structure containing necessary textures.
/// \param running A pointer to a boolean representing whether the game is running.
/// \param current_player A pointer to the Player structure containing the current player's data.
/// \param opponent A pointer to the Player structure containing the opponent's data.
/// \return void
void handle_game_mouse_button_down(SDL_Renderer *renderer, GameTextures *textures, bool *running, Player *current_player, Player *opponent);

/// \brief Handle mouse button up events during the game.
///
/// This function handles mouse button up events while the game is being played, such as interacting with the "Finish turn" button.
///
/// \param current_player A pointer to the Player structure containing the current player's data.
/// \param opponent A pointer to the Player structure containing the opponent's data.
/// \param finish_turn_button An SDL_Rect containing the position and dimensions of the "Finish turn" button.
/// \param window The SDL_Window whose title needs to be updated.
/// \param hover_save A pointer to a boolean representing whether the mouse is hovering over the "Save" button.
/// \param hover_exit A pointer to a boolean representing whether the mouse is hovering over the "Exit" button.
/// \param running A pointer to a boolean representing whether the game is running.
/// \param ai_state A pointer to the AI_State structure containing the AI's state data.
/// \return void
void handle_game_mouse_button_up(Player *current_player, Player *opponent, SDL_Rect finish_turn_button, SDL_Window *window, const bool *hover_save, const bool *hover_exit, bool *running, AI_State *ai_state);

/// \brief Handle game screen events.
///
/// This function handles game screen events, such as mouse button clicks, mouse movement, and quitting the game.
///
/// \param event A pointer to the SDL_Event structure to be processed.
/// \param renderer The SDL_Renderer to draw on.
/// \param textures A pointer to the GameTextures structure containing necessary textures.
/// \param current_player A pointer to the Player structure containing the current player's data.
/// \param opponent A pointer to the Player structure containing the opponent's data.
/// \param running A pointer to a boolean that indicates whether the game is still running.
/// \param finish_turn_button An SDL_Rect containing the position and dimensions of the "Finish turn" button.
/// \param hover_save A pointer to a boolean indicating whether the mouse is hovering over the "Save game" button.
/// \param hover_exit A pointer to a boolean indicating whether the mouse is hovering over the "Exit game" button.
/// \param window The SDL_Window whose title needs to be updated.
/// \param ai_state A pointer to the AI_State structure containing the AI's state data.
/// \return void
void handle_game_screen_events(SDL_Event *event, SDL_Renderer *renderer, GameTextures *textures, Player *current_player, Player *opponent, bool *running, SDL_Rect finish_turn_button, const bool *hover_save, const bool *hover_exit, SDL_Window *window, AI_State *ai_state);

/// \brief Shuffles the direction indices array.
///
/// The function shuffles the direction indices array in place using the Fisher-Yates algorithm.
/// This helps to ensure that the AI selects directions randomly without repeating the same direction.
///
/// \param dir_indices A pointer to an array of integers representing the direction indices.
/// \return void
void shuffle_directions(int *dir_indices);

/// \brief Executes the computer's turn in a Battleship game using a state-based AI strategy.
///
/// Handles the computer's turn in the game using AI, which follows a state-based strategy (SEARCH, TARGET, DESTROY).
///
/// \param renderer A pointer to the SDL_Renderer used for rendering the game.
/// \param textures A pointer to the GameTextures structure containing all the game textures.
/// \param computer A pointer to the Player structure representing the computer player.
/// \param opponent A pointer to the Player structure representing the human player.
/// \param ai_state A pointer to the AI_State enumeration, which represents the current state of the AI (SEARCH, TARGET, DESTROY).
/// \return void
void handle_computer_turn(SDL_Renderer *renderer, GameTextures *textures, Player *computer, Player *opponent, AI_State *ai_state);

/// \brief The game screen loop.
///
/// This function contains the main game loop for the Battleship game. It renders the game boards, handles input events, and updates the game state.
///
/// \param renderer The SDL_Renderer used for drawing.
/// \param window The SDL_Window whose title needs to be updated.
/// \param textures A pointer to the GameTextures structure containing the game's textures.
/// \param player1 A pointer to the Player structure containing player 1's data.
/// \param player2 A pointer to the Player structure containing player 2's data.
/// \param current_turn A pointer to an integer that indicates the current player's turn.
/// \param ai_state A pointer to the AI_State structure containing the AI's state data.
/// \return void
void game_screen(SDL_Renderer *renderer, SDL_Window *window, GameTextures *textures, Player *player1, Player *player2, int *current_turn, AI_State *ai_state);

/// \brief Frees resources and performs cleanup before exiting the game.
///
/// This function is responsible for freeing memory allocated for textures,
/// destroying the SDL renderer and window, and quitting SDL subsystems.
///
/// \param textures Pointer to the GameTextures structure to be freed.
/// \param renderer Pointer to the SDL_Renderer to be destroyed.
/// \param window Pointer to the SDL_Window to be destroyed.
/// \param font Pointer to the TTF_Font to be closed.
/// \return void
void cleanup(GameTextures *textures, SDL_Renderer *renderer, SDL_Window *window);

int main() {
    // Initialize SDL and SDL_image
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
                                          320, 320, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return -1;
    }//end if

    // Create an SDL renderer
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

    // Open the font
    font = TTF_OpenFont("Assets/Fonts/cambria.ttc", font_size);
    if (font == NULL) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        exit(2);
    }//end if

    // Initialize players
    Player player1;
    Player player2;
    int current_turn = 1;
    player1.is_turn = true;
    player2.is_turn = false;
    player1.has_shot = false;
    player2.has_shot = false;
    player1.can_shoot = true;
    player2.can_shoot = true;
    player1.remaining_ships = NUM_SHIPS;
    player2.remaining_ships = NUM_SHIPS;

    // Create main menu
    MainMenuOption menu_option = main_menu(renderer);
    if (menu_option == MAIN_MENU_EXIT) {
        // Exit the game
        cleanup(textures, renderer, window);
        return 0;
    } else if (menu_option == MAIN_MENU_LOAD) {
        AI_State ai_state;
        bool load_success = load_game(&player1, &player2, &current_turn, &ai_state);
        if (!load_success) {
            printf("Error loading saved game.\n");
            return -1;
        }//end if
        player1.is_turn = (current_turn == 1);
        player2.is_turn = (current_turn == 2);

        // Re-create the window and renderer for the loaded game
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        const char *window_title = (current_turn == 1) ? "Battleship - Player 1" : "Battleship - Player 2";
        window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
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
        textures = load_game_textures(renderer);
        if (textures == NULL) {
            printf("Failed to load game textures.\n");
            return -1;
        }//end if

        if (!player2.is_human) {
            ai_state = SEARCH;
        }//end if

        game_screen(renderer, window, textures, &player1, &player2, &current_turn, &ai_state);
    } else if (menu_option == MAIN_MENU_NEW_GAME_PVP) {
        SDL_DestroyRenderer(renderer); // Destroy the renderer for the main menu
        SDL_DestroyWindow(window);     // Destroy the window for the main menu

        // Create a new window and renderer for the game
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

        player1.is_human = true;
        player2.is_human = true;

        placement_phase_screen(renderer, textures, &player1);

        if (player1.remaining_ships != 5) {
            // Player 1 did not place all ships
            printf("Player 1 did not place all ships.\n");
            cleanup(textures, renderer, window);
            return -1;
        }//end if

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

        if (player2.remaining_ships != 5) {
            // Player 1 did not place all ships
            printf("Player 1 did not place all ships.\n");
            cleanup(textures, renderer, window);
            return -1;
        }//end if

        SDL_DestroyRenderer(renderer); // Destroy the renderer for player2
        SDL_DestroyWindow(window);     // Destroy the window for player2

        // Create a new window
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

        // Seed the random number generator
        pcg32_srandom(time(NULL), (intptr_t) &main);

        game_screen(renderer, window, textures, &player1, &player2, &current_turn, NULL);
    } else if (menu_option == MAIN_MENU_NEW_GAME_PVC) {
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

        player1.is_human = true;
        player2.is_human = false;

        placement_phase_screen(renderer, textures, &player1);

        if (player1.remaining_ships != 5) {
            // Player 1 did not place all ships
            printf("Player 1 did not place all ships.\n");
            cleanup(textures, renderer, window);
            return -1;
        }//end if

        SDL_DestroyRenderer(renderer); // Destroy the renderer for player1
        SDL_DestroyWindow(window);     // Destroy the window for player1

        placement_phase_computer(&player2);

        // Create a new window
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

        AI_State ai_state = SEARCH;

        game_screen(renderer, window, textures, &player1, &player2, &current_turn, &ai_state);
    }//end else if

    // Cleanup and exit
    cleanup(textures, renderer, window);
    return 0;
}//end main

// Function definitions

bool save_game(Player *player1, Player *player2, int current_turn, AI_State *ai_state) {
    // Save the game state to a file ("saved_game.dat")
    FILE *save_file = fopen("saved_game.dat", "wb");
    if (save_file == NULL) {
        return false;
    }//end if

    // Write player1, player2, current_turn and ai_state to the save file
    fwrite(player1, sizeof(Player), 1, save_file);
    fwrite(player2, sizeof(Player), 1, save_file);
    fwrite(&current_turn, sizeof(int), 1, save_file);
    fwrite(ai_state, sizeof(AI_State), 1, save_file);

    // Close the save file
    fclose(save_file);
    return true;
}//end save_game

bool load_game(Player *player1, Player *player2, int *current_turn, AI_State *ai_state) {
    // Load the game state from a file ("saved_game.dat")
    FILE *save_file = fopen("saved_game.dat", "rb");
    if (save_file == NULL) {
        return false;
    }//end if

    // Read player1, player2, current_turn, and ai_state from the save file
    fread(player1, sizeof(Player), 1, save_file);
    fread(player2, sizeof(Player), 1, save_file);
    fread(current_turn, sizeof(int), 1, save_file);
    fread(ai_state, sizeof(AI_State), 1, save_file);

    // Close the save file
    fclose(save_file);
    return true;
}//end load_game

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
    // Check if the mouse is inside the button
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
    for (int i = 0; i < 4; i++) {
        button_rects[i].x = (BOARD_SIZE * CELL_SIZE) / 2 - 105;
        button_rects[i].y = 85 + i * 60;
        button_rects[i].w = 210;
        button_rects[i].h = 40;
    }//end for
}//end init_main_menu

int handle_main_menu_events(SDL_Event *event, SDL_Rect *button_rects, int *hover_button, MainMenuOption *selected_option) {
    int running = 1;

    // Handle SDL_QUIT event
    if (event->type == SDL_QUIT) {
        running = 0;
    }//end if

    // Handle mouse button press
    else if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            int x = event->button.x;
            int y = event->button.y;

            // Check if any buttons were clicked
            for (int i = 0; i < 4; i++) {
                if (is_mouse_inside_button(x, y, button_rects[i])) {
                    *selected_option = (MainMenuOption)i;
                    return 0;
                }//end if
            }//end for
        }//end if
    }//end if

    // Handle mouse movement
    else if (event->type == SDL_MOUSEMOTION) {
        int x = event->motion.x;
        int y = event->motion.y;

        // Check if the mouse is hovering over any buttons
        *hover_button = -1;
        for (int i = 0; i < 4; i++) {
            if (is_mouse_inside_button(x, y, button_rects[i])) {
                *hover_button = i;
                break;
            }//end if
        }//end for
    }//end if

    return running;
}//end handle_main_menu_events

void render_main_menu(SDL_Renderer *renderer, SDL_Texture **background_frames, int frame_counter, SDL_Rect *button_rects, int hover_button) {
    // Render the animated background
    SDL_RenderCopy(renderer, background_frames[frame_counter], NULL, NULL);

    // Button labels
    const char *button_labels[] = {"New Game - PvP", "New Game - PvC", "Load", "Exit"};

    // Render buttons
    for (int i = 0; i < 4; i++) {
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
    SDL_Rect button_rects[4];
    init_main_menu(renderer, &background_frames, button_rects, &num_background_frames);

    // Initialize variables
    int running = 1;
    int frame_counter = 0;
    int hover_button = -1;

    // Main loop
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

        // Render the screen
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
            // Initialize the cells
            Cell *cell = &board->cells[i][j];
            cell->x = i;
            cell->y = j;
            cell->width = CELL_SIZE;
            cell->height = CELL_SIZE;
            cell->ship_index = -1;
            cell->occupied = false;
            cell->hit = false;
        }//end for
    }//end for
}//end initialize_game_board

void initialize_ships(Player *player) {
    // Iterate through all ships
    for (int i = 0; i < NUM_SHIPS; ++i) {
        // Set the size of the ship
        if (i == 0) {
            player->ships[i].size = 5;
        } else if (i == 1 ) {
            player->ships[i].size = 4;
        } else if (i == 2 || i == 3) {
            player->ships[i].size = 3;
        } else if (i == 4) {
            player->ships[i].size = 2;
        }//end else if

        // Initialize the ships values
        player->ships[i].hit_count = 0;
        player->ships[i].x = -1;
        player->ships[i].y = -1;
        player->ships[i].orientation = 0;
    }//end for
}//end initialize_ships

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

void place_ship(GameBoard *board, Ship *ship, int x, int y, int orientation, int ship_index) {
    // Update the ship's position
    ship->x = x;
    ship->y = y;
    ship->orientation = orientation;

    // Iterate through all cells of the ship
    for (int i = 0; i < ship->size; i++) {
        int cell_x = x + (orientation == 0 ? i : 0);
        int cell_y = y + (orientation == 1 ? i : 0);

        Cell *cell = &board->cells[cell_x][cell_y];
        cell->occupied = true;
        cell->ship_index = ship_index; // Add this line to update the ship_index
    }//end for
}//end place_ship

void place_random_ships(Player *current_player, Ship ships[], bool placed_ships[], int *ship_selected, int *orientation_to_reset) {
    // Reset the board and ships
    reset_placement_phase(ships, placed_ships, ship_selected, orientation_to_reset, &current_player->board);

    // Reset the remaining ships
    current_player->remaining_ships = 0;

    // Seed the random number generator
    pcg32_srandom(time(NULL), (intptr_t)&main);

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
        current_player->ships[i].size = ships[i].size;
        place_ship(&current_player->board, &current_player->ships[i], x, y, orientation, i);
        placed_ships[i] = true;

        current_player->placed_ships[i] = true;
        current_player->remaining_ships++;
        current_player->ships[i].hit_count = 0;
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

void set_button_color(SDL_Renderer *renderer, bool hover_state) {
    // Change the color based on the hover state
    if (hover_state) {
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

void render_placed_ships(SDL_Renderer *renderer, GameTextures *textures, Ship ships[], const bool placed_ships[], int board_x, int board_y) {
    // Loop through all ships
    for (int i = 0; i < NUM_SHIPS; i++) {
        // Render the ship if it is placed
        if (placed_ships[i]) {
            // Loop through each segment of the ship
            for (int j = 0; j < ships[i].size; j++) {
                int cell_x = ships[i].x + (ships[i].orientation == 0 ? j : 0);
                int cell_y = ships[i].y + (ships[i].orientation == 1 ? j : 0);
                SDL_Rect ship_rect = {board_x + cell_x * CELL_SIZE, board_y + cell_y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
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
    render_placed_ships(renderer, textures, ships, placed_ships, 400, 50);
}//end render_placement_grid_ships

void render_invalid_position_border(SDL_Renderer *renderer) {
    // Render red border around the grid
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
    SDL_Rect border_rect = {400, 50, 320, 320};
    SDL_RenderDrawRect(renderer, &border_rect);
}//end render_invalid_position_border

void remove_ship_from_board(GameBoard *board, Ship *ship, int x, int y, int orientation) {
    // Loop through each segment of the ship
    for (int i = 0; i < ship->size; i++) {

        // Get the cell coordinates
        int cell_x = x + (orientation == 0 ? i : 0);
        int cell_y = y + (orientation == 1 ? i : 0);

        // Remove the ship from the cell if it is within the board
        if (cell_x >= 0 && cell_x < BOARD_SIZE && cell_y >= 0 && cell_y < BOARD_SIZE) {
            board->cells[cell_x][cell_y].occupied = false;
        }//end if
    }//end for
}//end remove_ship_from_board

int find_ship_at_position(Player *player, int x, int y) {
    // Loop through all ships
    for (int i = 0; i < NUM_SHIPS; i++) {
        Ship *ship = &player->ships[i];

        // Check if the ship is at the given position
        if (ship->x <= x && x < ship->x + (ship->orientation == 0 ? ship->size : 1) &&
            ship->y <= y && y < ship->y + (ship->orientation == 1 ? ship->size : 1)) {
            return i;
        }//end if
    }//end for
    return -1;
}//end find_ship_at_position

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

void handle_placement_mouse_button_down(SDL_Event *event, bool *running, Player *current_player, int *ship_selected, bool *placed_ships, int *orientation, ButtonData *button_data, bool *invalid_click, int grid_mouse_x, int grid_mouse_y, const bool *valid_position, Ship *ships) {
    int x = event->button.x;
    int y = event->button.y;

    // Check if the user clicked on the exit button
    if (is_mouse_inside_button(x, y, button_data->exit_button)) {
        *running = false;
    }//end if

    // Check if the user clicked on the orientation button
    if (is_mouse_inside_button(x, y, button_data->orientation_button)) {
        *orientation = (*orientation + 1) % 2;
    } else {
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

    // Check if the user clicked on any button
    bool clicked_on_button = is_mouse_inside_button(x, y, button_data->exit_button) ||
                             is_mouse_inside_button(x, y, button_data->orientation_button) ||
                             is_mouse_inside_button(x, y, button_data->reset_button) ||
                             is_mouse_inside_button(x, y, button_data->random_button);

    bool clicked_on_available_ship = false;

    // Loop through each ship
    for (int i = 0; i < NUM_SHIPS; i++) {

        // Loop through each part of the ship
        for (int j = 0; j < ships[i].size; j++) {
            SDL_GetMouseState(&x, &y);
            SDL_Rect hover_ship_rect = {50, 50 + i * 50, ships[i].size * CELL_SIZE, CELL_SIZE};
            if (is_mouse_inside_button(x, y, hover_ship_rect)) {
                clicked_on_available_ship = true;
                break;
            }//end if
        }//end for
    }//end for

    // Update invalid_click only if the user clicked on an invalid position in placement mode (i.e., not on a button or a ship)
    if (clicked_on_button || clicked_on_available_ship) {
        *invalid_click = false;
    } else if (*ship_selected >= 0) {
        *invalid_click = !(*valid_position);
    }//end else if

    // Check if the user clicked on the grid
    if (grid_mouse_x >= 0 && grid_mouse_x < BOARD_SIZE && grid_mouse_y >= 0 && grid_mouse_y < BOARD_SIZE) {

        // Check if the user clicked on a placed ship
        if (*ship_selected == -1) {
            int clicked_ship_index = find_ship_at_position(current_player, grid_mouse_x, grid_mouse_y);

            // Check if the user clicked on a placed ship
            if (clicked_ship_index >= 0 && placed_ships[clicked_ship_index]) {
                Ship *clicked_ship = &ships[clicked_ship_index];
                remove_ship_from_board(&current_player->board, clicked_ship, clicked_ship->x, clicked_ship->y, clicked_ship->orientation);
                current_player->remaining_ships--;
                placed_ships[clicked_ship_index] = false;
                *ship_selected = clicked_ship_index;
            }//end if
        }//end if

        // Check if the user clicked on a valid position and if a ship is selected
        if (*ship_selected >= 0 && !placed_ships[*ship_selected] && *valid_position) {
            current_player->ships[*ship_selected].size = ships[*ship_selected].size;
            place_ship(&current_player->board, &current_player->ships[*ship_selected], grid_mouse_x, grid_mouse_y, *orientation, *ship_selected);
            placed_ships[*ship_selected] = true; // Set the ship as placed

            // Update player's placed ships
            current_player->placed_ships[*ship_selected] = placed_ships[*ship_selected];

            // Update player's ships
            current_player->remaining_ships++;
            current_player->ships[*ship_selected].hit_count = 0;

            *ship_selected = -1;
        }//end if
    }//end if
}//end handle_placement_mouse_button_down

void handle_placement_mouse_motion(SDL_Event *event, ButtonData *button_data, bool *placed_ships) {
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

    // Check if the user is hovering over the "Random Board" button
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
}//end handle_placement_mouse_motion

void handle_placement_phase_event(SDL_Event *event, bool *running, int *ship_selected, bool *placed_ships, Ship *ships, Player *current_player, int grid_mouse_x, int grid_mouse_y, const bool *valid_position, int *orientation, bool *invalid_click, ButtonData *button_data) {
    while (SDL_PollEvent(event)) {
        switch (event->type) {
            // Handle SDL_QUIT evenT
            case SDL_QUIT:
                *running = false;
                break;

            // Handle mouse button press event
            case SDL_MOUSEBUTTONDOWN:
                if (event->button.button == SDL_BUTTON_LEFT) {
                    handle_placement_mouse_button_down(event, running, current_player, ship_selected, placed_ships, orientation, button_data, invalid_click, grid_mouse_x, grid_mouse_y, valid_position, ships);
                }//end if
                break;

            // Handle mouse motion event
            case SDL_MOUSEMOTION:
                handle_placement_mouse_motion(event, button_data, placed_ships);
                break;

            // Handle mouse button up event
            case SDL_MOUSEBUTTONUP:
                if (event->button.button == SDL_BUTTON_LEFT) {
                    *invalid_click = false;
                }//end if
                break;
        }//end switch
    }//end while
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
    bool invalid_click = false;
    bool placed_ships[NUM_SHIPS] = {false};
    current_player->remaining_ships = 0;

    initialize_game_board(&current_player->board);

    // Initialize buttons
    SDL_Rect orientation_button = {50, 300, 275, 50}; // x, y, width, height
    SDL_Rect exit_button = {0, 0, 100, 50};
    SDL_Rect reset_button = {50, 400, 275, 50};
    SDL_Rect random_button = {50, 500, 275, 50};
    SDL_Rect finish_button = {425, 400, 275, 50};

    // Initialize the button data struct
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
    initialize_ships(current_player);

    // Main loop for the placement_phase_screen
    bool running = 1;
    while (running) {
        SDL_Event event;

        // Get the current mouse position on the grid
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        int grid_mouse_x = (mouse_x - 400) / CELL_SIZE;
        int grid_mouse_y = (mouse_y - 50) / CELL_SIZE;
        bool valid_position = is_position_valid(current_player, current_player->ships[ship_selected].size, grid_mouse_x, grid_mouse_y, orientation);

        // Handle events
        handle_placement_phase_event(&event, &running, &ship_selected, placed_ships, current_player->ships, current_player, grid_mouse_x, grid_mouse_y, &valid_position, &orientation, &invalid_click, &button_data);

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render screen background
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);

        // Render the ships on the left side of the screen
        render_placement_ships_left_side(renderer, textures, current_player->ships, placed_ships, black_texture, ship_selected);

        // Render the grid and the ships on the grid (if any).
        render_placement_grid_ships(renderer, textures, current_player->ships, placed_ships, ship_selected, orientation, grid_mouse_x, grid_mouse_y, valid_position);

        // Render exit option
        render_text(renderer, "Exit", exit_button.x + 25, exit_button.y + 10);

        // Render the buttons
        render_placement_buttons(renderer, &button_data, placed_ships, orientation, black_texture);

        // Render invalid position border
        if (invalid_click) {
            render_invalid_position_border(renderer);
        }//end if

        // Render the screen
        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / 60); // Limit frame rate to 60 FPS
    }//end while

    // Destroy textures
    SDL_DestroyTexture(background_texture);
    SDL_FreeSurface(black_surface);
    SDL_DestroyTexture(black_texture);
}//end placement_phase_screen

void placement_phase_computer(Player *computer) {
    // Initialize the computer's game board
    initialize_game_board(&computer->board);

    // Initialize the computer's ships
    initialize_ships(computer);

    // Place the computer's ships randomly
    int ship_selected = -1;
    int orientation = 0;
    bool placed_ships[NUM_SHIPS] = {false};
    place_random_ships(computer, computer->ships, placed_ships, &ship_selected, &orientation);
}//end placement_phase_computer

void render_player_board(SDL_Renderer *renderer, GameTextures *textures, Player *player, int board_x, int board_y) {
    // Render the placed ships on the player's board
    render_placed_ships(renderer, textures, player->ships, player->placed_ships, board_x, board_y);

    // Loop through the cells of the player's board and render the appropriate texture
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Cell cell = player->board.cells[x][y];
            SDL_Rect cell_rect = {board_x + x * CELL_SIZE, board_y + y * CELL_SIZE, CELL_SIZE, CELL_SIZE};

            // Render hit or miss textures
            if (cell.hit) {
                if (cell.occupied) {
                    SDL_RenderCopy(renderer, textures->hit_own_ship, NULL, &cell_rect);
                } else {
                    SDL_RenderCopy(renderer, textures->hit_ocean, NULL, &cell_rect);
                }//end else
            } else {
                // Render the ocean texture if the cell is not hit or occupied
                if (!cell.occupied) {
                    SDL_RenderCopy(renderer, textures->ocean, NULL, &cell_rect);
                }//end if
            }//end else
        }//end for
    }//end for
}//end render_player_board

void render_opponent_board(SDL_Renderer *renderer, GameTextures *textures, Player *opponent, int board_x, int board_y) {
    // Loop through the cells of the opponent's board and render the appropriate texture
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Cell cell = opponent->board.cells[x][y];
            SDL_Rect cell_rect = {board_x + x * CELL_SIZE, board_y + y * CELL_SIZE, CELL_SIZE, CELL_SIZE};

            // Render hit or miss textures
            if (cell.hit) {
                if (cell.occupied) {
                    SDL_RenderCopy(renderer, textures->hit_enemy_ship, NULL, &cell_rect);
                } else {
                    SDL_RenderCopy(renderer, textures->miss, NULL, &cell_rect);
                }//end else

            // Render the ocean texture if the cell is not hit or occupied
            } else {
                SDL_RenderCopy(renderer, textures->ocean, NULL, &cell_rect);
            }//end else
        }//end for
    }//end for
}//end render_opponent_board

void render_game_boards(SDL_Renderer *renderer, GameTextures *textures, Player *current_player, Player *opponent) {
    int board_x_offset = 50; // Adjust the horizontal spacing between the boards
    int board_y_offset = 100; // Adjust the vertical spacing from the top of the screen

    // Render the current player's board
    render_player_board(renderer, textures, current_player, board_x_offset, board_y_offset);

    // Render the opponent's board
    int opponent_board_x = 2 * board_x_offset + BOARD_SIZE * CELL_SIZE;
    render_opponent_board(renderer, textures, opponent, opponent_board_x, board_y_offset);
}//end render_game_boards

void render_game_hover_effect(SDL_Renderer *renderer, SDL_Texture *white_texture, int cell_x, int cell_y, int board_x, int board_y) {
    // Render hover effect on the specified cell and its corresponding x and y-axis cells
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (i != cell_y) {
            SDL_Rect hover_rect = {board_x + cell_x * CELL_SIZE, board_y + i * CELL_SIZE, CELL_SIZE, CELL_SIZE};
            SDL_RenderCopy(renderer, white_texture, NULL, &hover_rect);
        }//end if

        if (i != cell_x) {
            SDL_Rect hover_rect = {board_x + i * CELL_SIZE, board_y + cell_y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
            SDL_RenderCopy(renderer, white_texture, NULL, &hover_rect);
        }//end if
    }//end for
}//end render_game_hover_effect

void render_finish_turn_button(SDL_Renderer *renderer, SDL_Rect finish_turn_button, bool hover_finish_turn) {
    // Set button color based on mouse hover
    set_button_color(renderer, hover_finish_turn);

    // Render button background
    SDL_RenderFillRect(renderer, &finish_turn_button);

    // Render button shadow
    render_button_shadow(renderer, finish_turn_button);

    // Render finish text
    render_text(renderer, "Finish turn", finish_turn_button.x + 24, finish_turn_button.y + 10);
}//end render_finish_turn_button

void render_remaining_ships_text(SDL_Renderer *renderer, Player *current_player, Player *opponent) {
    char text_buffer[50];
    int text_y = 100 + BOARD_SIZE * CELL_SIZE + 10;

    // Render remaining ships text for the current player
    snprintf(text_buffer, sizeof(text_buffer), "Remaining ships: %d", current_player->remaining_ships);
    render_colored_text(renderer, text_buffer, 50, text_y, 255, 255, 255);

    // Render remaining ships text for the opponent
    snprintf(text_buffer, sizeof(text_buffer), "Remaining ships: %d", opponent->remaining_ships);
    int text_x = 2 * 50 + BOARD_SIZE * CELL_SIZE;
    render_colored_text(renderer, text_buffer, text_x, text_y, 255, 255, 255);
}//end render_remaining_ships_text

bool update_hit_count(Player *player, int ship_index) {
    // Update the hit count of the ship
    player->ships[ship_index].hit_count++;

    // Check if the ship has been sunk
    if (player->ships[ship_index].hit_count == player->ships[ship_index].size) {
        player->remaining_ships--;
        return true;
    }//end if
    return false;
}//end update_hit_count

void update_window_title(SDL_Window *window, int current_player_num) {
    // Update the window title to show the current player
    char title[50];
    snprintf(title, sizeof(title), "Battleship - Player %d", current_player_num);
    SDL_SetWindowTitle(window, title);
}//end update_window_title

void show_winner_message(SDL_Renderer *renderer, int winner_player_num) {
    // Create the winner message
    char message[50];
    sprintf(message, "Player %d won!", winner_player_num);

    // Render the winner message
    SDL_Color color = {255, 255, 255, 255};
    render_colored_text(renderer, message, 300, 500, color.r, color.g, color.b);
}//end show_winner_message

void handle_game_mouse_button_down(SDL_Renderer *renderer, GameTextures *textures, bool *running, Player *current_player, Player *opponent) {
    // Get the mouse position
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    // Get the x and y coordinates of the opponent's board
    int opponent_board_x = 2 * 50 + BOARD_SIZE * CELL_SIZE;
    int opponent_board_y = 100;

    // Check if the mouse click is within the opponent's board and if the player can shoot
    if (current_player->can_shoot && mouse_x >= opponent_board_x && mouse_x < opponent_board_x + BOARD_SIZE * CELL_SIZE &&
        mouse_y >= opponent_board_y && mouse_y < opponent_board_y + BOARD_SIZE * CELL_SIZE) {

        int cell_x = (mouse_x - opponent_board_x) / CELL_SIZE;
        int cell_y = (mouse_y - opponent_board_y) / CELL_SIZE;

        // Check if the cell is already hit
        if (!opponent->board.cells[cell_x][cell_y].hit) {
            opponent->board.cells[cell_x][cell_y].hit = true;

            // Set has_shot to true
            current_player->has_shot = true;

            // Check if the cell is occupied by an enemy ship
            if (opponent->board.cells[cell_x][cell_y].occupied) {
                // Update the hit_count of the respective ship and check if it's destroyed
                int ship_index = opponent->board.cells[cell_x][cell_y].ship_index;
                update_hit_count(opponent, ship_index);

                // Allow the player to continue shooting
                current_player->can_shoot = true;
            } else {
                // Show the "Finish turn" button
                current_player->can_shoot = false;
            }//end else
        }//end if
    }//end if

    // Check if the game is over (no enemy ships remaining)
    if (opponent->remaining_ships == 0) {
        // Show winning message
        int winner_player_num = current_player->is_turn ? 1 : 2;
        show_winner_message(renderer, winner_player_num);
        render_game_boards(renderer, textures, current_player, opponent);
        SDL_RenderPresent(renderer);
        SDL_Delay(3000); // Wait for 3 seconds before closing the game
        *running = 0;
    }//end if
}//end handle_game_mouse_button_down

void handle_game_mouse_button_up(Player *current_player, Player *opponent, SDL_Rect finish_turn_button, SDL_Window *window, const bool *hover_save, const bool *hover_exit, bool *running, AI_State *ai_state) {
    // Get the mouse position
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    // If the "Finish turn" button is visible and the player cannot shoot, check if the mouse click is within the button's area
    if (!current_player->can_shoot && is_mouse_inside_button(mouse_x, mouse_y, finish_turn_button)) {
        current_player->is_turn = !current_player->is_turn;
        opponent->is_turn = !opponent->is_turn;

        // Reset can_shoot variable
        current_player->can_shoot = true;
    }//end if

    if (*hover_save) {
        if (save_game(current_player, opponent, current_player->is_turn ? 1 : 2, ai_state)) {
            printf("Game saved successfully!");
        } else {
            printf("Error saving game!");
        }//end else
    }//end if

    if (*hover_exit) {
        *running = false;
    }//end if
}//end handle_game_mouse_button_up

void handle_game_screen_events(SDL_Event *event, SDL_Renderer *renderer, GameTextures *textures, Player *current_player, Player *opponent, bool *running, SDL_Rect finish_turn_button, const bool *hover_save, const bool *hover_exit, SDL_Window *window, AI_State *ai_state) {
    // Handle game screen events
    while (SDL_PollEvent(event)) {
        switch (event->type) {
            // Handle SDL_QUIT event
            case SDL_QUIT:
                *running = false;
                break;

            // Handle mouse button down event
            case SDL_MOUSEBUTTONDOWN:
                if (event->button.button == SDL_BUTTON_LEFT && current_player->is_human) {
                    handle_game_mouse_button_down(renderer, textures, running, current_player, opponent);
                }//end else if
                break;

            // Handle mouse button up event
            case SDL_MOUSEBUTTONUP:
                if (event->button.button == SDL_BUTTON_LEFT) {
                    handle_game_mouse_button_up(current_player, opponent, finish_turn_button, window, hover_save, hover_exit, running, ai_state);
                }//end if
                break;
        }//end switch
    }//end while
}//end handle_game_screen_events

void shuffle_directions(int *dir_indices) {
    for (int i = 3; i > 0; i--) {
        int j = (int) pcg32_boundedrand(i + 1);
        int temp = dir_indices[i];
        dir_indices[i] = dir_indices[j];
        dir_indices[j] = temp;
    }//end for
}//end shuffle_directions

void handle_computer_turn(SDL_Renderer *renderer, GameTextures *textures, Player *computer, Player *opponent, AI_State *ai_state) {
    // Initialize static variables for AI's state
    static int min_gap = 1;
    static int attempts = 0;
    static int direction = 0; // 0 for left, 1 for down, 2 for right, 3 for up
    static int last_hit_x = -1;
    static int last_hit_y = -1;
    static int initial_hit_x = -1;
    static int initial_hit_y = -1;
    static int segments_found = 0;
    static int destroyed_ships[NUM_SHIPS] = {0};
    static bool direction_fully_explored = false;
    static int dx[] = {-1, 0, 1, 0};
    static int dy[] = {0, 1, 0, -1};
    static int dir_indices[] = {0, 1, 2, 3};

    // Declare variables for the current shot
    int ship_index;
    int cell_x, cell_y;

    bool has_shot = false;
    bool update_min_gap = false;
    bool shot_successful = false;
    bool valid_cell_found = false;
    bool all_smaller_ships_destroyed = true;

    // Calculate the minimum gap size between the ships (the minimum gap is the size of the smallest ship that is not destroyed - 1)
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (opponent->ships[i].size < min_gap && !destroyed_ships[i]) {
            min_gap = opponent->ships[i].size - 1;
        }//end if
    }//end for

    // If all smaller ships are destroyed, increase the minimum gap by 1
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (opponent->ships[i].size < min_gap + 1 && !destroyed_ships[i]) {
            all_smaller_ships_destroyed = false;
        }//end if
    }//end for

    if (all_smaller_ships_destroyed) {
        min_gap++;
    }//end if

    do {
        // Handle AI states (SEARCH, TARGET, DESTROY)
        switch (*ai_state) {
            case SEARCH:
                // Reset variables
                attempts = 0;
                int search_attempts = 0;
                // Shuffle the direction indices
                shuffle_directions(dir_indices);

                // Try to find a valid cell to shoot
                while (!valid_cell_found && search_attempts < 100) {
                    // Choose a random cell to shoot
                    cell_x = (int) pcg32_boundedrand(BOARD_SIZE);
                    cell_y = (int) pcg32_boundedrand(BOARD_SIZE);

                    // Check if the cell hasn't been hit before and if it meets the minimum gap requirement
                    if (!opponent->board.cells[cell_x][cell_y].hit) {
                        bool meets_gap_requirement = true;

                        // Check if the cell meets the minimum gap requirement
                        for (int i = 0; i < 4; i++) {
                            int x = cell_x + dx[i];
                            int y = cell_y + dy[i];

                            // Check if the cell is within the board
                            if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
                                // Check if the cell has been hit before
                                if (opponent->board.cells[x][y].hit) {
                                    // Check if the cell meets the minimum gap requirement
                                    if (opponent->board.cells[x][y].ship_index != -1) {
                                        if (opponent->ships[opponent->board.cells[x][y].ship_index].size < min_gap) {
                                            meets_gap_requirement = false;
                                        }//end if
                                    } else {
                                        meets_gap_requirement = false;
                                    }//end else
                                }//end if
                            }//end if
                        }//end for

                        if (meets_gap_requirement) {
                            valid_cell_found = true;
                        }//end if
                    }//end if
                    search_attempts++;
                }//end while

                if (search_attempts == 100) {
                    // If the AI can't find a valid cell to shoot, it will shoot randomly until it finds a valid cell
                    do {
                        // Choose a random cell to shoot
                        cell_x = (int) pcg32_boundedrand(BOARD_SIZE);
                        cell_y = (int) pcg32_boundedrand(BOARD_SIZE);

                        // Check if the cell hasn't been hit before
                        if (!opponent->board.cells[cell_x][cell_y].hit) {
                            valid_cell_found = true;
                        }//end if
                    } while (!valid_cell_found);
                    printf("AI can't find a valid cell to shoot, shooting randomly\n");
                }//end if
                break;

            case TARGET:
            case DESTROY:
                valid_cell_found = false;

                // Try to find a valid cell to shoot in the current direction
                while (!valid_cell_found && attempts < 4) {
                    if (*ai_state == TARGET && !direction_fully_explored) {
                        // Choose a random direction to shoot
                        direction = dir_indices[attempts];
                        cell_x = initial_hit_x + dx[direction];
                        cell_y = initial_hit_y + dy[direction];
                    } else { // *ai_state == DESTROY or direction_fully_explored
                        cell_x = last_hit_x + dx[direction];
                        cell_y = last_hit_y + dy[direction];
                    }//end else

                    // Check if the cell is within the board and hasn't been hit before
                    if (cell_x >= 0 && cell_x < BOARD_SIZE && cell_y >= 0 && cell_y < BOARD_SIZE && !opponent->board.cells[cell_x][cell_y].hit) {
                        valid_cell_found = true;
                    } else {
                        // Increment the attempts counter
                        attempts++;

                        if (*ai_state == DESTROY) {
                            // If the AI has found the orientation and can't explore further in the first direction, explore the other direction
                            if (!direction_fully_explored) {
                                direction = (direction + 2) % 4;
                                last_hit_x = initial_hit_x;
                                last_hit_y = initial_hit_y;
                                direction_fully_explored = true;
                            }//end if

                            // If DESTROY state is not successful after two attempts, revert to TARGET state
                            if (attempts == 2) {
                                *ai_state = TARGET;
                                last_hit_x = initial_hit_x;
                                last_hit_y = initial_hit_y;
                            }//end if
                        }//end if
                    }//end else
                }//end while

                // If no valid cell is found in TARGET or DESTROY state, switch back to SEARCH state
                if (!valid_cell_found && (*ai_state == TARGET || *ai_state == DESTROY)) {
                    if (*ai_state == TARGET) {
                        direction = (direction + 1) % 4; // Try the next direction
                        attempts++; // Increment attempts count

                        // If all directions have been tried, switch back to SEARCH state
                        if (attempts >= 4) {
                            *ai_state = SEARCH;
                            attempts = 0;
                            direction = 0;
                            last_hit_x = -1;
                            last_hit_y = -1;
                            initial_hit_x = -1;
                            initial_hit_y = -1;
                            segments_found = 0;
                            direction_fully_explored = false;
                        }//end if
                    } else { // *ai_state == DESTROY
                        *ai_state = SEARCH;
                        attempts = 0;
                        direction = 0;
                        last_hit_x = -1;
                        last_hit_y = -1;
                        initial_hit_x = -1;
                        initial_hit_y = -1;
                        segments_found = 0;
                        direction_fully_explored = false;
                    }//end else
                    continue; // Continue with the next iteration of the do-while loop
                }//end if

                // If there's a sequence with more than 5 ship segments together, consider there's more than 1 ship there
                if (segments_found > 5) {
                    *ai_state = SEARCH;
                    attempts = 0;
                    direction = 0;
                    last_hit_x = -1;
                    last_hit_y = -1;
                    initial_hit_x = -1;
                    initial_hit_y = -1;
                    segments_found = 0;
                    direction_fully_explored = false;
                }//end if
                break;
        }//end switch

        // If the chosen cell hasn't been hit before
        if (!opponent->board.cells[cell_x][cell_y].hit) {
            // Mark the cell as hit
            opponent->board.cells[cell_x][cell_y].hit = true;
            has_shot = true;

            // If the cell is occupied by a ship
            if (opponent->board.cells[cell_x][cell_y].occupied) {
                // Update hit count of the ship
                ship_index = opponent->board.cells[cell_x][cell_y].ship_index;
                update_hit_count(opponent, ship_index);
                shot_successful = true;
                segments_found++;

                // If the ship is sunk update destroyed_ships array
                if (opponent->ships[ship_index].hit_count == opponent->ships[ship_index].size) {
                    destroyed_ships[ship_index] = true;
                }//end if

                // Update AI state based on the current state
                if (*ai_state == SEARCH) {
                    *ai_state = TARGET;
                    initial_hit_x = last_hit_x = cell_x;
                    initial_hit_y = last_hit_y = cell_y;
                } else if (*ai_state == TARGET || *ai_state == DESTROY) {
                    if (*ai_state == TARGET) {
                        // Update the state to DESTROY
                        *ai_state = DESTROY;
                    }//end if
                    last_hit_x = cell_x;
                    last_hit_y = cell_y;
                }//end else if

                // If the ship is sunk, reset AI state to SEARCH
                if (opponent->ships[ship_index].hit_count == opponent->ships[ship_index].size) {
                    *ai_state = SEARCH;
                    attempts = 0;
                    direction = 0;
                    last_hit_x = -1;
                    last_hit_y = -1;
                    initial_hit_x = -1;
                    initial_hit_y = -1;
                    segments_found = 0;
                    valid_cell_found = false;
                    direction_fully_explored = false;
                }//end if

                // Render the game boards again
                render_game_boards(renderer, textures, opponent, computer);
                SDL_RenderPresent(renderer);
                SDL_Delay(1000); // Delay for 1 second
            } else {
                // If the cell was not occupied by a ship, update AI state
                shot_successful = false;
                if (*ai_state == TARGET) {
                    attempts++;
                } else if (*ai_state == DESTROY) {
                    *ai_state = TARGET;
                    direction = (direction + 2) % 4; // Reverse direction
                    last_hit_x = initial_hit_x;
                    last_hit_y = initial_hit_y;
                    direction_fully_explored = true;
                }//end else if
            }//end else
        } else {
            // If the shot was unsuccessful, update AI state
            if (*ai_state == TARGET) {
                direction = dir_indices[attempts];
                attempts++;
                direction_fully_explored = false;
            } else if (*ai_state == DESTROY) {
                *ai_state = TARGET;
                direction = (direction + 2) % 4; // Reverse direction
                last_hit_x = initial_hit_x;
                last_hit_y = initial_hit_y;
            }//end else if
        }//end else
    } while (shot_successful && opponent->remaining_ships > 0);

    if (!has_shot) {
        printf("This should never happen\n");
    }//end if

    // If all opponent's ships are sunk, show the winner message
    if (opponent->remaining_ships == 0) {
        show_winner_message(renderer, 2);
    }//end if

    // Render the game boards one last time and wait for a second
    render_game_boards(renderer, textures, opponent, computer);
    render_remaining_ships_text(renderer, opponent, computer);
    SDL_RenderPresent(renderer);
    SDL_Delay(1000); // Delay for 1 second
}//end handle_computer_turn

void game_screen(SDL_Renderer *renderer, SDL_Window *window, GameTextures *textures, Player *player1, Player *player2, int *current_turn, AI_State *ai_state) {
    // Load background texture
    SDL_Texture *background_texture = IMG_LoadTexture(renderer, "Assets/game_screen_background.jpeg");

    // Create black surface
    SDL_Surface *black_surface = SDL_CreateRGBSurface(0, CELL_SIZE, CELL_SIZE, 32, 0, 0, 0, 0);
    SDL_SetSurfaceAlphaMod(black_surface, 50); // Set the alpha value to 128 (50% opacity)
    SDL_FillRect(black_surface, NULL, SDL_MapRGB(black_surface->format, 0, 0, 0));
    SDL_Texture *black_texture = SDL_CreateTextureFromSurface(renderer, black_surface);
    SDL_SetTextureBlendMode(black_texture, SDL_BLENDMODE_BLEND);

    // Initialize the variables
    bool running = true;
    bool hover_exit = false;
    bool hover_save = false;
    bool hover_finish_turn = false;
    SDL_Event event;

    // Initialize button
    SDL_Rect finish_turn_button = {800 / 2 - 100, 600 - 70, 200, 40};
    SDL_Rect save_button = {630, 550, 50, 30};
    SDL_Rect exit_button = {710, 550, 50, 30};

    // Main game loop
    while (running) {
        // Set the current player and opponent based on the current turn
        Player *current_player = *current_turn == 1 ? player1 : player2;
        Player *opponent = *current_turn == 1 ? player2 : player1;

        // Set the render draw color to white
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // Clear the screen
        SDL_RenderClear(renderer);

        // Render the background texture
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);

        // Render game boards for both players
        render_game_boards(renderer, textures, current_player, opponent);

        // Check if it is the computer's turn and handle it
        if (current_player->is_human == false) {
            handle_computer_turn(renderer, textures, current_player, opponent, ai_state);
            if (opponent->remaining_ships == 0) {
                break;
            }//end if
            current_player->is_turn = !current_player->is_turn;
            opponent->is_turn = !opponent->is_turn;
        } else {
            // Render remaining ships text for both players
            render_remaining_ships_text(renderer, current_player, opponent);
        }//end else

        // Get mouse position
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        // Calculate the cell coordinates based on the mouse position
        int opponent_board_x = 2 * 50 + BOARD_SIZE * CELL_SIZE;
        int opponent_board_y = 100;

        // Check if the mouse is hovering over the opponent's board
        if (mouse_x >= opponent_board_x && mouse_x < opponent_board_x + BOARD_SIZE * CELL_SIZE &&
            mouse_y >= opponent_board_y && mouse_y < opponent_board_y + BOARD_SIZE * CELL_SIZE) {

            int cell_x = (mouse_x - opponent_board_x) / CELL_SIZE;
            int cell_y = (mouse_y - opponent_board_y) / CELL_SIZE;

            // Render the hover effect
            if (current_player->can_shoot) {
                render_game_hover_effect(renderer, black_texture, cell_x, cell_y, opponent_board_x, opponent_board_y);
            }//end if
        }//end if

        // Check if the mouse is hovering over the save button
        hover_save = is_mouse_inside_button(mouse_x, mouse_y, save_button);

        // Check if the mouse is hovering over the exit button
        hover_exit = is_mouse_inside_button(mouse_x, mouse_y, exit_button);

        // Render the save button
        SDL_Color save_button_color = hover_save ? (SDL_Color){255, 255, 0, 255} : (SDL_Color){255, 255, 255, 255};
        render_colored_text(renderer, "Save", save_button.x, save_button.y, save_button_color.r, save_button_color.g, save_button_color.b);

        // Render the exit button
        SDL_Color exit_button_color = hover_exit ? (SDL_Color){255, 255, 0, 255} : (SDL_Color){255, 255, 255, 255};
        render_colored_text(renderer, "Exit", exit_button.x, exit_button.y, exit_button_color.r, exit_button_color.g, exit_button_color.b);

        // Check if the mouse is hovering over the exit button
        hover_exit = is_mouse_inside_button(mouse_x, mouse_y, exit_button);

        // Render the "Finish turn" button if the current player has shot and can't shoot anymore
        if (!current_player->can_shoot && current_player->has_shot) {
            // Check if the mouse is hovering over the "Finish turn" button
            hover_finish_turn = is_mouse_inside_button(mouse_x, mouse_y, finish_turn_button);

            // Render the "Finish turn" button
            render_finish_turn_button(renderer, finish_turn_button, hover_finish_turn);
        }//end if

        // Update the screen
        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / 60); // Limit frame rate to 60 FPS

        // Handle game screen events
        handle_game_screen_events(&event, renderer, textures, current_player, opponent, &running, finish_turn_button, &hover_save, &hover_exit, window, ai_state);

        // Change the current turn
        if (current_player->is_turn == false) {
            *current_turn = *current_turn == 1 ? 2 : 1;
            update_window_title(window, *current_turn);
        }//end if
    }//end while

    // Free resources
    SDL_FreeSurface(black_surface);
    SDL_DestroyTexture(black_texture);
    SDL_DestroyTexture(background_texture);
}//end game_screen

void cleanup(GameTextures *textures, SDL_Renderer *renderer, SDL_Window *window) {
    free(textures);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    TTF_CloseFont(font);
    TTF_Quit();
}//end cleanup
