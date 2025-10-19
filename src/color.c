#include "tte.h"

// 256-color lookup table for RGB conversion
static const int color_cube[6] = {0, 95, 135, 175, 215, 255};

void format_color_256_with_config(char *buffer, int fg, int bg, int bold, config_t *config) {
    // Handle no-color option
    if (config && config->no_color) {
        if (bold) {
            sprintf(buffer, "\033[1m");
        } else {
            buffer[0] = '\0';  // No color codes
        }
        return;
    }
    
    // Handle xterm-colors option (limit to basic 16 colors)
    if (config && config->xterm_colors) {
        if (fg >= 0) fg = fg % 16;
        if (bg >= 0) bg = bg % 16;
    }
    
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

// Legacy function for backwards compatibility
void format_color_256(char *buffer, int fg, int bg, int bold) {
    format_color_256_with_config(buffer, fg, bg, bold, NULL);
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
    // Bounds checking
    if (!stops || count <= 0) {
        return (rgb_color_t){255, 255, 255}; // Default white
    }
    
    if (count == 1) {
        return stops[0];
    }
    
    // Clamp position to valid range
    if (position <= 0.0f) return stops[0];
    if (position >= 1.0f) return stops[count - 1];
    
    // Check for invalid position (NaN, infinity)
    if (position != position || position < 0.0f || position > 1.0f) {
        return stops[0]; // Return first color for invalid positions
    }
    
    // Find which segment we're in
    float segment_size = 1.0f / (float)(count - 1);
    int segment = (int)(position / segment_size);
    
    // Bounds checking for segment
    if (segment < 0) segment = 0;
    if (segment >= count - 1) {
        return stops[count - 1];
    }
    
    // Local position within this segment
    float local_pos = (position - (segment * segment_size)) / segment_size;
    
    // Clamp local position
    if (local_pos < 0.0f) local_pos = 0.0f;
    if (local_pos > 1.0f) local_pos = 1.0f;
    
    return interpolate_rgb(stops[segment], stops[segment + 1], local_pos);
}

float calculate_gradient_position(int row, int col, int width, int height, 
                                gradient_direction_t direction, float angle) {
    float position = 0.0f;
    
    // Protect against invalid dimensions
    if (width <= 0 || height <= 0) {
        return 0.0f;
    }
    
    switch (direction) {
        case GRADIENT_HORIZONTAL:
            position = (width > 1) ? (float)col / (float)(width - 1) : 0.0f;
            break;
            
        case GRADIENT_VERTICAL:
            position = (height > 1) ? (float)row / (float)(height - 1) : 0.0f;
            break;
            
        case GRADIENT_DIAGONAL:
            // Diagonal from top-left to bottom-right
            if (width > 1 && height > 1) {
                position = ((float)col / (float)(width - 1) + (float)row / (float)(height - 1)) / 2.0f;
            } else {
                position = 0.0f;
            }
            break;
            
        case GRADIENT_RADIAL:
            // Radial from center
            {
                if (width == 1 && height == 1) {
                    position = 0.0f;
                } else {
                    float center_x = (width - 1) / 2.0f;
                    float center_y = (height - 1) / 2.0f;
                    float dx = col - center_x;
                    float dy = row - center_y;
                    float distance = sqrt(dx * dx + dy * dy);
                    float max_distance = sqrt(center_x * center_x + center_y * center_y);
                    
                    if (max_distance > 0.0f) {
                        position = distance / max_distance;
                    } else {
                        position = 0.0f;
                    }
                }
            }
            break;
            
        case GRADIENT_ANGLE:
            // Custom angle gradient
            {
                if (width > 1 && height > 1) {
                    float rad = angle * M_PI / 180.0f;
                    float cos_a = cos(rad);
                    float sin_a = sin(rad);
                    
                    // Normalize coordinates to [-1, 1]
                    float x = (2.0f * col / (float)(width - 1)) - 1.0f;
                    float y = (2.0f * row / (float)(height - 1)) - 1.0f;
                    
                    // Project onto angle direction
                    float projected = x * cos_a + y * sin_a;
                    
                    // Convert to [0, 1]
                    position = (projected + sqrt(2.0f)) / (2.0f * sqrt(2.0f));
                } else {
                    position = 0.0f;
                }
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

// Easing function implementations
float apply_easing(float t, easing_t easing) {
    // Clamp t to [0, 1]
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    
    switch (easing) {
        case EASE_LINEAR: return t;
        
        case EASE_IN_QUAD: return ease_in_quad(t);
        case EASE_OUT_QUAD: return ease_out_quad(t);
        case EASE_IN_OUT_QUAD: return ease_in_out_quad(t);
        
        case EASE_IN_CUBIC: return ease_in_cubic(t);
        case EASE_OUT_CUBIC: return ease_out_cubic(t);
        case EASE_IN_OUT_CUBIC: return ease_in_out_cubic(t);
        
        case EASE_IN_QUART: return t * t * t * t;
        case EASE_OUT_QUART: return 1.0f - powf(1.0f - t, 4);
        case EASE_IN_OUT_QUART: 
            return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 4) / 2.0f;
        
        case EASE_IN_QUINT: return t * t * t * t * t;
        case EASE_OUT_QUINT: return 1.0f - powf(1.0f - t, 5);
        case EASE_IN_OUT_QUINT:
            return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 5) / 2.0f;
        
        case EASE_IN_SINE: return ease_in_sine(t);
        case EASE_OUT_SINE: return ease_out_sine(t);
        case EASE_IN_OUT_SINE: return ease_in_out_sine(t);
        
        case EASE_IN_EXPO: return t == 0 ? 0 : powf(2, 10 * (t - 1));
        case EASE_OUT_EXPO: return t == 1 ? 1 : 1 - powf(2, -10 * t);
        case EASE_IN_OUT_EXPO:
            if (t == 0) return 0;
            if (t == 1) return 1;
            return t < 0.5f ? powf(2, 20 * t - 10) / 2 : (2 - powf(2, -20 * t + 10)) / 2;
        
        case EASE_IN_CIRC: return 1.0f - sqrt(1.0f - t * t);
        case EASE_OUT_CIRC: return sqrt(1.0f - (t - 1.0f) * (t - 1.0f));
        case EASE_IN_OUT_CIRC:
            return t < 0.5f 
                ? (1.0f - sqrt(1.0f - 4.0f * t * t)) / 2.0f
                : (sqrt(1.0f - 4.0f * (t - 1.0f) * (t - 1.0f)) + 1.0f) / 2.0f;
        
        case EASE_IN_BACK: {
                const float c1 = 1.70158f;
                const float c3 = c1 + 1.0f;
                return c3 * t * t * t - c1 * t * t;
            }
        case EASE_OUT_BACK: return ease_out_back(t);
        case EASE_IN_OUT_BACK:
            {
                const float c1 = 1.70158f;
                const float c2 = c1 * 1.525f;
                return t < 0.5f
                    ? (powf(2.0f * t, 2) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
                    : (powf(2.0f * t - 2.0f, 2) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
            }
        
        case EASE_IN_ELASTIC:
            {
                const float c4 = (2.0f * M_PI) / 3.0f;
                return t == 0 ? 0 : t == 1 ? 1 : -powf(2, 10 * t - 10) * sin((t * 10 - 10.75f) * c4);
            }
        case EASE_OUT_ELASTIC: return ease_out_elastic(t);
        case EASE_IN_OUT_ELASTIC:
            {
                const float c5 = (2.0f * M_PI) / 4.5f;
                return t == 0 ? 0 : t == 1 ? 1 :
                    t < 0.5f ? -(powf(2, 20 * t - 10) * sin((20 * t - 11.125f) * c5)) / 2
                             : (powf(2, -20 * t + 10) * sin((20 * t - 11.125f) * c5)) / 2 + 1;
            }
        
        case EASE_IN_BOUNCE: return 1.0f - ease_out_bounce(1.0f - t);
        case EASE_OUT_BOUNCE: return ease_out_bounce(t);
        case EASE_IN_OUT_BOUNCE:
            return t < 0.5f ? (1.0f - ease_out_bounce(1.0f - 2.0f * t)) / 2.0f
                            : (1.0f + ease_out_bounce(2.0f * t - 1.0f)) / 2.0f;
        
        default: return t; // Linear fallback
    }
}

float ease_in_quad(float t) {
    return t * t;
}

float ease_out_quad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float ease_in_out_quad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2) / 2.0f;
}

float ease_in_cubic(float t) {
    return t * t * t;
}

float ease_out_cubic(float t) {
    return 1.0f - powf(1.0f - t, 3);
}

float ease_in_out_cubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3) / 2.0f;
}

