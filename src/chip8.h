#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct chip8 {
  unsigned short pc;
  unsigned short opcode;
  unsigned short I;
  unsigned short sp;
  bool draw_flag;

  GLFWwindow *window;
  char display[64 * 32];
  unsigned short stack[16];
  unsigned char V[16];
  unsigned char memory[4096];
  unsigned char key[16];

  unsigned char delay_timer;
  unsigned char sound_timer;

  bool exit_flag;
  unsigned short exit_status;
} chip8;

enum Comparison { EQUAL, GREATERTHAN, LESSTHAN, NOTEQUAL };

enum RegOperationType {
  ASSIGNMENT,
  BITWISE_OR,
  BITWISE_AND,
  BITWISE_XOR,
  ADD,
  SUBTRACT,
  BITSHIFT_R,
  REVERSE_SUB,
  BITSHIFT_L = 0x000E
};

void initialise_chip8(chip8 *cpu);
void load_program(chip8 *cpu, char *program_name);
void executeCpuCycle(chip8 *cpu);

void drawDisplay(chip8 *cpu);
void clearDisplay(chip8 *cpu);
