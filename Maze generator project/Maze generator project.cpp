#include <iostream>
#include <vector>
#include <cstdlib> 
#include <ctime>   
#include <utility> 
#include <algorithm> 
#include <limits> 

// --- Platform-Specific Includes for Non-Blocking/Raw Input ---
#ifdef _WIN32
#include <conio.h> // For _getch()
#include <windows.h> // For Sleep
#else
#include <termios.h> // For tcgetattr, tcsetattr (POSIX systems like Linux/macOS)
#include <unistd.h>  // For STDIN_FILENO, usleep
#endif

using namespace std;

// --- Constants ---
const char WALL = '#';
const char PATH = ' ';
const char START = 'S';
const char END = 'E';
const char PLAYER = '@';

// --- Global State (Simplified) ---
int g_height = 0;
int g_width = 0;
int p_r = 0; // Current Player Row
int p_c = 0; // Current Player Column
int p_r_old = 0; // Previous Player Row (for updating the console)
int p_c_old = 0; // Previous Player Column (for updating the console)

// --- Platform-Specific Terminal State Storage ---
#ifndef _WIN32
struct termios g_old_termios;
#endif

// --- Terminal Control Functions ---

/**
 * @brief Sets the terminal into raw mode (non-canonical and no echo) for instant input.
 */
void set_terminal_raw() {
#ifdef _WIN32
    // On Windows, _getch() provides raw input.
#else
    // On POSIX systems (Linux, macOS)
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &g_old_termios); // Save old settings
    new_termios = g_old_termios;
    // Disable canonical mode (ICANON) and echo (ECHO)
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
#endif
}

/**
 * @brief Resets the terminal to its original, normal state.
 */
void reset_terminal() {
#ifndef _WIN32
    // Restore saved settings on POSIX systems
    tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
#endif
    // Ensure cursor is visible when the program exits
    cout << "\033[?25h";
}

// --- Maze and Memory Functions ---

/**
 * @brief Cleans up the dynamically allocated memory for the maze grid.
 * @param maze The dynamically allocated 2D array to delete.
 */
void cleanup_grid(char** maze) {
    if (maze) {
        for (int i = 0; i < g_height; ++i) {
            delete[] maze[i];
        }
        delete[] maze;
    }
}

/**
 * @brief Recursively carves paths using the Recursive Backtracker (DFS) algorithm.
 * (Logic remains the same, no changes needed for non-redrawing movement)
 */
void carve_path(char** maze, int r, int c) {
    maze[r][c] = PATH;

    // Define possible directions: (dr, dc). We look 2 steps away.
    vector<pair<int, int>> directions = {
        {-2, 0}, {+2, 0}, {0, -2}, {0, +2}
    };

    // Shuffle the directions to ensure random path carving
    random_shuffle(directions.begin(), directions.end());

    // Explore each random direction
    for (const auto& dir : directions) {
        int nr = r + dir.first;
        int nc = c + dir.second;

        // Check boundaries against the global dimensions
        if (nr > 0 && nr < g_height - 1 && nc > 0 && nc < g_width - 1) {
            // If the neighbor cell is still a wall (unvisited)
            if (maze[nr][nc] == WALL) {
                // Carve the wall between the current cell and the neighbor cell
                int wall_r = r + dir.first / 2;
                int wall_c = c + dir.second / 2;
                maze[wall_r][wall_c] = PATH;

                // Recursively call for the new cell
                carve_path(maze, nr, nc);
            }
        }
    }
}

// --- Input/Output and Game Logic Functions ---

/**
 * @brief Prints the game status header at the top of the screen.
 * @param message The message to display (e.g., instructions, win message).
 * Uses ANSI \033[H to move cursor to top-left before printing.
 */
void print_header(const string& message) {
    // Move cursor to Home (top-left)
    cout << "\033[H";
    cout << "+---[ MAZE: " << g_height << "x" << g_width << " ]---+\n";
    cout << message << "\n";
    cout << "Current Pos: (" << p_r << ", " << p_c << ")\n\n";
    cout.flush();
}

/**
 * @brief Prints the entire maze structure once.
 * @param maze The maze grid to display.
 */
