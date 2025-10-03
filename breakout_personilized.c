/**
 * @file breakout_personilized.c
 * @brief Advanced Breakout Game Implementation with Collision-Based Interpolation
 * 
 * This implementation features precise ball movement using ray casting and collision prediction,
 * smooth interpolation between collision points, and optimized collision detection system.
 * 
 * Key Features:
 *      - Ray casting for collision prediction
 *      - Collision-based interpolation for smooth movement
 *      - AABB collision detection for blocks
 *      - Precise wall, bar, and block collision handling
 * 
 * Author: 
 *      Lorang Strand, with some help from github copilot.
 */



// ===== GAME CONFIGURATION =====
#define NCOLS               5       // <- Ensure that height % (NCOLS * TILE_WIDTH) == 0
#define NROWS               16      // <- Ensure that width > (NROWS * TILE_HEIGHT)
#define TILE_HEIGHT         15
#define TILE_WIDTH          15 
#define SCALE_FACTOR        1000    // Fixed-point scaling for precise calculations
#define GAME_DELAY          5000    // Delay (clk) cycles for game speed control
#define BACKGROUND_COLOR    black
#define BLOCK_AREA_START_X  (width - (NCOLS * TILE_WIDTH))
static const unsigned long long __attribute__((used)) VGAaddress = 0xc8000000; // Memory storing pixels
static const unsigned long long __attribute__((used)) UART_BASE = 0xFF201000;            // JTAG UART base address

static const unsigned short height = 240;                               // Screen dimensions (required by platform)
static const unsigned short width = 320;

static char *lost = "\n===== You Lost =====\n";
static char *won =  "\n===== You Won =====\n";

static const unsigned int __attribute__((used)) red = 0x0000F0F0;       // Color code for red using 5-6-5 format       
static const unsigned int __attribute__((used)) green = 0x00000F0F;         
static const unsigned int __attribute__((used)) blue = 0x000000FF;          
static const unsigned int __attribute__((used)) white = 0x0000FFFF;         
static const unsigned int __attribute__((used)) black = 0x0;                
static const unsigned int __attribute__((used)) yellow = 0x0000FFE0;        
static const unsigned int __attribute__((used)) magenta = 0x0000F81F;       
static const unsigned int __attribute__((used)) cyan = 0x000007FF; 

static const unsigned int block_colors[6] = {red, green, blue, yellow, magenta, cyan};      // Array of colors for the blocks (used in round-robin fashion)



// Bar Configuration
#define BAR_X_POS           10
#define BAR_WIDTH           7
#define BAR_CENTER_HEIGHT   15
#define BAR_EDGE_HEIGHT     15
#define BAR_SPEED           15
#define BAR_COLOR_EDGES     red
#define BAR_COLOR_CENTER    green

// Ball Configuration
#define BALL_RADIUS         3
#define BALL_SPEED          1       // Pixels per frame progress
#define BALL_INIT_DIR_X     707     // Initial direction vector (normalized to SCALE_FACTOR)
#define BALL_INIT_DIR_Y     500
#define BALL_COLOR          blue


// ===== TYPE DEFINITIONS =====
/**
 * @brief Represents the current state of the game
 */
typedef enum _gameState
{
    Stopped = 0,
    Running = 1,
    Won = 2,
    Lost = 3,
    Exit = 4,
} GameState;

/**
 *  @brief Represents a single tile/block in the game
 */
typedef struct _tile
{
    unsigned char destroyed;        // 1 if the block is destroyed, 0 otherwise
    unsigned int pos_x;             // Top-left x position of the block
    unsigned int pos_y;             // Top-left y position of the block
    unsigned int color;            
} Tile;

/**
 * @brief Represents the player's bar
 */
typedef struct _bar
{
    unsigned int pos_x;             // x-coordinate of the left of the bar (constant)
    unsigned int pos_y;             // y-coordinate of the top of the bar
} Bar;


/**
 * @brief Types of collision objects in the game
 */
typedef enum {
    COLLISION_NONE = 0,
    COLLISION_WALL = 1,
    COLLISION_BAR = 2,
    COLLISION_BLOCK = 3
} CollisionType;

/**
 * @brief Specific wall that was hit during collision
 */
typedef enum {
    WALL_NONE = 0,
    WALL_LEFT = 1,
    WALL_RIGHT = 2,
    WALL_TOP = 3,
    WALL_BOTTOM = 4
} WallHit;

/**
 * @brief Represents the ball in the game
 */
