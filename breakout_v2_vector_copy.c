#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

/*
    Some not for myself.
    - How are errorhandling done in this project, and embedded system in general?
    - Is assert.h allowed to be used in this project?
    - If assert.h is allowed, should I use it in the assembly functions as well?
    - Why are the won/lost strings, height, width and font8x8 not defined as constants?
    - Why is the colors defined as variables and not constants?
    - Is it ok to redefine the colors as static constants?
    - What is the font8x8 variable used for?
    - What is the tiles variable used for and why is it of type unsigned char, and not as an array of Block types?
    - What is the reason for combining both snake_case and CamelCase in the same project?
    - Why can I not split the function definitions and declarations into separate files?
    - I find the sceleton code restricing, and I would like to implement more features, is this ok?

*/

/***************************************************************************************************
 * DON'T REMOVE THE VARIABLES BELOW THIS COMMENT                                                   *
 **************************************************************************************************/
static const unsigned long long __attribute__((used)) VGAaddress = 0xc8000000; // Memory storing pixels
static const unsigned int __attribute__((used)) red = 0x0000F0F0;           // Redefined as static constant.
static const unsigned int __attribute__((used)) green = 0x00000F0F;         // Redefined as static constant.
static const unsigned int __attribute__((used)) blue = 0x000000FF;          // Redefined as static constant.
static const unsigned int __attribute__((used)) yellow = 0x0000FFE0;        // Redefined as static constant.
static const unsigned int __attribute__((used)) magenta = 0x0000F81F;       // Redefined as static constant.
static const unsigned int __attribute__((used)) cyan = 0x000007FF;          // Redefined as static constant.
static const unsigned int __attribute__((used)) white = 0x0000FFFF;         // Redefined as static constant.
static const unsigned int __attribute__((used)) black = 0x0;                // Redefined as static constant.

// Don't change the name of this variables
#define NCOLS 10 // <- Supported value range: [1,18]
#define NROWS 16 // <- This variable might change.
#define TILE_SIZE 15 // <- Tile size, might change.

static char *won = "\n======= You Won ========\n";       // DON'T TOUCH THIS - keep the string as is     WHY IS THIS NOT A CONSTANT???
static char *lost = "\n====== You Lost ========\n";     // DON'T TOUCH THIS - keep the string as is     WHY IS THIS NOT A CONSTANT???
const unsigned short height = 240;        // DON'T TOUCH THIS - keep the value as is      WHY IS THIS NOT A CONSTANT???
const unsigned short width = 320;         // DON'T TOUCH THIS - keep the value as is      WHY IS THIS NOT A CONSTANT???
char font8x8[128][8];               // DON'T TOUCH THIS - this is a forward declaration  - -NO IDEA WHAT THIS IS USED FOR???
unsigned char tiles[NROWS][NCOLS] __attribute__((used)) = { 0 }; // DON'T TOUCH THIS - this is the tile map. NO IDEA WHAT THIS IS USED FOR ???
/**************************************************************************************************/

unsigned long long __attribute__((used)) UART_BASE = 0xFF201000; // JTAG UART base address

#define DUMMY               -1  // Dummy value to indicate no change
#define BACKGROUND_COLOR    black   // Background color of the playing field

#define BAR_X_POS           10      // X position of the bar
#define BAR_WIDTH           7       // Width of the bar
#define BAR_CENTER_HEIGHT   15      // Height of the center part of the bar
#define BAR_EDGE_HEIGHT     15      // Height of the edge parts of the bar
#define BAR_SPEED           15      // Number of pixels the bar moves up or down when receiving 'w' or 's' command
#define BAR_COLOR_EDGES     red     // Color of the edge parts of the bar
#define BAR_COLOR_CENTER    green   // Color of the center part of the bar

#define BALL_RADIUS         3       // Radius of the ball in pixels
#define BALL_SPEED          1       // Number of pixels the ball moves in its direction each frame
#define BALL_INIT_DIR_X     28378   // Initial x direction of the ball (unit vector scaled by SCALE_FACTOR)
#define BALL_INIT_DIR_Y     16384       // Initial y direction of the ball (unit vector scaled by SCALE_FACTOR)
#define SCALE_FACTOR        32767   // Scaling factor for fixed-point representation of direction vectors
#define BALL_COLOR          blue    // Color of the ball

