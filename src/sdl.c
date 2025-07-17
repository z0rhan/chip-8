#include "sdl.h"

bool init_SDL(sdl_t* sdl, const config_t* config)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    sdl->window_ = SDL_CreateWindow("CHIP-8",
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    config->widht_ * config->scale_factor_,
                                    config->height_ * config->scale_factor_,
                                    0);
    if (!sdl->window_)
    {
        SDL_Log("Failed to create SDL window: %s\n", SDL_GetError());
        return false;
    }

    sdl->renderer_ = SDL_CreateRenderer(sdl->window_, -1, SDL_RENDERER_ACCELERATED);
    if (!sdl->renderer_)
    {
        SDL_Log("Failed to create SDL renderer: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void clear_screen(const sdl_t* sdl, const config_t* config)
{
    const uint8_t r = (config->bg_color_ >> 24) & 0xFF;
    const uint8_t g = (config->bg_color_ >> 16) & 0xFF;
    const uint8_t b = (config->bg_color_ >> 8)  & 0xFF;
    const uint8_t a = (config->bg_color_ >> 0)  & 0xFF;

    SDL_SetRenderDrawColor(sdl->renderer_, r, g, b, a);
    SDL_RenderClear(sdl->renderer_);
}

void update_screen(const sdl_t* sdl)
{
    SDL_RenderPresent(sdl->renderer_);
}

void handle_input(chip8_t* chip)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // Stop the main loop is requested
            case SDL_QUIT:
                chip->state_ = QUIT;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    // Press escape key to stop the main loop
                    case SDLK_ESCAPE:
                        chip->state_ = QUIT;
                        break;

                }
                break;

            default:
                break;
        }
    }
}

void cleanup(sdl_t* sdl)
{
    SDL_DestroyRenderer(sdl->renderer_);
    SDL_DestroyWindow(sdl->window_);
    SDL_Quit();
}

