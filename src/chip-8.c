#include "chip-8.h"

#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

#ifdef DEBUG
void print_debug_info(chip8_t* chip);
#endif


bool init_chip8(chip8_t* chip, const char* rom_name)
{
    const uint32_t entry_point = 0x200; // ROM will be loaded to 0x200
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    }; // 5 bytes per character

    // Load font
    memcpy(&chip->ram_[0], font, sizeof(font));

    // Open ROM file
    FILE* rom = fopen(rom_name, "rb");
    if (!rom)
    {
        SDL_Log("Rom file %s is invlaid or does not exist\n",
                rom_name);
        return false;
    }

    // Check rom size
    fseek(rom, 0, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof(chip->ram_) - entry_point;
    rewind(rom);
    if (rom_size > max_size)
    {
        SDL_Log("Rom file %s is too big! Rom size %zu , Max size %zu\n",
                rom_name, rom_size, max_size);
        return false;
    }

    // Load the ROM to the RAM
    if (fread(&chip->ram_[entry_point], rom_size, 1, rom) != 1)
    {
        SDL_Log("Could not read rom file %s into CHIP8 memory\n",
                rom_name);
        return false;
    }

    fclose(rom);

    // Default options
    chip->state_        = RUNNING;
    chip->PC_           = entry_point;
    chip->rom_name_     = rom_name;
    chip->stack_ptr_    = &chip->stack_[0];

    return true; // Success
}

