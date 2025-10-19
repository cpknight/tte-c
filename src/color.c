#include "tte.h"

// 256-color lookup table for RGB conversion
static const int color_cube[6] = {0, 95, 135, 175, 215, 255};

void format_color_256(char *buffer, int fg, int bg, int bold) {
    if (fg >= 0 && bg >= 0) {
        if (bold) {
            sprintf(buffer, "\033[1;38;5;%d;48;5;%dm", fg, bg);
        } else {
            sprintf(buffer, "\033[38;5;%d;48;5;%dm", fg, bg);
        }
    } else if (fg >= 0) {
        if (bold) {
            sprintf(buffer, "\033[1;38;5;%dm", fg);
        } else {
            sprintf(buffer, "\033[38;5;%dm", fg);
        }
    } else if (bg >= 0) {
        sprintf(buffer, "\033[48;5;%dm", bg);
    } else {
        if (bold) {
            sprintf(buffer, "\033[1m");
        } else {
            sprintf(buffer, "\033[0m");
        }
    }
}

int rgb_to_256(int r, int g, int b) {
    // Clamp values
    r = (r < 0) ? 0 : (r > 255) ? 255 : r;
    g = (g < 0) ? 0 : (g > 255) ? 255 : g;
    b = (b < 0) ? 0 : (b > 255) ? 255 : b;
    
    // Convert to 6x6x6 color cube
    int ri = 0, gi = 0, bi = 0;
    for (int i = 1; i < 6; i++) {
        if (r > color_cube[i-1] + (color_cube[i] - color_cube[i-1]) / 2) ri = i;
        if (g > color_cube[i-1] + (color_cube[i] - color_cube[i-1]) / 2) gi = i;
        if (b > color_cube[i-1] + (color_cube[i] - color_cube[i-1]) / 2) bi = i;
    }
    
    return 16 + 36 * ri + 6 * gi + bi;
}

rgb_color_t interpolate_rgb(rgb_color_t color1, rgb_color_t color2, float progress) {
    // Clamp progress
    if (progress <= 0.0f) return color1;
    if (progress >= 1.0f) return color2;
    
    // Linear RGB interpolation
    rgb_color_t result;
    result.r = (int)(color1.r + (color2.r - color1.r) * progress);
    result.g = (int)(color1.g + (color2.g - color1.g) * progress);
    result.b = (int)(color1.b + (color2.b - color1.b) * progress);
    
    return result;
}

rgb_color_t interpolate_gradient(rgb_color_t *stops, int count, float position) {
    if (count <= 1) {
        return count == 1 ? stops[0] : (rgb_color_t){255, 255, 255};
    }
    
    // Clamp position
    if (position <= 0.0f) return stops[0];
    if (position >= 1.0f) return stops[count - 1];
    
    // Find which segment we're in
    float segment_size = 1.0f / (count - 1);
    int segment = (int)(position / segment_size);
    
    if (segment >= count - 1) {
        return stops[count - 1];
    }
    
    // Local position within this segment
    float local_pos = (position - (segment * segment_size)) / segment_size;
    
    return interpolate_rgb(stops[segment], stops[segment + 1], local_pos);
}

float calculate_gradient_position(int row, int col, int width, int height, 
                                gradient_direction_t direction, float angle) {
    float position = 0.0f;
    
    switch (direction) {
        case GRADIENT_HORIZONTAL:
            position = (float)col / (float)(width - 1);
            break;
            
        case GRADIENT_VERTICAL:
            position = (float)row / (float)(height - 1);
            break;
            
        case GRADIENT_DIAGONAL:
            // Diagonal from top-left to bottom-right
            position = ((float)col / (float)(width - 1) + (float)row / (float)(height - 1)) / 2.0f;
            break;
            
        case GRADIENT_RADIAL:
            // Radial from center
            {
                float center_x = (width - 1) / 2.0f;
                float center_y = (height - 1) / 2.0f;
                float dx = col - center_x;
                float dy = row - center_y;
                float distance = sqrt(dx * dx + dy * dy);
                float max_distance = sqrt(center_x * center_x + center_y * center_y);
                position = distance / max_distance;
            }
            break;
            
        case GRADIENT_ANGLE:
            // Custom angle gradient
            {
                float rad = angle * M_PI / 180.0f;
                float cos_a = cos(rad);
                float sin_a = sin(rad);
                
                // Normalize coordinates to [-1, 1]
                float x = (2.0f * col / (width - 1)) - 1.0f;
                float y = (2.0f * row / (height - 1)) - 1.0f;
                
                // Project onto angle direction
                float projected = x * cos_a + y * sin_a;
                
                // Convert to [0, 1]
                position = (projected + sqrt(2.0f)) / (2.0f * sqrt(2.0f));
            }
            break;
    }
    
    // Clamp to [0, 1]
    if (position < 0.0f) position = 0.0f;
    if (position > 1.0f) position = 1.0f;
    
    return position;
}

