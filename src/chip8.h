#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct chip8 {
  unsigned short pc;
  unsigned short opcode;
  unsigned short I;
  unsigned short sp;
  bool draw_flag;
  bool halted;
  unsigned short waiting_key_register;

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

enum Comparison {
  EQUAL = 0x0,
  GREATERTHAN = 0x1,
  LESSTHAN = 0x2,
  NOTEQUAL = 0x3
};

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

enum InstructionClassification {
  CONTROL = 0x0000,
  UNCOND_JUMP = 0x1000,
  ENTER_SUBROUTINE = 0x2000,
  REG_IMM_COMP_EQ = 0x3000,
  REG_IMM_COMP_NE = 0x4000,
  COND_JUMP = 0x5000,
  LOAD_IMM = 0x6000,
  ADD_IMM = 0x7000,
  REG_REG_OP = 0x8000,
  REG_REG_NE = 0x9000,
  LOAD_IMM_I = 0xA000,
  LOAD_IMM_PC = 0xB000,
  LOAD_RAND = 0xC000,
  DRAW_INSTRUCTION = 0xD000,
  KEYP_JUMP = 0xE000,
  TIMER_INSTRUCTION = 0xF000
};

void initialise_chip8(chip8 *cpu);
void load_program(chip8 *cpu, char *program_name);
void executeCpuCycle(chip8 *cpu);

void drawDisplay(chip8 *cpu);
void clearDisplay(chip8 *cpu);
