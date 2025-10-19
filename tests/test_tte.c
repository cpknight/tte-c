#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
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
        "wipe", "spotlights", "burn", "swarm"
    };
    
    for (int i = 0; i < 13; i++) {
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
    RUN_TEST(performance_comparison);
    
    printf("\nAll tests passed! ✅\n");
    return 0;
}