typedef struct _ball
{
    unsigned int pos_x;       // Current visual position
    unsigned int pos_y;
    signed int dir_x;         // x component of the ball's direction (unit vector scaled by SCALE_FACTOR)
    signed int dir_y;         // y component of the ball's direction (unit vector scaled by SCALE_FACTOR)
    unsigned int next_x;      // Next collision position
    unsigned int next_y;
    unsigned int start_x;     // Starting position for interpolation
    unsigned int start_y;
    int progress;             // Progress from start to next (0 to SCALE_FACTOR)
    int total_distance;       // Total distance from start to next collision
    CollisionType collision_type;   // Type of collision detected
    WallHit wall_hit;              // Specific wall that was hit (for wall collisions)
    unsigned int hit_block_row;        // Row of block that was hit (for block collisions). -1 if no block was hit
    unsigned int hit_block_col;        // Column of block that was hit (for block collisions). -1 if no block was hit
} Ball;



// ==== FUNCTION PROTOTYPES =====
void ClearScreen(unsigned int color);
void SetPixel(unsigned int x_coord, unsigned int y_coord, unsigned int color);
void draw_block(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color);
void game_delay(int cycles);
void DrawBar();      
void initialize_bar();
int ReadUart();
void WriteUart(char c);
void draw_ball();
void erase_ball(unsigned int x, unsigned int y);
void initialize_ball();
void print_ball();
void draw_playing_field();
void initilize_board_tiles();
void reset();
void play();
void wait_for_start();
void update_game_state();
void update_bar_state();
void update_ball_state();
void calculate_next_collision();
void interpolate_ball_position();
void handle_ball_collisions();
void clear_uart();
void write(char *str);
void write_int(int value);
void write_debug(char *msg, int value);
int abs(int value); // Custom abs function to avoid using math.h


// Initillize global variables
GameState currentState;
Tile board_tiles[NROWS][NCOLS] __attribute__((used)); 
Ball ball;
Bar bar;


/*
    Sets the entire screen to a specific color.
    Assumes colour in RGB888 format (0xRRGGBB).
    Uses the SetPixel assembly function to set each pixel.
    Arguments: color - color to fill the screen with (unsigned int, RGB565 format)
    Returns: None
*/
void ClearScreen(unsigned int color)
{
    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++)
        {
            SetPixel(x, y, color);
        }
    }
}

/*
    Sets a pixel at (x_coord, y_coord) to the specified color.
    Arguments:
    - x_coord: x-coordinate (unsigned int)
    - y_coord: y-coordinate (unsigned int)
    - color: color
    Returns: None
*/

asm("SetPixel: \n\t"
    "LDR R3, =VGAaddress \n\t"
    "LDR R3, [R3] \n\t"

    // Calculate address: base + (y * 320 + x) * 2
    "LSL R1, R1, #10 \n\t"
    "LSL R0, R0, #1 \n\t"
    "ADD R1, R0 \n\t"
    "STRH R2, [R3,R1] \n\t"

    "BX LR \n\t"
    "   .ltorg \n\t"                   // Force literal pool here
);


/*
    Reads a character from the JTAG UART.
    Arguments: None
    Returns:
    - R0: character read (unsigned int, only the lowest byte is used)
*/
asm("ReadUart:\n\t"
    "LDR R1, =UART_BASE \n\t"
    "LDR R1, [R1] \n\t"
    "LDR R0, [R1] \n\t"
    "BX LR \n\t"
    "   .ltorg \n\t"                   // Force literal pool here
);


/*
    Writes a character to the JTAG UART.
    Waits for the UART to be ready before writing.
    Arguments:
    - R0: character to write (unsigned int, only the lowest byte is used)
    Returns: None
*/

asm("WriteUart:\n\t"
    "LDR R1, =UART_BASE \n\t"           // Load UART base address
    "LDR R1, [R1] \n\t"                // Dereference to get actual address
    "MOV R2, R0 \n\t"                  // Save character to write in R2
    "write_wait_loop: \n\t"            // Loop label for waiting
    "LDR R0, [R1, #4] \n\t"           // Read control register (base + 4)
    "LSR R3, R0, #16 \n\t"            // Shift right by 16 to get bits 31-16 in lower bits
    "CMP R3, #0 \n\t"                 // Compare with 0
    "BEQ write_wait_loop \n\t"        // If zero (no space available), keep waiting
    "STR R2, [R1] \n\t"               // Write character to data register
    "BX LR \n\t"                       // Return
    "   .ltorg \n\t"                   // Force literal pool here
);

/*
    Writes a string to the JTAG UART. Iterates until it finds the null-termination character.
    Arguments:
    - str: pointer to the string to write (char*)
    Returns: None
*/
void write(char *str)
{
    if (!str) return; // Null pointer check
    
    while (*str)
    {
        WriteUart(*str++);
    }
}

/*
    Clears the UART display
    Arguements: None
    Returns: None
*/
void clear_uart()
{
    write("\033[2J\033[H"); // ANSI escape codes to clear screen and move cursor to home position
}

