#include "tte.h"

void effect_beams(terminal_t *term, int frame) {
    // Multiple beams sweep across canvas (rows and columns)
    int beam_width = 2;
    int beam_delay = 15; // Frames between beam groups
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        ch->visible = 0; // Start hidden
        ch->bold = 0;
        
        int illuminated = 0;
        
        // Row beams (horizontal sweeps)
        for (int beam_group = 0; beam_group < 3; beam_group++) {
            int beam_start = beam_group * beam_delay;
            if (frame >= beam_start) {
                int beam_pos = (frame - beam_start) * 2 - term->text_width;
                if (beam_pos >= ch->target.col - beam_width && 
                    beam_pos <= ch->target.col + beam_width) {
                    // Character is in beam path for this row
                    int row_match = (ch->target.row == beam_group * (term->text_height / 3));
                    if (row_match || abs(ch->target.row - beam_group * (term->text_height / 3)) <= 1) {
                        illuminated = 1;
                        ch->bold = 1;
                    }
                }
                // After beam passes, character remains visible
                if (beam_pos > ch->target.col + beam_width) {
                    int row_match = (ch->target.row == beam_group * (term->text_height / 3));
                    if (row_match || abs(ch->target.row - beam_group * (term->text_height / 3)) <= 1) {
                        ch->visible = 1;
                    }
                }
            }
        }
        
        // Column beams (vertical sweeps) - start after horizontal
        for (int beam_group = 0; beam_group < 2; beam_group++) {
            int beam_start = 60 + beam_group * beam_delay; // Start after row beams
            if (frame >= beam_start) {
                int beam_pos = (frame - beam_start) * 1 - term->text_height;
                if (beam_pos >= ch->target.row - beam_width && 
                    beam_pos <= ch->target.row + beam_width) {
                    // Character is in beam path for this column
                    int col_match = (ch->target.col == beam_group * (term->text_width / 2) + term->text_width / 4);
                    if (col_match || abs(ch->target.col - (beam_group * (term->text_width / 2) + term->text_width / 4)) <= 2) {
                        illuminated = 1;
                        ch->bold = 1;
                    }
                }
                // After beam passes, character remains visible
                if (beam_pos > ch->target.row + beam_width) {
                    int col_match = (ch->target.col == beam_group * (term->text_width / 2) + term->text_width / 4);
                    if (col_match || abs(ch->target.col - (beam_group * (term->text_width / 2) + term->text_width / 4)) <= 2) {
                        ch->visible = 1;
                    }
                }
            }
        }
        
        if (illuminated) {
            ch->visible = 1;
            ch->pos = ch->target;
        }
        
        // Final cleanup - ensure all characters are visible and effect completes
        if (frame > 150) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = 0;
            ch->active = 0;
        }
    }
}

// Simplified but effective versions of complex effects
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
    // Matrix digital rain effect - columns of falling characters
    char matrix_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int num_matrix_chars = sizeof(matrix_chars) - 1;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Each column starts at different times
        int col_start_frame = ch->target.col * 12 + ((ch->target.col * 7) % 20);
        
        if (frame < col_start_frame) {
            ch->visible = 0;
            continue;
        }
        
        // Calculate rain drop position - characters "fall" down the column
        int rain_progress = (frame - col_start_frame) / 3; // Slower falling
        int drop_row = rain_progress - term->text_height;
        
        // Create trailing effect - characters appear as the "rain" passes over them
        int trail_length = 8;
        int char_trail_pos = drop_row - ch->target.row;
        
        if (char_trail_pos >= -trail_length && char_trail_pos <= 2) {
            ch->visible = 1;
            ch->pos = ch->target;
            
            // Change character to matrix symbols during rain
            if (char_trail_pos >= -2 && char_trail_pos <= 2) {
                // Active rain area - cycle through matrix characters
                if (frame % 4 == 0) {
                    int char_seed = (ch->target.col * 31 + ch->target.row * 17 + frame / 4) % num_matrix_chars;
                    ch->ch = matrix_chars[char_seed];
                }
            }
            
            // Color based on position in trail
            if (char_trail_pos >= 0) {
                // Leading edge - bright white/green
                ch->color_fg = 15;  // White
                ch->bold = 1;
            } else if (char_trail_pos >= -2) {
                // Near edge - bright green
                ch->color_fg = 46;  // Bright green
                ch->bold = 1;
            } else if (char_trail_pos >= -4) {
                // Medium trail - green
                ch->color_fg = 40;  // Green
                ch->bold = 0;
            } else {
                // Fading trail - dark green
                ch->color_fg = 22;  // Dark green
                ch->bold = 0;
            }
            
        } else if (drop_row > ch->target.row + 2) {
            // Rain has passed - show original character with gradient color
            ch->visible = 1;
            ch->pos = ch->target;
            ch->ch = ch->original_ch;  // Restore original character
            ch->bold = 0;  // Use gradient color system
            
            // Mark as complete when all columns have finished raining
            if (frame > col_start_frame + (term->text_height + trail_length) * 3 + 60) {
                ch->active = 0;
            }
        }
    }
}

