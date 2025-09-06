#include "sdl.h"

bool init_SDL(sdl_t* sdl, const config_t* config)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s\n",
                SDL_GetError());
        return false;
    }

    sdl->window_ = SDL_CreateWindow("CHIP-8",
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    config->width_ * config->scale_factor_,
                                    config->height_ * config->scale_factor_,
                                    SDL_WINDOW_SHOWN);
    if (!sdl->window_)
    {
        SDL_Log("Failed to create SDL window: %s\n",
                SDL_GetError());
        return false;
    }

    sdl->renderer_ = SDL_CreateRenderer(sdl->window_, -1, SDL_RENDERER_ACCELERATED);
    if (!sdl->renderer_)
    {
        SDL_Log("Failed to create SDL renderer: %s\n",
                SDL_GetError());
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

void update_screen(const sdl_t* sdl, const config_t config, const chip8_t chip)
{
    SDL_Rect rect = {.x = 0, .y = 0,
                     .w = config.scale_factor_, .h = config.scale_factor_};

    // get the color values
    const uint8_t bg_r = (config.bg_color_ >> 24) & 0xFF;
    const uint8_t bg_g = (config.bg_color_ >> 16) & 0xFF;
    const uint8_t bg_b = (config.bg_color_ >> 8)  & 0xFF;
    const uint8_t bg_a = (config.bg_color_ >> 0)  & 0xFF;

    const uint8_t fg_r = (config.fg_color_ >> 24) & 0xFF;
    const uint8_t fg_g = (config.fg_color_ >> 16) & 0xFF;
    const uint8_t fg_b = (config.fg_color_ >> 8)  & 0xFF;
    const uint8_t fg_a = (config.fg_color_ >> 0)  & 0xFF;

    for (uint32_t i = 0; i < sizeof(chip.display_); i++)
    {
        // Convert 1D value i to 2D x,y coordinated
        rect.x = (i % config.width_) * config.scale_factor_;
        rect.y = (i / config.width_) * config.scale_factor_;
        
        if (chip.display_[i])
        {
            // Pixel is on, draw fg_color
            SDL_SetRenderDrawColor(sdl->renderer_, fg_r, fg_g, fg_b, fg_a);
            SDL_RenderFillRect(sdl->renderer_, &rect);

            if (config.outlines_)
            {
                // Draw the pixel outlines with bg_color
                SDL_SetRenderDrawColor(sdl->renderer_, bg_r, bg_g, bg_b, bg_a);
                SDL_RenderDrawRect(sdl->renderer_, &rect);
            }
        }
        else
        {
            // Pixel is off, draw bg_color
            SDL_SetRenderDrawColor(sdl->renderer_, bg_r, bg_g, bg_b, bg_a);
            SDL_RenderFillRect(sdl->renderer_, &rect);
        }
    }

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
                    case SDLK_ESCAPE:
                        // Press escape key to stop the main loop
                        chip->state_ = QUIT;
                        break;
                    case SDLK_SPACE:
                        // Press space bar to either pause or resume
                        if (chip->state_ == RUNNING)
                        {
                            chip->state_ = PAUSED;
                            puts("=====PAUSED=====");
                        }
                        else
                        {
                            chip->state_ = RUNNING;
                        }
                        break;

                    case SDLK_1: chip->keypad_[0x1] = true; break;
                    case SDLK_2: chip->keypad_[0x2] = true; break;
                    case SDLK_3: chip->keypad_[0x3] = true; break;
                    case SDLK_4: chip->keypad_[0xC] = true; break;

                    case SDLK_q: chip->keypad_[0x4] = true; break;
                    case SDLK_w: chip->keypad_[0x5] = true; break;
                    case SDLK_e: chip->keypad_[0x6] = true; break;
                    case SDLK_r: chip->keypad_[0xD] = true; break;

                    case SDLK_a: chip->keypad_[0x7] = true; break;
                    case SDLK_s: chip->keypad_[0x8] = true; break;
                    case SDLK_d: chip->keypad_[0x9] = true; break;
                    case SDLK_f: chip->keypad_[0xE] = true; break;

                    case SDLK_z: chip->keypad_[0xA] = true; break;
                    case SDLK_x: chip->keypad_[0x0] = true; break;
                    case SDLK_c: chip->keypad_[0xE] = true; break;
                    case SDLK_v: chip->keypad_[0xF] = true; break;

                    default:
                        break;
                }
                break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                    case SDLK_1: chip->keypad_[0x1] = false; break;
                    case SDLK_2: chip->keypad_[0x2] = false; break;
                    case SDLK_3: chip->keypad_[0x3] = false; break;
                    case SDLK_4: chip->keypad_[0xC] = false; break;

                    case SDLK_q: chip->keypad_[0x4] = false; break;
                    case SDLK_w: chip->keypad_[0x5] = false; break;
                    case SDLK_e: chip->keypad_[0x6] = false; break;
                    case SDLK_r: chip->keypad_[0xD] = false; break;

                    case SDLK_a: chip->keypad_[0x7] = false; break;
                    case SDLK_s: chip->keypad_[0x8] = false; break;
                    case SDLK_d: chip->keypad_[0x9] = false; break;
                    case SDLK_f: chip->keypad_[0xE] = false; break;

                    case SDLK_z: chip->keypad_[0xA] = false; break;
                    case SDLK_x: chip->keypad_[0x0] = false; break;
                    case SDLK_c: chip->keypad_[0xE] = false; break;
                    case SDLK_v: chip->keypad_[0xF] = false; break;

                    default:
                        break;
                }

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

