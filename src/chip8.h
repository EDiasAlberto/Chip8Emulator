// opcode bitmasks
const short OPCODE_BITMASK = 0xF000;
const short OPERAND_1_BITMASK = 0x0F00;
const short OPERAND_2_BITMASK = 0x00F0;
const short SUBTYPE_BITMASK = 0x000F;

typedef struct chip8 {
  unsigned short pc;
  unsigned short opcode;
  unsigned short I;
  unsigned short sp;

  char display[64 * 32];
  unsigned short stack[16];
  unsigned char V[16];
  unsigned char memory[4096];
  unsigned char key[16];

  unsigned char delay_timer;
  unsigned char sound_timer;
} chip8;

void initialise_chip8(chip8 *cpu);
void load_program(chip8 *cpu, char *program_name);
