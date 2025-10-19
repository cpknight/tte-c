#include "tte.h"

anchor_t parse_anchor(const char *anchor_str) {
    if (strcmp(anchor_str, "sw") == 0 || strcmp(anchor_str, "southwest") == 0) {
        return ANCHOR_SW;
    } else if (strcmp(anchor_str, "s") == 0 || strcmp(anchor_str, "south") == 0) {
        return ANCHOR_S;
    } else if (strcmp(anchor_str, "se") == 0 || strcmp(anchor_str, "southeast") == 0) {
        return ANCHOR_SE;
    } else if (strcmp(anchor_str, "e") == 0 || strcmp(anchor_str, "east") == 0) {
        return ANCHOR_E;
    } else if (strcmp(anchor_str, "ne") == 0 || strcmp(anchor_str, "northeast") == 0) {
        return ANCHOR_NE;
    } else if (strcmp(anchor_str, "n") == 0 || strcmp(anchor_str, "north") == 0) {
        return ANCHOR_N;
    } else if (strcmp(anchor_str, "nw") == 0 || strcmp(anchor_str, "northwest") == 0) {
        return ANCHOR_NW;
    } else if (strcmp(anchor_str, "w") == 0 || strcmp(anchor_str, "west") == 0) {
        return ANCHOR_W;
    } else if (strcmp(anchor_str, "c") == 0 || strcmp(anchor_str, "center") == 0) {
        return ANCHOR_C;
    }
    // Default to center if unknown
    return ANCHOR_C;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [options] <effect>\n", program_name);
    printf("\nOptions:\n");
    printf("  --frame-rate <fps>        Set animation frame rate (default: 240)\n");
    printf("  --canvas-width <width>    Set canvas width (0 = auto)\n");
    printf("  --canvas-height <height>  Set canvas height (0 = auto)\n");
    printf("  --no-final-newline        Suppress final newline (prevents scrolling)\n");
    printf("  --anchor-canvas <anchor>  Set canvas anchor point (sw/s/se/e/ne/n/nw/w/c)\n");
    printf("  --anchor-text <anchor>    Set text anchor point (sw/s/se/e/ne/n/nw/w/c)\n");
    printf("  --ignore-terminal-dimensions  Use canvas dimensions instead of terminal\n");
    printf("  --wrap-text               Enable text wrapping\n");
    printf("  --tab-width <width>       Set tab width (default: 4)\n");
    printf("  --xterm-colors            Force 8-bit color mode\n");
    printf("  --no-color                Disable all colors\n");
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
    printf("  highlight Scanning highlight bar reveals text\n");
    printf("  unstable  Characters jitter before settling\n");
    printf("  crumble   Text crumbles to dust particles\n");
    printf("  slice     Text revealed by slicing motions\n");
    printf("  pour      Characters flow like liquid\n");
    printf("  blackhole Gravitational pull with orbital motion\n");
    printf("  rings     Expanding concentric rings reveal text\n");
    printf("  synthgrid Synthwave-style grid with neon effects\n");
    printf("\nAnchor Points:\n");
    printf("  nw  n  ne     northwest  north  northeast\n");
    printf("  w   c   e  =  west      center east\n");
    printf("  sw  s  se     southwest south  southeast\n");
    printf("\nExample:\n");
    printf("  %s --no-final-newline --anchor-text c beams < input.txt\n", program_name);
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
        } else if (strcmp(argv[i], "--anchor-canvas") == 0) {
            if (i + 1 < argc) {
                config->anchor_canvas = parse_anchor(argv[++i]);
            }
        } else if (strcmp(argv[i], "--anchor-text") == 0) {
            if (i + 1 < argc) {
                config->anchor_text = parse_anchor(argv[++i]);
            }
        } else if (strcmp(argv[i], "--ignore-terminal-dimensions") == 0) {
            config->ignore_terminal_dimensions = 1;
        } else if (strcmp(argv[i], "--wrap-text") == 0) {
            config->wrap_text = 1;
        } else if (strcmp(argv[i], "--tab-width") == 0) {
            if (i + 1 < argc) {
                config->tab_width = atoi(argv[++i]);
                if (config->tab_width < 1) {
                    config->tab_width = 4;  // Default tab width
                }
            }
        } else if (strcmp(argv[i], "--xterm-colors") == 0) {
            config->xterm_colors = 1;
        } else if (strcmp(argv[i], "--no-color") == 0) {
            config->no_color = 1;
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
    } else if (strcmp(effect_name, "highlight") == 0) {
        return effect_highlight;
    } else if (strcmp(effect_name, "unstable") == 0) {
        return effect_unstable;
    } else if (strcmp(effect_name, "crumble") == 0) {
        return effect_crumble;
    } else if (strcmp(effect_name, "slice") == 0) {
        return effect_slice;
    } else if (strcmp(effect_name, "pour") == 0) {
        return effect_pour;
    } else if (strcmp(effect_name, "blackhole") == 0) {
        return effect_blackhole;
    } else if (strcmp(effect_name, "rings") == 0) {
        return effect_rings;
    } else if (strcmp(effect_name, "synthgrid") == 0) {
        return effect_synthgrid;
    }
    return NULL;
}