void setup_gradient_colors(config_t *config, const char *effect_name) {
    // Set up rich, complex gradients based on effect
    
    if (strcmp(effect_name, "matrix") == 0) {
        // Matrix: Rich green spectrum
        config->gradient_stops[0] = (rgb_color_t){0, 64, 0};      // Dark green
        config->gradient_stops[1] = (rgb_color_t){0, 128, 0};     // Forest green  
        config->gradient_stops[2] = (rgb_color_t){64, 192, 64};   // Medium green
        config->gradient_stops[3] = (rgb_color_t){128, 255, 128}; // Light green
        config->gradient_stops[4] = (rgb_color_t){192, 255, 192}; // Pale green
        config->gradient_count = 5;
        config->gradient_direction = GRADIENT_RADIAL;
        
    } else if (strcmp(effect_name, "fireworks") == 0) {
        // Fireworks: Fire spectrum
        config->gradient_stops[0] = (rgb_color_t){128, 0, 0};     // Dark red
        config->gradient_stops[1] = (rgb_color_t){255, 64, 0};    // Red-orange
        config->gradient_stops[2] = (rgb_color_t){255, 128, 0};   // Orange
        config->gradient_stops[3] = (rgb_color_t){255, 192, 0};   // Yellow-orange  
        config->gradient_stops[4] = (rgb_color_t){255, 255, 64};  // Yellow
        config->gradient_stops[5] = (rgb_color_t){255, 255, 192}; // Pale yellow
        config->gradient_count = 6;
        config->gradient_direction = GRADIENT_RADIAL;
        
    } else if (strcmp(effect_name, "decrypt") == 0) {
        // Decrypt: Terminal green
        config->gradient_stops[0] = (rgb_color_t){0, 80, 0};      // Dark terminal green
        config->gradient_stops[1] = (rgb_color_t){0, 160, 0};     // Medium green
        config->gradient_stops[2] = (rgb_color_t){64, 255, 64};   // Bright green
        config->gradient_stops[3] = (rgb_color_t){128, 255, 128}; // Light green
        config->gradient_count = 4;
        config->gradient_direction = GRADIENT_DIAGONAL;
        
    } else {
        // Default: Rich blue-cyan-white spectrum
        config->gradient_stops[0] = (rgb_color_t){0, 64, 128};    // Dark blue
        config->gradient_stops[1] = (rgb_color_t){0, 96, 192};    // Blue
        config->gradient_stops[2] = (rgb_color_t){0, 128, 255};   // Bright blue  
        config->gradient_stops[3] = (rgb_color_t){64, 192, 255};  // Light blue
        config->gradient_stops[4] = (rgb_color_t){128, 224, 255}; // Cyan
        config->gradient_stops[5] = (rgb_color_t){192, 240, 255}; // Light cyan
        config->gradient_stops[6] = (rgb_color_t){224, 248, 255}; // Pale cyan
        config->gradient_stops[7] = (rgb_color_t){255, 255, 255}; // White
        config->gradient_count = 8;
        
        // Random gradient direction for variety
        int directions[] = {GRADIENT_HORIZONTAL, GRADIENT_VERTICAL, GRADIENT_DIAGONAL, GRADIENT_RADIAL, GRADIENT_ANGLE};
        config->gradient_direction = directions[rand() % 5];
        if (config->gradient_direction == GRADIENT_ANGLE) {
            config->gradient_angle = (rand() % 360);  // Random angle
        }
    }
    
    config->gradient_steps = 64;  // High resolution for smooth gradients
}

