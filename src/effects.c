#include "tte.h"

void effect_beams(terminal_t *term, int frame) {
    int beam_width = 3;
    int beam_speed = 2;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Calculate beam position sweeping from left to right
        int beam_pos = (frame * beam_speed) - term->text_width;
        
        // Character becomes visible when beam passes over it
        if (beam_pos >= ch->target.col - beam_width && 
            beam_pos <= ch->target.col + beam_width) {
            ch->visible = 1;
            ch->pos = ch->target;
            // Brighten the gradient color for beam effect
            ch->bold = 1;  // Make gradient color bright
        }
        
        // Keep character visible after beam passes
        if (beam_pos > ch->target.col + beam_width) {
            ch->visible = 1;
            ch->active = 0; // Mark as complete
            // Return to normal gradient color (already set)
            ch->bold = 0;
        }
    }
}

// New effects (lightweight approximations)
void effect_typewriter(terminal_t *term, int frame) {
    int speed = 2; // chars per frame
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        int index = ch->target.row * term->text_width + ch->target.col;
        if (index / speed <= frame) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = (frame - (index / speed)) < 3 ? 1 : 0; // brief bright
            ch->active = 0;
        }
    }
}

void effect_wipe(terminal_t *term, int frame) {
    int wipe_speed = 2;
    int wipe_col = frame * wipe_speed;
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        if (ch->target.col <= wipe_col) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = (ch->target.col == wipe_col) ? 1 : 0;
            ch->active = 0;
        }
    }
}

void effect_spotlights(terminal_t *term, int frame) {
    // Two moving spotlights brighten characters where they pass
    int cx1 = (frame * 2) % term->text_width;
    int cy1 = (frame) % term->text_height;
    int cx2 = (term->text_width - (frame * 2) % term->text_width);
    int cy2 = (term->text_height - (frame) % term->text_height);
    int radius = 6;
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        int dx1 = ch->target.col - cx1;
        int dy1 = ch->target.row - cy1;
        int dx2 = ch->target.col - cx2;
        int dy2 = ch->target.row - cy2;
        int in1 = dx1*dx1 + dy1*dy1 <= radius*radius;
        int in2 = dx2*dx2 + dy2*dy2 <= radius*radius;
        if (in1 || in2) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = 1;
        } else if (frame > 80) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = 0;
            ch->active = 0;
        }
    }
}

void effect_burn(terminal_t *term, int frame) {
    // Vertical burn reveal from top with flicker
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        if (ch->target.row <= frame / 2) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = (rand() % 5 == 0) ? 1 : 0; // flicker
            ch->active = 0;
        }
    }
}

void effect_swarm(terminal_t *term, int frame) {
    // Characters swarm towards target
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        // Simple pseudo-random initial positions based on index
        int seed = (i * 1103515245 + 12345) & 0x7fffffff;
        int start_col = seed % (term->text_width * 2) - term->text_width;
        int start_row = (seed / 97) % (term->text_height * 2) - term->text_height;
        
        float t = frame / 60.0f; // progress
        if (t > 1.0f) t = 1.0f;
        
        ch->pos.col = start_col + (int)((ch->target.col - start_col) * t);
        ch->pos.row = start_row + (int)((ch->target.row - start_row) * t);
        ch->visible = 1;
        ch->bold = (t < 1.0f) ? 1 : 0;
        if (t >= 1.0f) ch->active = 0;
    }
}

void effect_waves(terminal_t *term, int frame) {
    float wave_frequency = 0.3f;
    float wave_amplitude = 2.0f;
    int wave_speed = 1;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Calculate wave effect
        float wave_offset = sin((ch->target.col * wave_frequency) + (frame * wave_speed * 0.1f)) * wave_amplitude;
        
        ch->pos.row = ch->target.row + (int)wave_offset;
        ch->pos.col = ch->target.col;
        
        // Fade in over time with wave effect on gradient colors
        if (frame > ch->target.col * 2) {
            ch->visible = 1;
            
            // Modify boldness based on wave position (keep gradient colors)
            float wave_color = (sin((ch->target.col * wave_frequency) + (frame * wave_speed * 0.1f)) + 1.0f) / 2.0f;
            if (wave_color > 0.7f) {
                ch->bold = 1;  // Bright wave peaks
            } else {
                ch->bold = 0;  // Normal gradient color
            }
        }
        
        // Eventually settle to final position
        if (frame > 200) {
            ch->pos.row = ch->target.row;
            ch->bold = 0;  // Keep gradient color, just remove bold
            ch->active = 0;
        }
    }
}

