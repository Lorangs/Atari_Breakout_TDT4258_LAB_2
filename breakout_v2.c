#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************************************************************************
 * DON'T REMOVE THE VARIABLES BELOW THIS COMMENT                                                   *
 **************************************************************************************************/
unsigned long long __attribute__((used)) VGAaddress = 0xc8000000; // Memory storing pixels
unsigned int __attribute__((used)) red = 0x0000F0F0;
unsigned int __attribute__((used)) green = 0x00000F0F;
unsigned int __attribute__((used)) blue = 0x000000FF;
unsigned int __attribute__((used)) white = 0x0000FFFF;
unsigned int __attribute__((used)) black = 0x0;

// Don't change the name of this variables
#define NCOLS 10 // <- Supported value range: [1,18]
#define NROWS 16 // <- This variable might change.
#define TILE_SIZE 15 // <- Tile size, might change.

char *won = "You Won";       // DON'T TOUCH THIS - keep the string as is
char *lost = "You Lost";     // DON'T TOUCH THIS - keep the string as is
unsigned short height = 240; // DON'T TOUCH THIS - keep the value as is
unsigned short width = 320;  // DON'T TOUCH THIS - keep the value as is
char font8x8[128][8];        // DON'T TOUCH THIS - this is a forward declaration
unsigned char tiles[NROWS][NCOLS] __attribute__((used)) = { 0 }; // DON'T TOUCH THIS - this is the tile map
/**************************************************************************************************/

unsigned long long __attribute__((used)) UART_BASE = 0xFF201000; // JTAG UART base address

#define BACKGROUND_COLOR black
#define BORDER_WIDTH 2  
#define BORDER_COLOR white

#define BAR_X_POS           10
#define BAR_WIDTH           7
#define BAR_CENTER_HEIGHT   15
#define BAR_EDGE_HEIGHT     15
#define BAR_SPEED           5
#define BAR_COLOR_EDGES     red
#define BAR_COLOR_CENTER    green

#define BALL_RADIUS         3
#define BALL_COLOR          blue

typedef struct _block
{
    unsigned char destroyed;
    unsigned int pos_x;
    unsigned int pos_y;
    unsigned int color;
} Block;


typedef struct _bar
{
    unsigned int pos_x;         // x-coordinate of the left of the bar (constant)
    unsigned int pos_y;         // y-coordinate of the top of the bar
} Bar;

typedef struct _ball
{
    unsigned int pos_x;
    unsigned int pos_y;
    float dir_xy[2];    // normalized direction vector
} Ball;

typedef enum _gameState
{
    Stopped = 0,
    Running = 1,
    Won = 2,
    Lost = 3,
    Exit = 4,
} GameState;




/*
Function declarations for all functions. Both assembly and C.
*/
void ClearScreen(unsigned int color);
void SetPixel(unsigned int x_coord, unsigned int y_coord, unsigned int color);
void draw_block(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color);
void draw_bar();
int ReadUart();
void WriteUart(char c);
void draw_ball();
void draw_playing_field();
void reset();
void play();
void wait_for_start();
void update_game_state();
void update_bar_state();
void write(char *str);


// Initillize global variables
GameState currentState = Stopped;
Block board_tiles[NROWS][NCOLS] __attribute__((used)); // game board
Ball ball = 
{
    .pos_x = BAR_X_POS + BAR_WIDTH + BALL_RADIUS,
    .pos_y = 120,
    .dir_xy = {1.0f, 0.0f}  // initially moving to the right
};
Bar bar = 
{ 
    .pos_x = BAR_X_POS,
    .pos_y = (unsigned int)(60 - (BAR_CENTER_HEIGHT/2 + BAR_EDGE_HEIGHT)) 
};


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

// assumes R0 = x-coord, R1 = y-coord, R2 = colorvalue
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
    "LDR R1, =0xFF201000 \n\t"
    "LDR R0, [R1]\n\t"
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
    "STR R0, [R1]\n\t"
    "BX LR"
);


