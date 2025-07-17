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
    if (!init_chip8(&chip))
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

        // Delay for approx 60 fps
        SDL_Delay(16);

        // Update changes
        update_screen(&sdl);
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
              {.widht_ = CHIP8_WIDHT,
               .height_ = CHIP8_HEIGHT,
               .fg_color_ = 0xFFFFFFFF,     // White
               .bg_color_ = 0xFF0000FF,     // Red
               .scale_factor_ = SCALE_FACTOR
              };
}