/*
    Writes an integer to the UART by converting it to a string.
    Arguments:
    - value: integer value to write
    Returns: None
*/
void write_int(int value)
{
    char buffer[12]; // Enough for 32-bit int (-2147483648 to 2147483647)
    char *ptr = buffer + sizeof(buffer) - 1;
    int is_negative = 0;
    
    *ptr = '\0'; // Null terminator
    
    // Handle negative numbers
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    
    // Handle zero case
    if (value == 0) {
        *(--ptr) = '0';
    }
    
    // Convert digits (reverse order)
    while (value > 0) {
        *(--ptr) = '0' + (value % 10);
        value /= 10;
    }
    
    // Add negative sign if needed
    if (is_negative) {
        *(--ptr) = '-';
    }
    
    write(ptr);
}

/*
    Writes a debug message followed by an integer value.
    Arguments:
    - msg: debug message string
    - value: integer value to display
    Returns: None
*/
void write_debug(char *msg, int value)
{
    write(msg);
    write_int(value);
    write("\n");
}

/*
    Simple delay function to control game frame rate.
    Arguments:
    - cycles: number of delay cycles (higher = longer delay)
    Returns: None
*/
void game_delay(int cycles)
{
    for (volatile int i = 0; i < cycles; i++);
}

int abs(int value)
{
    return (value < 0) ? -value : value;
}

/*
    Draws a block at position (x,y) with width w and height h.
    The block is drawn with a border of BORDER_WIDTH and color BORDER_COLOR.
    The inside of the block is filled with the given color.
    Arguments:
    - x: x-coordinate of the top-left corner of the block (unsigned int)
    - y: y-coordinate of the top-left corner of the block (unsigned int)
    - w: width of the block (unsigned int)
    - h: height of the block (unsigned int)
    - color: color to fill the block with (unsigned int, RGB888 format)
    Returns: None


    draw_block(10, 10, 6, 4, red, 0) will draw:

        r r r r r r
        r r r r r r
        r r r r r r
        r r r r r r
    
    where 'r' is the given color (red in this case). 
*/
void draw_block
(
    unsigned int x,         // x-coordinate of the top-left corner of the block
    unsigned int y,         // y-coordinate of the top-left corner of the block
    unsigned int w,         // width of the block
    unsigned int h,         // height of the block
    unsigned int color      // color to fill the block with (unsigned int, RGB888 format)
)
{
    if (x + w > width || y + h > height) 
    {
        write_debug("draw_block out of bounds: x=", x);
        write_debug(" y=", y);
        write_debug(" w=", w);
        write_debug(" h=", h);
        return; // Out of bounds, do nothing
    }
    unsigned int i, j;

    // Fill the entire block with the given color
    for (i = 0; i < h; i++){
        for (j = 0; j < w; j++){
            SetPixel(x + j, y + i, color);
        }
    }

}

/*
    Draws the bar at the current position stored in the bar struct.
    The bar is drawn with a border of BORDER_WIDTH and color BAR_COLOR_EDGES.
    The inside of the bar is filled with BAR_COLOR_CENTER.
    Arguments: None
    Returns: None
*/
void DrawBar()
{
    // Draw top part of Bar
    draw_block
    (
        bar.pos_x, 
        bar.pos_y, 
        BAR_WIDTH, 
        BAR_EDGE_HEIGHT, 
        BAR_COLOR_EDGES
    );    
    
    // Draw center part of Bar
    draw_block
    (
        bar.pos_x, 
        bar.pos_y + BAR_EDGE_HEIGHT, 
        BAR_WIDTH, 
        BAR_CENTER_HEIGHT, 
        BAR_COLOR_CENTER
    );     
    
    // Draw bottom part of Bar
    draw_block
    (
        bar.pos_x, 
        bar.pos_y + BAR_EDGE_HEIGHT + BAR_CENTER_HEIGHT, 
        BAR_WIDTH, 
        BAR_EDGE_HEIGHT, 
        BAR_COLOR_EDGES
    ); 
     
}


/*
    Initializes the bar position to the center of the screen.
    Arguments: None
    Returns: None
*/
void initialize_bar()
{
    bar.pos_x = BAR_X_POS;
    bar.pos_y = (unsigned int)(height/2 - (BAR_CENTER_HEIGHT/2 + BAR_EDGE_HEIGHT));
}

