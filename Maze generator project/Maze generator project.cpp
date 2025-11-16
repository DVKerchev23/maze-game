#include <iostream>
#include <vector>
#include <algorithm>
#include <conio.h>
#include <chrono> // For timing
#include <iomanip> // For setprecision and fixed
#include <limits> // Required for numeric_limits
#include <string> // Required for string

#define NOMINMAX

#include <windows.h>


using namespace std;

// --- TIMER ADDITION: Global timer variables
std::chrono::steady_clock::time_point g_start_time;
bool g_timer_started = false;
double g_final_time_seconds = 0.0; // To store the time when the game is won

char PLAYER = 245;
char WALL = 178;//176 176 178 219

const char PATH = ' ';
const char START = 'S';
const char END = 'E';


int g_height = 0;
int g_width = 0;
int p_r = 0; // Current Player Row
int p_c = 0; // Current Player Column
int p_r_old = 0; // Previous Player Row (for updating the console)
int p_c_old = 0; // Previous Player Column (for updating the console)

void reset_terminal();
char get_instant_input();

// Maze Generation/Memory
void cleanup_grid(char** maze);
void carve_path(char** maze, int r, int c);
char** setup_maze();

// Game Logic/Rendering
void print_header(const string& message);
void draw_initial_maze(char** maze);
void draw_player_update(char** maze);
int handle_input(char** maze, char input);
// Updated game_loop to manage the freeze on win
int game_loop(char** maze);
int main_menu();
int post_game_menu();

void reset_terminal() {
    // Ensure cursor is visible when the program exits using ANSI escape code.
    cout << "\033[?25h";
}

char get_instant_input() {
    return _getch();
}


// --- Maze and Memory Implementations ---

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
 * @param maze The maze grid being generated.
 * @param r Current row.
 * @param c Current column.
 */
void carve_path(char** maze, int r, int c) {
    maze[r][c] = PATH;

    // Define possible directions: (dr, dc) - checks 2 cells away to ensure walls remain
    vector<pair<int, int>> directions = {
        {-2, 0}, {+2, 0}, {0, -2}, {0, +2}
    };

    // Shuffle the directions to ensure random path carving
    random_shuffle(directions.begin(), directions.end());

    // Explore each random direction
    for (const auto& dir : directions) {
        int nr = r + dir.first;
        int nc = c + dir.second;

        // Check boundaries
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

/**
 * @brief Handles user input for size and initiates maze setup.
 * @return The dynamically allocated maze grid.
 */
char** setup_maze() {
    int size = 0;
    // Requirements: 10x10 to 50x50
    const int MIN_SIZE = 10;
    const int MAX_SIZE = 50;

    // --- User Input and Validation ---
    cout << "\033[H\033[J"; // Clear screen
    cout << "--- C++ Random Maze Generator ---\n";
    cout << "Enter the desired maze size (N for NxN, min " << MIN_SIZE << ", max " << MAX_SIZE << "): ";

    // Simple input validation loop
    while (!(cin >> size) || size < MIN_SIZE || size > MAX_SIZE) {
        cout << "Invalid input. Please enter a number between "
            << MIN_SIZE << " and " << MAX_SIZE << ": ";
        cin.clear(); // Clear error flags
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear bad input buffer
    }

    // --- Dimension Setup (Ensures odd size for the generation algorithm) ---
    // The algorithm requires odd dimensions for proper wall/path separation.
    g_height = size % 2 == 0 ? size + 1 : size;
    g_width = size % 2 == 0 ? size + 1 : size;

    cout << "\nGenerating a random maze of size " << g_height << "x" << g_width << "...\n";

    // --- Maze Allocation and Initialization (Dynamic Memory) ---
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

    // Set the Start and End points (Start at 1,1; End at g_height-2, g_width-2)
    maze[1][1] = START;
    maze[g_height - 2][g_width - 2] = END;

    // Clear any pending standard input buffer before starting the game
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    return maze;
}

// --- Game Logic and Rendering Implementations ---

/**
 * @brief Prints the game status header at the top of the screen.
 * @param message The message to display (e.g., instructions, win message).
 */
void print_header(const string& message) {
    // Move cursor to Home (top-left)
    cout << "\033[H";
    cout << "\033[1m+---[ C++ MAZE: " << g_height << "x" << g_width << " ]---+\033[0m\n";
    cout << message << "\n";

    // --- TIMER ADDITION: Display current elapsed time or final time ---
    cout << "Current Pos: (" << p_r << ", " << p_c << ") | ";

    if (g_final_time_seconds > 0.0) {
        // Display final time in bright green upon win
        cout << "\033[32;1mFinal Time: " << fixed << setprecision(2) << g_final_time_seconds << "s\033[0m";
    }
    else if (g_timer_started) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_start_time);
        double elapsed_time = duration.count() / 1000.0;
        // Clear to end of line to overwrite previous time
        cout << "Time: " << fixed << setprecision(2) << elapsed_time << "s\033[K";
    }
    else {
        cout << "Time: --.--s (Start moving!) \033[K";
    }
    cout << "\n\n";
    cout.flush();
}

