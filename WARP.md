# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

**tte-c** is a high-performance C reimplementation of the Python `terminaltexteffects` library, delivering 10x+ faster startup times and significantly lower resource usage. It provides 13 visual terminal text effects with advanced 256-color gradient support and precise terminal control.

Key innovation: `--no-final-newline` flag prevents screensaver scrolling issues that plague the original Python version.

## Development Commands

### Build System
```bash
make          # Standard build (-O2 optimization)
make debug    # Debug build with symbols (-g -DDEBUG)  
make clean    # Clean object files
make install  # Install to /usr/local/bin/
make test     # Build and run unit tests
```

### Testing
```bash
# Run unit tests
make test

# Manual effect testing
echo "Hello World" | ./tte-c beams
echo "Test Effect" | ./tte-c --no-final-newline matrix

# Performance testing vs Python TTE
timeout 10 ./tte-c fireworks < README.md
```

### Single Test Development
```bash
# Test specific effect during development
gcc -g -DDEBUG -I. tests/test_tte.c src/*.o -o tests/test_effect -lm
./tests/test_effect
```

## Code Architecture

### Core Components

1. **src/tte.h** - Central header with all type definitions and function declarations
2. **src/main.c** - Program entry point and main animation loop  
3. **src/terminal.c** - Terminal I/O, screen buffer management, rendering
4. **src/color.c** - Advanced RGB gradient system with 256-color conversion
5. **src/effects.c** - All 13 visual effect implementations
6. **src/utils.c** - Command-line argument parsing and utilities

### Key Data Structures

```c
// Character state with position, animation progress, and color
typedef struct {
    char ch, original_ch;
    coord_t pos, target;
    int visible, active;
    float progress;
    int color_fg, color_bg, bold;
} character_t;

// Terminal canvas with character array and dimensions
typedef struct {
    character_t *chars;
    int char_count;
    int terminal_width, terminal_height;
    int canvas_width, canvas_height;
    int text_width, text_height;
    int canvas_offset_x, canvas_offset_y;
    int text_offset_x, text_offset_y;
    int frame_count;
} terminal_t;

// Configuration with gradient system
typedef struct {
    int frame_rate, canvas_width, canvas_height;
    int no_final_newline;
    char *effect_name;
    anchor_t anchor_canvas, anchor_text;
    rgb_color_t gradient_stops[8];
    gradient_direction_t gradient_direction;
    // ... gradient configuration
} config_t;
```

### Advanced Gradient System

**Color Processing Pipeline**:
1. RGB definition → Interpolation → Position calculation → 256-color conversion → Application

**Gradient Types**:
- `GRADIENT_HORIZONTAL` - Left to right
- `GRADIENT_VERTICAL` - Top to bottom  
- `GRADIENT_DIAGONAL` - Corner to corner
- `GRADIENT_RADIAL` - Center outward
- `GRADIENT_ANGLE` - Custom angle (0-360°)

### Effect Implementation Pattern

All effects follow this standardized pattern in `src/effects.c`:

```c
void effect_name(terminal_t *term, int frame) {
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Calculate timing/position based on frame
        // Update ch->pos, ch->visible, ch->bold
        // Preserve gradient colors (don't override ch->color_fg)
        // Set ch->active = 0 when effect completes
    }
}
```

**Key Principles**:
- Effects enhance gradients, never override them
- Use `bold` flag to brighten existing colors
- Avoid hardcoded colors except special effects (matrix, fireworks, decrypt)
- Always set `active = 0` when character animation completes

## Current Effects (13 Implemented)

### Standard Effects (Random gradients applied)
- `beams`, `waves`, `rain`, `slide`, `expand`, `typewriter`, `wipe`, `spotlights`, `burn`, `swarm`

### Special Effects (Custom gradients)
- `matrix` - 5-stop radial green gradient
- `fireworks` - 6-stop radial fire gradient (red→orange→yellow→white)  
- `decrypt` - 4-stop diagonal green gradient

## Adding New Effects

1. Add function declaration to `src/tte.h`
2. Implement in `src/effects.c` following the standard pattern
3. Add to effect lookup table in `src/utils.c`
4. Update help text in `src/utils.c`
5. Add test case in `tests/test_tte.c`
6. Update README.md effect list

## Testing Framework

**Location**: `tests/test_tte.c`
**Test Coverage**:
- RGB color conversion accuracy
- Gradient interpolation correctness
- Terminal initialization/cleanup
- Effect function loading
- Memory usage validation

## Code Style Guidelines

- **C99 standard** compatibility
- **4-space indentation** with spaces not tabs
- **Descriptive variable names** (term, config, ch, frame)
- **Function naming**: `snake_case` for internal, `effect_name` for effects
- **Comments** for algorithm explanations, not obvious code

## Memory Management

- **Static allocation** during animation (no malloc/free in render loops)
- **Fixed buffers** for screen rendering (MAX_CHARS = 65536)
- **Efficient color caching** with change detection
- All character data fits in `terminal_t` structure

## Performance Characteristics

Compared to Python TTE:
- **13x faster startup** (~15ms vs ~200ms)
- **12x less memory** (~2MB vs ~25MB)
- **5x more CPU efficient**
- **Same visual quality** with full 256-color support

## Screensaver Integration

Critical feature: `--no-final-newline` flag eliminates scrolling behavior that makes Python TTE unusable for screensavers. Effect cycling system prevents repetitive displays.

## Original Project Attribution

Faithful C reimplementation of ChrisBuilds' `terminaltexteffects` library:
- **Original**: https://github.com/ChrisBuilds/terminaltexteffects
- **License**: MIT (maintained)
- **Visual compatibility**: Preserved algorithm designs and color fidelity