void draw_initial_maze(char** maze) {
    // ANSI code: \033[H moves cursor to Home, \033[J clears screen from cursor position down
    cout << "\033[H\033[J";

    // Print the header first (it clears the screen)
    print_header("Move with W/A/S/D or Arrow Keys. Press 'R' to reset or 'Q' to quit.");

    // The maze starts printing on row 5 (1-indexed) after the header lines.
    for (int i = 0; i < g_height; ++i) {
        for (int j = 0; j < g_width; ++j) {
            // Determine the character/color to print based on the cell content
            if (i == p_r && j == p_c) {
                // Initial player position
                cout << "\033[33;1m" << PLAYER << PLAYER << "\033[0m";
            }
            else if (maze[i][j] == WALL) {
                // Dark gray wall
                cout << "\033[38;5;238m" << WALL << WALL << "\033[0m";
            }
            else if (maze[i][j] == START) {
                // Bright green START
                cout << "\033[32;1m" << START << START << "\033[0m";
            }
            else if (maze[i][j] == END) {
                // Bright red END
                cout << "\033[31;1m" << END << END << "\033[0m";
            }
            else {
                // Path/Space
                cout << PATH << PATH;
            }
        }
        cout << "\n";
    }
    cout << "+---------------------------------+\n\n";
    cout.flush();
}

/**
 * @brief Updates the player's position on the screen without redrawing the whole maze.
 * @param maze The maze grid, used to determine what character was left behind.
 */
void draw_player_update(char** maze) {
    // Console Row = maze_row + 5 (5 is the offset for the 4 header lines + 1 blank line)
    // Console Col = maze_col * 2 + 1 (since each cell prints two characters, and console is 1-indexed)

    // --- Step 1: Erase Old Position and Restore Path/Start/End character ---
    char old_char = maze[p_r_old][p_c_old];

    // Move cursor to old position
    cout << "\033[" << (p_r_old + 5) << ";" << (p_c_old * 2 + 1) << "H";

    // Restore the character that was originally there (Path/Start/End)
    if (old_char == WALL) {
        // This case should ideally not happen if movement logic is correct, but handled defensively.
        cout << "\033[38;5;238m" << WALL << WALL << "\033[0m";
    }
    else if (old_char == START) {
        cout << "\033[32;1m" << START << START << "\033[0m";
    }
    else if (old_char == END) {
        cout << "\033[31;1m" << END << END << "\033[0m";
    }
    else { // PATH
        cout << PATH << PATH;
    }

    // --- Step 2: Draw New Player Position ---

    // Move cursor to new position
    cout << "\033[" << (p_r + 5) << ";" << (p_c * 2 + 1) << "H";

    // Draw the player
    cout << "\033[33;1m" << PLAYER << PLAYER << "\033[0m";

    // --- Step 3: Move cursor away to bottom of screen and print header again ---
    // (This ensures the next user input is on a new line and the header is updated)
    cout << "\033[" << (g_height + 6) << ";1H";

    print_header("Move with W/A/S/D or Arrow Keys. Press 'R' to reset or 'Q' to quit.");
    cout.flush();
}

/**
 * @brief Gets a character input instantly, without waiting for Enter.
 */
char get_instant_input() {
#ifdef _WIN32
    // Windows-specific instant input
    return _getch();
#else
    // POSIX-specific instant input (requires terminal to be in raw mode)
    return cin.get();
#endif
}

/**
 * @brief Attempts to move the player or handle control commands.
 * @param maze The maze grid.
 * @param input The character input.
 * @return Status code: 0 (No change), 1 (Move), 2 (Win), 3 (Reset), 4 (Quit).
 */
int handle_input(char** maze, char input) {
    int dr = 0; // Delta Row
    int dc = 0; // Delta Column

    // Check for control commands first
    if (input == 'q' || input == 'Q') return 4; // Quit
    if (input == 'r' || input == 'R') return 3; // Reset

    // Check for WASD or Arrow Key movement
    switch (input) {
    case 'w':
    case 'W':
        dr = -1; // Up
        break;
    case 's':
    case 'S':
        dr = 1;  // Down
        break;
    case 'a':
    case 'A':
        dc = -1; // Left
        break;
    case 'd':
    case 'D':
        dc = 1;  // Right
        break;
        // Arrow key handling (Escape sequence)
        // Arrow keys send a sequence: ESCAPE (\033), [, then A, B, C, or D
    case '\033': {
        // Read the next two characters for the sequence: ESC [ A/B/C/D
        char second_char = get_instant_input();
        if (second_char == '[') {
            switch (get_instant_input()) {
            case 'A': dr = -1; break; // Up Arrow
            case 'B': dr = 1;  break; // Down Arrow
            case 'C': dc = 1;  break; // Right Arrow
            case 'D': dc = -1; break; // Left Arrow
            }
        }
        break;
    }
    default:
        return 0; // Not a valid move command or control
    }

    // If dr or dc is set, process the potential move
    if (dr != 0 || dc != 0) {
        int next_r = p_r + dr;
        int next_c = p_c + dc;

        // 1. Check if the move is within bounds
        if (next_r >= 0 && next_r < g_height && next_c >= 0 && next_c < g_width) {
            // 2. Check if the move is not into a wall (Keeping existing hitbox logic)
            if (maze[next_r][next_c] != WALL) {
                // Store current position as old before updating to new
                p_r_old = p_r;
                p_c_old = p_c;

                p_r = next_r;
                p_c = next_c;

                // 3. Check for the win condition
                if (maze[p_r][p_c] == END) {
                    return 2; // Game won
                }
                return 1; // Successful move
            }
        }
    }
    return 0; // Move unsuccessful (hit wall or boundary)
}