void draw_block(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color)
{
    assert(w > BORDER_WIDTH && h > BORDER_WIDTH);
    unsigned int i, j;

    // Draw top border around the block
    for (i = 0; i < BORDER_WIDTH; i++){
        for (j = 0; j < w; j++){
            SetPixel(x + j, y + i, BORDER_COLOR);
        }
    }
    // Draw bottom border around the block. Assumes height > BORDER_WIDTH
    for (i = (h - BORDER_WIDTH); i < h; i++){
        for (j = 0; j < w; j++){
            SetPixel(x + j, y + i, BORDER_COLOR);
        }
    }
    // Draw leftmost border edge
    for (i = BORDER_WIDTH; i < (h - BORDER_WIDTH); i++){
        for (j = 0; j < BORDER_WIDTH; j++){
            SetPixel(x + j, y + i, BORDER_COLOR);
        }
    }
    // Draw rightmost border edge
    for (i = BORDER_WIDTH; i < (h - BORDER_WIDTH); i++){
        for (j = (w - BORDER_WIDTH); j < w; j++){
            SetPixel(x + j, y + i, BORDER_COLOR);
        }
    }

    // Fill the space inside the border with the given color
    for (i = BORDER_WIDTH; i < (h - BORDER_WIDTH); i++){
        for (j = BORDER_WIDTH; j < (w - BORDER_WIDTH); j++){
            SetPixel(x + j, y + i, color);
        }
    }
}

void draw_bar()
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
    Initilizes the board_tiles array with the correct positions and colors.
    Arguments:
    - c: color of the blocks (unsigned int, RGB888 format)
    Returns: None
*/
void initilize_board_tile(unsigned int c)
{
    for (int row = 0; row < NROWS; row++)
    {
        for (int col = 0; col < NCOLS; col++)
        {
            board_tiles[row][col].pos_x = width - (TILE_SIZE * (col + 1));
            board_tiles[row][col].pos_y = height - (TILE_SIZE * (row + 1));
            board_tiles[row][col].color = c;
            board_tiles[row][col].destroyed = 0;
        }
    }
}



/*
    Fill out the rightmmost part of the game with NROWS x NCOLS blocks
    Initilize the board_tiles array with the correct positions and colors.
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
    if (ball.pos_x + BALL_RADIUS >= 320) {
        currentState = Won;
        return;
    }
    // Game is lost if the ball drops below the bar.
    if (ball.pos_x - BALL_RADIUS <= 7) {
        currentState = Lost;
        return;
    }
    


    // TODO: Update balls position and direction



    // TODO: Hit Check with Blocks
    // HINT: try to only do this check when we potentially have a hit, as it is relatively expensive and can slow down game play a lot
}


/*
    Reads all characters in the UART Buffer and apply the respective bar position updates
    Arguments: None
    Returns: None
*/
void update_bar_state()
{
    int remaining = 0;
    // Read all chars in the UART Buffer and apply the respective bar position updates
    // HINT: w == 119 (0x77), s == 115 (0x73)
    // HINT Format: 0x00 'Remaining Chars':2 'Ready 0x80':2 'Char 0xXX':2, sample: 0x00018077 (1 remaining character, buffer is ready, current character is 'w')
    do
    {
        unsigned long long out = ReadUart();
        if (!(out & 0x8000))
        {
            // not valid - abort reading
            return;
        }
        if ((out & 0xFF) == 0x77) // 'w' - move up
        {
            if (bar.pos_y >= BAR_SPEED)
            {
                bar.pos_y -= BAR_SPEED;

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
            }
        }
        remaining = (out & 0xFF0000) >> 4;
    } while (remaining > 0);
}

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

void play()
{
    ClearScreen(black);
    // HINT: This is the main game loop
    while (1)
    {
        update_game_state();
        update_bar_state();
        if (currentState != Running)
        {
            break;
        }
        draw_playing_field();
        draw_ball();
        draw_bar(120); // TODO: replace the constant value with the current position of the bar
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
}

void wait_for_start()
{
    // TODO: Implement waiting behaviour until the user presses either w/s
}

int main(int argc, char *argv[])
{
    ClearScreen(black);
    initilize_board_tile(green);
    draw_playing_field();
    draw_ball();
    while(1)
    {
        update_bar_state();
        draw_bar(); 
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
