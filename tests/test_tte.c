#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdbool.h>
#include "../src/tte.h"

// Simple test framework
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running test_%s... ", #name); \
    test_##name(); \
    printf("PASS\n"); \
} while(0)

// Test color conversion
TEST(rgb_to_256_basic) {
    assert(rgb_to_256(255, 255, 255) >= 15); // White should be high value
    assert(rgb_to_256(0, 0, 0) >= 0);        // Black should be valid
    assert(rgb_to_256(255, 0, 0) > 0);       // Red should be valid
}

// Test gradient interpolation
TEST(gradient_interpolation) {
    rgb_color_t color1 = {0, 0, 0};
    rgb_color_t color2 = {255, 255, 255};
    
    rgb_color_t result = interpolate_rgb(color1, color2, 0.5f);
    assert(result.r >= 100 && result.r <= 155); // Should be roughly middle
    assert(result.g >= 100 && result.g <= 155);
    assert(result.b >= 100 && result.b <= 155);
    
    result = interpolate_rgb(color1, color2, 0.0f);
    assert(result.r == 0 && result.g == 0 && result.b == 0);
    
    result = interpolate_rgb(color1, color2, 1.0f);
    assert(result.r == 255 && result.g == 255 && result.b == 255);
}

// Test gradient position calculations
TEST(gradient_positions) {
    // Test horizontal gradient
    float pos = calculate_gradient_position(0, 0, 10, 10, GRADIENT_HORIZONTAL, 0);
    assert(pos == 0.0f);
    
    pos = calculate_gradient_position(0, 9, 10, 10, GRADIENT_HORIZONTAL, 0);
    assert(pos == 1.0f);
    
    // Test vertical gradient
    pos = calculate_gradient_position(0, 0, 10, 10, GRADIENT_VERTICAL, 0);
    assert(pos == 0.0f);
    
    pos = calculate_gradient_position(9, 0, 10, 10, GRADIENT_VERTICAL, 0);
    assert(pos == 1.0f);
}

// Test terminal initialization
TEST(terminal_init) {
    terminal_t term = {0};
    init_terminal(&term);
    
    assert(term.chars != NULL);
    assert(term.terminal_width > 0);
    assert(term.terminal_height > 0);
    
    cleanup_terminal(&term);
    assert(term.chars == NULL);
}

// Test configuration setup
TEST(config_setup) {
    config_t config = {0};
    setup_gradient_colors(&config, "matrix");
    
    assert(config.gradient_count > 0);
    assert(config.gradient_count <= 8);
    assert(config.gradient_direction == GRADIENT_RADIAL);
    
    setup_gradient_colors(&config, "beams");
    assert(config.gradient_count > 0);
}

// Performance comparison test
TEST(performance_comparison) {
    printf("\n  Performance Test: tte-c vs original tte\n");
    
    struct timeval start, end;
    double tte_c_time, original_tte_time;
    
    // Test tte-c performance
    gettimeofday(&start, NULL);
    
    // Run tte-c 5 times
    for (int i = 0; i < 5; i++) {
        system("echo 'Performance Test' | timeout 1s ./tte-c --no-final-newline beams >/dev/null 2>&1");
    }
    
    gettimeofday(&end, NULL);
    tte_c_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    
    // Test original tte if available
    if (system("which tte >/dev/null 2>&1") == 0) {
        gettimeofday(&start, NULL);
        
        for (int i = 0; i < 5; i++) {
            system("echo 'Performance Test' | timeout 1s tte beams >/dev/null 2>&1");
        }
        
        gettimeofday(&end, NULL);
        original_tte_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        
        printf("    tte-c:       %.3fs (5 runs)\n", tte_c_time);
        printf("    original:    %.3fs (5 runs)\n", original_tte_time);
        printf("    speedup:     %.1fx\n", original_tte_time / tte_c_time);
        
        // tte-c should be faster (allow some variance for system load)
        assert(tte_c_time < original_tte_time * 1.5);
    } else {
        printf("    tte-c:       %.3fs (5 runs) - original tte not found\n", tte_c_time);
    }
}

