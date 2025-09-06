#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "sdl.h"
#include "chip-8.h"

// Original CHIP-8 resolution
#define CHIP8_WIDTH  64
#define CHIP8_HEIGHT 32
// Default scale factor
#define SCALE_FACTOR 10
// Colors
#define WHITE 0xFFFFFFFF
#define BLACK 0x000000FF
// Delay
#define DELAY_FOR_60_HZ 16
// CPU clock speed
#define INST_PER_SEC 500

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

    // Seed random number generator
    srand(time(NULL));

    // Main loop
    while (chip.state_ != QUIT)
    {
        // Handle inputs
        handle_input(&chip);

        // Pause
        if (chip.state_ == PAUSED) continue;

        // Get time before running instructions
        const uint64_t before_frame = SDL_GetPerformanceCounter();

        // Emulate CHIP-8 instructions for this frame 60Hz
        for (uint32_t it = 0; it < config.inst_per_sec_ / DELAY_FOR_60_HZ; it++)
        {
            emulate_instruction(&chip, config);
        }
        // Get time elasped after running instructions
        const uint64_t after_frame = SDL_GetPerformanceCounter();

        const double time_elasped = (double)((after_frame - before_frame) * 1000) / SDL_GetPerformanceFrequency();

        // Delay for approx 60 fps
        SDL_Delay(DELAY_FOR_60_HZ > time_elasped ? (DELAY_FOR_60_HZ - time_elasped) : 0);

        // Update changes
        update_screen(&sdl, config, chip);
        
        // Update delay and sound timers
        // TODO
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
              {.width_          = CHIP8_WIDTH,
               .height_         = CHIP8_HEIGHT,
               .fg_color_       = WHITE,
               .bg_color_       = BLACK,
               .scale_factor_   = SCALE_FACTOR,
               .outlines_       = true,
               .inst_per_sec_   = INST_PER_SEC
              };
}