/**
 * @brief Prints the entire maze structure once, clearing the screen.
 * @param maze The maze grid to display.
 */
void draw_initial_maze(char** maze) {
    // ANSI code: \033[H moves cursor to Home, \033[J clears screen
    cout << "\033[H\033[J";

    // Print the header first
    print_header("Move with W/A/S/D. Press 'R' to reset or 'Q' to quit.");

    // The maze starts printing on row 5 (1-indexed) after the header lines.
    for (int i = 0; i < g_height; ++i) {
        for (int j = 0; j < g_width; ++j) {
            // Check for player first
            if (i == p_r && j == p_c) {
                // Initial player position (Bright Yellow)
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
    cout << "\033[1m+---------------------------------+\033[0m\n\n";
    cout.flush();
}

/**
 * @brief Updates the player's position on the screen using cursor control
 * without redrawing the whole maze (efficient rendering).
 * @param maze The maze grid, used to determine what character was left behind.
 */
void draw_player_update(char** maze) {
    // Row offset is 5 (4 header lines + 1 blank line before maze starts)
    // Column offset is 1 (1-indexed console, and each maze cell prints 2 chars)

    // --- Step 1: Erase Old Position and Restore Path/Start/End character ---
    char old_char = maze[p_r_old][p_c_old];

    // Move cursor to old position: Row=p_r_old + 5, Col=p_c_old * 2 + 1
    cout << "\033[" << (p_r_old + 5) << ";" << (p_c_old * 2 + 1) << "H";

    // Restore the character that was originally there
    if (old_char == START) {
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

    // Draw the player (Bright Yellow)
    cout << "\033[33;1m" << PLAYER << PLAYER << "\033[0m";

    // --- Step 3: Update Header and move cursor to bottom ---
    // Move cursor to the bottom of the screen
    cout << "\033[" << (g_height + 6) << ";1H";

    print_header("Move with W/A/S/D. Press 'R' to reset or 'Q' to quit.");
    cout.flush();
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

    // Check for WASD movement
    switch (input) {
    case 'w': case 'W': dr = -1; break; // Up
    case 's': case 'S': dr = 1;  break; // Down
    case 'a': case 'A': dc = -1; break; // Left
    case 'd': case 'D': dc = 1;  break; // Right
    default:
        return 0; // Not a valid move command or control
    }

    // If dr or dc is set, process the potential move
    if (dr != 0 || dc != 0) {
        int next_r = p_r + dr;
        int next_c = p_c + dc;

        // 1. Check if the move is within bounds and not a wall
        if (next_r >= 0 && next_r < g_height &&
            next_c >= 0 && next_c < g_width &&
            maze[next_r][next_c] != WALL)
        {
            // --- TIMER ADDITION: Start the timer on the first move ---
            if (!g_timer_started) {
                g_start_time = std::chrono::steady_clock::now();
                g_timer_started = true;
                g_final_time_seconds = 0.0; // Reset final time
            }

            // Store current position as old before updating to new
            p_r_old = p_r;
            p_c_old = p_c;

            p_r = next_r;
            p_c = next_c;

            // 2. Check for the win condition
            if (maze[p_r][p_c] == END) {
                return 2; // Game won
            }
            return 1; // Successful move
        }
    }
    return 0; // Move unsuccessful (hit wall or boundary)
}

/**
 * @brief The main game loop function. Runs until the player quits or wins.
 * @param maze The maze grid.
 * @return Status code: 0 (Quit), 1 (Win), 2 (Reset - Should not happen here)
 */
int game_loop(char** maze) {
    // Player starts at (1, 1) as set in setup_maze
    p_r = 1;
    p_c = 1;
    p_r_old = 1;
    p_c_old = 1;

    // --- TIMER ADDITION: Reset timer state for a new game ---
    g_timer_started = false;
    g_final_time_seconds = 0.0;

    // Draw the entire maze once before the loop
    draw_initial_maze(maze);

    while (true) {
        // Use _getch() for instant, non-blocking input
        // Note: Using get_instant_input() here means the loop will consume CPU aggressively
        // in a real application, you might add a small sleep or use a different input method.
        char input = get_instant_input();

        int status = handle_input(maze, input);

        if (status == 4) { // Quit
            return 0; // Indicate quit
        }

        if (status == 1) { // Successful Move
            draw_player_update(maze);
        }

        if (status == 3 || status == 2) { // Reset or Win
            if (status == 2) {
                // --- TIMER ADDITION: Calculate final time upon win ---
                auto end_time = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - g_start_time);
                g_final_time_seconds = duration.count() / 1000.0;
                g_timer_started = false; // Stop the timer

                // Update player position one last time (P over E)
                draw_player_update(maze);

                // Print the final win message in the header and FREEZE the screen
                cout << "\033[2;1H\033[K\033[32;1m*** CONGRATULATIONS! YOU REACHED THE END (E)! ***\033[0m\n";

                // Update header again to show final time clearly
                cout << "\033[" << (g_height + 6) << ";1H";
                print_header("Game Over! See final time above.");
                cout.flush();

                // Return 1 to signal that the player won the maze
                return 1;
            }

            // If status == 3 (Reset), reset positions and timer state
            if (status == 3) {
                // Reset player position to START (1,1)
                p_r_old = p_r;
                p_c_old = p_c;
                p_r = 1;
                p_c = 1;

                g_timer_started = false;
                g_final_time_seconds = 0.0;

                // Update the screen for the reset
                draw_player_update(maze);
            }
        }
    }
}

// --- NEW POST-GAME MENU IMPLEMENTATION ---

/**
 * @brief Displays the menu after the maze is won.
 * @return 1 (Play New), 2 (Main Menu), 0 (Quit).
 */
int post_game_menu() {
    int choice = 0;
    while (true) {
        // Clear screen below the maze and print the menu
        cout << "\033[" << (g_height + 8) << ";1H"; // Move cursor a few lines below the maze/footer
        cout << "\033[J"; // Clear screen from here down

        cout << "\033[1m+---------------------------------+\033[0m\n";
        cout << "\033[1m|      \033[33;1mWhat would you like to do?\033[0m     |\033[0m\n";
        cout << "\033[1m+---------------------------------+\033[0m\n";
        cout << "| 1. \033[32;1mPlay New Maze\033[0m                     |\n";
        cout << "| 2. \033[34;1mBack to Main Menu\033[0m                 |\n";
        cout << "| 3. \033[31;1mQuit Game\033[0m                           |\n";
        cout << "\033[1m+---------------------------------+\033[0m\n";
        cout << "Enter your choice (1-3): ";

        if (cin >> choice) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer
            switch (choice) {
            case 1: return 1; // Play New Maze
            case 2: return 2; // Back to Main Menu
            case 3: return 0; // Quit Game
            default:
                cout << "\nInvalid choice. Please enter 1, 2, or 3. Press any key to continue...";
                get_instant_input();
                break;
            }
        }
        else {
            // Handle non-integer input
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "\nInvalid input. Please enter a number. Press any key to continue...";
            get_instant_input();
        }
    }
}

// --- MAIN MENU IMPLEMENTATION ---

/**
 * @brief Displays the main menu and handles user selection.
 * @return 1 for Play, 0 for Quit.
 */
int main_menu() {
    int choice = 0;
    while (true) {
        cout << "\033[H\033[J"; // Clear screen
        cout << "\033[1m+----------------------------------+\033[0m\n";
        cout << "\033[1m|        WELCOME TO THE MAZE        |\033[0m\n";
        cout << "\033[1m+----------------------------------+\033[0m\n";
        cout << "| 1. \033[32;1mPlay Game\033[0m                       |\n";
        cout << "| 2. \033[36;1mLevels                       \033[0m   |\n";
        cout << "| 3. \033[34;1mSettings                     \033[0m   |\n";
        cout << "| 4. \033[35;1mAbout us                     \033[0m   |\n";
        cout << "| 5. \033[31;1mQuit\033[0m                            |\n";
        cout << "\033[1m+----------------------------------+\033[0m\n";
        cout << "Enter your choice (1-3): ";

        if (cin >> choice) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer
            switch (choice) {
            case 1:
                return 1; // Play Game
            case 2:
                break; 
            case 3:
                break;
            case 4:
                break;
            case 5:
                return 0;
                
            default:
                cout << "\nInvalid choice, try again";
                get_instant_input();
                break;
            }
        }
        else {
            // Handle non-integer input
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "\nInvalid input. Please enter a number. Press any key to continue...";
            get_instant_input();
        }
    }
}

/**
 * @brief Main function to initialize, run, and clean up the maze game.
 */
int main() {
    // Main program loop to handle game restarts and menu navigation
    while (true) {
        char** maze = nullptr;
        int menu_choice = main_menu();

        if (menu_choice == 0) {
            break; // Quit from main menu
        }

        if (menu_choice == 1) { // Play Game
            // Loop for playing one or more mazes
            while (true) {
                // 1. Setup the maze (and get size input)
                maze = setup_maze();

                // 2. Start the game loop
                int game_status = game_loop(maze);


                if (game_status == 0) {
                    // Quit during game
                    cleanup_grid(maze); // Cleanup maze if allocated
                    maze = nullptr;
                    break;
                }

                // If game_status == 1 (Win), show the post-game menu
                if (game_status == 1) {
                    // 3. Show the menu right over the screen freeze
                    int post_game_choice = post_game_menu();

                    // 4. Cleanup the maze memory after the user has made a choice
                    cleanup_grid(maze);
                    maze = nullptr;

                    if (post_game_choice == 1) {
                        continue; // Play New Maze (loops back to setup_maze)
                    }
                    if (post_game_choice == 2) {
                        break; // Back to Main Menu (breaks inner loop, outer loop runs main_menu)
                    }
                    if (post_game_choice == 0) {
                        goto exit_program; // Jump to clean exit
                    }
                }
            }
        }
    }

exit_program:
    reset_terminal();
    cout << "\033[H\033[J";
    cout << "Program finished. Goodbye!\n";
    return 0;
}