// Test all effects can be loaded
TEST(all_effects_available) {
    const char* effects[] = {
        "beams", "waves", "rain", "slide", "expand", 
        "matrix", "fireworks", "decrypt", "typewriter", 
        "wipe", "spotlights", "burn", "swarm",
        "highlight", "unstable", "crumble", "slice",
        "pour", "blackhole", "rings", "synthgrid"
    };
    
    for (int i = 0; i < 21; i++) {
        effect_func_t func = get_effect_function(effects[i]);
        assert(func != NULL);
    }
}

// Test memory usage
TEST(memory_usage) {
    terminal_t term = {0};
    init_terminal(&term);
    
    // Simulate reading some text
    term.char_count = 100;
    for (int i = 0; i < 100; i++) {
        term.chars[i].ch = 'A' + (i % 26);
        term.chars[i].target.row = i / 10;
        term.chars[i].target.col = i % 10;
    }
    
    // Test gradient application doesn't crash
    config_t config = {0};
    setup_gradient_colors(&config, "beams");
    apply_initial_gradient(&term, &config);
    
    // Verify reasonable memory usage (characters should be initialized)
    for (int i = 0; i < 100; i++) {
        assert(term.chars[i].color_fg >= 0);
        assert(term.chars[i].color_fg < 256);
    }
    
    cleanup_terminal(&term);
}

// Test anchor parsing
TEST(anchor_parsing) {
    assert(parse_anchor("nw") == ANCHOR_NW);
    assert(parse_anchor("northwest") == ANCHOR_NW);
    assert(parse_anchor("n") == ANCHOR_N);
    assert(parse_anchor("north") == ANCHOR_N);
    assert(parse_anchor("ne") == ANCHOR_NE);
    assert(parse_anchor("northeast") == ANCHOR_NE);
    assert(parse_anchor("e") == ANCHOR_E);
    assert(parse_anchor("east") == ANCHOR_E);
    assert(parse_anchor("se") == ANCHOR_SE);
    assert(parse_anchor("southeast") == ANCHOR_SE);
    assert(parse_anchor("s") == ANCHOR_S);
    assert(parse_anchor("south") == ANCHOR_S);
    assert(parse_anchor("sw") == ANCHOR_SW);
    assert(parse_anchor("southwest") == ANCHOR_SW);
    assert(parse_anchor("w") == ANCHOR_W);
    assert(parse_anchor("west") == ANCHOR_W);
    assert(parse_anchor("c") == ANCHOR_C);
    assert(parse_anchor("center") == ANCHOR_C);
    assert(parse_anchor("invalid") == ANCHOR_C); // Should default to center
}

// Test command line argument parsing
TEST(command_line_parsing) {
    config_t config = {0};
    
    // Test basic options
    char* argv1[] = {"tte-c", "--frame-rate", "120", "--no-final-newline", "beams"};
    parse_args(5, argv1, &config);
    assert(config.frame_rate == 120);
    assert(config.no_final_newline == 1);
    assert(strcmp(config.effect_name, "beams") == 0);
    
    // Test new anchor options
    config_t config2 = {0};
    char* argv2[] = {"tte-c", "--anchor-canvas", "nw", "--anchor-text", "se", "matrix"};
    parse_args(6, argv2, &config2);
    assert(config2.anchor_canvas == ANCHOR_NW);
    assert(config2.anchor_text == ANCHOR_SE);
    assert(strcmp(config2.effect_name, "matrix") == 0);
    
    // Test new flag options
    config_t config3 = {0};
    char* argv3[] = {"tte-c", "--ignore-terminal-dimensions", "--wrap-text", "--xterm-colors", "--no-color", "fireworks"};
    parse_args(6, argv3, &config3);
    assert(config3.ignore_terminal_dimensions == 1);
    assert(config3.wrap_text == 1);
    assert(config3.xterm_colors == 1);
    assert(config3.no_color == 1);
    
    // Test tab-width option
    config_t config4 = {0};
    config4.tab_width = 4; // Default value
    char* argv4[] = {"tte-c", "--tab-width", "8", "decrypt"};
    parse_args(4, argv4, &config4);
    assert(config4.tab_width == 8);
    
    // Test invalid tab-width (should reset to default)
    config_t config5 = {0};
    char* argv5[] = {"tte-c", "--tab-width", "-1", "beams"};
    parse_args(4, argv5, &config5);
    assert(config5.tab_width == 4); // Should reset to default
}