void effect_fireworks(terminal_t *term, int frame) {
    // Multiple firework shells launch from bottom and explode
    int num_shells = 5;
    int shell_delay = 20; // Frames between shell launches
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Determine which firework shell this character belongs to
        int shell_id = (ch->target.col + ch->target.row * 7) % num_shells;
        int shell_launch_frame = shell_id * shell_delay;
        
        if (frame < shell_launch_frame) {
            ch->visible = 0;
            continue;
        }
        
        // Calculate shell explosion point (not necessarily character's final position)
        int shell_explode_col = (shell_id * term->text_width / num_shells) + (term->text_width / (num_shells * 2));
        int shell_explode_row = term->text_height / 3 + (shell_id % 3) * (term->text_height / 6);
        
        int launch_duration = 40;
        int explode_frame = shell_launch_frame + launch_duration;
        int explosion_duration = 50;
        
        if (frame >= shell_launch_frame && frame < explode_frame) {
            // Launch phase - shell travels from bottom to explosion point
            float launch_progress = (float)(frame - shell_launch_frame) / (float)launch_duration;
            
            // Only show characters that are part of the shell (not all characters)
            int is_shell_char = ((ch->target.col == shell_explode_col) || 
                               (abs(ch->target.col - shell_explode_col) <= 1)) &&
                              ((ch->target.row == shell_explode_row) ||
                               (abs(ch->target.row - shell_explode_row) <= 1));
            
            if (is_shell_char) {
                ch->visible = 1;
                ch->pos.col = shell_explode_col;
                ch->pos.row = term->text_height - 1 - (int)((term->text_height - 1 - shell_explode_row) * launch_progress);
                
                // Bright shell color during launch
                ch->color_fg = 226;  // Bright yellow
                ch->bold = 1;
            }
            
        } else if (frame >= explode_frame && frame < explode_frame + explosion_duration) {
            // Explosion phase - characters explode outward from shell position
            int explode_time = frame - explode_frame;
            float explode_progress = (float)explode_time / (float)explosion_duration;
            
            // Calculate direction from explosion point to character's final position
            int dx = ch->target.col - shell_explode_col;
            int dy = ch->target.row - shell_explode_row;
            float distance = sqrt(dx * dx + dy * dy);
            
            // Only explode characters within reasonable distance of shell
            if (distance <= 8) {
                ch->visible = 1;
                
                // Move from explosion point to final position
                ch->pos.col = shell_explode_col + (int)(dx * explode_progress);
                ch->pos.row = shell_explode_row + (int)(dy * explode_progress);
                
                // Color progression during explosion: white -> red -> orange -> yellow
                if (explode_time < 8) {
                    ch->color_fg = 15;   // White (initial flash)
                    ch->bold = 1;
                } else if (explode_time < 18) {
                    ch->color_fg = 196;  // Bright red
                    ch->bold = 1;
                } else if (explode_time < 30) {
                    ch->color_fg = 208;  // Orange
                    ch->bold = 1;
                } else {
                    ch->color_fg = 226;  // Yellow (fading)
                    ch->bold = 0;
                }
            }
            
        } else if (frame >= explode_frame + explosion_duration) {
            // Settling phase - characters settle to final positions with gradient colors
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = 0;  // Use gradient color system
            
            if (frame > explode_frame + explosion_duration + 30) {
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

void effect_crumble(terminal_t *term, int frame) {
    // Text crumbling to dust - characters break apart and fall down
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Each character starts crumbling at different times based on position
        int crumble_start = (ch->target.row * 10) + (ch->target.col * 3) + (i % 15);
        int crumble_duration = 80;
        
        if (frame < crumble_start) {
            // Character is still intact
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = 0;
        } else if (frame < crumble_start + crumble_duration) {
            // Character is crumbling - show falling motion
            ch->visible = 1;
            
            int fall_time = frame - crumble_start;
            float fall_progress = (float)fall_time / (float)crumble_duration;
            
            // Add some horizontal drift based on character index
            int drift_seed = i * 1103515245 + 12345;
            int drift_direction = (drift_seed & 1) ? 1 : -1;
            int horizontal_drift = (int)(fall_progress * 3.0f * drift_direction);
            
            // Vertical falling motion with acceleration
            int fall_distance = (int)(fall_progress * fall_progress * 15.0f);
            
            ch->pos.row = ch->target.row + fall_distance;
            ch->pos.col = ch->target.col + horizontal_drift;
            
            // Fade and flicker as it crumbles
            ch->bold = (rand() % 4 == 0) ? 0 : 1;
            
        } else {
            // Character has finished crumbling - invisible
            ch->visible = 0;
            ch->active = 0;
        }
        
        // Final cleanup - show all characters in final positions after effect
        if (frame > crumble_start + crumble_duration + 60) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = 0;
            ch->active = 0;
        }
    }
}

void effect_slice(terminal_t *term, int frame) {
    // Text slicing from multiple directions - characters reveal as if cut by slicing motions
    int num_slices = 4;
    int slice_width = 3;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        ch->visible = 0;
        ch->bold = 0;
        
        int revealed = 0;
        
        // Multiple slicing motions from different angles
        for (int slice_id = 0; slice_id < num_slices; slice_id++) {
            int slice_start = slice_id * 20;
            
            if (frame >= slice_start) {
                int slice_time = frame - slice_start;
                
                switch (slice_id) {
                    case 0: // Horizontal slice from left
                        if (slice_time * 2 >= ch->target.col - slice_width && 
                            slice_time * 2 <= ch->target.col + slice_width) {
                            revealed = 1;
                            ch->bold = 1;
                        }
                        if (slice_time * 2 > ch->target.col + slice_width) {
                            revealed = 1;
                        }
                        break;
                        
                    case 1: // Vertical slice from top
                        if (slice_time >= ch->target.row - slice_width && 
                            slice_time <= ch->target.row + slice_width) {
                            revealed = 1;
                            ch->bold = 1;
                        }
                        if (slice_time > ch->target.row + slice_width) {
                            revealed = 1;
                        }
                        break;
                        
                    case 2: // Diagonal slice from top-left
                        {
                            int diagonal_pos = slice_time - (term->text_width + term->text_height) / 2;
                            int char_diagonal = ch->target.col - ch->target.row;
                            if (diagonal_pos >= char_diagonal - slice_width && 
                                diagonal_pos <= char_diagonal + slice_width) {
                                revealed = 1;
                                ch->bold = 1;
                            }
                            if (diagonal_pos > char_diagonal + slice_width) {
                                revealed = 1;
                            }
                        }
                        break;
                        
                    case 3: // Diagonal slice from top-right
                        {
                            int diagonal_pos = slice_time - (term->text_width + term->text_height) / 2;
                            int char_diagonal = ch->target.col + ch->target.row;
                            if (diagonal_pos >= char_diagonal - slice_width && 
                                diagonal_pos <= char_diagonal + slice_width) {
                                revealed = 1;
                                ch->bold = 1;
                            }
                            if (diagonal_pos > char_diagonal + slice_width) {
                                revealed = 1;
                            }
                        }
                        break;
                }
            }
        }
        
        if (revealed) {
            ch->visible = 1;
            ch->pos = ch->target;
        }
        
        // Final cleanup
        if (frame > 120) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = 0;
            ch->active = 0;
        }
    }
}