/**
 * @brief The main game loop function.
 * @param maze The maze grid.
 * @return The final status of the game (0 for success).
 */
int game_loop(char** maze) {
    // Player position is set to START (1, 1) on entry or reset.
    p_r = 1;
    p_c = 1;
    p_r_old = 1;
    p_c_old = 1;

    // Draw the entire maze once before the loop
    draw_initial_maze(maze);

    while (true) {
        char input = get_instant_input(); // Get input without waiting for Enter

        if (cin.eof()) {
            break;
        }

        int status = handle_input(maze, input);

        if (status == 4) { // Quit
            break;
        }

        if (status == 1) { // Successful Move
            // Only update the player's position on screen, no full redraw!
            draw_player_update(maze);
        }

        if (status == 3 || status == 2) { // Reset or Win
            if (status == 2) {
                // Use ANSI codes to display the win message above the maze
                cout << "\033[2;1H\033[K\033[32;1m*** CONGRATULATIONS! YOU REACHED THE END (E)! ***\033[0m\n";
                // Add a small delay for the user to read the win message before reset
#ifdef _WIN32
                Sleep(1500);
#else
                usleep(1500000);
#endif
            }

            // Reset player position (Old and Current)
            p_r_old = p_r; // The old position becomes the last position before reset
            p_c_old = p_c;
            p_r = 1;
            p_c = 1;

            // Update the screen for the reset
            draw_player_update(maze);
        }
    }
    return 0; // Success
}


/**
 * @brief Handles user input for size and initiates maze setup.
 * @return The dynamically allocated maze grid.
 */
char** setup_maze() {
    int size = 0;
    const int MIN_SIZE = 10;
    const int MAX_SIZE = 50;

    // --- User Input and Validation ---
    cout << "--- C++ Random Maze Generator ---\n";
    cout << "Enter the desired maze size (N for NxN, min " << MIN_SIZE << ", max " << MAX_SIZE << "): ";

    // Simple input validation loop
    while (!(cin >> size) || size < MIN_SIZE || size > MAX_SIZE) {
        cout << "Invalid input. Please enter a number between "
            << MIN_SIZE << " and " << MAX_SIZE << ": ";
        cin.clear(); // Clear error flags
        // Clear bad input buffer: 256 is a reliable size for most console lines
        cin.ignore(256, '\n');
    }

    // --- Dimension Setup (Ensures odd size for the generation algorithm) ---
    g_height = size % 2 == 0 ? size + 1 : size;
    g_width = size % 2 == 0 ? size + 1 : size;

    cout << "\nGenerating a random maze of size " << g_height << "x" << g_width << "...\n";

    // --- Maze Allocation and Initialization ---
    char** maze = new char* [g_height];
    for (int i = 0; i < g_height; ++i) {
        maze[i] = new char[g_width];
        for (int j = 0; j < g_width; ++j) {
            maze[i][j] = WALL;
        }
    }

    // --- Maze Generation ---
    srand(time(0));

    // Start carving from (1, 1), the first non-boundary cell
    carve_path(maze, 1, 1);

    // Set the Start and End points
    maze[1][1] = START;
    maze[g_height - 2][g_width - 2] = END;

    // Clear any pending input buffer before starting the game
    cin.ignore(256, '\n');

    return maze;
}

/**
 * @brief Main function to initialize, run, and clean up the maze game.
 */
int main() {
    // 1. Get the maze setup while still in standard input mode
    char** maze = setup_maze();

    // 2. Set terminal to raw mode for instant key presses
    set_terminal_raw();

    // 3. Start the game loop
    int result = game_loop(maze);

    // 4. Reset terminal and clean up
    reset_terminal();
    cleanup_grid(maze);

    cout << "Program finished. Thank you for playing!\n";

    return result;
}