/*
    Reads all characters in the UART Buffer and apply the respective bar position updates
    Arguments: None
    Returns: None
*/
void update_bar_state()
{
    int remaining = 0;
    do
    {
        unsigned long long out = ReadUart();

        // not valid - abort reading
        if (!(out & 0x8000))
        {
            return;
        }

        if ((out & 0xFF) == 0x77) // 'w' - move up
        {
            if (bar.pos_y >= BAR_SPEED) bar.pos_y -= BAR_SPEED;
            else bar.pos_y = 0;
            
            // draw the space where the bar was previously 
            draw_block
            (
                bar.pos_x, 
                bar.pos_y + BAR_EDGE_HEIGHT + BAR_CENTER_HEIGHT + BAR_EDGE_HEIGHT, 
                BAR_WIDTH, 
                BAR_SPEED, 
                BACKGROUND_COLOR
            );

            DrawBar();
            if (ball.dir_x < 0) calculate_next_collision(); // recalculate ball trajectory only if ball is moving towards the bar      
        }
        else if ((out & 0xFF) == 0x73) // 's' - move down
        {
            // draw the space where the bar was previously
            draw_block
            (
                bar.pos_x, 
                bar.pos_y , 
                BAR_WIDTH, 
                BAR_SPEED, 
                BACKGROUND_COLOR
            );

            if (bar.pos_y <= ((height - BAR_CENTER_HEIGHT - BAR_EDGE_HEIGHT*2) - BAR_SPEED)) bar.pos_y += BAR_SPEED;
            else bar.pos_y = height - BAR_CENTER_HEIGHT - BAR_EDGE_HEIGHT - BAR_EDGE_HEIGHT;

            DrawBar();
            if (ball.dir_x < 0) calculate_next_collision(); // recalculate ball trajectory only if ball is moving towards the bar
        }
        remaining = (out & 0xFF0000) >> 4;
    } while (remaining > 0);
}

/*
    Draws a ball at the center of the screen.
    BALL_SIZE defines the width and height of the ball.
    Center of the ball is at (width/2, height/2) and indicates the center of the ball.
    Arguments: None
    Returns: None

    Example for BALL_SIZE = 7:
          +      
        + + +      
      + + + + +   
    + + + + + + +   
      + + + + +
        + + +
          +
*/
void draw_ball()
{
    for (int i = -BALL_RADIUS; i <= BALL_RADIUS; i++)
    {
        for (int j = -BALL_RADIUS; j <= BALL_RADIUS; j++)
        {
            if (abs(i) + abs(j) <= BALL_RADIUS && 
                ball.pos_x + j < width && 
                ball.pos_y + i < height
            ) 
            {
                SetPixel(ball.pos_x + j, ball.pos_y + i, BALL_COLOR);
            }
        }
    }
}


/**
 *   Erases the ball given by (x, y) coordinates over it with the background color.
    Arguments: None
    Returns: None
 */
void erase_ball(unsigned int x, unsigned int y)
{
    for (int i = -BALL_RADIUS; i <= BALL_RADIUS; i++)
    {
        for (int j = -BALL_RADIUS; j <= BALL_RADIUS; j++)
        {
            if (abs(i) + abs(j) <= BALL_RADIUS &&
                ball.pos_x + j < width && 
                ball.pos_y + i < height
            ) 
            {
                SetPixel(x + j, y + i, BACKGROUND_COLOR);
            }
        }
    }
}
/*
    Prints the current state of the ball to the UART for debugging purposes.
    Arguments: None
    Returns: None
*/
void print_ball()
{
    write_debug("Ball Pos:\t", ball.pos_x);
    write_debug("\t\t", ball.pos_y);
    write_debug("Direction:\t", ball.dir_x);
    write_debug("\t\t", ball.dir_y);
    write_debug("Next:\t\t", ball.next_x);
    write_debug("\t\t", ball.next_y);
    write_debug("Start:\t\t", ball.start_x);
    write_debug("\t\t", ball.start_y);
    write_debug("Progress:\t", ball.progress);
    write_debug("Total Distance:\t", ball.total_distance);
    write_debug("Collision Type:\t", ball.collision_type);
    write_debug("Hit Block Row:\t", ball.hit_block_row);
    write_debug("Hit Block Col:\t", ball.hit_block_col);
    write_debug("Wall Hit:\t", ball.wall_hit);
    write("\n");
}

/*
    Initializes the ball position to the center of the screen and sets its initial direction to the right.
    Arguments: None
    Returns: None
*/
void initialize_ball()
{
    ball.pos_x = BAR_X_POS + BAR_WIDTH + BALL_RADIUS;
    ball.pos_y = (unsigned int)(height/2);
    ball.dir_x = BALL_INIT_DIR_X;
    ball.dir_y = BALL_INIT_DIR_Y;
    
    // Initialize interpolation system
    calculate_next_collision();
}