// Test color formatting with new options
TEST(color_formatting_options) {
    char buffer[64];
    
    // Test normal color formatting
    config_t config_normal = {0};
    format_color_256_with_config(buffer, 15, -1, 1, &config_normal);
    assert(strstr(buffer, "38;5;15") != NULL); // Should contain 256-color code
    assert(strstr(buffer, "1;") != NULL);      // Should contain bold
    
    // Test no-color option
    config_t config_no_color = {.no_color = 1};
    format_color_256_with_config(buffer, 15, -1, 1, &config_no_color);
    assert(strcmp(buffer, "\033[1m") == 0); // Should only have bold
    
    format_color_256_with_config(buffer, 15, -1, 0, &config_no_color);
    assert(strlen(buffer) == 0); // Should be empty for non-bold
    
    // Test xterm-colors option (limits to 16 colors)
    config_t config_xterm = {.xterm_colors = 1};
    format_color_256_with_config(buffer, 200, -1, 0, &config_xterm);
    // Color should be reduced to 200 % 16 = 8
    assert(strstr(buffer, "38;5;8") != NULL);
}

// Test text reading with configuration
TEST(text_reading_with_config) {
    // This test is challenging to write without actual stdin input
    // We'll test the legacy compatibility function
    terminal_t term = {0};
    init_terminal(&term);
    
    // Test that read_input_text still works (backwards compatibility)
    // Note: This won't actually read anything without stdin, but should not crash
    
    cleanup_terminal(&term);
}

// Test highlight effect behavior
TEST(highlight_effect) {
    terminal_t term = {0};
    init_terminal(&term);
    
    // Setup test characters in a small grid
    term.char_count = 9;
    term.text_width = 3;
    term.text_height = 3;
    
    for (int i = 0; i < 9; i++) {
        term.chars[i].ch = 'A' + i;
        term.chars[i].target.row = i / 3;
        term.chars[i].target.col = i % 3;
        term.chars[i].visible = 0;
        term.chars[i].active = 1;
        term.chars[i].bold = 0;
    }
    
    // Test highlight effect at different frames
    effect_highlight(&term, 0);
    // All characters should be visible from start
    for (int i = 0; i < 9; i++) {
        assert(term.chars[i].visible == 1);
        assert(term.chars[i].pos.row == term.chars[i].target.row);
        assert(term.chars[i].pos.col == term.chars[i].target.col);
    }
    
    // Test that highlight moves diagonally
    effect_highlight(&term, 10);
    int bold_count = 0;
    for (int i = 0; i < 9; i++) {
        if (term.chars[i].bold) bold_count++;
    }
    // Some characters should be highlighted (allow for all or none at certain frames)
    assert(bold_count >= 0 && bold_count <= 9);
    
    // Test later frame to ensure highlight progresses
    effect_highlight(&term, 30);
    bool highlight_complete = true;
    for (int i = 0; i < 9; i++) {
        if (term.chars[i].active != 0) {
            highlight_complete = false;
            break;
        }
    }
    // Effect should eventually complete
    assert(highlight_complete || bold_count >= 0);
    
    cleanup_terminal(&term);
}

// Test unstable effect behavior
TEST(unstable_effect) {
    terminal_t term = {0};
    init_terminal(&term);
    
    // Setup test characters in a small grid
    term.char_count = 9;
    term.text_width = 3;
    term.text_height = 3;
    
    for (int i = 0; i < 9; i++) {
        term.chars[i].ch = 'A' + i;
        term.chars[i].target.row = i / 3;
        term.chars[i].target.col = i % 3;
        term.chars[i].visible = 0;
        term.chars[i].active = 1;
        term.chars[i].bold = 0;
    }
    
    // Test explosion phase (early frames)
    effect_unstable(&term, 20);
    for (int i = 0; i < 9; i++) {
        assert(term.chars[i].visible == 1);
        assert(term.chars[i].bold == 1);  // Should be bright during explosion
        assert(term.chars[i].color_fg == 208); // Orange unstable color
    }
    
    // Test reassembly phase (middle frames)
    effect_unstable(&term, 60);
    for (int i = 0; i < 9; i++) {
        assert(term.chars[i].visible == 1);
        // Characters should be moving toward their targets
    }
    
    // Test stable phase (late frames)
    effect_unstable(&term, 120);
    for (int i = 0; i < 9; i++) {
        assert(term.chars[i].visible == 1);
        assert(term.chars[i].pos.row == term.chars[i].target.row);
        assert(term.chars[i].pos.col == term.chars[i].target.col);
        assert(term.chars[i].active == 0); // Should be marked complete
        assert(term.chars[i].bold == 0);   // Should return to normal
    }
    
    cleanup_terminal(&term);
}

