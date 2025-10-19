#include "tte.h"

void print_usage(const char *program_name) {
    printf("Usage: %s [options] <effect>\n", program_name);
    printf("\nOptions:\n");
    printf("  --frame-rate <fps>        Set animation frame rate (default: 240)\n");
    printf("  --canvas-width <width>    Set canvas width (0 = auto)\n");
    printf("  --canvas-height <height>  Set canvas height (0 = auto)\n");
    printf("  --no-final-newline        Suppress final newline (prevents scrolling)\n");
    printf("  -h, --help               Show this help message\n");
    printf("\nEffects:\n");
    printf("  beams     Light beams sweep across the text\n");
    printf("  waves     Wave motion across characters\n");
    printf("  rain      Characters fall like rain\n");
    printf("  slide     Text slides into position\n");
    printf("  expand    Text expands from center point\n");
    printf("  matrix    Matrix digital rain effect\n");
    printf("  fireworks Characters launch and explode like fireworks\n");
    printf("  decrypt   Movie-style decryption effect\n");
    printf("  typewriter Sequential character typing\n");
    printf("  wipe      Left-to-right reveal wipe\n");
    printf("  spotlights Moving spotlight illumination\n");
    printf("  burn      Vertical burning reveal with flicker\n");
    printf("  swarm     Characters swarm into position\n");
    printf("\nExample:\n");
    printf("  %s --no-final-newline beams < input.txt\n", program_name);
}

void parse_args(int argc, char *argv[], config_t *config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--frame-rate") == 0) {
            if (i + 1 < argc) {
                config->frame_rate = atoi(argv[++i]);
                if (config->frame_rate <= 0) {
                    config->frame_rate = DEFAULT_FRAME_RATE;
                }
            }
        } else if (strcmp(argv[i], "--canvas-width") == 0) {
            if (i + 1 < argc) {
                config->canvas_width = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--canvas-height") == 0) {
            if (i + 1 < argc) {
                config->canvas_height = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--no-final-newline") == 0) {
            config->no_final_newline = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (argv[i][0] != '-') {
            // This is the effect name
            config->effect_name = argv[i];
        }
    }
}

effect_func_t get_effect_function(const char *effect_name) {
    if (strcmp(effect_name, "beams") == 0) {
        return effect_beams;
    } else if (strcmp(effect_name, "waves") == 0) {
        return effect_waves;
    } else if (strcmp(effect_name, "rain") == 0) {
        return effect_rain;
    } else if (strcmp(effect_name, "slide") == 0) {
        return effect_slide;
    } else if (strcmp(effect_name, "expand") == 0) {
        return effect_expand;
    } else if (strcmp(effect_name, "matrix") == 0) {
        return effect_matrix;
    } else if (strcmp(effect_name, "fireworks") == 0) {
        return effect_fireworks;
    } else if (strcmp(effect_name, "decrypt") == 0) {
        return effect_decrypt;
    } else if (strcmp(effect_name, "typewriter") == 0) {
        return effect_typewriter;
    } else if (strcmp(effect_name, "wipe") == 0) {
        return effect_wipe;
    } else if (strcmp(effect_name, "spotlights") == 0) {
        return effect_spotlights;
    } else if (strcmp(effect_name, "burn") == 0) {
        return effect_burn;
    } else if (strcmp(effect_name, "swarm") == 0) {
        return effect_swarm;
    }
    return NULL;
}