/*
    Calculates the next collision point from the current ball position using ray casting.
    This function traces the ball's path until it hits a wall, block, or bar.
    Arguments: None
    Returns: None
*/
void calculate_next_collision()
{
    // Store starting position for interpolation
    ball.start_x = ball.pos_x;
    ball.start_y = ball.pos_y;
    
    // Reset collision info
    ball.collision_type = COLLISION_NONE;
    ball.wall_hit = WALL_NONE;
    ball.hit_block_row = -1;
    ball.hit_block_col = -1;
    
    // Current position for ray casting (using fixed-point for precision)
    int current_x = ball.pos_x * SCALE_FACTOR;
    int current_y = ball.pos_y * SCALE_FACTOR;
    
    // Direction step - use larger steps for efficiency
    int step_x = ball.dir_x * BALL_SPEED;  
    int step_y = ball.dir_y * BALL_SPEED; 
    
    
    int iterations = 0;
    // Limit iterations to avoid infinite loops in case of errors
    while (iterations < SCALE_FACTOR)
    {
        // Advance position by one step
        current_x += step_x;
        current_y += step_y;
        iterations++;
        
        // Convert back to integer coordinates for collision checking
        int check_x = current_x / SCALE_FACTOR;
        int check_y = current_y / SCALE_FACTOR;
        
        // Check if ball goes out of bounds 
        if (check_x - BALL_RADIUS < 0)
        {
            ball.collision_type = COLLISION_WALL;
            ball.wall_hit = WALL_LEFT;
            break;
        }
        if (check_x + BALL_RADIUS > width)
        {
            ball.collision_type = COLLISION_WALL;
            ball.wall_hit = WALL_RIGHT;
            break;
        }
        if (check_y - BALL_RADIUS < 0) 
        {
            ball.collision_type = COLLISION_WALL;
            ball.wall_hit = WALL_TOP;
            break;
        }
        if (check_y + BALL_RADIUS > height)
        {
            ball.collision_type = COLLISION_WALL;
            ball.wall_hit = WALL_BOTTOM;
            break;
        }
        
        // Check bar collision
        if (check_x - BALL_RADIUS < bar.pos_x + BAR_WIDTH &&
            check_x + BALL_RADIUS > bar.pos_x &&
            check_y + BALL_RADIUS > bar.pos_y &&
            check_y - BALL_RADIUS < bar.pos_y + BAR_EDGE_HEIGHT + BAR_CENTER_HEIGHT + BAR_EDGE_HEIGHT)
        {
            ball.collision_type = COLLISION_BAR;
            break; 
        }
        
        // Check block collisions - only if current  iteration is in block area
        if (check_x + BALL_RADIUS >= BLOCK_AREA_START_X)
        {
            // Check rightmost point of the ball for block collision
            int ball_right_col = (check_x + BALL_RADIUS - BLOCK_AREA_START_X) / TILE_WIDTH;
            int ball_right_row = check_y  / TILE_HEIGHT;
            
            if (ball_right_col >= 0 && ball_right_col < NCOLS && 
                ball_right_row >= 0 && ball_right_row < NROWS &&
                !board_tiles[ball_right_row][ball_right_col].destroyed)
            {
                ball.collision_type = COLLISION_BLOCK;
                ball.hit_block_row = ball_right_row;
                ball.hit_block_col = ball_right_col;
                break; 
            }

            // Check leftmost point of the ball for block collision
            int ball_left_col = (check_x - BALL_RADIUS - BLOCK_AREA_START_X) / TILE_WIDTH;
            int ball_left_row = check_y / TILE_HEIGHT;

            if (ball_left_col >= 0 && ball_left_col < NCOLS && 
                ball_left_row >= 0 && ball_left_row < NROWS &&
                !board_tiles[ball_left_row][ball_left_col].destroyed)
            {
                ball.collision_type = COLLISION_BLOCK;
                ball.hit_block_row = ball_left_row;
                ball.hit_block_col = ball_left_col;
                break; 
            }

            // Check topmost point of the ball for block collision
            int ball_top_col = check_x / TILE_WIDTH;
            int ball_top_row = (check_y - BALL_RADIUS) / TILE_HEIGHT;

            if (ball_top_col >= 0 && ball_top_col < NCOLS && 
                ball_top_row >= 0 && ball_top_row < NROWS &&
                !board_tiles[ball_top_row][ball_top_col].destroyed)
            {
                ball.collision_type = COLLISION_BLOCK;
                ball.hit_block_row = ball_top_row;
                ball.hit_block_col = ball_top_col;
                break; 
            }

            // Check bottommost point of the ball for block collision
            int ball_bottom_col = check_x / TILE_WIDTH;
            int ball_bottom_row = (check_y + BALL_RADIUS) / TILE_HEIGHT;

            if (ball_bottom_col >= 0 && ball_bottom_col < NCOLS && 
                ball_bottom_row >= 0 && ball_bottom_row < NROWS &&
                !board_tiles[ball_bottom_row][ball_bottom_col].destroyed)
            {
                ball.collision_type = COLLISION_BLOCK;
                ball.hit_block_row = ball_bottom_row;
                ball.hit_block_col = ball_bottom_col;
                break; 
            }    
        }
    }
    
    // Store the collision point
    ball.next_x = current_x / SCALE_FACTOR;
    ball.next_y = current_y / SCALE_FACTOR;
    
    // Calculate total distance for interpolation (in pixels)
    int dx = ball.next_x - ball.start_x;
    int dy = ball.next_y - ball.start_y;

    // using eulers method to estimate sqrt(dx² + dy²) with integer arithmetic
    ball.total_distance = SCALE_FACTOR / 2;                 // Initial guess
    int target_distance_squared = dx * dx + dy * dy;
    int prev_distance = 0;
    for (int i = 0; i < 10; i++) {
        ball.total_distance = (ball.total_distance + target_distance_squared / ball.total_distance) / 2;
        if (abs(ball.total_distance - prev_distance) < 2) {
            break; // Stop if change is minimal
        }
        prev_distance = ball.total_distance;
    }

    
    // Reset progress
    ball.progress = 0;
}

