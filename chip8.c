#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#include <time.h>
#define NANOS_PER_MILLI 1000000L

// This interpreter was written primarily using Cowgod's reference
// Several references are made to sections of Cowgod's document throughout this program
// You can find one mirror here: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.1

#define INSTRUCTIONS_PER_FRAME 10
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define SCALE_FACTOR 16

// Static Sprites (2.4)
#define SPRITE_0_OFFSET 0 // where in memory this sprite goes
#define SPRITE_1_OFFSET 5
#define SPRITE_2_OFFSET 10
#define SPRITE_3_OFFSET 15
#define SPRITE_4_OFFSET 20
#define SPRITE_5_OFFSET 25
#define SPRITE_6_OFFSET 30
#define SPRITE_7_OFFSET 35
#define SPRITE_8_OFFSET 40
#define SPRITE_9_OFFSET 45
#define SPRITE_A_OFFSET 50
#define SPRITE_B_OFFSET 55
#define SPRITE_C_OFFSET 60
#define SPRITE_D_OFFSET 65
#define SPRITE_E_OFFSET 70
#define SPRITE_F_OFFSET 75

const uint8_t SPRITE_0[5] = {
    0b11110000,
    0b10010000,
    0b10010000,
    0b10010000,
    0b11110000
};

const uint8_t SPRITE_1[5] = {
    0b00100000, 
    0b01100000, 
    0b00100000, 
    0b00100000, 
    0b01110000
};

const uint8_t SPRITE_2[5] = {
    0b11110000,
    0b00010000,
    0b11110000,
    0b10000000,
    0b11110000
};

const uint8_t SPRITE_3[5] = {
    0b11110000,
    0b00010000,
    0b11110000,
    0b00010000,
    0b11110000
};

const uint8_t SPRITE_4[5] = {
    0b10010000,
    0b10010000,
    0b11110000,
    0b00010000,
    0b00010000
};

const uint8_t SPRITE_5[5] = {
    0b11110000,
    0b10000000,
    0b11110000,
    0b00010000,
    0b11110000
};

const uint8_t SPRITE_6[5] = {
    0b11110000,
    0b10000000,
    0b11110000,
    0b10010000,
    0b11110000
};

const uint8_t SPRITE_7[5] = {
    0b11110000,
    0b00010000,
    0b00100000,
    0b01000000,
    0b01000000
};

const uint8_t SPRITE_8[5] = {
    0b11110000,
    0b10010000,
    0b11110000,
    0b10010000,
    0b11110000
};

const uint8_t SPRITE_9[5] = {
    0b11110000,
    0b10010000,
    0b11110000,
    0b00010000,
    0b11110000
};

const uint8_t SPRITE_A[5] = {
    0b11110000,
    0b10010000,
    0b11110000,
    0b10010000,
    0b10010000
};

const uint8_t SPRITE_B[5] = {
    0b11100000,
    0b10010000,
    0b11100000,
    0b10010000,
    0b11100000
};

const uint8_t SPRITE_C[5] = {
    0b11110000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b11110000
};

const uint8_t SPRITE_D[5] = {
    0b11100000,
    0b10010000,
    0b10010000,
    0b10010000,
    0b11100000
};

const uint8_t SPRITE_E[5] = {
    0b11110000,
    0b10000000,
    0b11110000,
    0b10000000,
    0b11110000
};

const uint8_t SPRITE_F[5] = {
    0b11110000,
    0b10000000,
    0b11110000,
    0b10000000,
    0b10000000
};