float ease_in_sine(float t) {
    return 1.0f - cos((t * M_PI) / 2.0f);
}

float ease_out_sine(float t) {
    return sin((t * M_PI) / 2.0f);
}

float ease_in_out_sine(float t) {
    return -(cos(M_PI * t) - 1.0f) / 2.0f;
}

float ease_in_back(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}

float ease_out_back(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3) + c1 * powf(t - 1.0f, 2);
}

float ease_out_bounce(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        return n1 * (t -= 1.5f / d1) * t + 0.75f;
    } else if (t < 2.5f / d1) {
        return n1 * (t -= 2.25f / d1) * t + 0.9375f;
    } else {
        return n1 * (t -= 2.625f / d1) * t + 0.984375f;
    }
}

float ease_out_elastic(float t) {
    const float c4 = (2.0f * M_PI) / 3.0f;
    return t == 0 ? 0 : t == 1 ? 1 : powf(2, -10 * t) * sin((t * 10 - 0.75f) * c4) + 1;
}

// HSV color functions
hsv_color_t rgb_to_hsv(rgb_color_t rgb) {
    hsv_color_t hsv;
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    
    float max = fmaxf(r, fmaxf(g, b));
    float min = fminf(r, fminf(g, b));
    float diff = max - min;
    
    // Value
    hsv.v = max;
    
    // Saturation
    hsv.s = (max == 0.0f) ? 0.0f : diff / max;
    
    // Hue
    if (diff == 0.0f) {
        hsv.h = 0.0f;
    } else if (max == r) {
        hsv.h = 60.0f * fmod((g - b) / diff, 6.0f);
    } else if (max == g) {
        hsv.h = 60.0f * ((b - r) / diff + 2.0f);
    } else {
        hsv.h = 60.0f * ((r - g) / diff + 4.0f);
    }
    
    if (hsv.h < 0.0f) hsv.h += 360.0f;
    
    return hsv;
}