/*
    Interpolates the ball's visual position between start and next collision points.
    Arguments: None
    Returns: None
*/
void interpolate_ball_position()
{
    if (ball.total_distance <= 0)
    {
        return; // No movement needed
    }
    
    if (ball.progress >= ball.total_distance)
    {
        ball.pos_x = ball.next_x;
        ball.pos_y = ball.next_y;
        return;         // Already at or past the collision point
    }
    
    // Calculate differences
    int dx = ball.next_x - ball.start_x;
    int dy = ball.next_y - ball.start_y;
    
    // Interpolate position based on progress
    ball.pos_x = ball.start_x + (dx * ball.progress) / ball.total_distance;
    ball.pos_y = ball.start_y + (dy * ball.progress) / ball.total_distance;
}

/*
    Handles collision logic when the ball reaches its calculated collision point.
    This function determines what was hit and updates the direction accordingly.
    Arguments: None
    Returns: None
*/
void handle_ball_collisions()
{
    // Use recorded collision type to determine response
    switch (ball.collision_type)
    {
    case COLLISION_WALL:
        switch (ball.wall_hit)
        {
            case WALL_LEFT:
                ball.dir_x = -ball.dir_x; 
                ball.pos_x = BALL_RADIUS; // Prevent going out of bounds
                break;
            case WALL_RIGHT:
                ball.dir_x = -ball.dir_x; 
                ball.pos_x = width - BALL_RADIUS; // Prevent going out of bounds
                break;
            case WALL_TOP:
                ball.dir_y = -ball.dir_y; 
                ball.pos_y = BALL_RADIUS; // Prevent going out of bounds
                break;
            case WALL_BOTTOM:
                ball.dir_y = -ball.dir_y; 
                ball.pos_y = height - BALL_RADIUS; // Prevent going out of bounds
                break;
            default:
                break;
        }
        break;

    case COLLISION_BAR:
        {
            // Calculate where on the bar the ball hit (relative to bar center)
            int bar_total_height = BAR_EDGE_HEIGHT + BAR_CENTER_HEIGHT + BAR_EDGE_HEIGHT;
            int bar_center_y = bar.pos_y + bar_total_height / 2;
            
            // Hit position relative to center (positive = below center, negative = above center)
            int hit_offset = ball.pos_y - bar_center_y;
            int max_offset = bar_total_height / 2;
            
            // Map hit position to angle range [-60°, +60°]
            // Normalize to [-866, +866] where 866 ≈ sin(60°) * SCALE_FACTOR
            int angle_factor = (hit_offset * 866) / max_offset;
            
            // Clamp to ±60 degrees
            if (angle_factor > 866) angle_factor = 866;
            if (angle_factor < -866) angle_factor = -866;
            
            // Set Y direction: positive = downward, negative = upward
            ball.dir_y = angle_factor;
            
            // Calculate X direction to maintain unit vector magnitude (SCALE_FACTOR)
            // We want: sqrt(dir_x² + dir_y²) = SCALE_FACTOR
            // So: dir_x = sqrt(SCALE_FACTOR² - dir_y²)
            
            int dir_y_squared = (ball.dir_y * ball.dir_y) / SCALE_FACTOR; // Scale down to prevent overflow
            int target_x_squared = SCALE_FACTOR - dir_y_squared;
            
            // Newton's method to calculate sqrt(target_x_squared * SCALE_FACTOR)
            int dir_x_magnitude = SCALE_FACTOR / 2; // Initial guess
            int prev_x_magnitude = 0;
            for (int i = 0; i < 10; i++) {
                dir_x_magnitude = (dir_x_magnitude + (target_x_squared * SCALE_FACTOR) / dir_x_magnitude) / 2;
                if (abs(dir_x_magnitude - prev_x_magnitude) < 2) {
                    break; // Stop if change is minimal
                }
                prev_x_magnitude = dir_x_magnitude;
            }
        
            ball.dir_x = dir_x_magnitude;
            
            // Correct position to prevent ball from going into the bar
            ball.pos_x = bar.pos_x + BAR_WIDTH + BALL_RADIUS;
        }
        break;

    case COLLISION_BLOCK:
        // Verify we have a valid block collision recorded
        if (ball.hit_block_row < 0 || ball.hit_block_col < 0 || 
            ball.hit_block_row >= NROWS || ball.hit_block_col >= NCOLS)
        {
            write_debug("Invalid block collision at row ", ball.hit_block_row);
            write_debug(" and col ", ball.hit_block_col);
            return; // No valid block collision recorded
        }
        
        // Check if the block is already destroyed
        if (board_tiles[ball.hit_block_row][ball.hit_block_col].destroyed)
        {
            write_debug("Block already destroyed at row ", ball.hit_block_row);
            write_debug(" and col ", ball.hit_block_col);
            return; // Block already destroyed
        }
        
        // Get the block that was hit
        int row = ball.hit_block_row;
        int col = ball.hit_block_col;

        board_tiles[row][col].destroyed = 1;
        
        // Erase the block by drawing over it with the background color
        draw_block(
            board_tiles[row][col].pos_x, 
            board_tiles[row][col].pos_y, 
            TILE_WIDTH, 
            TILE_HEIGHT, 
            BACKGROUND_COLOR
        );

        // Get block boundaries
        int block_left = board_tiles[row][col].pos_x;
        int block_right = board_tiles[row][col].pos_x + TILE_WIDTH;
        int block_top = board_tiles[row][col].pos_y;
        int block_bottom = board_tiles[row][col].pos_y + TILE_HEIGHT;

        // Calculate ball bounding box for collision side determination
        int ball_left = ball.pos_x - BALL_RADIUS;
        int ball_right = ball.pos_x + BALL_RADIUS;
        int ball_top = ball.pos_y - BALL_RADIUS;
        int ball_bottom = ball.pos_y + BALL_RADIUS;
        
        // Determine collision side based on overlap amounts
        int overlap_left = ball_right - block_left;
        int overlap_right = block_right - ball_left;
        int overlap_top = ball_bottom - block_top;
        int overlap_bottom = block_bottom - ball_top;
        
        // Find minimum overlap to determine primary collision side
        int min_overlap = overlap_left;
        WallHit collision_side = WALL_LEFT;
        
        if (overlap_right < min_overlap) {
            min_overlap = overlap_right;
            collision_side = WALL_RIGHT;
        }
        if (overlap_top < min_overlap) {
            min_overlap = overlap_top;
            collision_side = WALL_TOP;
        }
        if (overlap_bottom < min_overlap) {
            min_overlap = overlap_bottom;
            collision_side = WALL_BOTTOM;
        }
        
        switch (collision_side) {
            case WALL_LEFT:
                ball.dir_x = -ball.dir_x; 
                ball.pos_x = block_left - BALL_RADIUS;  // Prevent sticking into the block
                break;
            case WALL_RIGHT: 
                ball.dir_x = -ball.dir_x; 
                ball.pos_x = block_right + BALL_RADIUS; 
                break;
            case WALL_TOP:
                ball.dir_y = -ball.dir_y; 
                ball.pos_y = block_top - BALL_RADIUS; 
                break;
            case WALL_BOTTOM:
                ball.dir_y = -ball.dir_y; 
                ball.pos_y = block_bottom + BALL_RADIUS; 
                break;
            default:
                break;
        }
        break;
    default:
        break;
    }
}