void effect_pour(terminal_t *term, int frame) {
    // Liquid pouring effect - characters flow like liquid from top to bottom
    int pour_speed = 2;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        // Start pouring from different columns at different times
        int pour_start = ch->target.col * 8;
        
        if (frame >= pour_start) {
            ch->visible = 1;
            
            // Calculate liquid flow position
            int pour_time = frame - pour_start;
            int flow_row = (pour_time * pour_speed) - term->text_height;
            
            // Add some horizontal spreading like liquid
            int spread_seed = (ch->target.col * 31 + pour_time / 5) % 100;
            int spread = (spread_seed < 20) ? -1 : (spread_seed > 80) ? 1 : 0;
            
            // Characters appear as the liquid "pours" past them
            if (flow_row >= ch->target.row) {
                ch->pos = ch->target;
                ch->pos.col += spread;  // Add liquid spreading
                
                // Liquid-like wobbling motion
                float wobble = sin((float)frame * 0.3f + ch->target.col * 0.5f) * 0.5f;
                ch->pos.col += (int)wobble;
                
                // Bright while liquid is actively flowing
                ch->bold = (flow_row - ch->target.row < 5) ? 1 : 0;
            } else {
                ch->visible = 0;
            }
            
            // Eventually settle to final position
            if (frame > pour_start + term->text_height * 2 + 40) {
                ch->pos = ch->target;
                ch->bold = 0;
                ch->active = 0;
            }
        }
    }
}