typedef struct _tile
{
    unsigned char destroyed;
    unsigned int pos_x;
    unsigned int pos_y;
    unsigned int color;
} Tile;


typedef struct _bar
{
    unsigned int pos_x;         // x-coordinate of the left of the bar (constant)
    unsigned int pos_y;         // y-coordinate of the top of the bar
} Bar;

typedef struct _ball
{
    unsigned int pos_x;
    unsigned int pos_y;
    signed int dir_x;         // x component of the ball's direction (unit vector scaled by SCALE_FACTOR)
    signed int dir_y;         // y component of the ball's direction (unit vector scaled by SCALE_FACTOR)
} Ball;

typedef enum _gameState
{
    Stopped = 0,
    Running = 1,
    Won = 2,
    Lost = 3,
    Exit = 4,
} GameState;


// Array of colors for the blocks
static const unsigned int block_colors[6] = {red, green, blue, yellow, magenta, cyan};

/*
Function declarations for all functions. Both assembly and C.
*/
void ClearScreen(unsigned int color);
void SetPixel(unsigned int x_coord, unsigned int y_coord, unsigned int color);
void draw_block(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color);
void DrawBar(unsigned int y);
void initialize_bar();
int ReadUart();
void WriteUart(char c);
void draw_ball();
void initialize_ball();
void draw_playing_field();
void initilize_board_tiles();
void reset();
void play();
void wait_for_start();
void update_game_state();
void update_bar_state();
void update_ball_state();
void handle_block_collision();
void write(char *str);
void write_int(int value);
void write_debug(char *msg, int value);
void game_delay(int cycles);
void clear_uart();


// Initillize global variables
GameState currentState;
Tile board_tiles[NROWS][NCOLS] __attribute__((used)); // game board
Ball ball;
Bar bar;


/*
    Sets the entire screen to a specific color.
    Assumes colour in RGB888 format (0xRRGGBB).
    Uses the SetPixel assembly function to set each pixel.
    Arguments:
    - R0: color (unsigned int, RGB888 format)
    Returns: None
*/
asm("ClearScreen: \n\t"
    "   PUSH {R4-R11, LR} \n\t"         // Save registers that will be used
    "   MOV R8, R0 \n\t"               // Store color in R8
    "   MOV R4, #0 \n\t"               // y = 0
    "   LDR R6, =height \n\t"          // Load address of height
    "   LDRH R6, [R6] \n\t"             // Load height value
    "   LDR R7, =width \n\t"           // Load address of width
    "   LDRH R7, [R7] \n\t"             // Load width value
    "y_loop_start: \n\t"
    "   CMP R4, R6    \n\t"             // compare y with SCREEN_HEIGHT (240)
    "   BEQ y_loop_end \n\t"
    "   MOV R5, #0 \n\t"               // x = 0
    "x_loop_start: \n\t"
    "   CMP R5, R7 \n\t"                // compare x with SCREEN_WIDTH (320)
    "   BEQ x_loop_end \n\t"
    "   MOV R0, R5 \n\t"               // R0 = x
    "   MOV R1, R4 \n\t"               // R1 = y
    "   MOV R2, R8 \n\t"               // R2 = color
    "   BL SetPixel \n\t"              // Call SetPixel(x, y, color)
    "   ADD R5, R5, #1 \n\t"           // x++
    "   B x_loop_start \n\t"
    "x_loop_end: \n\t"
    "   ADD R4, R4, #1 \n\t"           // y++
    "   B y_loop_start \n\t"
    "y_loop_end: \n\t"
    "   POP {R4-R11, PC} \n\t"          // Restore registers and return
);

