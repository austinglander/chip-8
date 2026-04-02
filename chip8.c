#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#define NANOS_PER_MILLI 1000000L

// This interpreter was written primarily using Cowgod's reference
// Several references are made to sections of Cowgod's document
// You can find one mirror here: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.1

#define INSTRUCTIONS_PER_FRAME 10
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

typedef struct {
    uint8_t memory[4096]; // 2.1
    uint8_t V[16]; // 2.2
    uint16_t I;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t PC;
    uint8_t SP;
    uint16_t stack[16];
    uint8_t keyboard[16]; // 2.3
    uint8_t display[DISPLAY_WIDTH][DISPLAY_HEIGHT]; // 2.4 (column major)
} chip8;

chip8 C8 = {0};

void execute_instruction() {
    // 3.0
    if (C8.PC % 2 != 0) {
        printf("Failed to execute instruction at 0x%03X; misaligned PC\n", C8.PC);
        exit(1);
    }

    uint16_t opcode = (C8.memory[C8.PC] << 8) + (C8.memory[C8.PC + 1]);
    uint16_t nnn = opcode & 0x0FFF;
    uint8_t nibble = opcode & 0x000F;
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t kk = opcode & 0x00FF;
    printf("Executing instruction 0x%04X from location 0x%03X; nnn=0x%03X, nibble=%d, x=%d, y=%d, kk=%d\n", opcode, C8.PC, nnn, nibble, x, y, kk);
    C8.PC += 2;

    switch (opcode & 0xF000) { // switch over first nibble
        case 0x0000:
            switch (opcode) {
                case 0x00E0: // CLS
                    memset(&C8.display, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
                    break;
                case 0x00EE: // RET
                    // Diverges from Cowgod by decrementing SP first. Makes stack[0] usable
                    C8.PC = C8.stack[--C8.SP];
                    break;
                default:
                    printf("Attempted to execute unknown instruction %04X", opcode);
                    exit(1);
                    break;
            }
            break;
        case 0x1000: // JP addr
            C8.PC = nnn;
            break;
        case 0x2000: // CALL addr
            // Diverges from Cowgod by incrementing SP after pushing PC. Makes stack[0] usable
            C8.stack[C8.SP++] = C8.PC;
            C8.PC = nnn;
            break;
        case 0x3000: // SE Vx, byte
            if (C8.V[x] == kk) C8.PC += 2;
            break;
        case 0x4000: // SNE Vx, byte
            if (C8.V[x] != kk) C8.PC += 2;
            break;
        case 0x5000: // SE Vx, Vy
            if (opcode & 0x000F != 0) {
                printf("Attempted to execute unknown instruction %04X", opcode);
                exit(1);
            }
            if (C8.V[x] == C8.V[y]) C8.PC += 2;
            break;
        case 0x6000: // LD Vx, byte
            C8.V[x] = kk;
            break;
        case 0x7000: // ADD Vx, byte
            C8.V[x] += kk;
            break;
        case 0x8000: // Register arithmetic
            switch (opcode & 0x000F) {
                case 0: // LD Vx, Vy
                    C8.V[x] = C8.V[y];
                    break;
                case 1: // OR Vx, Vy
                    C8.V[x] |= C8.V[y];
                    break;
                case 2: // AND Vx, Vy
                    C8.V[x] &= C8.V[y];
                    break;
                case 3: // XOR Vx, Vy
                    C8.V[x] ^= C8.V[y];
                    break;
                case 4: // ADD Vx, Vy
                    if ((uint16_t)C8.V[x] + C8.V[y] > 255) C8.V[0xF] = 1;
                    else C8.V[0xF] = 0;
                    C8.V[x] += C8.V[y];
                    break;
                case 5: // SUB Vx, Vy
                    if (C8.V[x] > C8.V[y]) C8.V[0xF] = 1; // TODO: understand why this is > and not >=
                    else C8.V[0xF] = 0;
                    C8.V[x] -= C8.V[y];
                case 6: // SHR Vx {, Vy}
                    if (C8.V[x] % 2 == 1) C8.V[0xF] = 1;
                    else C8.V[0xF] = 0;
                    C8.V[x] >>= 1;
                    break;
                case 7: // SUBN Vx, Vy
                    if (C8.V[y] > C8.V[x]) C8.V[0xF] = 1;
                    else C8.V[0xF] = 0;
                    C8.V[x] = C8.V[y] - C8.V[x];
                    break;
                case 0xE: // SHL Vx {, Vy}
                    if (C8.V[x] >= 128) C8.V[0xF] = 1;
                    else C8.V[0xF] = 0;
                    C8.V[x] <<= 1;
                    break;
                default:
                    printf("Attempted to execute unknown instruction %04X", opcode);
                    exit(1);
            }
            break;
        case 0x9000: // SNE Vx, Vy
            if (C8.V[x] != C8.V[y]) C8.PC += 2;
            break;
        case 0xA000: // LD I, addr
            C8.I = nnn;
            break;
        case 0xB000: // JP V0, addr
            C8.PC = nnn + C8.V[0];
            break;
        case 0xC000: // RND Vx, byte
            C8.V[x] = (rand() % 256) & kk;
            break;
        case 0xD000: // DRW Vx, Vy, nibble
            // Draws sprites left to right, top to bottom
            uint8_t draw_buffer[16];
            memcpy(draw_buffer, &C8.memory[C8.I], nibble);
            for (int row = 0; row < nibble; row++) {
                for (int column = 0; column < 8; column++) { // All sprites are 8 bits wide
                    uint8_t sprite_bit = (draw_buffer[row] >> (7 - column)) & 1;
                    uint8_t* write_location = &C8.display[(C8.V[x] + column) % DISPLAY_WIDTH][(C8.V[y] + row) % DISPLAY_HEIGHT];
                    // If both the sprite bit and existing bit here are on (1), set VF to 1
                    if (*write_location == 1 && sprite_bit == 1) C8.V[0xF] = 1;
                    else C8.V[0xF] = 0;
                    *write_location |= sprite_bit;
                    // Sanity check TODO: Remove
                    if (*write_location > 1 || sprite_bit > 1) {
                        printf("A pixel value got set to something greater than 1\n");
                        exit(1);
                    }
                }
            }
            break;
        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x9E: // SKP Vx
                    if (C8.V[x] > 15) {
                        printf("Attempted to check down state of %X key (does not exist)\n", C8.V[x]);
                        exit(1);
                    }
                    if (C8.keyboard[C8.V[x]] == 1) C8.PC += 2;
                    break;
                case 0xA1: // SKNP Vx
                    if (C8.V[x] > 15) {
                        printf("Attempted to check down state of %X key (does not exist)\n", C8.V[x]);
                        exit(1);
                    }
                    if (C8.keyboard[C8.V[x]] == 0) C8.PC += 2;
                    break;
                default:
                    printf("Attempted to execute unknown instruction %04X", opcode);
                    exit(1);
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0A: // LD Vx, K
                    uint8_t key_pressed = 0;
                    for (int i = 0; i < 16; i++) {
                        if (C8.keyboard[i] == 1) {
                            C8.V[x] = i;
                            key_pressed = 1;
                            break;
                        }
                    }
                    // If no key press was detected, re-run this instruction
                    if (!key_pressed) C8.PC -= 2;
                    break;
                case 0x15: // LD DT, Vx
                    C8.delay_timer = C8.V[x];
                    break;
                case 0x18: // LD ST, Vx
                    C8.sound_timer = C8.V[x];
                    break;
                case 0x1E: // ADD I, Vx
                    C8.I += C8.V[x];
                    break;
                case 0x29: // LD F, Vx
                    // TODO
                    C8.I = 0;
                    printf("Running unimplemented Fx29 instruction\n");
                    break;
                case 0x33: // LD B, Vx
                    // TODO
                    printf("Running unimplemented Fx33 instruction\n");
                    break;
                case 0x55: // LD [I], Vx
                    memcpy(&C8.memory[C8.I], C8.V, x + 1); // x = n -> copy n + 1 regs to mem
                    break;
                case 0x65: // LD Vx, [I]
                    memcpy(C8.V, &C8.memory[C8.I], x + 1); // x = n -> copy mem into n + 1 regs
                    break;
                default:
                    printf("Attempted to execute unknown instruction %04X", opcode);
                    exit(1);
            }
            break;
        default:
            printf("Attempted to execute unknown instruction %04X", opcode);
            exit(1);
    }
}

void render() {
    printf("Called render()\n\n");
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 17 * NANOS_PER_MILLI;
    nanosleep(&ts, NULL);
}


int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }

    // Read program into memory
    FILE* fptr;
    fptr = fopen(argv[1], "rb");
    if (fptr == NULL) {
        printf("Unable to open file '%s'\n", argv[1]);
        return 1;
    }
    fread(&C8.memory[0x200], sizeof(uint8_t), sizeof(C8.memory) - 0x200, fptr);
    C8.PC = 0x200;
    

    uint8_t running = 1;
    while (running) {
        // Execute instructions
        for (int i = 0; i < INSTRUCTIONS_PER_FRAME; i++) {
            execute_instruction();
        }

        // Update timers (2.5)
        if (C8.delay_timer > 0) C8.delay_timer--;
        if (C8.sound_timer > 0) C8.sound_timer--;

        // Draw frame
        render();
    }

        // Update timers
        // Render a frame from the memory buffer using SDL 2
    return 0;
}


// Display
// Load n bytes from memory location
// I think I need to adjust my approach, 
// as it's only clean implementationfor byte boundaries