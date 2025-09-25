#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

static char *won = "You Won";       // DON'T TOUCH THIS - keep the string as is     WHY IS THIS NOT A CONSTANT???
static char *lost = "You Lost";     // DON'T TOUCH THIS - keep the string as is     WHY IS THIS NOT A CONSTANT???
const unsigned short height = 240;        // DON'T TOUCH THIS - keep the value as is      WHY IS THIS NOT A CONSTANT???
const unsigned short width = 320;         // DON'T TOUCH THIS - keep the value as is      WHY IS THIS NOT A CONSTANT???
char font8x8[128][8];               // DON'T TOUCH THIS - this is a forward declaration  - -NO IDEA WHAT THIS IS USED FOR???
unsigned char tiles[NROWS][NCOLS] __attribute__((used)) = { 0 }; // DON'T TOUCH THIS - this is the tile map. NO IDEA WHAT THIS IS USED FOR ???
/**************************************************************************************************/

unsigned long long __attribute__((used)) UART_BASE = 0xFF201000; // JTAG UART base address

#define PI                  3.14159265358979323846
#define DUMMY               -1      // Dummy constant to be used as inupt to DrawBar. The actual y position is read from the bar struct. The function prototype is kept as is to keep the skeleton code intact and lectureres happy.
#define ANGLE_STEPS_TOTAL  512      // Number of steps the different angles in the game. total 360 degrees in a circle. 360 / 512 = 0.703125 degrees per step

#define BACKGROUND_COLOR    black   // Background color of the playing field

#define BAR_X_POS           10      // XTOTAL position of the bar
#define BAR_WIDTH           7       // Width of the bar
#define BAR_CENTER_HEIGHT   15      // Height of the center part of the bar
#define BAR_EDGE_HEIGHT     15      // Height of the edge parts of the bar
#define BAR_SPEED           15      // Number of pixels the bar moves up or down when receiving 'w' or 's' command
#define BAR_COLOR_EDGES     red     // Color of the edge parts of the bar
#define BAR_COLOR_CENTER    green   // Color of the center part of the bar

#define BALL_RADIUS         3       // Radius of the ball in pixels
#define BALL_SPEED          5       // Number of pixels the ball moves in its direction each frame
#define INITIAL_BALL_ANGLE  0      // Initial angle of the ball in degrees (0 to 360)
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
    unsigned int angle;         // angle in degrees. An angle of 0 gives a horizontal rightwards direction of travel. an angle of 90 gives a vertically upwards direction of travel.
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

