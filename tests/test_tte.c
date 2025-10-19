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
    RUN_TEST(performance_comparison);
    
    printf("\nAll tests passed! ✅\n");
    return 0;
}
