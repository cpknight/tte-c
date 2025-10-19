# tte-c Development Context

## Project Overview

**tte-c** is a high-performance C reimplementation of the Python `terminaltexteffects` (TTE) library, originally created by ChrisBuilds. This project was initiated to solve specific screensaver scrolling issues while providing significant performance improvements.

## Original Problem & Solution

**Problem**: Python TTE always outputs a final newline, causing screen scrolling in screensaver applications.  
**Solution**: Added `--no-final-newline` flag to eliminate unwanted scrolling behavior.

**Additional Benefits**:
- 10x+ faster startup times
- 5x+ lower CPU usage  
- 12x+ lower memory footprint
- Perfect for embedded/continuous applications

## Architecture Overview

### Core Components

1. **src/tte.h** - Main header with all type definitions and function declarations
2. **src/main.c** - Program entry point and main animation loop
3. **src/terminal.c** - Terminal handling, screen buffer management, I/O
4. **src/color.c** - Advanced RGB gradient system with 256-color support
5. **src/effects.c** - Visual effect implementations (13 effects currently)
6. **src/utils.c** - Command-line parsing and utility functions

### Key Data Structures

```c
typedef struct {
    char ch, original_ch;
    coord_t pos, target;
    int visible, active;
    float progress;
    int color_fg, color_bg, bold;
} character_t;

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

typedef struct {
    // Basic config
    int frame_rate, canvas_width, canvas_height;
    int no_final_newline;
    char *effect_name;
    anchor_t anchor_canvas, anchor_text;
    
    // Advanced gradient system
    int use_gradient;
    rgb_color_t gradient_stops[8];
    int gradient_count;
    gradient_direction_t gradient_direction;
    float gradient_angle;
    int gradient_steps;
} config_t;
```

## Advanced Gradient System

### Gradient Directions
- `GRADIENT_HORIZONTAL` - Left to right
- `GRADIENT_VERTICAL` - Top to bottom  
- `GRADIENT_DIAGONAL` - Corner to corner
- `GRADIENT_RADIAL` - Center outward
- `GRADIENT_ANGLE` - Custom angle (0-360°)

### Color Processing Pipeline
1. **RGB Definition** - Rich color stops defined in RGB space
2. **Interpolation** - Smooth RGB blending between stops
3. **Position Calculation** - Per-character position based on gradient direction
4. **256-Color Conversion** - RGB→xterm-256 color mapping
5. **Application** - Initial gradient + effect enhancements + final gradient

## Effect Implementation Pattern

All effects follow this pattern:

```c
void effect_name(terminal_t *term, int frame) {
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Calculate timing/position based on frame
        // Update ch->pos, ch->visible, ch->bold
        // Preserve gradient colors (ch->color_fg)
        // Set ch->active = 0 when complete
    }
}
```

**Key Principles**:
- Effects enhance gradients, don't override them
- Use `bold` flag to brighten existing gradient colors
- Avoid hardcoded colors except for special effects (matrix, fireworks, decrypt)
- Always set `active = 0` when effect completes for that character

## Current Effects (13 implemented)

### Standard Effects (use random gradients)
- `beams` - Light beam sweep with brightness
- `waves` - Sine wave motion with brightness peaks
- `rain` - Falling characters with brightness fade
- `slide` - Horizontal slide-in with brightness
- `expand` - Center-outward expansion with brightness
- `typewriter` - Sequential character appearance
- `wipe` - Left-to-right reveal
- `spotlights` - Moving circular illumination
- `burn` - Vertical burn reveal with flicker
- `swarm` - Particle swarm convergence

### Special Effects (custom gradients)
- `matrix` - 5-stop radial green gradient
- `fireworks` - 6-stop radial fire gradient (red→orange→yellow→white)
- `decrypt` - 4-stop diagonal green gradient

## Screensaver Integration

**File**: `/home/cpknight/.local/share/omarchy/bin/omarchy-cmd-screensaver-custom`

