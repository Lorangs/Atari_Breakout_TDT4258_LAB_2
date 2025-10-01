# Advanced Breakout Game for DE1-SoC

[![Platform](https://img.shields.io/badge/Platform-DE1--SoC-blue)](https://cpulator.01xz.net/?sys=arm-de1soc)
[![Language](https://img.shields.io/badge/Language-C%2FARM%20Assembly-orange)]()
[![Course](https://img.shields.io/badge/Course-TDT4258-green)](https://www.ntnu.edu/studies/courses/TDT4258#tab=omEmnet)

## 🎮 Overview

This project implements an advanced **Breakout** game designed to run on the DE1-SoC Computer system via the CPUlator ARM emulator. The game features sophisticated collision detection, physics-based ball movement, and smooth interpolation for enhanced gameplay experience.

### 🌟 Key Features

- **Ray Casting Collision System**: Predicts collisions before they occur for smooth movement
- **Variable Angle Reflection**: Ball reflection angle depends on where it hits the bar (-60° to +60°)
- **Newton's Method Physics**: Integer-based square root calculations for precise ball physics
- **Fixed-Point Arithmetic**: Maintains precision without floating-point operations
- **Assembly Optimization**: Critical functions implemented in ARM assembly for performance
- **Real-time UART Control**: Responsive bar movement via JTAG UART interface

## 🎯 Game Mechanics

### Ball Physics
- **Collision Prediction**: Uses ray casting to calculate next collision point
- **Smooth Interpolation**: Ball moves smoothly between collision points
- **Variable Reflection**: Hit position on bar determines reflection angle
  - Center hit: Straight reflection
  - Edge hits: Up to ±60° angle variation
- **Consistent Speed**: Trigonometric integer calculation is estimated with Newton's method.

### Controls
- **W**: Move bar up
- **S**: Move bar down  
- **SPACE**: Start game / Restart after game over
- **ENTER**: Exit game

### Win/Loss Conditions
- **Win**: Move the Ball to teh far right side of the screen
- **Lose**: Ball passes behind the bar (left edge of screen)

## 🏗️ Technical Architecture

### Hardware Integration
- **Target Platform**: DE1-SoC Computer (ARM Cortex-A9)
- **Display**: VGA framebuffer (320×240, 16-bit RGB565)
- **Memory Mapping**: 
  - VGA Buffer: `0xC8000000`
  - JTAG UART: `0xFF201000`

### Software Architecture

#### Core Components

1. **Collision System**
   ```c
   typedef enum {
       COLLISION_NONE = 0,
       COLLISION_WALL = 1,
       COLLISION_BAR = 2,
       COLLISION_BLOCK = 3
   } CollisionType;
   ```

2. **Ball State Management**
   - Position tracking with sub-pixel precision
   - Direction vectors using fixed-point arithmetic (SCALE_FACTOR = 1000)
   - Collision prediction and interpolation system

3. **Physics Engine**
   - Newton's method for square root calculations
   - Integer-only arithmetic for embedded system compatibility
   - Angle calculations using trigonometric approximations

#### Assembly Functions

The game includes optimized ARM assembly functions for performance-critical operations:

- **`SetPixel`**: Direct VGA memory access with bounds checking
- **`ReadUart`**: Non-blocking UART character reading
- **`WriteUart`**: Blocking UART character writing with flow control


## 🔧 Configuration

### Game Parameters
```c
#define NCOLS               5       // Number of block columns
#define NROWS               16      // Number of block rows  
#define TILE_HEIGHT         15      // Block height in pixels
#define TILE_WIDTH          15      // Block width in pixels
#define SCALE_FACTOR        1000    // Fixed-point scaling
#define GAME_DELAY          5000    // Game speed control
#define BALL_RADIUS         3       // Ball size
#define BAR_SPEED           15      // Bar movement speed
```

### Color Scheme
- **Background**: Black
- **Ball**: Blue
- **Bar Edges**: Red  
- **Bar Center**: Green
- **Blocks**: Cycling through red, green, blue, yellow, magenta, cyan

## 🚀 Getting Started

### Prerequisites
- Access to [CPUlator ARM DE1-SoC emulator](https://cpulator.01xz.net/?sys=arm-de1soc)
- Basic understanding of C programming and ARM assembly

### Compilation & Execution

1. **Load the emulator**: Navigate to the CPUlator ARM DE1-SoC system
2. **Load source code**: Copy `breakout_personilized.c` into the editor
3. **Compile**: Click "Compile and Load" 
4. **Run**: Execute the program
5. **Play**: Use JTAG UART terminal for game interaction

### Controls Setup
- Ensure JTAG UART terminal is visible for game controls
- Characters 'w' and 's' control bar movement
- SPACE and ENTER keys control game state

## 🧮 Mathematical Implementation

### Newton's Method for Square Roots
The game uses Newton's method for calculating square roots in integer arithmetic:

```c
// Newton's method: x_{n+1} = (x_n + N/x_n) / 2
for (int i = 0; i < 10; i++) {
    dir_x_magnitude = (dir_x_magnitude + (target_x_squared * SCALE_FACTOR) / dir_x_magnitude) / 2;
    if (abs(dir_x_magnitude - prev_x_magnitude) < 2) break;
}
```

### Angle Calculations
Variable reflection angles using trigonometric approximations:
```c
// Map hit position to ±60° range using sin(60°) ≈ 866/1000
int angle_factor = (hit_offset * 866) / max_offset;
ball.dir_y = angle_factor;
```

## 🔍 Code Structure

### Main Game Loop
1. **State Management**: Check win/loss conditions
2. **Input Processing**: Handle UART commands for bar movement
3. **Physics Update**: Calculate ball position and collisions  
4. **Rendering**: Update VGA display
5. **Timing Control**: Frame rate management

### Collision Detection Pipeline
1. **Ray Casting**: Project ball path to find next collision
2. **Object Testing**: Check walls, bar, and blocks along path
3. **Impact Response**: Calculate new direction based on collision type
4. **Position Correction**: Prevent object penetration

## 🎨 Graphics System

### VGA Interface
- **Resolution**: 320×240 pixels
- **Color Format**: 16-bit RGB565 (5 red, 6 green, 5 blue bits)
- **Memory Access**: Direct framebuffer manipulation via assembly
- **Double Buffering**: Immediate pixel updates for smooth animation

### Rendering Pipeline
1. **Background Clear**: Fill screen with background color
2. **Block Rendering**: Draw remaining blocks in grid pattern
3. **Bar Drawing**: Three-section bar with edge/center colors
4. **Ball Animation**: Erase previous position, draw new position

## 📊 Performance Considerations

### Optimization Techniques
- **Assembly Functions**: Critical path operations in ARM assembly
- **Fixed-Point Math**: Avoids floating-point overhead  
- **Collision Prediction**: Reduces per-frame collision checks
- **Bounded Iterations**: Newton's method with early termination
- **Memory Access**: Direct VGA buffer manipulation

### Real-time Constraints
- **Frame Rate**: Controlled by `GAME_DELAY` parameter
- **Interrupt Handling**: Non-blocking UART reads
- **Memory Bandwidth**: Optimized pixel operations

## 🐛 Debugging Features

The game includes some debugging output via UART:

Debug functions:
```c
void write_debug(char *msg, int value);  // Debug message with integer
void print_ball();                       // Complete ball state dump
```

## 📚 References

- **DE1-SoC Computer Documentation**: Hardware specifications and memory mapping
- **ARM Cortex-A9 Technical Reference**: Assembly instruction set and optimization
- **CPUlator Documentation**: Emulator-specific features and limitations
- **Digital Design Principles**: VGA timing and color encoding standards

## 👨‍💻 Author

**Lorang Strand**  
*TDT4258 - Low Level Programming*  
*Norwegian University of Science and Technology (NTNU)*

---

