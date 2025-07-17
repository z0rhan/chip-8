#ifndef SDL_HH
#define SDL_HH

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include "chip-8.h"

typedef struct
{
    uint32_t    widht_;
    uint32_t    height_;
    uint32_t    fg_color_;      // RGBA(8 bits each)
    uint32_t    bg_color_;      // RGBA(8 bits each)
    uint32_t    scale_factor_;
} config_t;

typedef struct
{
    SDL_Window*     window_;
    SDL_Renderer*   renderer_;
} sdl_t;

// Initialize SDL and its subsystem
bool init_SDL(sdl_t* sdl, const config_t* config);

// Draw the screen with given color
void clear_screen(const sdl_t* sdl, const config_t* config);

// Update any changes and swap the render buffer
void update_screen(const sdl_t* sdl);

// Handle any input from the user
void handle_input(chip8_t* chip);

// Cleanup when the program is terminated
void cleanup(sdl_t* sdl);

#endif // SDL_HH