// sine table for integer trigonometry 0 to 360 degrees scaled by 32767 (2^15-1)
static const int16_t sine_table[360] = 
{   
    0, 571, 1143, 1714, 2285, 2855, 3425, 3993, 4560, 5125, 5689, 6252, 6812, 7370, 7927, 8480, 9031, 9580, 10125, 10667, 11206, 11742, 12274, 12803, 13327, 13847, 14364, 14875, 15383, 15885, 16383, 16876, 17363, 17846, 18323, 18794, 19259, 19719, 20173, 20620, 21062, 21497, 21925, 22347, 22761, 23169, 23570, 23964, 24350, 24729, 25100, 25464, 25820, 26168, 26509, 26841, 27165, 27480, 27787, 28086, 28377, 28658, 28931, 29195, 29450, 29696, 29934, 30162, 30381, 30590, 30790, 30981, 31163, 31335, 31497, 31650, 31793, 31927, 32050, 32164, 32269, 32363, 32448, 32522, 32587, 32642, 32687, 32722, 32747, 32762, 32767, 32762, 32747, 32722, 32687, 32642, 32587, 32522, 32448, 32363, 32269, 32164, 32050, 31927, 31793, 31650, 31497, 31335, 31163, 30981, 30790, 30590, 30381, 30162, 29934, 29696, 29450, 29195, 28931, 28658, 28377, 28086, 27787, 27480, 27165, 26841, 26509, 26168, 25820, 25464, 25100, 24729, 24350, 23964, 23570, 23169, 22761, 22347, 21925, 21497, 21062, 20620, 20173, 19719, 19259, 18794, 18323, 17846, 17363, 16876, 16383, 15885, 15383, 14875, 14364, 13847, 13327, 12803, 12274, 11742, 11206, 10667, 10125, 9580, 9031, 8480, 7927, 7370, 6812, 6252, 5689, 5125, 4560, 3993, 3425, 2855, 2285, 1714, 1143, 571, 0, -571, -1143, -1714, -2285, -2855, -3425, -3993, -4560, -5125, -5689, -6252, -6812, -7370, -7927, -8480, -9031, -9580, -10125, -10667, -11206, -11742, -12274, -12803, -13327, -13847, -14364, -14875, -15383, -15885, -16383, -16876, -17363, -17846, -18323, -18794, -19259, -19719, -20173, -20620, -21062, -21497, -21925, -22347, -22761, -23169, -23570, -23964, -24350, -24729, -25100, -25464, -25820, -26168, -26509, -26841, -27165, -27480, -27787, -28086, -28377, -28658, -28931, -29195, -29450, -29696, -29934, -30162, -30381, -30590, -30790, -30981, -31163, -31335, -31497, -31650, -31793, -31927, -32050, -32164, -32269, -32363, -32448, -32522, -32587, -32642, -32687, -32722, -32747, -32762, -32767, -32762, -32747, -32722, -32687, -32642, -32587, -32522, -32448, -32363, -32269, -32164, -32050, -31927, -31793, -31650, -31497, -31335, -31163, -30981, -30790, -30590, -30381, -30162, -29934, -29696, -29450, -29195, -28931, -28658, -28377, -28086, -27787, -27480, -27165, -26841, -26509, -26168, -25820, -25464, -25100, -24729, -24350, -23964, -23570, -23169, -22761, -22347, -21925, -21497, -21062, -20620, -20173, -19719, -19259, -18794, -18323, -17846, -17363, -16876, -16383, -15885, -15383, -14875, -14364, -13847, -13327, -12803, -12274, -11742, -11206, -10667, -10125, -9580, -9031, -8480, -7927, -7370, -6812, -6252, -5689, -5125, -4560, -3993, -3425, -2855, -2285, -1714, -1143, -571
};

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
void set_ball_angle(unsigned int angle_steps);
void draw_playing_field();
void initilize_board_tiles();
void reset();
void play();
void wait_for_start();
void update_game_state();
void update_bar_state();
void update_ball_state();
void write(char *str);
void clear_uart();

// Integer trigonometry functions
int16_t int_sin(unsigned int angle_degrees);
int16_t int_cos(unsigned int angle_degrees);


// Initillize global variables
GameState currentState;
Tile board_tiles[NROWS][NCOLS] __attribute__((used)); // game board
Ball ball;
Bar bar;

/*
    Integer sine function using lookup table
    Arguments:
    - angle_degrees: angle in degrees (0-360)
    Returns: sine value scaled by 65535 (i.e., sin(90°) = 65535, meaning 1.0)
*/
int16_t int_sin(unsigned int angle_degrees)
{
    // Ensure angle is in range 0-360
    int angle = angle_degrees % 360;
    return sine_table[angle];
}

/*
    Integer cosine function using lookup table
    Arguments:
    - angle_degrees: angle in degrees (0-360) 
    Returns: cosine value scaled by 65535 (i.e., cos(0°) = 65535, meaning 1.0)
*/
int16_t int_cos(unsigned int angle_degrees)
{
    // cos(x) = sin(x + 90)
    return int_sin((angle_degrees + 90));
}

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
    "LSL R1, R1, #10 \n\t"
    "LSL R0, R0, #1 \n\t"
    "ADD R1, R0 \n\t"
    "STRH R2, [R3,R1] \n\t"
    "BX LR");


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
    Arguments:
    - R0: character to write (unsigned int, only the lowest byte is used)
    Returns: None
