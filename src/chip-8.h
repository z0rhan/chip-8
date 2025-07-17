#ifndef CHIP_8_HH
#define CHIP_8_HH

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    QUIT,
    RUNNING,
    PAUSED
} emulator_state_t;

typedef struct
{
    emulator_state_t state_;
} chip8_t;

// Setup the emulator
bool init_chip8(chip8_t* chip);

#endif // CHIP_8_HH
