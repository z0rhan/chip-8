#ifndef CHIP_8_HH
#define CHIP_8_HH

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t    width_;
    uint32_t    height_;
    uint32_t    fg_color_;      // RGBA(8 bits each)
    uint32_t    bg_color_;      // RGBA(8 bits each)
    uint32_t    scale_factor_;
    bool        outlines_;      // Draw pixel outlines
} config_t;

typedef enum
{
    QUIT,
    RUNNING,
    PAUSED
} emulator_state_t;

typedef struct
{
    uint16_t    opcode;
    uint16_t    NNN;        // 12 bit address
    uint8_t     NN;         // 8 bit address
    uint8_t     N;          // 4 bit address
    uint8_t     X;          // 4 bit register identifier
    uint8_t     Y;          // 4 bit register identifier
} instruction_t;

typedef struct
{
    emulator_state_t    state_;

    uint8_t             ram_[4096];
    bool                display_[64*32];
    uint16_t            stack_[12];
    uint16_t*           stack_ptr_;
    uint8_t             V_[16];
    uint16_t            I_;
    uint16_t            PC_;
    uint8_t             delay_timer_;
    uint8_t             sound_timer_;
    bool                keypad_[16]; // 0x0 to 0xF

    const char*         rom_name_;
    instruction_t       inst_;
} chip8_t;

// Setup the emulator
bool init_chip8(chip8_t* chip, const char* rom_name);

// Execute the instructions
void emulate_instruction(chip8_t* chip, const config_t config);

#endif // CHIP_8_HH