void effect_blackhole(terminal_t *term, int frame) {
    // Gravitational text distortion - characters get pulled toward center point
    int center_row = term->text_height / 2;
    int center_col = term->text_width / 2;
    int effect_duration = 100;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        ch->visible = 1;
        
        if (frame < effect_duration) {
            // Calculate gravitational pull toward center
            int dx = center_col - ch->target.col;
            int dy = center_row - ch->target.row;
            float distance = sqrt(dx * dx + dy * dy);
            
            if (distance > 0) {
                // Calculate orbital motion progress
                float progress = (float)frame / (float)effect_duration;
                
                // Apply orbital motion
                float angle_offset = frame * 0.1f + i * 0.3f;
                float orbit_radius = distance * (1.0f - progress * 0.7f);
                
                ch->pos.col = center_col + (int)(cos(angle_offset) * orbit_radius);
                ch->pos.row = center_row + (int)(sin(angle_offset) * orbit_radius);
                
                // Characters get brighter as they approach center
                ch->bold = (distance < 8) ? 1 : 0;
            } else {
                ch->pos = ch->target;
            }
            
        } else {
            // Characters return to original positions
            int return_time = frame - effect_duration;
            int return_duration = 60;
            
            if (return_time < return_duration) {
                float return_progress = (float)return_time / (float)return_duration;
                
                // Ease-out motion back to original position
                float ease_progress = 1.0f - powf(1.0f - return_progress, 3.0f);
                
                // Get current orbital position
                int dx = center_col - ch->target.col;
                int dy = center_row - ch->target.row;
                float distance = sqrt(dx * dx + dy * dy);
                float angle_offset = effect_duration * 0.1f + i * 0.3f;
                float orbit_radius = distance * 0.3f;
                
                int orbit_col = center_col + (int)(cos(angle_offset) * orbit_radius);
                int orbit_row = center_row + (int)(sin(angle_offset) * orbit_radius);
                
                ch->pos.col = orbit_col + (int)((ch->target.col - orbit_col) * ease_progress);
                ch->pos.row = orbit_row + (int)((ch->target.row - orbit_row) * ease_progress);
                
                ch->bold = 0;
            } else {
                ch->pos = ch->target;
                ch->bold = 0;
                ch->active = 0;
            }
        }
    }
}

