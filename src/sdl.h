#ifndef SDL_HH
#define SDL_HH

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include "chip-8.h"

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
void update_screen(const sdl_t* sdl, const config_t config, const chip8_t chip);

// Handle any input from the user
void handle_input(chip8_t* chip);

// Cleanup when the program is terminated
void cleanup(sdl_t* sdl);

#endif // SDL_HH