/*
    Updates the ball position based on its current direction, speed and collisions.
    Arguments: None
    Returns: None
*/
void update_ball_state()
{

    int prev_x = ball.pos_x;
    int prev_y = ball.pos_y;

    // Advance progress towards next collision
    ball.progress += BALL_SPEED;
    
    // Check if the ball has reached the collision point
    if ((ball.progress + BALL_RADIUS) >= ball.total_distance)
    {
        // Move to collision point
        ball.pos_x = ball.next_x;
        ball.pos_y = ball.next_y;

        // Handle the collision and change direction
        handle_ball_collisions();
        
        // Calculate next collision from this new position
        calculate_next_collision();

    }
    else
    {
        // Interpolate position between start and next collision for smooth movement
        interpolate_ball_position();
    }

    erase_ball(prev_x, prev_y); // Erase ball at previous position
    draw_ball();                // Draw ball at new position
}



/*
    Initilizes the board_tiles array with the correct positions and give each block a different color.
    Arguments: None
    Returns: None
*/
void initilize_board_tiles()
{
    for (int row = 0; row < NROWS; row++)
    {
        for (int col = 0; col < NCOLS; col++)
        {
            board_tiles[row][col].pos_x = width - (TILE_WIDTH * (NCOLS - col));
            board_tiles[row][col].pos_y = height - (TILE_HEIGHT * (NROWS - row));
            board_tiles[row][col].color = block_colors[(col + row) % 6];  
            board_tiles[row][col].destroyed = 0;
        }
    }
}