void effect_rings(terminal_t *term, int frame) {
    // Expanding ring effects - concentric rings expand outward revealing text
    int center_row = term->text_height / 2;
    int center_col = term->text_width / 2;
    int num_rings = 5;
    int ring_delay = 15;
    int ring_width = 3;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        ch->visible = 0;
        ch->bold = 0;
        
        // Calculate distance from center
        int dx = ch->target.col - center_col;
        int dy = ch->target.row - center_row;
        float distance = sqrt(dx * dx + dy * dy);
        
        // Check if character is revealed by any expanding ring
        for (int ring_id = 0; ring_id < num_rings; ring_id++) {
            int ring_start = ring_id * ring_delay;
            
            if (frame >= ring_start) {
                int ring_time = frame - ring_start;
                float ring_radius = ring_time * 0.8f;
                
                // Character is revealed when ring passes over it
                if (ring_radius >= distance - ring_width && 
                    ring_radius <= distance + ring_width) {
                    ch->visible = 1;
                    ch->pos = ch->target;
                    ch->bold = 1;  // Bright ring edge
                    break;
                } else if (ring_radius > distance + ring_width) {
                    // Ring has passed - character remains visible
                    ch->visible = 1;
                    ch->pos = ch->target;
                    // Keep existing bold state from inner rings
                }
            }
        }
        
        // Final cleanup
        if (frame > 150) {
            ch->visible = 1;
            ch->pos = ch->target;
            ch->bold = 0;
            ch->active = 0;
        }
    }
}

void effect_synthgrid(terminal_t *term, int frame) {
    // Synthwave grid backgrounds - retro-style grid with neon highlighting  
    int grid_spacing = 6;
    int scan_speed = 2;
    
    for (int i = 0; i < term->char_count; i++) {
        character_t *ch = &term->chars[i];
        
        ch->visible = 1;
        ch->pos = ch->target;
        
        // Create synthwave grid effect
        int is_grid_line = ((ch->target.row % grid_spacing) == 0) || 
                          ((ch->target.col % grid_spacing) == 0);
        
        // Scanning line effect
        int scan_line = (frame * scan_speed) % (term->text_height + 20);
        int is_scan_line = (ch->target.row == scan_line) || 
                          (abs(ch->target.row - scan_line) == 1);
        
        // Perspective grid lines (getting closer at bottom)
        int perspective_divisor = (term->text_height > 2) ? (term->text_height / 2) : 1;
        int perspective_line = term->text_height - 1 - 
                              ((frame / 3) % perspective_divisor);
        int is_perspective = (ch->target.row == perspective_line);
        
        // Color and brightness based on grid position
        if (is_scan_line) {
            // Bright scanning line - use cyan/white
            ch->color_fg = 51;   // Bright cyan
            ch->bold = 1;
        } else if (is_perspective) {
            // Perspective lines - magenta
            ch->color_fg = 201;  // Bright magenta  
            ch->bold = 1;
        } else if (is_grid_line) {
            // Grid lines - dark blue
            ch->color_fg = 25;   // Dark blue
            ch->bold = 0;
        } else {
            // Regular text - use gradient colors (will be set by gradient system)
            ch->bold = 0;
        }
        
        // Add some flicker to grid lines for retro effect
        if (is_grid_line && frame % 8 == 0) {
            ch->bold = (rand() % 10 < 3) ? 1 : 0;
        }
        
        // Effect completes after several scan cycles
        if (frame > 200) {
            ch->bold = 0;  // Return to gradient colors
            ch->active = 0;
        }
    }
}