*/

asm("WriteUart:\n\t"
    "LDR R1, =UART_BASE \n\t"
    "LDR R1, [R1] \n\t"
    "STR R0, [R1] \n\t"
    "BX LR"
);

/*
    Writes a string to the JTAG UART. Iterates until it finds the null-termination character.
    Arguments:
    - str: pointer to the string to write (char*)
    Returns: None
*/
void write(char *str)
{
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
    // Bounds check to ensure ball position is within screen limits
    assert(ball.pos_x >= 0 && ball.pos_x < width);
    assert(ball.pos_y >= 0 && ball.pos_y < height);

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
    ball.angle = INITIAL_BALL_ANGLE;
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
    //clear previous ball position
    draw_block
    (
        ball.pos_x - BALL_RADIUS,
        ball.pos_y - BALL_RADIUS,
        2 * BALL_RADIUS + 1,
        2 * BALL_RADIUS + 1,
        BACKGROUND_COLOR
    );

    
    printf("cos %d\n", (int)(int_cos(30)/sizeof(int16_t)));
    printf("sin %d\n", (int)(int_sin(30)/65535));
    // Update ball position based on current direction and speed
    ball.pos_x += (int)((BALL_SPEED * int_cos(ball.angle)) / 65535);      // Divide by 65535 to scale back to normal range
    ball.pos_y -= (int)((BALL_SPEED * int_sin(ball.angle)) / 65535);        // Subtract because screen y-coordinates increase downwards

    // Draw the ball at its new position
    draw_ball();
}


void check_ball_collisions()
{

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
            board_tiles[row][col].pos_x = width - (TILE_SIZE * (col + 1));
            board_tiles[row][col].pos_y = height - (TILE_SIZE * (row + 1));
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

    // Game is won if the ball reaches the right side of the screen
    if (ball.pos_x + BALL_RADIUS >= width) {
        currentState = Won;
        return;
    }
    // Game is lost if the ball drops below the bar.
    if (ball.pos_x + BALL_RADIUS <= BAR_X_POS) {
        currentState = Lost;
        return;
    }
    


    // TODO: Update balls position and direction



    // TODO: Hit Check with Blocks
    // HINT: try to only do this check when we potentially have a hit, as it is relatively expensive and can slow down game play a lot
}





void play()
{
    while (1)
    {
        update_game_state();
        update_bar_state();
        if (currentState != Running)
        {
            break;
        }
        update_ball_state();
        draw_ball();
        DrawBar(DUMMY);
    }
    if (currentState == Won)
    {
        write(won);
    }
    else if (currentState == Lost)
    {
        write(lost);
    }
    else if (currentState == Exit)
    {
        return;
    }
    currentState = Stopped;
}

// It must initialize the game
void reset()
{
    currentState = Stopped;

    // Hint: This is draining the UART buffer
    int remaining = 0;
    do
    {
        unsigned long long out = ReadUart();
        if (!(out & 0x8000))
        {
            // not valid - abort reading
            return;
        }
        remaining = (out & 0xFF0000) >> 4;
    } while (remaining > 0);

    // TODO: You might want to reset other state in here
    clear_uart();
    ClearScreen(BACKGROUND_COLOR);
    initilize_board_tiles();
    draw_playing_field();
    initialize_bar();
    DrawBar(DUMMY);
    initialize_ball();
    draw_ball();
    wait_for_start();
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
    wait_for_start();

    for (int i = 0; i < 100; i++)
    {
        update_ball_state();
    }

    while(1)
    {
        update_bar_state();
    }


/*     // HINT: This loop allows the user to restart the game after loosing/winning the previous game
    while (1)
    {
        wait_for_start();
        play();
        reset();
        if (currentState == Exit)
        {
            break;
        }
    } */
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
