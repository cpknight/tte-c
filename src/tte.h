#ifndef TTE_H
#define TTE_H

// For nanosleep and timespec
#define _POSIX_C_SOURCE 199309L
#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_LINES 1024
#define MAX_COLS 1024
#define MAX_CHARS 65536
#define DEFAULT_FRAME_RATE 240

// ANSI escape sequences
#define ANSI_CLEAR_SCREEN "\033[2J"
#define ANSI_CURSOR_HOME "\033[H"
#define ANSI_CURSOR_UP "\033[A"
#define ANSI_HIDE_CURSOR "\033[?25l"
#define ANSI_SHOW_CURSOR "\033[?25h"
#define ANSI_SAVE_CURSOR "\033[s"
#define ANSI_RESTORE_CURSOR "\033[u"

// Color support
#define ANSI_RESET "\033[0m"

// Text anchoring
typedef enum {
    ANCHOR_SW, ANCHOR_S, ANCHOR_SE,
    ANCHOR_E, ANCHOR_NE, ANCHOR_N,
    ANCHOR_NW, ANCHOR_W, ANCHOR_C
} anchor_t;

// Gradient directions
typedef enum {
    GRADIENT_HORIZONTAL,
    GRADIENT_VERTICAL, 
    GRADIENT_DIAGONAL,
    GRADIENT_RADIAL,
    GRADIENT_ANGLE
} gradient_direction_t;

// Easing function types
typedef enum {
    EASE_LINEAR,
    EASE_IN_QUAD, EASE_OUT_QUAD, EASE_IN_OUT_QUAD,
    EASE_IN_CUBIC, EASE_OUT_CUBIC, EASE_IN_OUT_CUBIC,
    EASE_IN_QUART, EASE_OUT_QUART, EASE_IN_OUT_QUART,
    EASE_IN_QUINT, EASE_OUT_QUINT, EASE_IN_OUT_QUINT,
    EASE_IN_SINE, EASE_OUT_SINE, EASE_IN_OUT_SINE,
    EASE_IN_EXPO, EASE_OUT_EXPO, EASE_IN_OUT_EXPO,
    EASE_IN_CIRC, EASE_OUT_CIRC, EASE_IN_OUT_CIRC,
    EASE_IN_BACK, EASE_OUT_BACK, EASE_IN_OUT_BACK,
    EASE_IN_ELASTIC, EASE_OUT_ELASTIC, EASE_IN_OUT_ELASTIC,
    EASE_IN_BOUNCE, EASE_OUT_BOUNCE, EASE_IN_OUT_BOUNCE
} easing_t;

// Background effect types
typedef enum {
    BACKGROUND_NONE,
    BACKGROUND_STARS,
    BACKGROUND_MATRIX_RAIN,
    BACKGROUND_PARTICLES,
    BACKGROUND_GRID,
    BACKGROUND_WAVES,
    BACKGROUND_PLASMA
} background_effect_t;

// HSV color structure
typedef struct {
    float h, s, v; // Hue (0-360), Saturation (0-1), Value (0-1)
} hsv_color_t;

// Gradient preset types
typedef enum {
    GRADIENT_PRESET_CUSTOM,
    GRADIENT_PRESET_RAINBOW,
    GRADIENT_PRESET_FIRE,
    GRADIENT_PRESET_OCEAN,
    GRADIENT_PRESET_SUNSET,
    GRADIENT_PRESET_FOREST,
    GRADIENT_PRESET_ICE,
    GRADIENT_PRESET_NEON,
    GRADIENT_PRESET_PASTEL
} gradient_preset_t;

// RGB color structure for interpolation
typedef struct {
    int r, g, b;
} rgb_color_t;

typedef struct {
    int row;
    int col;
} coord_t;

typedef struct {
    char ch;
    char original_ch;  // Store original character for decrypt effect
    coord_t pos;
    coord_t target;
    int visible;
    int active;
    float progress;
    int color_fg;      // 256-color foreground
    int color_bg;      // 256-color background
    int bold;
} character_t;

typedef struct {
    int frame_rate;
    int canvas_width;
    int canvas_height;
    int no_final_newline;
    char *effect_name;
    anchor_t anchor_canvas;
    anchor_t anchor_text;
    int use_gradient;
    rgb_color_t gradient_stops[8];  // RGB color stops
    int gradient_count;
    gradient_direction_t gradient_direction;
    float gradient_angle;  // For angled gradients (degrees)
    int gradient_steps;   // Number of color steps to generate
    gradient_preset_t gradient_preset;
    
    // Background effects
    background_effect_t background_effect;
    int background_intensity;  // 0-100
    
    // Advanced gradient options
    char *gradient_colors_string;  // Custom gradient colors from command line
    int auto_gradient;             // Generate gradient automatically
    
    // New command line options
    int ignore_terminal_dimensions;
    int wrap_text;
    int tab_width;
    int xterm_colors;  // Force 8-bit color mode
    int no_color;      // Disable all colors
} config_t;

