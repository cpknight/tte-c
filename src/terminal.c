#include "tte.h"

void get_terminal_size(int *width, int *height) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        *width = w.ws_col;
        *height = w.ws_row;
    } else {
        *width = 80;
        *height = 24;
    }
}

void init_terminal(terminal_t *term) {
    get_terminal_size(&term->terminal_width, &term->terminal_height);
    
    // Default canvas size to terminal size
    term->canvas_width = term->terminal_width;
    term->canvas_height = term->terminal_height;
    
    term->chars = malloc(MAX_CHARS * sizeof(character_t));
    term->char_count = 0;
    term->frame_count = 0;
}

void cleanup_terminal(terminal_t *term) {
    if (term->chars) {
        free(term->chars);
        term->chars = NULL;
    }
}

void read_input_text_with_config(terminal_t *term, config_t *config) {
    char buffer[4096];
    int row = 0;
    int col = 0;
    int max_col = 0;
    
    while (fgets(buffer, sizeof(buffer), stdin)) {
        int len = strlen(buffer);
        
        // Remove trailing newline
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            len--;
        }
        
        col = 0;
        for (int i = 0; i < len && term->char_count < MAX_CHARS; i++) {
            if (buffer[i] == '\t') {
                // Handle tabs with configurable tab width
                int spaces = config->tab_width - (col % config->tab_width);
                col += spaces;
            } else if (buffer[i] != ' ') {
                // Handle text wrapping if enabled
                if (config->wrap_text && col >= term->terminal_width) {
                    row++;
                    col = 0;
                }
                
                character_t *ch = &term->chars[term->char_count++];
                ch->ch = buffer[i];
                ch->original_ch = buffer[i];  // Store original for decrypt effect
                ch->target.row = row;
                ch->target.col = col;
                ch->pos.row = row;
                ch->pos.col = col;
                ch->visible = 0;
                ch->active = 1;
                ch->progress = 0.0f;
                ch->color_fg = 15;  // Default white
                ch->color_bg = -1;  // No background
                ch->bold = 0;
                col++;
            } else {
                col++;
            }
        }
        
        if (col > max_col) {
            max_col = col;
        }
        row++;
    }
    
    // Store actual text dimensions
    term->text_width = max_col;
    term->text_height = row;
    
    // Handle ignore terminal dimensions option
    if (config->ignore_terminal_dimensions) {
        // Use canvas dimensions instead of terminal dimensions
        if (term->canvas_width == 0) {
            term->canvas_width = max_col;
        }
        if (term->canvas_height == 0) {
            term->canvas_height = row;
        }
    } else {
        // Adjust canvas size if needed (default to text size if not specified)
        if (term->canvas_width == 0) {
            term->canvas_width = max_col;
        }
        if (term->canvas_height == 0) {
            term->canvas_height = row;
        }
    }
}

// Legacy function for backwards compatibility
void read_input_text(terminal_t *term) {
    config_t default_config = {
        .tab_width = 4,
        .wrap_text = 0,
        .ignore_terminal_dimensions = 0
    };
    read_input_text_with_config(term, &default_config);
}

void render_frame_with_config(terminal_t *term, config_t *config) {
    // Create screen buffer with color info
    static char screen[MAX_LINES][MAX_COLS];
    static int screen_fg[MAX_LINES][MAX_COLS];
    static int screen_bg[MAX_LINES][MAX_COLS];
    static int screen_bold[MAX_LINES][MAX_COLS];
    static int initialized = 0;
    
    if (!initialized) {
        for (int i = 0; i < MAX_LINES; i++) {
            for (int j = 0; j < MAX_COLS; j++) {
                screen[i][j] = ' ';
                screen_fg[i][j] = -1;
                screen_bg[i][j] = -1;
                screen_bold[i][j] = 0;
            }
        }
        initialized = 1;
    }
    
    // Clear screen buffer for current canvas area
    int start_row = term->canvas_offset_y;
    int start_col = term->canvas_offset_x;
    int end_row = start_row + term->canvas_height;
    int end_col = start_col + term->canvas_width;
    
    for (int i = start_row; i < end_row && i < MAX_LINES; i++) {
        for (int j = start_col; j < end_col && j < MAX_COLS; j++) {
            screen[i][j] = ' ';
            screen_fg[i][j] = -1;
            screen_bg[i][j] = -1;
            screen_bold[i][j] = 0;
        }
    }
    
    // Render background effects if enabled
    if (config && config->background_effect != BACKGROUND_NONE) {
        render_background_to_screen(screen, screen_fg, screen_bg, screen_bold, 
                                  term, config, term->frame_count);
    }
    
    // Place visible characters with positioning and color
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        if (ch->visible) {
            // Apply text offset and canvas offset
            int final_row = ch->pos.row + term->text_offset_y + term->canvas_offset_y;
            int final_col = ch->pos.col + term->text_offset_x + term->canvas_offset_x;
            
            if (final_row >= 0 && final_row < term->terminal_height &&
                final_col >= 0 && final_col < term->terminal_width &&
                final_row < MAX_LINES && final_col < MAX_COLS) {
                screen[final_row][final_col] = ch->ch;
                screen_fg[final_row][final_col] = ch->color_fg;
                screen_bg[final_row][final_col] = ch->color_bg;
                screen_bold[final_row][final_col] = ch->bold;
            }
        }
    }
    
    // Output to terminal with colors
    printf(ANSI_CURSOR_HOME);
    char color_buffer[64];
    int current_fg = -1, current_bg = -1, current_bold = 0;
    
    for (int i = 0; i < term->terminal_height && i < MAX_LINES; i++) {
        for (int j = 0; j < term->terminal_width && j < MAX_COLS; j++) {
            // Check if color needs to change
            if (screen_fg[i][j] != current_fg || screen_bg[i][j] != current_bg || 
                screen_bold[i][j] != current_bold) {
                // Only output color codes if we have valid color data for non-space chars
                if (screen[i][j] != ' ' && screen_fg[i][j] >= 0) {
                    format_color_256_with_config(color_buffer, screen_fg[i][j], screen_bg[i][j], screen_bold[i][j], config);
                    printf("%s", color_buffer);
                }
                current_fg = screen_fg[i][j];
                current_bg = screen_bg[i][j];
                current_bold = screen_bold[i][j];
            }
            putchar(screen[i][j]);
        }
        if (i < term->terminal_height - 1) {
            putchar('\n');
        }
    }
    if (!config || !config->no_color) {
        printf(ANSI_RESET);  // Reset colors at end unless no-color is enabled
    }
    fflush(stdout);
}

// Legacy function for backwards compatibility
void render_frame(terminal_t *term) {
    render_frame_with_config(term, NULL);
}

void sleep_frame(int frame_rate) {
    if (frame_rate > 0) {
        struct timespec req = {
            .tv_sec = 0,
            .tv_nsec = (1000000000L / frame_rate)
        };
        nanosleep(&req, NULL);
    }
}