void effect_rain(terminal_t *term, int frame) {
    int fall_speed = 1;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Start characters at top of screen with staggered timing
        int start_frame = ch->target.col * 5 + (i % 20) * 3;
        
        if (frame >= start_frame) {
            ch->visible = 1;
            
            // Calculate falling position
            int fall_distance = (frame - start_frame) * fall_speed;
            ch->pos.row = -term->text_height + fall_distance;
            ch->pos.col = ch->target.col;
            
            // Modify brightness as it falls (keep gradient colors)
            float fall_progress = (float)(ch->pos.row + term->text_height) / (float)(ch->target.row + term->text_height);
            if (fall_progress < 0.5f) {
                ch->bold = 1;  // Bright while falling
            } else {
                ch->bold = 0;  // Normal gradient color
            }
            
            // Stop at target position
            if (ch->pos.row >= ch->target.row) {
                ch->pos.row = ch->target.row;
                ch->bold = 0;  // Keep gradient color
                ch->active = 0;
            }
        }
    }
}

void effect_slide(terminal_t *term, int frame) {
    int slide_speed = 2;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Start characters off-screen to the left
        int start_frame = ch->target.row * 5;
        
        if (frame >= start_frame) {
            ch->visible = 1;
            
            // Calculate sliding position
            int slide_distance = (frame - start_frame) * slide_speed;
            ch->pos.row = ch->target.row;
            ch->pos.col = -term->text_width + slide_distance;
            
            // Brighten while sliding
            if (ch->pos.col < ch->target.col) {
                ch->bold = 1;
            } else {
                ch->bold = 0;
            }
            
            // Stop at target position
            if (ch->pos.col >= ch->target.col) {
                ch->pos.col = ch->target.col;
                ch->bold = 0;  // Keep gradient color
                ch->active = 0;
            }
        }
    }
}

void effect_expand(terminal_t *term, int frame) {
    int center_row = term->text_height / 2;
    int center_col = term->text_width / 2;
    float expand_speed = 0.5f;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Calculate distance from center
        int dx = ch->target.col - center_col;
        int dy = ch->target.row - center_row;
        int distance = (int)sqrt(dx * dx + dy * dy);
        
        int start_frame = distance * 5;
        
        if (frame >= start_frame) {
            ch->visible = 1;
            
            // Expand from center
            float progress = (frame - start_frame) * expand_speed;
            if (progress > 1.0f) {
                progress = 1.0f;
                ch->bold = 0;  // Keep gradient color
                ch->active = 0;
            } else {
                ch->bold = 1;  // Bright while expanding
            }
            
            ch->pos.row = center_row + (int)(dy * progress);
            ch->pos.col = center_col + (int)(dx * progress);
        }
    }
}

void effect_matrix(terminal_t *term, int frame) {
    // Matrix digital rain effect
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Staggered appearance based on column
        int start_frame = ch->target.col * 8 + (frame % 40);
        
        if (frame >= start_frame) {
            ch->visible = 1;
            ch->pos = ch->target;
            
            // Matrix green color scheme
            int time_visible = frame - start_frame;
            if (time_visible < 20) {
                // Bright green when first appearing
                ch->color_fg = 46;  // Bright green
                ch->bold = 1;
            } else if (time_visible < 60) {
                // Medium green
                ch->color_fg = 40;  // Green
                ch->bold = 0;
            } else {
                // Dark green
                ch->color_fg = 22;  // Dark green
                ch->bold = 0;
            }
            
            if (time_visible > 100) {
                ch->bold = 0;  // Keep matrix gradient color
                ch->active = 0;
            }
        }
    }
}

void effect_fireworks(terminal_t *term, int frame) {
    int center_row = term->text_height / 2;
    int center_col = term->text_width / 2;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Calculate distance from center for launch timing
        int dx = ch->target.col - center_col;
        int dy = ch->target.row - center_row;
        int distance = (int)sqrt(dx * dx + dy * dy);
        
        int launch_frame = distance * 4;
        int explode_frame = launch_frame + 30;
        
        if (frame >= launch_frame && frame < explode_frame) {
            // Launch phase - move from bottom to explosion point
            ch->visible = 1;
            float progress = (float)(frame - launch_frame) / 30.0f;
            ch->pos.row = term->text_height + (int)((ch->target.row - term->text_height) * progress);
            ch->pos.col = ch->target.col;
            
            // Bright yellow/white trail
            ch->color_fg = 226;  // Bright yellow
            ch->bold = 1;
            
        } else if (frame >= explode_frame) {
            // Explosion phase - appear at final position
            ch->visible = 1;
            ch->pos = ch->target;
            
            // Color explosion: red -> orange -> yellow -> white
            int explode_time = frame - explode_frame;
            if (explode_time < 10) {
                ch->color_fg = 196;  // Bright red
                ch->bold = 1;
            } else if (explode_time < 20) {
                ch->color_fg = 208;  // Orange
                ch->bold = 1;
            } else if (explode_time < 30) {
                ch->color_fg = 226;  // Yellow
                ch->bold = 0;
            } else {
                // Keep gradient color (was set at initialization)
                ch->bold = 0;
                ch->active = 0;
            }
        }
    }
}

