#include "chip-8.h"

bool init_chip8(chip8_t* chip)
{
    // TODO:
    chip->state_ = RUNNING;
    return true; // Success
}