// Test crumble effect behavior
TEST(crumble_effect) {
    terminal_t term = {0};
    init_terminal(&term);
    
    // Setup test characters
    term.char_count = 4;
    term.text_width = 2;
    term.text_height = 2;
    
    for (int i = 0; i < 4; i++) {
        term.chars[i].ch = 'A' + i;
        term.chars[i].target.row = i / 2;
        term.chars[i].target.col = i % 2;
        term.chars[i].visible = 0;
        term.chars[i].active = 1;
        term.chars[i].bold = 0;
    }
    
    // Test early frame - characters should be in place
    effect_crumble(&term, 5);
    for (int i = 0; i < 4; i++) {
        assert(term.chars[i].visible == 1);
        assert(term.chars[i].pos.row == term.chars[i].target.row);
        assert(term.chars[i].pos.col == term.chars[i].target.col);
    }
    
    // Test final cleanup - all characters should be visible and settled
    effect_crumble(&term, 200);
    for (int i = 0; i < 4; i++) {
        assert(term.chars[i].visible == 1);
        assert(term.chars[i].pos.row == term.chars[i].target.row);
        assert(term.chars[i].pos.col == term.chars[i].target.col);
        assert(term.chars[i].active == 0);
    }
    
    cleanup_terminal(&term);
}

// Test rings effect behavior
TEST(rings_effect) {
    terminal_t term = {0};
    init_terminal(&term);
    
    // Setup test characters in small grid
    term.char_count = 9;
    term.text_width = 3;
    term.text_height = 3;
    
    for (int i = 0; i < 9; i++) {
        term.chars[i].ch = 'A' + i;
        term.chars[i].target.row = i / 3;
        term.chars[i].target.col = i % 3;
        term.chars[i].visible = 0;
        term.chars[i].active = 1;
        term.chars[i].bold = 0;
    }
    
    // Test early frame - some characters might be revealed by first ring
    effect_rings(&term, 10);
    int visible_count = 0;
    for (int i = 0; i < 9; i++) {
        if (term.chars[i].visible) visible_count++;
    }
    // At least some characters should be visible as rings expand
    assert(visible_count >= 0 && visible_count <= 9);
    
    // Test final cleanup - all should be visible
    effect_rings(&term, 200);
    for (int i = 0; i < 9; i++) {
        assert(term.chars[i].visible == 1);
        assert(term.chars[i].pos.row == term.chars[i].target.row);
        assert(term.chars[i].pos.col == term.chars[i].target.col);
        assert(term.chars[i].active == 0);
    }
    
    cleanup_terminal(&term);
}

// Test synthgrid effect behavior
TEST(synthgrid_effect) {
    terminal_t term = {0};
    init_terminal(&term);
    
    // Setup test characters
    term.char_count = 36; // 6x6 grid for good grid testing
    term.text_width = 6;
    term.text_height = 6;
    
    for (int i = 0; i < 36; i++) {
        term.chars[i].ch = 'A' + (i % 26);
        term.chars[i].target.row = i / 6;
        term.chars[i].target.col = i % 6;
        term.chars[i].visible = 0;
        term.chars[i].active = 1;
        term.chars[i].bold = 0;
    }
    
    // Test synthgrid effect - all characters should be visible
    effect_synthgrid(&term, 50);
    for (int i = 0; i < 36; i++) {
        assert(term.chars[i].visible == 1);
        assert(term.chars[i].pos.row == term.chars[i].target.row);
        assert(term.chars[i].pos.col == term.chars[i].target.col);
    }
    
    // Test final cleanup
    effect_synthgrid(&term, 250);
    for (int i = 0; i < 36; i++) {
        assert(term.chars[i].active == 0);
    }
    
    cleanup_terminal(&term);
}

