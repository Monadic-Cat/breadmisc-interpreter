#include <stdio.h>
#include "breadmisc.h"

const size_t term_height = 24;
const size_t term_width = 80;
const size_t term_size = term_height * term_width;

const size_t free_area = 4000;

// Sum things that precede execution area.
// Memory mapped hardware sort of thing.
const size_t mem_pre_size = term_size + free_area;

void refresh_terminal(struct machine* mach) {
   printf("\033[0;0"); // Move terminal cursor to top left corner.
   for(int r = 0; i < term_height; r++) {
      for(int c = 0; i < term_width; c++) {
         printf(mach->mem[(r * term_width) + c]);
         printf("\033[0;%b", r);
      }
   }
}

// This setup for hooking into the virtual machine is clearly flawed.
// Decoding instructions should be abstracted away entirely.
void handler(struct machine* mach) {
   // This is executed before every instruction.
   printf("Ran an instruction, lol.\n");
   struct instruction inst = decode_instruction(
      mach->mem[mach->regs.rp]
      );
   if(inst.fptr == machine_move) {
      uint16_t data = inst.instruction_specific_data;
      if((data & 1) && (data & (1 << 1))) {
         machine_exec_single(mach);
         refresh_terminal();
      }
   }
}

int main(int argc, char* argv[]) {
   printf("Hello.\n");
   FILE* f;
   size_t f_size;
   if(argc > 1) {
      f = fopen(argv[1], "rb");
      if(f == NULL) {
         printf("Failed to open indicated file. Exiting.\n");
         return 1;
      }
      // I hear this can be tricked. Do more research later.
      // Possibly use Rust instead.
      fseek(f, 0L, SEEK_END);
      f_size = ftell(f);
      rewind(f);
   } else {
      printf("No input file. Exiting.\n");
      return 2;
   }
   struct machine example = make_machine(mem_pre_size + f_size);
   if(!example.initialized) {
      printf("Machine failed to initialize. Exiting.\n");
      return 3;
   }
   // Read input file into memory, in word size blocks.
   fread(example.mem, 4, f_size / 4, f);
   fclose(f); // Close file. Never forget to do this. AAAHHH
   example.regs.rp = mem_pre_size;
   
   machine_exec_handled(&example, handler);
}