void calculate_offsets(terminal_t *term, anchor_t canvas_anchor, anchor_t text_anchor) {
    // Calculate canvas position in terminal
    switch (canvas_anchor) {
        case ANCHOR_SW:
            term->canvas_offset_x = 0;
            term->canvas_offset_y = term->terminal_height - term->canvas_height;
            break;
        case ANCHOR_S:
            term->canvas_offset_x = (term->terminal_width - term->canvas_width) / 2;
            term->canvas_offset_y = term->terminal_height - term->canvas_height;
            break;
        case ANCHOR_SE:
            term->canvas_offset_x = term->terminal_width - term->canvas_width;
            term->canvas_offset_y = term->terminal_height - term->canvas_height;
            break;
        case ANCHOR_E:
            term->canvas_offset_x = term->terminal_width - term->canvas_width;
            term->canvas_offset_y = (term->terminal_height - term->canvas_height) / 2;
            break;
        case ANCHOR_NE:
            term->canvas_offset_x = term->terminal_width - term->canvas_width;
            term->canvas_offset_y = 0;
            break;
        case ANCHOR_N:
            term->canvas_offset_x = (term->terminal_width - term->canvas_width) / 2;
            term->canvas_offset_y = 0;
            break;
        case ANCHOR_NW:
            term->canvas_offset_x = 0;
            term->canvas_offset_y = 0;
            break;
        case ANCHOR_W:
            term->canvas_offset_x = 0;
            term->canvas_offset_y = (term->terminal_height - term->canvas_height) / 2;
            break;
        case ANCHOR_C:
        default:
            term->canvas_offset_x = (term->terminal_width - term->canvas_width) / 2;
            term->canvas_offset_y = (term->terminal_height - term->canvas_height) / 2;
            break;
    }
    
    // Calculate text position within canvas
    switch (text_anchor) {
        case ANCHOR_SW:
            term->text_offset_x = 0;
            term->text_offset_y = term->canvas_height - term->text_height;
            break;
        case ANCHOR_S:
            term->text_offset_x = (term->canvas_width - term->text_width) / 2;
            term->text_offset_y = term->canvas_height - term->text_height;
            break;
        case ANCHOR_SE:
            term->text_offset_x = term->canvas_width - term->text_width;
            term->text_offset_y = term->canvas_height - term->text_height;
            break;
        case ANCHOR_E:
            term->text_offset_x = term->canvas_width - term->text_width;
            term->text_offset_y = (term->canvas_height - term->text_height) / 2;
            break;
        case ANCHOR_NE:
            term->text_offset_x = term->canvas_width - term->text_width;
            term->text_offset_y = 0;
            break;
        case ANCHOR_N:
            term->text_offset_x = (term->canvas_width - term->text_width) / 2;
            term->text_offset_y = 0;
            break;
        case ANCHOR_NW:
            term->text_offset_x = 0;
            term->text_offset_y = 0;
            break;
        case ANCHOR_W:
            term->text_offset_x = 0;
            term->text_offset_y = (term->canvas_height - term->text_height) / 2;
            break;
        case ANCHOR_C:
        default:
            term->text_offset_x = (term->canvas_width - term->text_width) / 2;
            term->text_offset_y = (term->canvas_height - term->text_height) / 2;
            break;
    }
}

int get_gradient_color(int *gradient_colors, int gradient_count, float position) {
    if (gradient_count <= 1) {
        return gradient_count == 1 ? gradient_colors[0] : 15; // Default white
    }
    
    // Clamp position to [0, 1]
    if (position <= 0.0f) return gradient_colors[0];
    if (position >= 1.0f) return gradient_colors[gradient_count - 1];
    
    // Find which segment of the gradient we're in
    float segment_size = 1.0f / (gradient_count - 1);
    int segment = (int)(position / segment_size);
    
    // Handle edge case where position == 1.0
    if (segment >= gradient_count - 1) {
        return gradient_colors[gradient_count - 1];
    }
    
    // Interpolate between the two colors in this segment
    float local_position = (position - (segment * segment_size)) / segment_size;
    
    // For now, simple threshold-based interpolation
    // Real tte does RGB interpolation, but this gives good results
    if (local_position < 0.5f) {
        return gradient_colors[segment];
    } else {
        return gradient_colors[segment + 1];
    }
}

void apply_initial_gradient(terminal_t *term, config_t *config) {
    if (!config->use_gradient || config->gradient_count == 0) {
        return;
    }
    
    // Apply rich gradient to all characters at initialization
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Calculate gradient position based on direction
        float grad_pos = calculate_gradient_position(
            ch->target.row, ch->target.col,
            term->text_width, term->text_height,
            config->gradient_direction, config->gradient_angle
        );
        
        // Get RGB color from gradient
        rgb_color_t rgb = interpolate_gradient(config->gradient_stops, config->gradient_count, grad_pos);
        
        // Convert to 256-color and apply
        ch->color_fg = rgb_to_256(rgb.r, rgb.g, rgb.b);
        ch->bold = 0;
    }
}

void apply_final_gradient(terminal_t *term, config_t *config) {
    if (!config->use_gradient || config->gradient_count == 0) {
        return;
    }
    
    // Apply rich gradient to completed characters
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        if (!ch->active) { // Only apply to completed characters
            // Calculate gradient position based on direction
            float grad_pos = calculate_gradient_position(
                ch->target.row, ch->target.col,
                term->text_width, term->text_height,
                config->gradient_direction, config->gradient_angle
            );
            
            // Get RGB color from gradient
            rgb_color_t rgb = interpolate_gradient(config->gradient_stops, config->gradient_count, grad_pos);
            
            // Convert to 256-color and apply
            ch->color_fg = rgb_to_256(rgb.r, rgb.g, rgb.b);
            ch->bold = 0; // Final text usually not bold
        }
    }
}