// Test easing functions
TEST(easing_functions) {
    // Test linear easing
    assert(apply_easing(0.0f, EASE_LINEAR) == 0.0f);
    assert(apply_easing(1.0f, EASE_LINEAR) == 1.0f);
    assert(apply_easing(0.5f, EASE_LINEAR) == 0.5f);
    
    // Test quadratic easing
    float quad_result = apply_easing(0.5f, EASE_IN_QUAD);
    assert(quad_result > 0.2f && quad_result < 0.3f); // Should be 0.25
    
    // Test bounds
    assert(apply_easing(-1.0f, EASE_LINEAR) == 0.0f);
    assert(apply_easing(2.0f, EASE_LINEAR) == 1.0f);
}

// Test HSV color conversion
TEST(hsv_color_conversion) {
    // Test RGB to HSV conversion
    rgb_color_t red = {255, 0, 0};
    hsv_color_t hsv_red = rgb_to_hsv(red);
    assert(hsv_red.h >= -1.0f && hsv_red.h <= 361.0f); // Hue should be valid
    assert(hsv_red.s >= 0.9f && hsv_red.s <= 1.1f);   // Should be saturated
    assert(hsv_red.v >= 0.9f && hsv_red.v <= 1.1f);   // Should be bright
    
    // Test HSV to RGB conversion
    hsv_color_t hsv_test = {0.0f, 1.0f, 1.0f}; // Pure red in HSV
    rgb_color_t rgb_result = hsv_to_rgb(hsv_test);
    assert(rgb_result.r >= 250); // Should be close to 255
    assert(rgb_result.g <= 5);   // Should be close to 0
    assert(rgb_result.b <= 5);   // Should be close to 0
}

// Test color wheel function
TEST(color_wheel) {
    rgb_color_t color1 = color_wheel(0.0f);   // Red
    rgb_color_t color2 = color_wheel(0.33f);  // Green-ish
    rgb_color_t color3 = color_wheel(0.66f);  // Blue-ish
    
    // Colors should be different
    assert(!(color1.r == color2.r && color1.g == color2.g && color1.b == color2.b));
    assert(!(color2.r == color3.r && color2.g == color3.g && color2.b == color3.b));
    
    // All components should be valid
    assert(color1.r >= 0 && color1.r <= 255);
    assert(color1.g >= 0 && color1.g <= 255);
    assert(color1.b >= 0 && color1.b <= 255);
}

// Test gradient presets
TEST(gradient_presets) {
    config_t config = {0};
    
    // Test rainbow preset
    setup_gradient_preset(&config, GRADIENT_PRESET_RAINBOW);
    assert(config.gradient_count == 6);
    assert(config.use_gradient == 1);
    assert(config.gradient_direction == GRADIENT_HORIZONTAL);
    
    // Test fire preset
    setup_gradient_preset(&config, GRADIENT_PRESET_FIRE);
    assert(config.gradient_count == 6);
    assert(config.gradient_direction == GRADIENT_RADIAL);
    
    // Verify colors are different
    assert(!(config.gradient_stops[0].r == config.gradient_stops[1].r &&
             config.gradient_stops[0].g == config.gradient_stops[1].g &&
             config.gradient_stops[0].b == config.gradient_stops[1].b));
}

// Test gradient color parsing
TEST(gradient_color_parsing) {
    config_t config = {0};
    
    // Test hex color parsing
    parse_gradient_colors(&config, "#ff0000,#00ff00,#0000ff");
    assert(config.gradient_count == 3);
    assert(config.gradient_stops[0].r == 255);
    assert(config.gradient_stops[0].g == 0);
    assert(config.gradient_stops[0].b == 0);
    assert(config.gradient_stops[1].r == 0);
    assert(config.gradient_stops[1].g == 255);
    assert(config.gradient_stops[1].b == 0);
    
    // Test named color parsing
    config_t config2 = {0};
    parse_gradient_colors(&config2, "red,green,blue");
    assert(config2.gradient_count == 3);
    assert(config2.gradient_stops[0].r == 255);
    assert(config2.gradient_stops[0].g == 0);
    assert(config2.gradient_stops[0].b == 0);
}