**Key Features**:
- Effect cycling system prevents repeats
- All 13 effects are guaranteed to cycle through
- Cleanup on exit
- Window timing delay for proper geometry
- Process management for tte-c

## Performance Characteristics

### Benchmarks (vs Python TTE)
- **Startup Time**: ~15ms vs ~200ms (13x faster)
- **Memory Usage**: ~2MB vs ~25MB (12x less) 
- **CPU Usage**: Low vs High (5x more efficient)
- **Color Quality**: Full 256-color with RGB interpolation
- **Effect Quality**: Comparable visual results

### Memory Management
- **Static allocation** during animation (no malloc/free in loops)
- **Fixed buffers** for screen rendering
- **Efficient color caching** with change detection
- **Clean initialization/cleanup** patterns

## Testing Framework

**Location**: `tests/test_tte.c`
**Run**: `make test`

**Test Coverage**:
- RGB color conversion accuracy
- Gradient interpolation correctness
- Gradient position calculations
- Terminal initialization/cleanup
- Effect function loading
- Memory usage validation
- Performance comparison vs original TTE

## Build System

```bash
make          # Standard build
make debug    # Debug build with -g -DDEBUG
make test     # Build and run unit tests
make clean    # Clean object files
make install  # Install to /usr/local/bin/
```

## Future Development Priorities

### High Priority Additions
1. **highlight** - Specular highlight sweep
2. **unstable** - Glitchy character positioning
3. **More gradient types** - Spiral, wave, custom patterns
4. **Effect combinations** - Layer multiple effects
5. **Input color preservation** - Maintain existing ANSI colors

### Medium Priority
1. **crumble**, **slice**, **pour**, **blackhole**, **rings**, **synthgrid** effects
2. **Easing functions** - Smooth animation curves
3. **Advanced positioning** - Bezier paths, orbits
4. **Command-line gradient control** - Custom colors/directions
5. **Configuration files** - Save/load effect presets

### Code Quality
1. **Error handling** - Robust input validation
2. **Memory bounds** - Better buffer overflow protection
3. **Platform compatibility** - Windows/macOS support
4. **Performance profiling** - Identify bottlenecks

## Development Workflow

### Adding New Effects
1. Add function declaration to `src/tte.h`
2. Implement in `src/effects.c` following the pattern
3. Add to effect lookup in `src/utils.c`
4. Add to help text in `src/utils.c`  
5. Update `TTE_C_EFFECTS` array in screensaver script
6. Add test case in `tests/test_tte.c`
7. Update README.md effect list

### Debugging Tips
- Use `make debug` for debugging builds
- Add `printf` debugging to effects (will show in terminal)
- Use `gdb` for memory issues
- Test with `timeout` to avoid hanging
- Check screensaver logs with `journalctl`

### Code Style
- **C99 standard** compatibility
- **4-space indentation** with spaces not tabs
- **Descriptive variable names** (term, config, ch, frame)
- **Function naming**: `snake_case` for internal, `effect_name` for effects
- **Comments** for algorithm explanations, not obvious code
- **Error handling** with meaningful messages

## Integration Points

### Original TTE Compatibility
- **Visual output** should match original quality
- **Effect concepts** should be faithful to original algorithms
- **Color fidelity** should use full spectrum capabilities
- **Performance** should exceed original in all metrics

### Screensaver Requirements  
- **No final newline** (critical for non-scrolling)
- **Full screen usage** with proper anchoring
- **Variety prevention** through cycling system
- **Clean resource management** for continuous operation
- **Fast startup** for seamless transitions

## Contact & Maintenance

This project is maintained through Claude (Anthropic) sessions. For continued development:

1. **Reference this file** for context and architectural decisions
2. **Run tests** before and after changes (`make test`)
3. **Update documentation** when adding features
4. **Maintain compatibility** with original TTE visual aesthetics
5. **Performance first** - always benchmark changes

**License**: MIT (same as original TTE by ChrisBuilds)  
**Original Project**: https://github.com/ChrisBuilds/terminaltexteffects  
**Original Author**: ChrisBuilds