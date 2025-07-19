#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "sdl.h"
#include "chip-8.h"

// Original CHIP-8 resolution
#define CHIP8_WIDHT  64
#define CHIP8_HEIGHT 32
// Default scale factor
#define SCALE_FACTOR 20

// Setup the options for the emulator
void setup_config(config_t* config, int argc, char* argv[]);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <rom_name>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    // Setup emulator options
    config_t config = {0};
    setup_config(&config, argc, argv);

    // Initialize SDL
    sdl_t sdl = {0};
    if (!init_SDL(&sdl, &config))
    {
        exit(EXIT_FAILURE);
    }

    chip8_t chip = {0};
    const char* rom_name = argv[1];
    if (!init_chip8(&chip, rom_name))
    {
        exit(EXIT_FAILURE);
    }

    // Initial screen clear to bg color
    clear_screen(&sdl, &config);

    // Main loop
    while (chip.state_ != QUIT)
    {
        // Handle inputs
        handle_input(&chip);

        if (chip.state_ == PAUSED) continue;

        // Emulate CHIP-8 instructions
        emulate_instruction(&chip, config);

        // Delay for approx 60 fps
        SDL_Delay(16);

        // Update changes
        update_screen(&sdl, config, chip);
    }

    // Final cleanup
    cleanup(&sdl);

    return 0;
}

void setup_config(config_t* config, int argc, char* argv[])
{
    // Default values for now
    // TODO: allow flags to modify the initial options
    *config = (config_t)
              {.width_          = CHIP8_WIDHT,
               .height_         = CHIP8_HEIGHT,
               .fg_color_       = 0xFFFFFFFF,     // Black
               .bg_color_       = 0x000000FF,     // White
               .scale_factor_   = SCALE_FACTOR,
               .outlines_       = true
              };
}
