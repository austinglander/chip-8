# chip-8
Interpreter for the CHIP-8 programming language

# Usage

```bash
gcc chip8.c -lSDL2 -lSDL2_mixer -o c8.o
./c8.o <rom file>
```

# Configuration
Change the `INSTRUCTIONS_PER_FRAME` macro to adjust the speed of the interpreter. 10-20 is standard.

# Dependencies

- SDL2
- SDL2 Mixer

## Dependency Installation

```bash
sudo dnf install SDL2-devel
sudo dnf install SDL2_mixer-devel
```