/*
    Sets a pixel at (x_coord, y_coord) to the specified color.
    Assumes colour in RGB888 format (0xRRGGBB).
    Arguments:
    - R0: x-coordinate (unsigned int)
    - R1: y-coordinate (unsigned int)
    - R2: color (unsigned int, RGB888 format)
    Returns: None
*/
asm("SetPixel: \n\t"
    "LDR R3, =VGAaddress \n\t"
    "LDR R3, [R3] \n\t"

    // check if x_coord and y_coord are within bounds
    "CMP R0, #0 \n\t"
    "BLT end_setpixel \n\t"
    "CMP R0, #320 \n\t" // check if x_coord < 320
    "BGE end_setpixel \n\t"
    "CMP R1, #0 \n\t"
    "BLT end_setpixel \n\t"
    "CMP R1, #240 \n\t" // check if y_coord < 240
    "BGE end_setpixel \n\t"


    "LSL R1, R1, #10 \n\t"
    "LSL R0, R0, #1 \n\t"
    "ADD R1, R0 \n\t"
    "STRH R2, [R3,R1] \n\t"
    "end_setpixel: \n\t"
    "BX LR"


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
    "BX LR");


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
    "BX LR"                            // Return
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
    unsigned int i, j;

    // Fill the entire block with the given color
    for (i = 0; i < h; i++){
        for (j = 0; j < w; j++){
            SetPixel(x + j, y + i, color);
        }
    }

}

/*
    Draws the bar at the specified y position.
    If y is DUMMY, the bar is drawn at the current position stored in the bar struct.
    The bar is drawn with a border of BORDER_WIDTH and color BAR_COLOR_EDGES.
    The inside of the bar is filled with BAR_COLOR_CENTER.
    Arguments:
    - y: y-coordinate of the top of the bar (unsigned int) or DUMMY to use current position
    Returns: None
*/
void DrawBar(unsigned int y)
{

    // Draw the bar at the specified y position.
    if (y != DUMMY)
    {
        // Draw top part of Bar
        draw_block
        (
            bar.pos_x, 
            y, 
            BAR_WIDTH, 
            BAR_EDGE_HEIGHT, 
            BAR_COLOR_EDGES
        );    
        
        // Draw center part of Bar
        draw_block
        (
            bar.pos_x, 
            y + BAR_EDGE_HEIGHT, 
            BAR_WIDTH, 
            BAR_CENTER_HEIGHT, 
            BAR_COLOR_CENTER
        );     
        
        // Draw bottom part of Bar
        draw_block
        (
            bar.pos_x, 
            y + BAR_EDGE_HEIGHT + BAR_CENTER_HEIGHT, 
            BAR_WIDTH, 
            BAR_EDGE_HEIGHT, 
            BAR_COLOR_EDGES
        ); 
    } else   // Draw the bar at the current position stored in the bar struct.
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
            if (bar.pos_y >= BAR_SPEED)
            {
                bar.pos_y -= BAR_SPEED;
                DrawBar(DUMMY);

                // draw the space where the bar was previously 
                draw_block
                (
                    bar.pos_x, 
                    bar.pos_y + BAR_EDGE_HEIGHT + BAR_CENTER_HEIGHT + BAR_EDGE_HEIGHT, 
                    BAR_WIDTH, 
                    BAR_SPEED, 
                    BACKGROUND_COLOR
                );
            }
            else if (bar.pos_y > 0)
            {
                bar.pos_y = 0;
                DrawBar(DUMMY);

                // draw the space where the bar was previously 
                draw_block
                (
                    bar.pos_x, 
                    bar.pos_y + BAR_EDGE_HEIGHT + BAR_CENTER_HEIGHT + BAR_EDGE_HEIGHT, 
                    BAR_WIDTH, 
                    bar.pos_y, 
                    BACKGROUND_COLOR
                );
            }
        }
        else if ((out & 0xFF) == 0x73) // 's' - move down
        {
            if (bar.pos_y <= ((height - BAR_CENTER_HEIGHT - BAR_EDGE_HEIGHT*2) - BAR_SPEED))
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

                bar.pos_y += BAR_SPEED;
                DrawBar(DUMMY);
            }
            else if (bar.pos_y < (height - BAR_CENTER_HEIGHT - BAR_EDGE_HEIGHT*2))
            {
                // draw the space where the bar was previously
                draw_block
                (
                    bar.pos_x, 
                    bar.pos_y , 
                    BAR_WIDTH, 
                    (height - BAR_CENTER_HEIGHT - BAR_EDGE_HEIGHT*2) - bar.pos_y, 
                    BACKGROUND_COLOR
                );

                bar.pos_y = height - BAR_CENTER_HEIGHT - BAR_EDGE_HEIGHT*2;
                DrawBar(DUMMY);
            }
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
            if (abs(i) + abs(j) <= BALL_RADIUS)
            {
                SetPixel(ball.pos_x + j, ball.pos_y + i, BALL_COLOR);
            }
        }
    }
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
}