void emulate_instruction(chip8_t* chip, const config_t config)
{
    // Since ram stores 8 bit, we shift the first section by 8 bits and
    // | the result with the next section to get the full 16 bit
    chip->inst_.opcode = chip->ram_[chip->PC_] << 8 | chip->ram_[chip->PC_ + 1];
    chip->PC_ += 2; // Since the opcodes are 2 bytes each


    chip->inst_.NNN = chip->inst_.opcode & 0xFFF;
    chip->inst_.NN = chip->inst_.opcode & 0xFF;
    chip->inst_.N = chip->inst_.opcode & 0xF;
    chip->inst_.X = (chip->inst_.opcode >> 8) & 0xF;
    chip->inst_.Y = (chip->inst_.opcode >> 4) & 0xF;

#ifdef DEBUG
    print_debug_info(chip);
#endif

    // Interpret opcode
    switch ((chip->inst_.opcode >> 12) & 0xF)
    {
        case 0x00:
            if (chip->inst_.NN == 0xE0)
            {
                // 0x00E0: Clear the screen
                memset(&chip->display_[0], false, sizeof(chip->display_));
            }
            else if (chip->inst_.NN == 0xEE)
            {
                // 0xOOEE: Returns from subroutine
                chip->PC_ = *--chip->stack_ptr_;
            }
            break;

        case 0x1:
            // 0x1NNN: jump to address NNN
            chip->PC_ = chip->inst_.NNN;
            break;

        case 0x2:
            // 0x2NNN: Call subroutine at NNN
            *chip->stack_ptr_++ = chip->PC_; // to return from subroutine later
            chip->PC_ = chip->inst_.NNN;
            break;

        case 0x3:
            // 0x3XNN: if (VX == NN) skip the next instruction
            if (chip->V_[chip->inst_.X] == chip->inst_.NN)
            {
                chip->PC_ += 2;
            }
            break;

        case 0x4:
            // 0x4XNN: if (VX != NN) skip the next instruction
            if (chip->V_[chip->inst_.X] != chip->inst_.NN)
            {
                chip->PC_ += 2;
            }
            break;

        case 0x5:
            // 0x5XY0: if (VX == VY) skip the next instruction
            if (chip->inst_.N != 0) break; // Wrong opcode

            if (chip->V_[chip->inst_.X] == chip->V_[chip->inst_.Y])
            {
                chip->PC_ += 2;
            }
            break;

        case 0x6:
            // 0x6XNN: Set VX register to NN
            chip->V_[chip->inst_.X] = chip->inst_.NN;
            break;
        
        case 0x7:
            // 0x7XNN: Add NN to VX register
            chip->V_[chip->inst_.X] += chip->inst_.NN;
            break;

        case 0x8:
            switch (chip->inst_.N)
            {
                case 0x0:
                    // 0x8XY0: Set the value of VY to VX
                    chip->V_[chip->inst_.X] = chip->V_[chip->inst_.Y];
                    break;

                case 0x1:
                    // 0x8XY1: Set VX to bitwise OR of VX and VY
                    chip->V_[chip->inst_.X] |= chip->V_[chip->inst_.Y];
                    break;

                case 0x2:
                    // 0x8XY2: Set VX to bitwise AND of VX and VY
                    chip->V_[chip->inst_.X] &= chip->V_[chip->inst_.Y];
                    break;

                case 0x3:
                    // 0x8XY3: Set VX to XOR of VX and VY
                    chip->V_[chip->inst_.X] ^= chip->V_[chip->inst_.Y];
                    break;

                case 0x4:
                    // 0x8XY4: Set VX += VY, set carry flag to 1 if overflow
                    if ((uint16_t)(chip->V_[chip->inst_.X] + chip->V_[chip->inst_.Y]) > 255)
                    {
                        chip->V_[0xF] = 1;
                    }
                    else
                    {
                        chip->V_[0xF] = 0;
                    }
                    chip->V_[chip->inst_.X] += chip->V_[chip->inst_.Y];
                    break;

                case 0x5:
                    // 0x8XY5: Set VX -= VY, set carry flag to 0 if underflow and 1 otherwise
                    if (chip->V_[chip->inst_.X] < chip->V_[chip->inst_.Y])
                    {
                        chip->V_[0xF] = 0;
                    }
                    else
                    {
                        chip->V_[0xF] = 1;
                    }
                    chip->V_[chip->inst_.X] -= chip->V_[chip->inst_.Y];
                    break;

                case 0x6:
                    // 0x8XY6: Set VX >>= 1, set carry flag to least significant bit prior to shifting
                    chip->V_[0xF] = chip->V_[chip->inst_.X] & 1;
                    chip->V_[chip->inst_.X] >>= 1;
                    break;

                case 0x7:
                    // 0x8XY7: Set VX = VY - VX, set carry flag to 0 if underflow and 1 otherwise
                    if (chip->V_[chip->inst_.Y] < chip->V_[chip->inst_.X])
                    {
                        chip->V_[0xF] = 0;
                    }
                    else
                    {
                        chip->V_[0xF] = 1;
                    }
                    chip->V_[chip->inst_.X] = chip->V_[chip->inst_.Y] - chip->V_[chip->inst_.X];
                    break;

                case 0xE:
                    // 0x8XYE: Set VX <<= 1, set carry flag to most significant bit prior to shifting
                    chip->V_[0xF] = (chip->V_[chip->inst_.X] & 0x80) >> 7; // 1000 0000
                    chip->V_[chip->inst_.X] <<= 1;
                    break;

                default:
                    // Wrong opcode
                    break;
            }
            break;
        
        case 0x9:
            // 0x9XY0: Skip next instruction if VX != VY
            if (chip->inst_.N != 0) break; // Wrong opcode
            
            if (chip->V_[chip->inst_.X] != chip->V_[chip->inst_.Y])
            {
                chip->PC_ += 2;
            }
            break;

        case 0xA:
            // 0xANNN: Set index register to NNN
            chip->I_ = chip->inst_.NNN;
            break;

        case 0xB:
            // 0xBNNN: Jump to NNN + V0
            chip->PC_ = chip->inst_.NNN + chip->V_[0];
            break;

        case 0xC:
            // 0xCXNN: Set VX = (rand() % 256) & NN
            chip->V_[chip->inst_.X] = (rand() % 256) & chip->inst_.NN;
            break;

        case 0xD: {
            // 0xDXYN: Draw height N at VX, VY reading form mem locations I
            uint8_t x_start = chip->V_[chip->inst_.X];
            uint8_t y_start = chip->V_[chip->inst_.Y];

            chip->V_[0xF] = 0; // Initialize carry flag to 0

            // N rows of sprite
            for (uint8_t row = 0; row < chip->inst_.N; ++row) {
                // Get row of sprite data
                uint8_t sprite_byte = chip->ram_[chip->I_ + row];
                uint8_t y = (y_start + row) % config.height_;

                for (uint8_t col = 0; col < 8; ++col) {
                    uint8_t x = (x_start + col) % config.width_;
                    uint8_t sprite_bit = (sprite_byte >> (7 - col)) & 0x1;

                    if (sprite_bit) {
                        bool* pixel = &chip->display_[y * config.width_ + x];
                        if (*pixel) {
                            chip->V_[0xF] = 1; // Collision
                        }
                        *pixel ^= 1;
                    }
                }
            }
            break;
        }
        
        case 0xE:
            if (chip->inst_.NN == 0x9E)
            {
                // 0xEX9E: skip the next instruction if the key in VX is pressed
                if (chip->keypad_[chip->V_[chip->inst_.X]])
                {
                    chip->PC_ += 2;
                }
            }
            else if (chip->inst_.NN == 0xA1)
            {
                // 0xEXA1: skip the next instruction if the key in VX is not pressed
                if (!chip->keypad_[chip->V_[chip->inst_.X]])
                {
                    chip->PC_ += 2;
                }
            }
            break;

        case 0xF:
            // TODO
            switch (chip->inst_.NN)
            {
                case 0x0A:
                    // 0xFX0A: set VX = get_key(), wait for key press and store it in VX
                    bool any_key_pressed = false;
                    for (uint8_t offset = 0; offset < sizeof(chip->keypad_); offset++)
                    {
                        if (chip->keypad_[offset])
                        {
                            chip->V_[chip->inst_.X] = offset;
                            any_key_pressed = true;
                        }
                    }

                    if (!any_key_pressed)
                    {
                        chip->PC_ -= 2;
                    }
                    break;

                case 0x1E:
                    // 0xFX1E: add VX to I
                    chip->I_ += chip->V_[chip->inst_.X];
                    break;

                case 0x07:
                    // 0xFX07: Set VX = delay_timer
                    chip->V_[chip->inst_.X] = chip->delay_timer_;
                    break;

                case 0x15:
                    // 0xFX15: Set delay_timer = VX
                    chip->delay_timer_ = chip->V_[chip->inst_.X];
                    break;

                case 0x18:
                    // 0xFX18: Set sound_timer = VX
                    chip->sound_timer_ = chip->V_[chip->inst_.X];
                    break;

                case 0x29:
                    // 0xFX29: Set I to sprite location in memory for character in VX
                    chip->I_ = 5 * chip->V_[chip->inst_.X];
                    break;

                case 0x33:
                    // 0xFX33: Set BCD value of VX at I
                    // I for 3 digits, I+1 for 2 digits and I+2 for 1 digit
                    uint8_t bcd = chip->V_[chip->inst_.X];
                    chip->ram_[chip->I_ + 2] = bcd % 10;
                    bcd /= 10;
                    chip->ram_[chip->I_ + 1] = bcd % 10;
                    bcd /= 10;
                    chip->ram_[chip->I_] = bcd;
                    break;

                case 0x55:
                    // 0xFX55: Register dump V0-VX inclusive to memory offset from I
                    // SCHIP does not increment I, CHIP8 does increment I
                    for (uint8_t it = 0; it <= chip->inst_.X; it++)
                    {
                        chip->ram_[chip->I_ + it] = chip->V_[it];
                    }
                    break;

                case 0x65:
                    // 0xFX65: Register load V0-VX inclusive from memory offset from I
                    // SCHIP does not increment I, CHIP8 does increment I
                    for (uint8_t it = 0; it <= chip->inst_.X; it++)
                    {
                         chip->V_[it] = chip->ram_[chip->I_ + it];
                    }
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}

#ifdef DEBUG
void print_debug_info(chip8_t* chip)
{
    printf("\nAddress: 0x%04X, Opcode: 0x%04X, Desc: ",
            chip->PC_ - 2, chip->inst_.opcode);

    switch ((chip->inst_.opcode >> 12) & 0xF)
    {
        case 0x00:
            if (chip->inst_.NN == 0xE0)
            {
                // 0x00E0: Clear the screen
                printf("Clear the screen\n");
            }
            else if (chip->inst_.NN == 0xEE)
            {
                // 0xOOEE: Returns from subroutine
                printf("Return from subroutine to address [0x%04X]\n",
                        *(chip->stack_ptr_ - 1));
            }
            else
            {
                // Unimplemented for now
                printf("Unimplemented instruction\n");
            }
            break;

        case 0x1:
            // 0x1NNN: jump to address NNN
            printf("Jump to address NNN [0x%03x]\n",
                    chip->inst_.NNN);
            break;

        case 0x2:
            // 0x2NNN: Call subroutine at NNN
            printf("Call subroutine at NNN [0x%03x]\n",
                    chip->inst_.NNN);
            *chip->stack_ptr_++ = chip->PC_; // to return from subroutine later
            chip->PC_ = chip->inst_.NNN;
            break;

        case 0x3:
            // 0x3XNN: if (VX == NN) skip the next instruction
            printf("Checking V%X (0x%02X) == NN (0x%02X), skip next instruction if true\n",
                    chip->inst_.X, chip->V_[chip->inst_.X], chip->inst_.NN);
            break;

        case 0x4:
            // 0x4XNN: if (VX != NN) skip the next instruction
            printf("Checking V%X (0x%02X) != NN (0x%02X), skip next instruction if true\n",
                    chip->inst_.X, chip->V_[chip->inst_.X], chip->inst_.NN);
            break;

        case 0x5:
            // 0x5XY0: if (VX == VY) skip the next instruction
            printf("Checking V%X (0x%02X) == V%X (0x%02X), skip next instruction if true\n",
                    chip->inst_.X, chip->V_[chip->inst_.X],
                    chip->inst_.Y, chip->V_[chip->inst_.Y]);
            break;

        case 0x6:
            // 0x6XNN: Set VX register to NN
            printf("Set the V%X register to NN [0x%02x]\n",
                    chip->inst_.X, chip->inst_.NN);
            break;

        case 0x7:
            // 0x7XNN: Add NN to VX register
            printf("Add NN [0x%02x] to V%X register\n",
                    chip->inst_.NN, chip->inst_.X);
            break;

        case 0x8:
            switch (chip->inst_.N)
            {
                case 0x0:
                    // 0x8XY0: Set the value of VY to VX
                    printf("Set the value of V%X [0x%02X] = V%X [0x%02x]\n",
                            chip->inst_.X, chip->V_[chip->inst_.X],
                            chip->inst_.Y, chip->V_[chip->inst_.Y]);
                    break;

                case 0x1:
                    // 0x8XY1: Set VX to bitwise OR of VX and VY
                    printf("Set the value of V%X [0x%02X] |= V%X [0x%02x], Result: 0x%02X\n",
                            chip->inst_.X, chip->V_[chip->inst_.X],
                            chip->inst_.Y, chip->V_[chip->inst_.Y],
                            chip->V_[chip->inst_.X] | chip->V_[chip->inst_.Y]);
                    break;

                case 0x2:
                    // 0x8XY2: Set VX to bitwise AND of VX and VY
                    printf("Set the value of V%X [0x%02X] &= V%X [0x%02x], Result: 0x%02X\n",
                            chip->inst_.X, chip->V_[chip->inst_.X],
                            chip->inst_.Y, chip->V_[chip->inst_.Y],
                            chip->V_[chip->inst_.X] & chip->V_[chip->inst_.Y]);
                    break;

                case 0x3:
                    // 0x8XY3: Set VX to XOR of VX and VY
                    printf("Set the value of V%X [0x%02X] ^= V%X [0x%02x], Result: 0x%02X\n",
                            chip->inst_.X, chip->V_[chip->inst_.X],
                            chip->inst_.Y, chip->V_[chip->inst_.Y],
                            chip->V_[chip->inst_.X] ^ chip->V_[chip->inst_.Y]);
                    break;

                case 0x4:
                    // 0x8XY4: Set VX += VY, set carry flag to 1 if overflow
                    printf("Set the value of V%X [0x%02X] += V%X [0x%02x], Set VF to 1 if overflow\n",
                            chip->inst_.X, chip->V_[chip->inst_.X],
                            chip->inst_.Y, chip->V_[chip->inst_.Y]);

                    printf("Result: 0x%02X, VF = %X\n",
                            chip->V_[chip->inst_.X] + chip->V_[chip->inst_.Y],
                            ((uint16_t)(chip->V_[chip->inst_.X] + chip->V_[chip->inst_.Y]) > 255));
                    break;

                case 0x5:
                    // 0x8XY5: Set VX -= VY, set carry flag to 0 if underflow
                    printf("Set the value of V%X [0x%02X] -= V%X [0x%02x], Set VF to 0 if underflow\n",
                            chip->inst_.X, chip->V_[chip->inst_.X],
                            chip->inst_.Y, chip->V_[chip->inst_.Y]);

                    printf("Result: 0x%02X, VF = %X\n",
                            chip->V_[chip->inst_.X] + chip->V_[chip->inst_.Y],
                            (chip->V_[chip->inst_.X] > chip->V_[chip->inst_.Y]));
                    break;

                case 0x6:
                    // 0x8XY6: Set VX >>= 1, set carry flag to least significant bit prior to shifting
                    printf("Set the value of V%X [0x%02X] >>= 1, Set VF to the least significant bit\n",
                            chip->inst_.X, chip->V_[chip->inst_.X]);

                    printf("Result: 0x%02X, VF = %X\n",
                            chip->V_[chip->inst_.X] >> 1,
                            chip->V_[chip->inst_.X] & 1);
                    break;

                case 0x7:
                    // 0x8XY7: Set VX = VY - VX, set carry flag to 0 if underflow
                    printf("Set the value of V%X = V%X [0x%02X] - V%X [0x02%X], Set VF to 0 if underflow\n",
                            chip->inst_.X,
                            chip->inst_.Y, chip->V_[chip->inst_.Y],
                            chip->inst_.X, chip->V_[chip->inst_.X]);

                    printf("Result: 0x%02X, VF = %X\n",
                            chip->V_[chip->inst_.Y] - chip->V_[chip->inst_.X],
                            (chip->V_[chip->inst_.Y] < chip->V_[chip->inst_.X]));
                    break;

                case 0xE:
                    // 0x8XYE: Set VX <<= 1, set carry flag to most significant bit prior to shifting
                    printf("Set the value of V%X [0x%02X] <<= 1, Set VF to the most significant bit\n",
                            chip->inst_.X, chip->V_[chip->inst_.X]);

                    printf("Result: 0x%02X, VF = %X\n",
                            chip->V_[chip->inst_.X] << 1,
                            chip->V_[chip->inst_.X] & 0x80); // 1000 0000
                    break;

                default:
                    printf("Unimplemented instruction\n");
                    break;
            }
            break;

        case 0x9:
            // 0x9XY0: Skip next instruction if VX != VY
            printf("Checking V%X (0x%02X) != V%X (0x%02X), skip next instruction if true\n",
                    chip->inst_.X, chip->V_[chip->inst_.X],
                    chip->inst_.Y, chip->V_[chip->inst_.Y]);
            break;
        
        case 0xA:
            // 0xANNN: Set index register to NNN
            printf("Set the index register to NNN [0x%04x]\n",
                    chip->inst_.NNN);
            break;

        case 0xB:
            // 0xBNNN: Jump to NNN + V0
            printf("Set PC to NNN [0x%02X] + V0 [0x%04x], Result: [0x%02X]\n",
                    chip->inst_.NNN, chip->V_[0],
                    chip->inst_.NNN + chip->V_[0]);
            break;

        case 0xC:
            // 0xCXNN: Set VX = (rand() % 256) & NN
            printf("Set V%X = (rand() %% 256) & NN (ox%02X)\n",
                    chip->inst_.X, chip->inst_.NN);
            break;

        case 0xD:
            // 0xDXYN: draw at (X, Y) with height N
            printf("Draw N (%u) height sprite at V%X (0x%02x), V%X (0x%02X) "
                    "from memory location I (0x%04X).\n",
                    chip->inst_.N,
                    chip->inst_.X, chip->V_[chip->inst_.X],
                    chip->inst_.Y, chip->V_[chip->inst_.Y],
                    chip->I_);
            break;

        case 0xE:
            if (chip->inst_.NN == 0x9E)
            {
                // 0xEX9E: skip the next instruction if the key in VX is pressed
                printf("Skip next instruction if key in V%X (0x%02X) is pressed, Keypad value: %d\n",
                        chip->inst_.X, chip->V_[chip->inst_.X],
                        chip->keypad_[chip->V_[chip->inst_.X]]);
            }
            else if (chip->inst_.NN == 0xA1)
            {
                // 0xEXA1: skip the next instruction if the key in VX is not pressed
                printf("Skip next instruction if key in V%X (0x%02X) is not pressed, Keypad value: %d\n",
                        chip->inst_.X, chip->V_[chip->inst_.X],
                        chip->keypad_[chip->V_[chip->inst_.X]]);
            }
            break;

        case 0xF:
            // TODO
            switch (chip->inst_.NN)
            {
                case 0x0A:
                    // 0xFX0A: set VX = get_key(), wait for key press and store it in VX
                    printf("Wait for key press and store it in V%X\n",
                            chip->inst_.X);
                    break;

                case 0x1E:
                    // 0xFX1E: add VX to I
                    printf("Add V%X [0x%02X] to I [0x%04X]\n",
                            chip->inst_.X, chip->V_[chip->inst_.X],
                            chip->I_);
                    break;

                case 0x07:
                    // 0xFX07: Set VX = delay_timer
                    printf("Set V%X = delay_timer [0x%02X]\n",
                            chip->inst_.X, chip->delay_timer_);
                    break;

                case 0x15:
                    // 0xFX15: Set delay_timer = VX
                    printf("Set delay_timer = V%X [0x%02X]\n",
                            chip->inst_.X, chip->V_[chip->inst_.X]);
                    break;

                case 0x18:
                    // 0xFX18: Set sound_timer = VX
                    printf("Set sound_timer = V%X [0x%02X]\n",
                            chip->inst_.X, chip->V_[chip->inst_.X]);
                    break;

                case 0x29:
                    // 0xFX29: Set I to sprite location in memory for character in VX
                    printf("Set I to memory location for character in V%X [0x%02X], Result: 0x%02X\n",
                            chip->inst_.X, chip->V_[chip->inst_.X],
                            5 * chip->V_[chip->inst_.X]);
                    break;

                case 0x33:
                    // 0xFX33: Set BCD value of VX at I
                    // I for 3 digits, I+1 for 2 digits and I+2 for 1 digit
                    printf("Store BCD representation of V%X [0x%02X] at memory from I [0x%02X]\n",
                            chip->inst_.X, chip->V_[chip->inst_.X], chip->I_);
                    break;

                case 0x55:
                    // 0xFX55: Register dump V0-VX inclusive to memory offset from I
                    // SCHIP does not increment I, CHIP8 does increment I
                    printf("Register dump V0-V%X [0x%02X] inclusive at memory from I [0x%02X]\n",
                            chip->inst_.X, chip->V_[chip->inst_.X], chip->I_);
                    break;

                case 0x65:
                    // 0xFX65: Register load V0-VX inclusive from memory offset from I
                    // SCHIP does not increment I, CHIP8 does increment I
                    printf("Register load V0-V%X [0x%02X] inclusive from memory from I [0x%02X]\n",
                            chip->inst_.X, chip->V_[chip->inst_.X], chip->I_);
                    break;

                default:
                    break;
            }
            break;

        default:
            printf("Unimplemented instruction\n");
            break;
    }
}
#endif // DEBUG