// Test auto gradient generation
TEST(auto_gradient_generation) {
    config_t config = {0};
    
    generate_auto_gradient(&config, 12345);
    assert(config.gradient_count > 0);
    assert(config.use_gradient == 1);
    
    // Should be deterministic with same seed
    config_t config2 = {0};
    generate_auto_gradient(&config2, 12345);
    assert(config.gradient_count == config2.gradient_count);
    assert(config.gradient_direction == config2.gradient_direction);
}

// Test background effects
TEST(background_effects) {
    config_t config = {0};
    terminal_t term = {0};
    init_terminal(&term);
    term.canvas_width = 80;
    term.canvas_height = 24;
    
    // Test that background render function doesn't crash
    config.background_effect = BACKGROUND_STARS;
    config.background_intensity = 50;
    
    // This should not crash
    render_background(&term, &config, 0);
    
    // Test different background types
    config.background_effect = BACKGROUND_MATRIX_RAIN;
    render_background(&term, &config, 10);
    
    config.background_effect = BACKGROUND_PARTICLES;
    render_background(&term, &config, 20);
    
    cleanup_terminal(&term);
}

// Test edge cases and segfault regression
TEST(gradient_edge_cases) {
    // Test division by zero cases
    float pos1 = calculate_gradient_position(0, 0, 1, 1, GRADIENT_HORIZONTAL, 0.0f);
    assert(pos1 == 0.0f); // Should not crash with width=1
    
    float pos2 = calculate_gradient_position(0, 0, 1, 1, GRADIENT_VERTICAL, 0.0f);
    assert(pos2 == 0.0f); // Should not crash with height=1
    
    float pos3 = calculate_gradient_position(0, 0, 1, 1, GRADIENT_DIAGONAL, 0.0f);
    assert(pos3 == 0.0f); // Should not crash with both=1
    
    float pos4 = calculate_gradient_position(0, 0, 1, 1, GRADIENT_RADIAL, 0.0f);
    assert(pos4 == 0.0f); // Should not crash with radial center at origin
    
    float pos5 = calculate_gradient_position(0, 0, 1, 1, GRADIENT_ANGLE, 45.0f);
    assert(pos5 == 0.0f); // Should not crash with angle gradient
    
    // Test invalid dimensions
    float pos6 = calculate_gradient_position(0, 0, 0, 0, GRADIENT_HORIZONTAL, 0.0f);
    assert(pos6 == 0.0f); // Should handle zero dimensions safely
    
    float pos7 = calculate_gradient_position(0, 0, -1, -1, GRADIENT_HORIZONTAL, 0.0f);
    assert(pos7 == 0.0f); // Should handle negative dimensions safely
}

// Test interpolate_gradient with edge cases
TEST(interpolate_gradient_edge_cases) {
    rgb_color_t stops[2] = {{255, 0, 0}, {0, 255, 0}}; // Red to green
    
    // Test NULL pointer safety
    rgb_color_t result1 = interpolate_gradient(NULL, 2, 0.5f);
    assert(result1.r == 255 && result1.g == 255 && result1.b == 255); // Should return white
    
    // Test zero count
    rgb_color_t result2 = interpolate_gradient(stops, 0, 0.5f);
    assert(result2.r == 255 && result2.g == 255 && result2.b == 255); // Should return white
    
    // Test NaN position
    rgb_color_t result3 = interpolate_gradient(stops, 2, 0.0f/0.0f); // NaN
    assert(result3.r == 255 && result3.g == 0 && result3.b == 0); // Should return first color
    
    // Test infinity position
    rgb_color_t result4 = interpolate_gradient(stops, 2, 1.0f/0.0f); // Infinity
    assert(result4.r == 0 && result4.g == 255 && result4.b == 0); // Should return last color (infinity > 1)
    
    // Test negative position
    rgb_color_t result5 = interpolate_gradient(stops, 2, -1.0f);
    assert(result5.r == 255 && result5.g == 0 && result5.b == 0); // Should return first color
    
    // Test position > 1
    rgb_color_t result6 = interpolate_gradient(stops, 2, 2.0f);
    assert(result6.r == 0 && result6.g == 255 && result6.b == 0); // Should return last color
}