typedef struct
{
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
    // printf("Executing instruction 0x%04X from location 0x%03X; nnn=0x%03X, nibble=%d, x=%d, y=%d, kk=%d\n", opcode, C8.PC, nnn, nibble, x, y, kk);
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
                    break;
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
                    switch (C8.V[x]) {
                        case 0:
                            C8.I = SPRITE_0_OFFSET;
                            break;
                        case 1:
                            C8.I = SPRITE_1_OFFSET;
                            break;
                        case 2:
                            C8.I = SPRITE_2_OFFSET;
                            break;
                        case 3:
                            C8.I = SPRITE_3_OFFSET;
                            break;
                        case 4:
                            C8.I = SPRITE_4_OFFSET;
                            break;
                        case 5:
                            C8.I = SPRITE_5_OFFSET;
                            break;
                        case 6:
                            C8.I = SPRITE_6_OFFSET;
                            break;
                        case 7:
                            C8.I = SPRITE_7_OFFSET;
                            break;
                        case 8:
                            C8.I = SPRITE_8_OFFSET;
                            break;
                        case 9:
                            C8.I = SPRITE_9_OFFSET;
                            break;
                        case 0xA:
                            C8.I = SPRITE_A_OFFSET;
                            break;
                        case 0xB:
                            C8.I = SPRITE_B_OFFSET;
                            break;
                        case 0xC:
                            C8.I = SPRITE_C_OFFSET;
                            break;
                        case 0xD:
                            C8.I = SPRITE_D_OFFSET;
                            break;
                        case 0xE:
                            C8.I = SPRITE_E_OFFSET;
                            break;
                        case 0xF:
                            C8.I = SPRITE_F_OFFSET;
                            break;
                        default:
                            printf("Attempted to load nonexistent sprite for digit %X\n", C8.V[x]);
                            exit(1);
                    }
                    break;
                case 0x33: // LD B, Vx
                    C8.memory[C8.I] = C8.V[x] / 100;
                    C8.memory[C8.I+1] = (C8.V[x] % 100) / 10;
                    C8.memory[C8.I+2] = C8.V[x] % 10;
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

// Loads sprite data for 0-F into the interpreter area of Chip-8 memory
void load_static_sprites() {
    memcpy(&C8.memory[SPRITE_0_OFFSET], SPRITE_0, 5);
    memcpy(&C8.memory[SPRITE_1_OFFSET], SPRITE_1, 5);
    memcpy(&C8.memory[SPRITE_2_OFFSET], SPRITE_2, 5);
    memcpy(&C8.memory[SPRITE_3_OFFSET], SPRITE_3, 5);
    memcpy(&C8.memory[SPRITE_4_OFFSET], SPRITE_4, 5);
    memcpy(&C8.memory[SPRITE_5_OFFSET], SPRITE_5, 5);
    memcpy(&C8.memory[SPRITE_6_OFFSET], SPRITE_6, 5);
    memcpy(&C8.memory[SPRITE_7_OFFSET], SPRITE_7, 5);
    memcpy(&C8.memory[SPRITE_8_OFFSET], SPRITE_8, 5);
    memcpy(&C8.memory[SPRITE_9_OFFSET], SPRITE_9, 5);
    memcpy(&C8.memory[SPRITE_A_OFFSET], SPRITE_A, 5);
    memcpy(&C8.memory[SPRITE_B_OFFSET], SPRITE_B, 5);
    memcpy(&C8.memory[SPRITE_C_OFFSET], SPRITE_C, 5);
    memcpy(&C8.memory[SPRITE_D_OFFSET], SPRITE_D, 5);
    memcpy(&C8.memory[SPRITE_E_OFFSET], SPRITE_E, 5);
    memcpy(&C8.memory[SPRITE_F_OFFSET], SPRITE_F, 5);
}

void render(SDL_Renderer* renderer) {
    // Black background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // White foreground
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int col = 0; col < DISPLAY_WIDTH * SCALE_FACTOR; col++) {
        for (int row = 0; row < DISPLAY_HEIGHT * SCALE_FACTOR; row++) {
            if (C8.display[col / SCALE_FACTOR][row / SCALE_FACTOR]) {
                SDL_RenderDrawPoint(renderer, col, row);
            }
        }
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }

    // Read sprites into memory
    load_static_sprites();

    // Read program into memory
    FILE *fptr;
    fptr = fopen(argv[1], "rb");
    if (fptr == NULL) {
        printf("Unable to open file '%s'\n", argv[1]);
        return 1;
    }
    fread(&C8.memory[0x200], sizeof(uint8_t), sizeof(C8.memory) - 0x200, fptr);
    C8.PC = 0x200;

    // Init rendering engine
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window* window;
    SDL_CreateWindowAndRenderer(DISPLAY_WIDTH * SCALE_FACTOR, DISPLAY_HEIGHT * SCALE_FACTOR, 0, &window, &renderer);
    
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    uint8_t running = 1;
    while (running) {
        // Execute instructions
        for (int i = 0; i < INSTRUCTIONS_PER_FRAME; i++) {
            execute_instruction();
        }

        // TODO: play sound

        // Update timers (2.5)
        if (C8.delay_timer > 0) C8.delay_timer--;
        if (C8.sound_timer > 0) C8.sound_timer--;

        // Draw frame
        render(renderer);

        // Check for window close action
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) running = 0;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}