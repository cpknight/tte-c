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

#endif // TTE_H