/*
    Draws the playing field by iterating through the board_tiles array and drawing each block.
    Gives colors to each of the blocks.
    Arguments: None
    Returns: None
*/
void draw_playing_field()
{
    for (int row = 0; row < NROWS; row++)
    {
        for (int col = 0; col < NCOLS; col++)
        {
            draw_block
            (
                board_tiles[row][col].pos_x,
                board_tiles[row][col].pos_y,
                TILE_WIDTH,
                TILE_HEIGHT,
                board_tiles[row][col].color
            );   
        }
    }
}

/*
    Updates the game state based on win/loss conditions.
    Arguments: None
    Returns: None
*/
void update_game_state()
{
    if (currentState != Running)
    {
        return;
    }

    // Game is lost if the ball goes past the left edge (behind the bar)
    if (ball.pos_x - BALL_RADIUS <= 0) {
        currentState = Lost;
        return;
    }
    
    // Game is won if the ball reaches the right side of the screen
    if (ball.pos_x + BALL_RADIUS >= width) {
        currentState = Won;
        return;
    }
}


/*
    Resets the game state to initial conditions.
    Waits for the user to press SPACE to start the game or ENTER to exit.
    Arguments: None
    Returns: None
*/
void reset()
{
    // Intialize global variables
    initilize_board_tiles();
    initialize_bar();
    initialize_ball();

    // Draw initial screen
    ClearScreen(BACKGROUND_COLOR);
    DrawBar();
    draw_ball();
    draw_playing_field();

    currentState = Stopped;
    while (1)
    {
        unsigned long long out = ReadUart();
        if (out & 0x8000) // Check if data is valid
        {
            char c = out & 0xFF; // Extract the character
            if (c == 0x20) // SPACE key
            {
                currentState = Running;
                return;
            }
            else if (c == 0x0A) // ENTER key
            {
                currentState = Exit;
                return;
            }
        }
    }
}

/*
    Waits for the user to press SPACE to start the game.
    If the user presses ENTER, the game state is set to Exit.
    Arguments: None
    Returns: None
*/
void wait_for_start()
{
    // Intialize global variables
    clear_uart();
    ClearScreen(BACKGROUND_COLOR);
    initilize_board_tiles();
    initialize_bar();
    initialize_ball();

    // Draw initial screen
    DrawBar();
    draw_ball();
    draw_playing_field();

    currentState = Stopped;
    write("Press SPACE to start, ENTER to exit.\n");
    while (1)
    {
        unsigned long long out = ReadUart();
        if (out & 0x8000) // Check if data is valid
        {
            char c = out & 0xFF; // Extract the character
            if (c == 0x20) // SPACE key
            {
                currentState = Running;
                return;
            }
            else if (c == 0x0A) // ENTER key
            {
                currentState = Exit;
                return;
            }
        }
    }
}

/*
    Main game loop that continues until the game ends (win/loss).
    It updates the game state, bar position, and ball position in each iteration.
    Arguments: None
    Returns: None
*/
void play()
{
    while (1)
    {
        update_game_state();        // Check win/loss conditions
        
        if (currentState != Running)
        {
            break;  
        }

        update_bar_state();         
        
        update_ball_state();        
        
        game_delay(GAME_DELAY); // Adjust this value to make game faster/slower
    }
    
    // Game has ended - display appropriate message
    if (currentState == Won)
    {
        write(won);
        write("\nPress SPACE to play again or ENTER to exit.\n");
    }
    else if (currentState == Lost)
    {
        write(lost);
        write("\nPress SPACE to play again or ENTER to exit.\n");
    }
    
    // Set state to stopped so the main loop can handle restart/exit
    currentState = Stopped;
}


/*
    Entry point of the program. Initializes the game and enters the main loop.
    Arguments:
    - argc: argument count (not used)
    - argv: argument vector (not used)
    Returns:
    - int: exit status (0 for success)
*/
int main(int argc, char *argv[])
{
    wait_for_start();
    while (1)
    {
        play();
        reset();
        if (currentState == Exit)
        {
            break;
        }
    } 
    clear_uart();
    write("GoodBye. Thanks for playing! Hope you had fun :)\n");
    return 0;
}
