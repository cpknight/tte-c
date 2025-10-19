#include "tte.h"

int main(int argc, char *argv[]) {
    config_t config = {
        .frame_rate = DEFAULT_FRAME_RATE,
        .canvas_width = 0,
        .canvas_height = 0,
        .no_final_newline = 0,
        .effect_name = NULL,
        .anchor_canvas = ANCHOR_C,
        .anchor_text = ANCHOR_C,
        .use_gradient = 1,
        .gradient_direction = GRADIENT_HORIZONTAL,
        .gradient_angle = 0.0f,
        .gradient_steps = 64,
        .ignore_terminal_dimensions = 0,
        .wrap_text = 0,
        .tab_width = 4,
        .xterm_colors = 0,
        .no_color = 0
    };
    
    terminal_t term = {0};
    
    // Parse command line arguments
    parse_args(argc, argv, &config);
    
    if (!config.effect_name) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Get effect function
    effect_func_t effect_func = get_effect_function(config.effect_name);
    if (!effect_func) {
        fprintf(stderr, "Unknown effect: %s\n", config.effect_name);
        return 1;
    }
    
    // Set up sophisticated gradients based on effect
    setup_gradient_colors(&config, config.effect_name);
    
    // Initialize terminal and read input
    init_terminal(&term);
    read_input_text_with_config(&term, &config);
    
    // Set canvas dimensions (0 means use full terminal)
    if (config.canvas_width > 0) {
        term.canvas_width = config.canvas_width;
    } else if (config.canvas_width == 0) {
        term.canvas_width = term.terminal_width;
    }
    if (config.canvas_height > 0) {
        term.canvas_height = config.canvas_height;
    } else if (config.canvas_height == 0) {
        term.canvas_height = term.terminal_height;
    }
    
    // Calculate text and canvas positioning offsets
    calculate_offsets(&term, config.anchor_canvas, config.anchor_text);
    
    // Apply initial gradient to all characters
    apply_initial_gradient(&term, &config);
    
    // Setup terminal for animation
    printf(ANSI_HIDE_CURSOR);
    fflush(stdout);
    
    // Run animation
    int frame = 0;
    int max_frames = 1000; // Reasonable limit
    
    while (frame < max_frames) {
        effect_func(&term, frame);
        
        // Check if animation is complete
        int active_chars = 0;
        for (int i = 0; i < term.char_count; i++) {
            if (term.chars[i].active) {
                active_chars++;
            }
        }
        
        // Apply final gradient when all effects are done
        if (active_chars == 0) {
            apply_final_gradient(&term, &config);
        }
        
        render_frame_with_config(&term, &config);
        
        if (active_chars == 0 && frame > 60) {
            break;
        }
        
        sleep_frame(config.frame_rate);
        frame++;
    }
    
    // Restore cursor and optionally suppress final newline
    printf(ANSI_SHOW_CURSOR);
    if (!config.no_final_newline) {
        printf("\n");
    }
    fflush(stdout);
    
    cleanup_terminal(&term);
    return 0;
}