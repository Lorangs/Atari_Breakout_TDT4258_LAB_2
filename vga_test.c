#define VGA_PIXEL_BASE  0xc8000000
unsigned long long __attribute__((used)) VGaddress = VGA_PIXEL_BASE;
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

typedef enum { false = 0, true = 1 } bool;

volatile unsigned short * const pixel_buffer = (volatile unsigned short *) VGA_PIXEL_BASE;

// Translates 24-bit RGB888 color to 16-bit RGB565 format. Example: rgb_888_to_565(255, 0, 0) for red.
static inline unsigned short rgb_888_to_565(unsigned char r, unsigned char g, unsigned char b) {
    return ( ((r & 0xF8) << 8) |    // Red: top 5 bits to bits 11-15
             ((g & 0xFC) << 3) |    // Green: top 6 bits to bits 5-10
             ((b & 0xF8) >> 3) );   // Blue: top 5 bits to bits 0-4
}

// Block structure definition
typedef struct _block
{
    unsigned char destroyed;
    unsigned char deleted;
    unsigned int pos_x;
    unsigned int pos_y;
    unsigned int color;
} Block;

void SetPixel(unsigned int x, unsigned int y, unsigned int color);
asm("SetPixel: \n\t"
	"LDR R3, =VGaddress \n\t"       // Load address of VGAaddress variable
	"LDR R3, [R3] \n\t"             // Load the actual VGA base address
	"LSL R1, R1, #10 \n\t"          // bitmask by 10 to place y value in bit 17..10
	"LSL R0, R0, #1 \n\t"           // bitmask by 1 to place x value in bit 9..1
	"ADD R1, R0 \n\t"               // offset
	"STRH R2, [R3,R1] \n\t"         // store halfword color to pixel address
	"BX LR"                         // Return from subroutine
);                       
	

// Clears the entire screen to a specific colour.
void ClearScreen(unsigned int color) 
{
    for (unsigned int y = 0; y < SCREEN_HEIGHT; y++) {
        for (unsigned int x = 0; x < SCREEN_WIDTH; x++) {
			SetPixel(x, y, color);
        }
    }
}

int main()
{
    ClearScreen(0x000000); // Clear the screen to black
    return 0;
}