void effect_decrypt(terminal_t *term, int frame) {
    // Movie-style decryption effect
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Each character gets decrypted at a different time
        int decrypt_start = (ch->target.row * 15) + (ch->target.col * 3) + (i % 30);
        int decrypt_duration = 60;
        
        if (frame >= decrypt_start) {
            ch->visible = 1;
            
            int decrypt_progress = frame - decrypt_start;
            
            if (decrypt_progress < decrypt_duration) {
                // Cycling through random characters during decrypt
                if (frame % 4 == 0) {
                    // Change character periodically during decryption
                    char random_chars[] = "0123456789ABCDEF@#$%&*";
                    ch->ch = random_chars[rand() % (sizeof(random_chars) - 1)];
                }
                
                // Color progression: red -> yellow -> green
                float progress = (float)decrypt_progress / (float)decrypt_duration;
                if (progress < 0.5f) {
                    ch->color_fg = 196;  // Red
                    ch->bold = 1;
                } else if (progress < 0.8f) {
                    ch->color_fg = 226;  // Yellow
                    ch->bold = 1;
                } else {
                    ch->color_fg = 46;   // Green
                    ch->bold = 0;
                }
                
                ch->pos = ch->target;
            } else {
                // Decryption complete - show original character
                ch->ch = ch->original_ch;  // Restore original character
                // Keep gradient color (was set at initialization)
                ch->bold = 0;
                ch->pos = ch->target;
                ch->active = 0;
            }
        }
    }
}

void effect_highlight(terminal_t *term, int frame) {
    // Specular highlight that runs diagonally across the text
    int highlight_width = 8;
    float highlight_speed = 1.5f;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // All characters are visible from the start
        ch->visible = 1;
        ch->pos = ch->target;
        
        // Calculate diagonal highlight position (bottom-left to top-right)
        float diagonal_pos = (frame * highlight_speed) - (term->text_width + term->text_height);
        float char_diagonal = ch->target.col - ch->target.row; // Diagonal coordinate
        
        // Character is highlighted when diagonal sweep passes over it
        if (diagonal_pos >= char_diagonal - highlight_width && 
            diagonal_pos <= char_diagonal + highlight_width) {
            // Calculate highlight intensity based on distance from center
            float distance = fabs(diagonal_pos - char_diagonal);
            float intensity = 1.0f - (distance / highlight_width);
            
            // Brighten character during highlight with intensity falloff
            ch->bold = (intensity > 0.3f) ? 1 : 0;
        } else {
            // Normal gradient color when not highlighted
            ch->bold = 0;
        }
        
        // Effect completes when highlight has passed all characters
        if (diagonal_pos > term->text_width + highlight_width) {
            ch->active = 0;
        }
    }
}

void effect_unstable(terminal_t *term, int frame) {
    // Characters spawn jumbled, explode to canvas edges, then reassemble
    int center_row = term->text_height / 2;
    int center_col = term->text_width / 2;
    int explosion_duration = 40;
    int reassembly_duration = 60;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        ch->visible = 1;
        
        if (frame < explosion_duration) {
            // Phase 1: Explosion - characters move from center to edges
            float progress = (float)frame / (float)explosion_duration;
            
            // Calculate explosion direction for this character
            int seed = i * 1103515245 + 12345;
            float angle = ((seed & 0xFFFF) / 65535.0f) * 2.0f * M_PI;
            
            // Explosion distance increases over time
            int explosion_radius = (int)(progress * (term->text_width + term->text_height));
            
            ch->pos.row = center_row + (int)(sin(angle) * explosion_radius);
            ch->pos.col = center_col + (int)(cos(angle) * explosion_radius);
            
            // Orange/red unstable color during explosion
            ch->color_fg = 208;  // Orange
            ch->bold = 1;
            
        } else if (frame < explosion_duration + reassembly_duration) {
            // Phase 2: Reassembly - characters move from edges to final positions
            int reassembly_frame = frame - explosion_duration;
            float progress = (float)reassembly_frame / (float)reassembly_duration;
            
            // Ease-out motion (exponential decay)
            float ease_progress = 1.0f - powf(1.0f - progress, 3.0f);
            
            // Calculate starting position from explosion
            int seed = i * 1103515245 + 12345;
            float angle = ((seed & 0xFFFF) / 65535.0f) * 2.0f * M_PI;
            int explosion_radius = term->text_width + term->text_height;
            
            int start_row = center_row + (int)(sin(angle) * explosion_radius);
            int start_col = center_col + (int)(cos(angle) * explosion_radius);
            
            // Interpolate from explosion position to target
            ch->pos.row = start_row + (int)((ch->target.row - start_row) * ease_progress);
            ch->pos.col = start_col + (int)((ch->target.col - start_col) * ease_progress);
            
            // Transition from unstable color to gradient
            if (progress < 0.5f) {
                ch->color_fg = 208;  // Orange
                ch->bold = 1;
            } else {
                // Return to gradient color (will be set by gradient system)
                ch->bold = 0;
            }
            
        } else {
            // Phase 3: Stable - characters at final positions
            ch->pos = ch->target;
            ch->bold = 0;  // Normal gradient color
            ch->active = 0;
        }
    }
}
