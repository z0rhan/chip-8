#include "chip-8.h"

#include <stdio.h>
#include <SDL2/SDL.h>

#ifdef DEBUG
void print_debug_info(chip8_t* chip)
{
    printf("Address: 0x%04X, Opcode: 0x%04X, Desc: ",
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

        // case 0x8:
        //     if (chip->inst_.N == 0)
        //     {
        //         // 0x8XY0: Set the value of VY to VX
        //         printf("Set the V%X register to V%X [0x%02x]\n",
        //                 chip->inst_.X, chip->inst_.Y, chip->V_[chip->inst_.Y]);
        //     }
        //     else if (chip->inst_.N == 1)
        //     {
        //         // 0x8XY1: Set VX to bitwise OR of VX and VY
        //         printf("Set the V%X register to V%X OR V%X [0x%02x]\n",
        //                 chip->inst_.X, chip->inst_.X, chip->inst_.Y,
        //                 chip->V_[chip->inst_.X] | chip->V_[chip->inst_.Y]);
        //     }
        //     else if (chip->inst_.N == 2)
        //     {
        //         // 0x8XY2: Set VX to bitwise AND of VX and VY
        //         printf("Set the V%X register to V%X AND V%X [0x%02x]\n",
        //                 chip->inst_.X, chip->inst_.X, chip->inst_.Y,
        //                 chip->V_[chip->inst_.X] & chip->V_[chip->inst_.Y]);
        //     }
        //     else if (chip->inst_.N == 3)
        //     {
        //         // 0x8XY2: Set VX to XOR of VX and VY
        //         printf("Set the V%X register to V%X XOR V%X [0x%02x]\n",
        //                 chip->inst_.X, chip->inst_.X, chip->inst_.Y,
        //                 chip->V_[chip->inst_.X] ^ chip->V_[chip->inst_.Y]);
        //     }
        //     break;
        // 
        
        case 0xA:
            // 0xANNN: Set index register to NNN
            printf("Set the index register to NNN [0x%04x]\n",
                    chip->inst_.NNN);
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

        default:
            printf("Unimplemented instruction\n");
            break;
    }
}
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

        case 0x6:
            // 0x6XNN: Set VX register to NN
            chip->V_[chip->inst_.X] = chip->inst_.NN;
            break;
        
        case 0x7:
            // 0x7XNN: Add NN to VX register
            chip->V_[chip->inst_.X] += chip->inst_.NN;
            break;

        // case 0x8:
        //     if (chip->inst_.N == 0)
        //     {
        //         // 0x8XY0: Set the value of VY to VX
        //         chip->V_[chip->inst_.X] = chip->V_[chip->inst_.Y];
        //     }
        //     else if (chip->inst_.N == 1)
        //     {
        //         // 0x8XY1: Set VX to bitwise OR of VX and VY
        //         chip->V_[chip->inst_.X] |= chip->V_[chip->inst_.Y];
        //     }
        //     else if (chip->inst_.N == 2)
        //     {
        //         // 0x8XY2: Set VX to bitwise AND of VX and VY
        //         chip->V_[chip->inst_.X] &= chip->V_[chip->inst_.Y];
        //     }
        //     else if (chip->inst_.N == 3)
        //     {
        //         // 0x8XY2: Set VX to XOR of VX and VY
        //         chip->V_[chip->inst_.X] ^= chip->V_[chip->inst_.Y];
        //     }
        //     break;

        case 0xA:
            // 0xANNN: Set index register to NNN
            chip->I_ = chip->inst_.NNN;
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

                for (uint8_t bit = 0; bit < 8; ++bit) {
                    uint8_t x = (x_start + bit) % config.width_;
                    uint8_t sprite_bit = (sprite_byte >> (7 - bit)) & 0x1;

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

        default:
            break;
    }
}