rgb_color_t hsv_to_rgb(hsv_color_t hsv) {
    rgb_color_t rgb;
    
    float c = hsv.v * hsv.s;
    float x = c * (1.0f - fabsf(fmod(hsv.h / 60.0f, 2.0f) - 1.0f));
    float m = hsv.v - c;
    
    float r, g, b;
    
    if (hsv.h >= 0 && hsv.h < 60) {
        r = c; g = x; b = 0;
    } else if (hsv.h >= 60 && hsv.h < 120) {
        r = x; g = c; b = 0;
    } else if (hsv.h >= 120 && hsv.h < 180) {
        r = 0; g = c; b = x;
    } else if (hsv.h >= 180 && hsv.h < 240) {
        r = 0; g = x; b = c;
    } else if (hsv.h >= 240 && hsv.h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    rgb.r = (int)((r + m) * 255.0f);
    rgb.g = (int)((g + m) * 255.0f);
    rgb.b = (int)((b + m) * 255.0f);
    
    return rgb;
}

rgb_color_t color_wheel(float position) {
    // Generate rainbow colors using HSV
    hsv_color_t hsv;
    hsv.h = position * 360.0f;  // Full hue range
    hsv.s = 1.0f;               // Full saturation
    hsv.v = 1.0f;               // Full value
    
    return hsv_to_rgb(hsv);
}

// Advanced gradient functions
void setup_gradient_preset(config_t *config, gradient_preset_t preset) {
    switch (preset) {
        case GRADIENT_PRESET_RAINBOW:
            config->gradient_stops[0] = color_wheel(0.0f);      // Red
            config->gradient_stops[1] = color_wheel(1.0f/6.0f); // Orange
            config->gradient_stops[2] = color_wheel(2.0f/6.0f); // Yellow
            config->gradient_stops[3] = color_wheel(3.0f/6.0f); // Green
            config->gradient_stops[4] = color_wheel(4.0f/6.0f); // Blue
            config->gradient_stops[5] = color_wheel(5.0f/6.0f); // Purple
            config->gradient_count = 6;
            config->gradient_direction = GRADIENT_HORIZONTAL;
            break;
            
        case GRADIENT_PRESET_FIRE:
            config->gradient_stops[0] = (rgb_color_t){64, 0, 0};     // Dark red
            config->gradient_stops[1] = (rgb_color_t){128, 0, 0};    // Red
            config->gradient_stops[2] = (rgb_color_t){255, 64, 0};   // Red-orange
            config->gradient_stops[3] = (rgb_color_t){255, 128, 0};  // Orange
            config->gradient_stops[4] = (rgb_color_t){255, 192, 0};  // Yellow-orange
            config->gradient_stops[5] = (rgb_color_t){255, 255, 64}; // Yellow
            config->gradient_count = 6;
            config->gradient_direction = GRADIENT_RADIAL;
            break;
            
        case GRADIENT_PRESET_OCEAN:
            config->gradient_stops[0] = (rgb_color_t){0, 32, 64};    // Deep blue
            config->gradient_stops[1] = (rgb_color_t){0, 64, 128};   // Dark blue
            config->gradient_stops[2] = (rgb_color_t){0, 128, 192};  // Blue
            config->gradient_stops[3] = (rgb_color_t){64, 192, 255}; // Light blue
            config->gradient_stops[4] = (rgb_color_t){128, 224, 255};// Cyan
            config->gradient_stops[5] = (rgb_color_t){192, 240, 255};// Pale cyan
            config->gradient_count = 6;
            config->gradient_direction = GRADIENT_VERTICAL;
            break;
            
        case GRADIENT_PRESET_SUNSET:
            config->gradient_stops[0] = (rgb_color_t){128, 0, 128};  // Purple
            config->gradient_stops[1] = (rgb_color_t){255, 64, 128}; // Pink
            config->gradient_stops[2] = (rgb_color_t){255, 128, 64}; // Orange-pink
            config->gradient_stops[3] = (rgb_color_t){255, 192, 0};  // Orange
            config->gradient_stops[4] = (rgb_color_t){255, 255, 128};// Light yellow
            config->gradient_count = 5;
            config->gradient_direction = GRADIENT_HORIZONTAL;
            break;
            
        case GRADIENT_PRESET_FOREST:
            config->gradient_stops[0] = (rgb_color_t){0, 64, 0};     // Dark green
            config->gradient_stops[1] = (rgb_color_t){0, 128, 0};    // Green
            config->gradient_stops[2] = (rgb_color_t){64, 192, 64};  // Light green
            config->gradient_stops[3] = (rgb_color_t){128, 255, 128};// Pale green
            config->gradient_stops[4] = (rgb_color_t){192, 255, 192};// Very pale green
            config->gradient_count = 5;
            config->gradient_direction = GRADIENT_DIAGONAL;
            break;
            
        case GRADIENT_PRESET_ICE:
            config->gradient_stops[0] = (rgb_color_t){192, 224, 255};// Pale blue
            config->gradient_stops[1] = (rgb_color_t){224, 240, 255};// Very pale blue
            config->gradient_stops[2] = (rgb_color_t){240, 248, 255};// Almost white
            config->gradient_stops[3] = (rgb_color_t){255, 255, 255};// White
            config->gradient_count = 4;
            config->gradient_direction = GRADIENT_RADIAL;
            break;
            
        case GRADIENT_PRESET_NEON:
            config->gradient_stops[0] = (rgb_color_t){255, 0, 255};  // Magenta
            config->gradient_stops[1] = (rgb_color_t){0, 255, 255};  // Cyan
            config->gradient_stops[2] = (rgb_color_t){255, 255, 0};  // Yellow
            config->gradient_stops[3] = (rgb_color_t){255, 0, 128};  // Hot pink
            config->gradient_count = 4;
            config->gradient_direction = GRADIENT_ANGLE;
            config->gradient_angle = 45.0f;
            break;
            
        case GRADIENT_PRESET_PASTEL:
            config->gradient_stops[0] = (rgb_color_t){255, 192, 203};// Light pink
            config->gradient_stops[1] = (rgb_color_t){255, 218, 185};// Peach
            config->gradient_stops[2] = (rgb_color_t){255, 255, 186};// Light yellow
            config->gradient_stops[3] = (rgb_color_t){186, 255, 201};// Light green
            config->gradient_stops[4] = (rgb_color_t){186, 225, 255};// Light blue
            config->gradient_stops[5] = (rgb_color_t){221, 160, 221};// Plum
            config->gradient_count = 6;
            config->gradient_direction = GRADIENT_HORIZONTAL;
            break;
            
        case GRADIENT_PRESET_CUSTOM:
        default:
            // Keep existing gradient
            break;
    }
    
    config->gradient_preset = preset;
    config->use_gradient = 1;
}

void parse_gradient_colors(config_t *config, const char *colors_string) {
    // Parse color string like "#ff0000,#00ff00,#0000ff" or "red,green,blue"
    // For simplicity, implement basic hex color parsing
    config->gradient_count = 0;
    
    char *colors_copy = malloc(strlen(colors_string) + 1);
    strcpy(colors_copy, colors_string);
    char *token = strtok(colors_copy, ",");
    
    while (token && config->gradient_count < 8) {
        rgb_color_t color = {255, 255, 255}; // Default white
        
        // Remove whitespace
        while (*token == ' ') token++;
        
        if (token[0] == '#' && strlen(token) == 7) {
            // Hex format like #ff0000
            unsigned int r, g, b;
            if (sscanf(token, "#%02x%02x%02x", &r, &g, &b) == 3) {
                color.r = r;
                color.g = g;
                color.b = b;
            }
        } else {
            // Named colors (simplified)
            if (strcmp(token, "red") == 0) color = (rgb_color_t){255, 0, 0};
            else if (strcmp(token, "green") == 0) color = (rgb_color_t){0, 255, 0};
            else if (strcmp(token, "blue") == 0) color = (rgb_color_t){0, 0, 255};
            else if (strcmp(token, "yellow") == 0) color = (rgb_color_t){255, 255, 0};
            else if (strcmp(token, "cyan") == 0) color = (rgb_color_t){0, 255, 255};
            else if (strcmp(token, "magenta") == 0) color = (rgb_color_t){255, 0, 255};
            else if (strcmp(token, "white") == 0) color = (rgb_color_t){255, 255, 255};
            else if (strcmp(token, "black") == 0) color = (rgb_color_t){0, 0, 0};
        }
        
        config->gradient_stops[config->gradient_count] = color;
        config->gradient_count++;
        
        token = strtok(NULL, ",");
    }
    
    free(colors_copy);
    
    if (config->gradient_count > 0) {
        config->gradient_preset = GRADIENT_PRESET_CUSTOM;
        config->use_gradient = 1;
    }
}

void generate_auto_gradient(config_t *config, int seed) {
    // Generate a random gradient based on seed
    srand(seed);
    
    // Choose a random preset
    gradient_preset_t presets[] = {
        GRADIENT_PRESET_RAINBOW, GRADIENT_PRESET_FIRE, GRADIENT_PRESET_OCEAN,
        GRADIENT_PRESET_SUNSET, GRADIENT_PRESET_FOREST, GRADIENT_PRESET_ICE,
        GRADIENT_PRESET_NEON, GRADIENT_PRESET_PASTEL
    };
    
    int preset_index = rand() % 8;
    setup_gradient_preset(config, presets[preset_index]);
    
    // Randomize direction
    gradient_direction_t directions[] = {
        GRADIENT_HORIZONTAL, GRADIENT_VERTICAL, GRADIENT_DIAGONAL, 
        GRADIENT_RADIAL, GRADIENT_ANGLE
    };
    config->gradient_direction = directions[rand() % 5];
    
    if (config->gradient_direction == GRADIENT_ANGLE) {
        config->gradient_angle = rand() % 360;
    }
}

// Background effect implementations
void render_background(terminal_t *term, config_t *config, int frame) {
    if (!config || config->background_effect == BACKGROUND_NONE) {
        return;
    }
    
    switch (config->background_effect) {
        case BACKGROUND_STARS:
            render_stars_background(term, config, frame);
            break;
        case BACKGROUND_MATRIX_RAIN:
            render_matrix_background(term, config, frame);
            break;
        case BACKGROUND_PARTICLES:
            render_particles_background(term, config, frame);
            break;
        case BACKGROUND_GRID:
            render_grid_background(term, config, frame);
            break;
        case BACKGROUND_WAVES:
            render_waves_background(term, config, frame);
            break;
        case BACKGROUND_PLASMA:
            render_plasma_background(term, config, frame);
            break;
        default:
            break;
    }
}

void render_stars_background(terminal_t *term, config_t *config, int frame) {
    // Animated starfield background
    int star_count = (config->background_intensity * term->canvas_width * term->canvas_height) / 2000;
    
    for (int star = 0; star < star_count; star++) {
        // Use deterministic positions based on star index and frame
        int seed = star * 1103515245 + frame / 10; // Update slowly
        int row = (seed % term->canvas_height);
        int col = ((seed / term->canvas_height) % term->canvas_width);
        
        // Twinkling effect
        int brightness_seed = (star * 7 + frame / 3) % 100;
        if (brightness_seed < config->background_intensity) {
            // Create a pseudo background character
            character_t star_char;
            star_char.ch = (brightness_seed < 20) ? '*' : '.';
            star_char.pos.row = row;
            star_char.pos.col = col;
            star_char.visible = 1;
            star_char.color_fg = (brightness_seed < 30) ? 15 : 7; // White or gray
            star_char.bold = (brightness_seed < 10) ? 1 : 0;
            
            // Add to character array temporarily for rendering
            // Note: This is a simplified approach - in production we'd use separate background buffer
        }
    }
}

void render_matrix_background(terminal_t *term, config_t *config, int frame) {
    // Matrix-style falling characters background
    int column_density = config->background_intensity / 10;
    if (column_density < 1) column_density = 1;
    
    char matrix_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int num_chars = sizeof(matrix_chars) - 1;
    
    for (int col = 0; col < term->canvas_width; col += column_density) {
        // Each column has its own falling sequence
        int col_seed = col * 31 + frame / 5;
        int fall_pos = (col_seed % (term->canvas_height + 10)) - 5;
        
        if (fall_pos >= 0 && fall_pos < term->canvas_height) {
            // Create falling character
            for (int trail = 0; trail < 5; trail++) {
                int char_row = fall_pos - trail;
                if (char_row >= 0 && char_row < term->canvas_height) {
                    character_t bg_char;
                    bg_char.ch = matrix_chars[(col_seed + trail) % num_chars];
                    bg_char.pos.row = char_row;
                    bg_char.pos.col = col;
                    bg_char.visible = 1;
                    bg_char.color_fg = (trail == 0) ? 46 : (22 + trail * 4); // Bright to dark green
                    bg_char.bold = (trail < 2) ? 1 : 0;
                }
            }
        }
    }
}

void render_particles_background(terminal_t *term, config_t *config, int frame) {
    // Floating particles background
    int particle_count = (config->background_intensity * term->canvas_width * term->canvas_height) / 1000;
    
                char particles[] = "+*.";
    int num_particles = sizeof(particles) - 1;
    
    for (int p = 0; p < particle_count; p++) {
        int seed = p * 1234567 + frame / 8;
        float x = (seed % 10000) / 10000.0f * term->canvas_width;
        float y = fmod((seed / 10000.0f + frame * 0.01f), 1.0f) * term->canvas_height;
        
        int row = (int)y;
        int col = (int)x;
        
        if (row >= 0 && row < term->canvas_height && col >= 0 && col < term->canvas_width) {
            character_t particle;
            particle.ch = particles[p % num_particles];
            particle.pos.row = row;
            particle.pos.col = col;
            particle.visible = 1;
            particle.color_fg = 8 + (p % 8); // Various dim colors
            particle.bold = 0;
        }
    }
}

void render_grid_background(terminal_t *term, config_t *config, int frame) {
    // Animated grid pattern background
    int grid_spacing = 8 - (config->background_intensity / 20);
    if (grid_spacing < 2) grid_spacing = 2;
    
    // Animated pulse
    int pulse = (int)(50.0f + 30.0f * sin(frame * 0.1f));
    
    for (int row = 0; row < term->canvas_height; row += grid_spacing) {
        for (int col = 0; col < term->canvas_width; col++) {
            character_t grid_char;
            grid_char.ch = '-';
            grid_char.pos.row = row;
            grid_char.pos.col = col;
            grid_char.visible = 1;
            grid_char.color_fg = pulse;
            grid_char.bold = 0;
        }
    }
    
    for (int col = 0; col < term->canvas_width; col += grid_spacing) {
        for (int row = 0; row < term->canvas_height; row++) {
            character_t grid_char;
            grid_char.ch = '|';
            grid_char.pos.row = row;
            grid_char.pos.col = col;
            grid_char.visible = 1;
            grid_char.color_fg = pulse;
            grid_char.bold = 0;
        }
    }
}

void render_waves_background(terminal_t *term, config_t *config, int frame) {
    // Sine wave background patterns
    float wave_frequency = 0.2f + (config->background_intensity / 500.0f);
    
    for (int row = 0; row < term->canvas_height; row++) {
        float wave1 = sin((row * wave_frequency) + (frame * 0.05f));
        float wave2 = sin((row * wave_frequency * 1.3f) + (frame * 0.03f));
        
        int col1 = (int)((wave1 + 1.0f) * term->canvas_width * 0.5f);
        int col2 = (int)((wave2 + 1.0f) * term->canvas_width * 0.5f);
        
        if (col1 >= 0 && col1 < term->canvas_width) {
            character_t wave_char;
            wave_char.ch = '~';
            wave_char.pos.row = row;
            wave_char.pos.col = col1;
            wave_char.visible = 1;
            wave_char.color_fg = 36; // Cyan
            wave_char.bold = 0;
        }
        
        if (col2 >= 0 && col2 < term->canvas_width && col2 != col1) {
            character_t wave_char;
            wave_char.ch = '~';
            wave_char.pos.row = row;
            wave_char.pos.col = col2;
            wave_char.visible = 1;
            wave_char.color_fg = 33; // Blue
            wave_char.bold = 0;
        }
    }
}

void render_plasma_background(terminal_t *term, config_t *config, int frame) {
    // Plasma-like background using mathematical functions
    float time = frame * 0.02f;
    
    for (int row = 0; row < term->canvas_height; row += 2) {
        for (int col = 0; col < term->canvas_width; col += 2) {
            float x = col / (float)term->canvas_width;
            float y = row / (float)term->canvas_height;
            
            // Complex plasma equation
            float plasma = sin(x * 10.0f + time)
                         + sin(y * 8.0f + time * 1.5f)
                         + sin((x + y) * 12.0f + time * 2.0f)
                         + sin(sqrt(x*x + y*y) * 15.0f + time * 0.8f);
            
            plasma = (plasma + 4.0f) / 8.0f; // Normalize to 0-1
            
            int color_index = (int)(plasma * config->background_intensity / 10.0f);
            if (color_index > 0) {
                character_t plasma_char;
                plasma_char.ch = (color_index > 5) ? '#' : '.';
                plasma_char.pos.row = row;
                plasma_char.pos.col = col;
                plasma_char.visible = 1;
                // Color cycle through spectrum
                hsv_color_t hsv = {plasma * 360.0f, 0.8f, 0.6f};
                rgb_color_t rgb = hsv_to_rgb(hsv);
                plasma_char.color_fg = rgb_to_256(rgb.r, rgb.g, rgb.b);
                plasma_char.bold = 0;
            }
        }
    }
}

// Background rendering directly to screen buffer
void render_background_to_screen(char screen[MAX_LINES][MAX_COLS], 
                                int screen_fg[MAX_LINES][MAX_COLS],
                                int screen_bg[MAX_LINES][MAX_COLS],
                                int screen_bold[MAX_LINES][MAX_COLS],
                                terminal_t *term, config_t *config, int frame) {
    if (!config || config->background_effect == BACKGROUND_NONE) {
        return;
    }
    
    // Calculate screen area to render background
    int start_row = term->canvas_offset_y;
    int start_col = term->canvas_offset_x;
    int end_row = start_row + term->canvas_height;
    int end_col = start_col + term->canvas_width;
    
    switch (config->background_effect) {
        case BACKGROUND_STARS:
            {
                int star_count = (config->background_intensity * term->canvas_width * term->canvas_height) / 2000;
                for (int star = 0; star < star_count; star++) {
                    int seed = star * 1103515245 + frame / 10;
                    int row = start_row + (seed % term->canvas_height);
                    int col = start_col + ((seed / term->canvas_height) % term->canvas_width);
                    
                    int brightness_seed = (star * 7 + frame / 3) % 100;
                    if (brightness_seed < config->background_intensity && 
                        row >= 0 && row < MAX_LINES && col >= 0 && col < MAX_COLS) {
                        if (screen[row][col] == ' ') { // Don't overwrite text
                            screen[row][col] = (brightness_seed < 20) ? '*' : '.';
                            screen_fg[row][col] = (brightness_seed < 30) ? 15 : 7;
                            screen_bold[row][col] = (brightness_seed < 10) ? 1 : 0;
                        }
                    }
                }
            }
            break;
            
        case BACKGROUND_MATRIX_RAIN:
            {
                char matrix_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
                int num_chars = sizeof(matrix_chars) - 1;
                int column_density = config->background_intensity / 10;
                if (column_density < 1) column_density = 1;
                
                for (int col = start_col; col < end_col; col += column_density) {
                    int col_seed = col * 31 + frame / 5;
                    int fall_pos = start_row + (col_seed % (term->canvas_height + 10)) - 5;
                    
                    for (int trail = 0; trail < 5; trail++) {
                        int char_row = fall_pos - trail;
                        if (char_row >= start_row && char_row < end_row && 
                            char_row >= 0 && char_row < MAX_LINES && col >= 0 && col < MAX_COLS) {
                            if (screen[char_row][col] == ' ') {
                                screen[char_row][col] = matrix_chars[(col_seed + trail) % num_chars];
                                screen_fg[char_row][col] = (trail == 0) ? 46 : (22 + trail * 4);
                                screen_bold[char_row][col] = (trail < 2) ? 1 : 0;
                            }
                        }
                    }
                }
            }
            break;
            
        case BACKGROUND_PARTICLES:
            {
                char particles[] = "+*.";
                int num_particles = sizeof(particles) - 1;
                int particle_count = (config->background_intensity * term->canvas_width * term->canvas_height) / 1000;
                
                for (int p = 0; p < particle_count; p++) {
                    int seed = p * 1234567 + frame / 8;
                    float x = (seed % 10000) / 10000.0f * term->canvas_width;
                    float y = fmod((seed / 10000.0f + frame * 0.01f), 1.0f) * term->canvas_height;
                    
                    int row = start_row + (int)y;
                    int col = start_col + (int)x;
                    
                    if (row >= start_row && row < end_row && col >= start_col && col < end_col &&
                        row >= 0 && row < MAX_LINES && col >= 0 && col < MAX_COLS) {
                        if (screen[row][col] == ' ') {
                            screen[row][col] = particles[p % num_particles];
                            screen_fg[row][col] = 8 + (p % 8);
                            screen_bold[row][col] = 0;
                        }
                    }
                }
            }
            break;
            
        case BACKGROUND_GRID:
            {
                int grid_spacing = 8 - (config->background_intensity / 20);
                if (grid_spacing < 2) grid_spacing = 2;
                int pulse = (int)(50.0f + 30.0f * sin(frame * 0.1f));
                
                // Horizontal grid lines
                for (int row = start_row; row < end_row; row += grid_spacing) {
                    for (int col = start_col; col < end_col && col < MAX_COLS; col++) {
                        if (row >= 0 && row < MAX_LINES && screen[row][col] == ' ') {
                            screen[row][col] = '-';
                            screen_fg[row][col] = pulse;
                            screen_bold[row][col] = 0;
                        }
                    }
                }
                
                // Vertical grid lines
                for (int col = start_col; col < end_col; col += grid_spacing) {
                    for (int row = start_row; row < end_row && row < MAX_LINES; row++) {
                        if (col >= 0 && col < MAX_COLS && screen[row][col] == ' ') {
                            screen[row][col] = '|';
                            screen_fg[row][col] = pulse;
                            screen_bold[row][col] = 0;
                        }
                    }
                }
            }
            break;
            
        case BACKGROUND_WAVES:
            {
                float wave_frequency = 0.2f + (config->background_intensity / 500.0f);
                
                for (int row = start_row; row < end_row && row < MAX_LINES; row++) {
                    float wave1 = sin(((row - start_row) * wave_frequency) + (frame * 0.05f));
                    float wave2 = sin(((row - start_row) * wave_frequency * 1.3f) + (frame * 0.03f));
                    
                    int col1 = start_col + (int)((wave1 + 1.0f) * term->canvas_width * 0.5f);
                    int col2 = start_col + (int)((wave2 + 1.0f) * term->canvas_width * 0.5f);
                    
                    if (col1 >= start_col && col1 < end_col && col1 < MAX_COLS) {
                        if (screen[row][col1] == ' ') {
                            screen[row][col1] = '~';
                            screen_fg[row][col1] = 36; // Cyan
                            screen_bold[row][col1] = 0;
                        }
                    }
                    
                    if (col2 >= start_col && col2 < end_col && col2 != col1 && col2 < MAX_COLS) {
                        if (screen[row][col2] == ' ') {
                            screen[row][col2] = '~';
                            screen_fg[row][col2] = 33; // Blue
                            screen_bold[row][col2] = 0;
                        }
                    }
                }
            }
            break;
            
        case BACKGROUND_PLASMA:
            {
                float time = frame * 0.02f;
                
                for (int row = start_row; row < end_row; row += 2) {
                    for (int col = start_col; col < end_col; col += 2) {
                        if (row >= 0 && row < MAX_LINES && col >= 0 && col < MAX_COLS) {
                            float x = (col - start_col) / (float)term->canvas_width;
                            float y = (row - start_row) / (float)term->canvas_height;
                            
                            float plasma = sin(x * 10.0f + time)
                                         + sin(y * 8.0f + time * 1.5f)
                                         + sin((x + y) * 12.0f + time * 2.0f)
                                         + sin(sqrt(x*x + y*y) * 15.0f + time * 0.8f);
                            
                            plasma = (plasma + 4.0f) / 8.0f;
                            
                            int color_index = (int)(plasma * config->background_intensity / 10.0f);
                            if (color_index > 0 && screen[row][col] == ' ') {
                                screen[row][col] = (color_index > 5) ? '#' : '.';
                                hsv_color_t hsv = {plasma * 360.0f, 0.8f, 0.6f};
                                rgb_color_t rgb = hsv_to_rgb(hsv);
                                screen_fg[row][col] = rgb_to_256(rgb.r, rgb.g, rgb.b);
                                screen_bold[row][col] = 0;
                            }
                        }
                    }
                }
            }
            break;
            
        default:
            break;
    }
}