void handle_block_collision()
{
    // Check if the ball is in the x range of the blocks
    if ((ball.pos_x + BALL_RADIUS) < (width - (NCOLS * TILE_SIZE)))
    {
        return; // Ball is not in block area
    }

    // Calculate the range of tiles the ball might be colliding with
    static const int offset = width - (NCOLS * TILE_SIZE);

    int left_tip_col = (ball.pos_x - BALL_RADIUS - offset) / TILE_SIZE;
    int right_tip_col = (ball.pos_x + BALL_RADIUS - offset) / TILE_SIZE;
    int top_tip_row = (ball.pos_y - BALL_RADIUS) / TILE_SIZE;
    int bottom_tip_row = (ball.pos_y + BALL_RADIUS) / TILE_SIZE;

    // Clamp to valid ranges
    if (left_tip_col < 0) left_tip_col = 0;
    else if (left_tip_col > NCOLS) left_tip_col = NCOLS - 1;
    if (right_tip_col < 0) right_tip_col = 0;
    else if (right_tip_col > NCOLS) right_tip_col = NCOLS - 1;
    if (top_tip_row < 0) top_tip_row = 0;
    else if (top_tip_row >= NROWS) top_tip_row = NROWS - 1;
    if (bottom_tip_row < 0) bottom_tip_row = 0;
    else if (bottom_tip_row >= NROWS) bottom_tip_row = NROWS - 1;

    // Variables to track collision sides
    int hit_horizontal = 0; // 1 for top/bottom collision
    int hit_vertical = 0;   // 1 for left/right collision
    
    // Calculate ball bounding box
    int ball_left = ball.pos_x - BALL_RADIUS;
    int ball_right = ball.pos_x + BALL_RADIUS;
    int ball_top = ball.pos_y - BALL_RADIUS;
    int ball_bottom = ball.pos_y + BALL_RADIUS;

    for (int row = top_tip_row; row <= bottom_tip_row; row++)
    {       
        for (int col = left_tip_col; col <= right_tip_col; col++)
        {
            if (board_tiles[row][col].destroyed)
            {
                continue; // Skip already destroyed blocks
            }
            
            // Get block boundaries
            int block_left = board_tiles[row][col].pos_x;
            int block_right = board_tiles[row][col].pos_x + TILE_SIZE;
            int block_top = board_tiles[row][col].pos_y;
            int block_bottom = board_tiles[row][col].pos_y + TILE_SIZE;
            
            // AABB collision detection - only proceed if actual collision
            if (ball_right >= block_left && ball_left <= block_right &&
                ball_bottom >= block_top && ball_top <= block_bottom)
            {
                // Collision detected - destroy the block
                board_tiles[row][col].destroyed = 1;
                
                // Erase the block by drawing over it with the background color
                draw_block(
                    board_tiles[row][col].pos_x, 
                    board_tiles[row][col].pos_y, 
                    TILE_SIZE, 
                    TILE_SIZE, 
                    BACKGROUND_COLOR
                );
                
                // Determine collision side based on overlap amounts
                int overlap_left = ball_right - block_left;
                int overlap_right = block_right - ball_left;
                int overlap_top = ball_bottom - block_top;
                int overlap_bottom = block_bottom - ball_top;
                
                // Find minimum overlap to determine primary collision side
                int min_overlap = overlap_left;
                int collision_side = 0; // 0=left, 1=right, 2=top, 3=bottom
                
                if (overlap_right < min_overlap) {
                    min_overlap = overlap_right;
                    collision_side = 1;
                }
                if (overlap_top < min_overlap) {
                    min_overlap = overlap_top;
                    collision_side = 2;
                }
                if (overlap_bottom < min_overlap) {
                    min_overlap = overlap_bottom;
                    collision_side = 3;
                }
                
                // Set collision flags and correct ball position
                switch (collision_side) {
                    case 0: // Hit left side of block
                        hit_vertical = 1;
                        ball.pos_x = block_left - BALL_RADIUS;
                        break;
                    case 1: // Hit right side of block
                        hit_vertical = 1;
                        ball.pos_x = block_right + BALL_RADIUS;
                        break;
                    case 2: // Hit top side of block
                        hit_horizontal = 1;
                        ball.pos_y = block_top - BALL_RADIUS;
                        break;
                    case 3: // Hit bottom side of block
                        hit_horizontal = 1;
                        ball.pos_y = block_bottom + BALL_RADIUS;
                        break;
                    default:
                        break;
                }
            }
        }
    }
    
    // Apply direction changes based on collision flags
    if (hit_horizontal)
    {
        ball.dir_y = -ball.dir_y; // Reverse y direction
    }
    if (hit_vertical)
    {
        ball.dir_x = -ball.dir_x; // Reverse x direction
    }
}
/*
    Updates the ball position based on its current direction, speed and collisions.
    Arguments: None
    Returns: None
*/
void update_ball_state()
{
/*     if (currentState != Running){
        return;
    }
 */
    // Erase the ball at its current position by drawing over it with the background color
    draw_block
    (
        ball.pos_x - BALL_RADIUS, 
        ball.pos_y - BALL_RADIUS, 
        BALL_RADIUS * 2 + 1,        // +1 to include the center pixel
        BALL_RADIUS * 2 + 1,        // +1 to include the center pixel
        BACKGROUND_COLOR
    );

    // Check for collision with blocks updating position
    handle_block_collision();
    
    // Update ball position based on current direction and speed with proper rounding
    // Add SCALE_FACTOR/2 before division to round instead of truncate
    int move_x = (ball.dir_x * BALL_SPEED + SCALE_FACTOR/2) / SCALE_FACTOR;
    int move_y = (ball.dir_y * BALL_SPEED + SCALE_FACTOR/2) / SCALE_FACTOR;
    
    ball.pos_x += move_x;
    ball.pos_y -= move_y;  // Subtracting because screen y-coordinates increase downwards

    // Check for collision with top wall
    if (ball.pos_y - BALL_RADIUS <= 0)
    {
        ball.dir_y = -ball.dir_y; // Reverse y direction
        ball.pos_y = BALL_RADIUS; // Prevent sticking to the wall
    }
    // Check for collision with bottom wall
    else if (ball.pos_y + BALL_RADIUS >= height)
    {
        ball.dir_y = -ball.dir_y; // Reverse y direction
        ball.pos_y = height - BALL_RADIUS; // Prevent sticking to the wall
    }

    int bar_collision = 0;
    // Check for collision with bar
    if (ball.pos_x - BALL_RADIUS <= bar.pos_x + BAR_WIDTH &&
        ball.pos_x + BALL_RADIUS >= bar.pos_x &&
        ball.pos_y + BALL_RADIUS >= bar.pos_y &&
        ball.pos_y - BALL_RADIUS <= bar.pos_y + BAR_EDGE_HEIGHT + BAR_CENTER_HEIGHT + BAR_EDGE_HEIGHT)
    {
        bar_collision = 1;
        ball.dir_x = -ball.dir_x; // Reverse x direction
        
        ball.dir_y = - ball.dir_y;
        //ball.pos_x = bar.pos_x + BAR_WIDTH + BALL_RADIUS; // Prevent sticking to the bar
    }

    draw_ball();    // Draw the ball at its new position
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
            board_tiles[row][col].pos_x = width - (TILE_SIZE * (NCOLS - col));
            board_tiles[row][col].pos_y = height - (TILE_SIZE * (NROWS - row));
            board_tiles[row][col].color = block_colors[(col + row) % 6];  
            board_tiles[row][col].destroyed = 0;
        }
    }
}