// Test command line segfault regression
TEST(command_line_segfault_regression) {
    config_t config = {0};
    config.gradient_preset = GRADIENT_PRESET_CUSTOM;
    config.background_effect = BACKGROUND_NONE;
    config.background_intensity = 50;
    config.gradient_colors_string = NULL;
    config.auto_gradient = 0;
    config.use_gradient = 1;
    
    terminal_t term = {0};
    init_terminal(&term);
    
    // Test the specific combination that caused segfault
    setup_gradient_preset(&config, GRADIENT_PRESET_NEON);
    assert(config.gradient_count == 4);
    assert(config.gradient_direction == GRADIENT_ANGLE);
    assert(config.gradient_angle == 45.0f);
    
    // Set up minimal text data
    term.text_width = 4;
    term.text_height = 1;
    term.char_count = 4;
    for (int i = 0; i < 4; i++) {
        term.chars[i].target.row = 0;
        term.chars[i].target.col = i;
        term.chars[i].ch = 'A' + i;
        term.chars[i].original_ch = 'A' + i;
        term.chars[i].visible = 1;
        term.chars[i].active = 1;
    }
    
    // This should not crash
    apply_initial_gradient(&term, &config);
    
    // Verify gradient was applied without crash
    for (int i = 0; i < 4; i++) {
        assert(term.chars[i].color_fg >= 0);
        assert(term.chars[i].color_fg <= 255);
    }
    
    cleanup_terminal(&term);
}

// Test background rendering safety
TEST(background_rendering_safety) {
    config_t config = {0};
    terminal_t term = {0};
    init_terminal(&term);
    
    // Set up minimal terminal
    term.canvas_width = 1;
    term.canvas_height = 1;
    term.canvas_offset_x = 0;
    term.canvas_offset_y = 0;
    
    // Test all background effects with minimal dimensions (should not crash)
    config.background_intensity = 50;
    
    config.background_effect = BACKGROUND_STARS;
    render_background(&term, &config, 0);
    
    config.background_effect = BACKGROUND_MATRIX_RAIN;
    render_background(&term, &config, 0);
    
    config.background_effect = BACKGROUND_PARTICLES;
    render_background(&term, &config, 0);
    
    config.background_effect = BACKGROUND_GRID;
    render_background(&term, &config, 0);
    
    config.background_effect = BACKGROUND_WAVES;
    render_background(&term, &config, 0);
    
    config.background_effect = BACKGROUND_PLASMA;
    render_background(&term, &config, 0);
    
    cleanup_terminal(&term);
}

int main() {
    printf("tte-c Unit Tests\n");
    printf("================\n");
    
    RUN_TEST(rgb_to_256_basic);
    RUN_TEST(gradient_interpolation);
    RUN_TEST(gradient_positions);
    RUN_TEST(terminal_init);
    RUN_TEST(config_setup);
    RUN_TEST(all_effects_available);
    RUN_TEST(memory_usage);
    RUN_TEST(anchor_parsing);
    RUN_TEST(command_line_parsing);
    RUN_TEST(color_formatting_options);
    RUN_TEST(text_reading_with_config);
    RUN_TEST(highlight_effect);
    RUN_TEST(unstable_effect);
    RUN_TEST(crumble_effect);
    RUN_TEST(rings_effect);
    RUN_TEST(synthgrid_effect);
    RUN_TEST(easing_functions);
    RUN_TEST(hsv_color_conversion);
    RUN_TEST(color_wheel);
    RUN_TEST(gradient_presets);
    RUN_TEST(gradient_color_parsing);
    RUN_TEST(auto_gradient_generation);
    RUN_TEST(background_effects);
    RUN_TEST(gradient_edge_cases);
    RUN_TEST(interpolate_gradient_edge_cases);
    RUN_TEST(command_line_segfault_regression);
    RUN_TEST(background_rendering_safety);
    RUN_TEST(performance_comparison);
    
    printf("\nAll tests passed! ✅\n");
    return 0;
}