typedef struct {
    character_t *chars;
    int char_count;
    int terminal_width;
    int terminal_height;
    int canvas_width;
    int canvas_height;
    int text_width;
    int text_height;
    int canvas_offset_x;
    int canvas_offset_y;
    int text_offset_x;
    int text_offset_y;
    int frame_count;
} terminal_t;

// Effect function pointer type
typedef void (*effect_func_t)(terminal_t *term, int frame);

// Core functions
void init_terminal(terminal_t *term);
void cleanup_terminal(terminal_t *term);
void get_terminal_size(int *width, int *height);
void read_input_text(terminal_t *term);
void read_input_text_with_config(terminal_t *term, config_t *config);
void render_frame(terminal_t *term);
void render_frame_with_config(terminal_t *term, config_t *config);
void sleep_frame(int frame_rate);

// Effect functions
void effect_beams(terminal_t *term, int frame);
void effect_waves(terminal_t *term, int frame);
void effect_rain(terminal_t *term, int frame);
void effect_slide(terminal_t *term, int frame);
void effect_expand(terminal_t *term, int frame);
void effect_matrix(terminal_t *term, int frame);
void effect_fireworks(terminal_t *term, int frame);
void effect_decrypt(terminal_t *term, int frame);
void effect_typewriter(terminal_t *term, int frame);
void effect_wipe(terminal_t *term, int frame);
void effect_spotlights(terminal_t *term, int frame);
void effect_burn(terminal_t *term, int frame);
void effect_swarm(terminal_t *term, int frame);
void effect_highlight(terminal_t *term, int frame);
void effect_unstable(terminal_t *term, int frame);
void effect_crumble(terminal_t *term, int frame);
void effect_slice(terminal_t *term, int frame);
void effect_pour(terminal_t *term, int frame);
void effect_blackhole(terminal_t *term, int frame);
void effect_rings(terminal_t *term, int frame);
void effect_synthgrid(terminal_t *term, int frame);

// Utility functions
void parse_args(int argc, char *argv[], config_t *config);
effect_func_t get_effect_function(const char *effect_name);
void print_usage(const char *program_name);
anchor_t parse_anchor(const char *anchor_str);

// Color functions
void format_color_256(char *buffer, int fg, int bg, int bold);
void format_color_256_with_config(char *buffer, int fg, int bg, int bold, config_t *config);
rgb_color_t interpolate_rgb(rgb_color_t color1, rgb_color_t color2, float progress);
rgb_color_t interpolate_gradient(rgb_color_t *stops, int count, float position);
int rgb_to_256(int r, int g, int b);
float calculate_gradient_position(int row, int col, int width, int height, 
                                gradient_direction_t direction, float angle);
void setup_gradient_colors(config_t *config, const char *effect_name);
void apply_initial_gradient(terminal_t *term, config_t *config);
void apply_final_gradient(terminal_t *term, config_t *config);

// Anchoring functions
void calculate_offsets(terminal_t *term, anchor_t canvas_anchor, anchor_t text_anchor);

// Easing functions
float apply_easing(float t, easing_t easing);
float ease_in_quad(float t);
float ease_out_quad(float t);
float ease_in_out_quad(float t);
float ease_in_cubic(float t);
float ease_out_cubic(float t);
float ease_in_out_cubic(float t);
float ease_in_sine(float t);
float ease_out_sine(float t);
float ease_in_out_sine(float t);
float ease_out_bounce(float t);
float ease_out_elastic(float t);
float ease_out_back(float t);

// HSV color functions
hsv_color_t rgb_to_hsv(rgb_color_t rgb);
rgb_color_t hsv_to_rgb(hsv_color_t hsv);
rgb_color_t color_wheel(float position);

// Advanced gradient functions
void setup_gradient_preset(config_t *config, gradient_preset_t preset);
void parse_gradient_colors(config_t *config, const char *colors_string);
void generate_auto_gradient(config_t *config, int seed);

// Background effects
void render_background(terminal_t *term, config_t *config, int frame);
void render_stars_background(terminal_t *term, config_t *config, int frame);
void render_matrix_background(terminal_t *term, config_t *config, int frame);
void render_particles_background(terminal_t *term, config_t *config, int frame);
void render_grid_background(terminal_t *term, config_t *config, int frame);
void render_waves_background(terminal_t *term, config_t *config, int frame);
void render_plasma_background(terminal_t *term, config_t *config, int frame);
void render_background_to_screen(char screen[MAX_LINES][MAX_COLS], 
                                int screen_fg[MAX_LINES][MAX_COLS],
                                int screen_bg[MAX_LINES][MAX_COLS],
                                int screen_bold[MAX_LINES][MAX_COLS],
                                terminal_t *term, config_t *config, int frame);

#endif // TTE_H