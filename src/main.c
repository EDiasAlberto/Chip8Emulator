#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  chip8 *cpu = malloc(sizeof(chip8));

  if (cpu == NULL) {
    perror("ERROR ALLOCATING MEMORY FOR CPU STRUCT");
  }

  initialise_chip8(cpu);
}