/*
    Draws the playing field by iterating through the board_tiles array and drawing each block.
    Gives colors to each of the blocks.
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
                TILE_SIZE,
                TILE_SIZE,
                board_tiles[row][col].color
            );   
        }
    }
}

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


void play()
{
    // Game loop - continues until game ends
    while (1)
    {
        // Update game logic
        update_game_state();        // Check win/loss conditions
        // Exit if game is no longer running
        if (currentState != Running)
        {
            break;
        }

        update_bar_state();         // Handle player input and bar movement
        
        // Update and draw ball (update_ball_state already handles drawing)
        update_ball_state();
        
        // Add a small delay to control frame rate and prevent overwhelming the system
        // This makes the game playable and reduces CPU usage
        //game_delay(1000); // Adjust this value to make game faster/slower
    }
    
    // Handle end-of-game messages and cleanup
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

// It must initialize the game
void reset()
{
    // Intialize global variables
    initilize_board_tiles();
    initialize_bar();
    initialize_ball();

    // Draw initial screen
    ClearScreen(BACKGROUND_COLOR);
    DrawBar(DUMMY);
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
    initilize_board_tiles();
    initialize_bar();
    initialize_ball();

    // Draw initial screen
    clear_uart();
    ClearScreen(BACKGROUND_COLOR);
    DrawBar(DUMMY);
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

int main(int argc, char *argv[])
{
    // HINT: This loop allows the user to restart the game after loosing/winning the previous game
    while (1)
    {
        wait_for_start();
        play();
        reset();
        if (currentState == Exit)
        {
            break;
        }
    } 
    return 0;
}

// THIS IS FOR THE OPTIONAL TASKS ONLY

// HINT: How to access the correct bitmask
// sample: to get character a's bitmask, use
// font8x8['a']
char font8x8[128][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0000 (nul)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0001
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0002
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0003
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0004
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0005
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0006
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0007
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0008
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0009
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000A
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000B
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000C
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000D
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000F
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0010
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0011
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0012
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0013
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0014
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0015
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0016
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0017
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0018
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0019
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001A
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001B
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001C
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001D
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001F
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0020 (space)
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // U+0021 (!)
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0022 (")
    {0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // U+0023 (#)
    {0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // U+0024 ($)
    {0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // U+0025 (%)
    {0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // U+0026 (&)
    {0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0027 (')
    {0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // U+0028 (()
    {0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // U+0029 ())
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // U+002A (*)
    {0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // U+002B (+)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+002C (,)
    {0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // U+002D (-)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+002E (.)
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // U+002F (/)
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // U+0030 (0)
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // U+0031 (1)
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // U+0032 (2)
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // U+0033 (3)
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // U+0034 (4)
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // U+0035 (5)
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // U+0036 (6)
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // U+0037 (7)
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // U+0038 (8)
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // U+0039 (9)
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+003A (:)
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+003B (;)
    {0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // U+003C (<)
    {0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // U+003D (=)
    {0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // U+003E (>)
    {0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // U+003F (?)
    {0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // U+0040 (@)
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // U+0041 (A)
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // U+0042 (B)
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // U+0043 (C)
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // U+0044 (D)
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // U+0045 (E)
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // U+0046 (F)
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // U+0047 (G)
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // U+0048 (H)
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0049 (I)
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // U+004A (J)
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // U+004B (K)
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // U+004C (L)
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // U+004D (M)
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // U+004E (N)
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // U+004F (O)
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // U+0050 (P)
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // U+0051 (Q)
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // U+0052 (R)
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // U+0053 (S)
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0054 (T)
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // U+0055 (U)
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0056 (V)
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // U+0057 (W)
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // U+0058 (X)
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // U+0059 (Y)
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // U+005A (Z)
    {0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00}, // U+005B ([)
    {0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00}, // U+005C (\)
    {0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00}, // U+005D (])
    {0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // U+005E (^)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // U+005F (_)
    {0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0060 (`)
    {0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00}, // U+0061 (a)
    {0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00}, // U+0062 (b)
    {0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00}, // U+0063 (c)
    {0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00}, // U+0064 (d)
    {0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00}, // U+0065 (e)
    {0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00}, // U+0066 (f)
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0067 (g)
    {0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00}, // U+0068 (h)
    {0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0069 (i)
    {0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E}, // U+006A (j)
    {0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00}, // U+006B (k)
    {0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+006C (l)
    {0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00}, // U+006D (m)
    {0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00}, // U+006E (n)
    {0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00}, // U+006F (o)
    {0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F}, // U+0070 (p)
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78}, // U+0071 (q)
    {0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00}, // U+0072 (r)
    {0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00}, // U+0073 (s)
    {0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00}, // U+0074 (t)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00}, // U+0075 (u)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0076 (v)
    {0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00}, // U+0077 (w)
    {0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00}, // U+0078 (x)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0079 (y)
    {0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00}, // U+007A (z)
    {0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00}, // U+007B ({)
    {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, // U+007C (|)
    {0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00}, // U+007D (})
    {0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+007E (~)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // U+007F
};
