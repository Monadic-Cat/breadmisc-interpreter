#include <stdlib.h>
#include <stdio.h>
#include "breadmisc.h"

bool condition_always(uint32_t a, uint32_t b) {
   return true;
}
bool condition_equal_to(uint32_t a, uint32_t b) {
   return a == b;
}
bool condition_greater_than(uint32_t a, uint32_t b) {
   return a > b;
}
bool condition_less_than(uint32_t a, uint32_t b) {
   return a < b;
}

bool condition_neg_always(uint32_t a, uint32_t b) {
   return false;
}
bool condition_neg_equal_to(uint32_t a, uint32_t b) {
   return a != b;
}
bool condition_neg_greater_than(uint32_t a, uint32_t b) {
   return a <= b;
}
bool condition_neg_less_than(uint32_t a, uint32_t b) {
   return a >= b;
}

// It might be a good idea to swap the always and never codes.
predicate lookup_condition(uint8_t a) {
   switch(a) {
      case 0: return condition_always; break;
      case 1: return condition_equal_to; break;
      case 2: return condition_greater_than; break;
      case 3: return condition_less_than; break;
      case 4: return condition_neg_always; break;
      case 5: return condition_neg_equal_to; break;
      case 6: return condition_neg_greater_than; break;
      case 7: return condition_neg_less_than; break;
   }
}

bool check_condition(struct machine* mach, struct instruction inst) {
   return inst.condition_operation(
      *(mach->regs.register_index[inst.condition_register_a]),
      *(mach->regs.register_index[inst.condition_register_b])
      );
}

void machine_add(struct machine* mach, uint16_t data) {
   bool immediate = data & 1;
   uint8_t first_register = (data & (0b11 << 1)) >> 1;
   uint8_t second_register = (data & (0b11 << 6)) >> 6;

   if(immediate) {
      *(mach->regs.register_index[first_register]) +=
         mach->mem[mach->regs.rp];
      if(second_register != first_register) {
         *(mach->regs.register_index[second_register]) +=
            mach->mem[mach->regs.rp];
      }
      mach->regs.rp++;
   } else {
      *(mach->regs.register_index[first_register]) +=
         *(mach->regs.register_index[second_register]);
   }
}
void machine_move(struct machine* mach, uint16_t data) {
   bool memory = data & 1;
   bool write_or_immediate = (data & (1 << 1)) >> 1;
   uint8_t first_register = (data & (0b11111 << 2)) >> 2;
   uint8_t second_register = (data & (0b11111 << 7)) >> 7;

   if(!(memory && write_or_immediate)) {
      uint32_t new_value;
      if(write_or_immediate) new_value = mach->mem[mach->regs.rp++];
      else if(memory) new_value = mach->mem[mach->mem[mach->regs.rp++]];
      else new_value = *(mach->regs.register_index[second_register]);
		
      *(mach->regs.register_index[first_register]) = new_value;
   } else {
      mach->mem[mach->mem[mach->regs.rp++]] = *(mach->regs.register_index[second_register]);
   }
}

bool two_bit_predicate(uint8_t table, uint8_t a, uint8_t b) {
   if(!a && !b) {
      return table & 1;
   } else if(!a && b) {
      return table & (1 << 1);
   } else if(a && !b) {
      return table & (1 << 2);
   } else if(a && b) {
      return table & (1 << 3);
   }
}

void machine_bitwise_operation(struct machine* mach, uint16_t data) {
   uint8_t first_register = 0; // Always r0 and another register.
   uint8_t second_register = data & 0b11111;
   uint8_t truth_table = (data & (0xf << 5)) >> 5;

   uint32_t new_value = 0;
   for(int i = 0; i < 32; i++) {
      new_value |= (two_bit_predicate(truth_table,
                                      (*(mach->regs.register_index[first_register]) & (1 << i)) >> i,
                                      (*(mach->regs.register_index[second_register]) & (1 << i)) >> i
                       ) ? 1:0) << i;
   }
   *(mach->regs.register_index[first_register]) = new_value;
}
void machine_bitwise_shift(struct machine* mach, uint16_t data) {
   bool direction = data & 1;
   bool immediate = (data & (1 << 1)) >> 1;
   uint8_t first_register = (data & (0b11111 << 2)) >> 2;
   uint8_t second_register = (data & (0b11111 << 7)) >> 7;

   uint32_t shift_amount = immediate ? mach->mem[mach->regs.rp++] : mach->regs.r[second_register];
   if(direction) {
      *(mach->regs.register_index[first_register]) >>= shift_amount;
   } else {
      *(mach->regs.register_index[first_register]) <<= shift_amount;
   }
}

iptr lookup_instruction(uint8_t code) {
   switch(code) {
      case 0: return machine_add; break;
      case 1: return machine_move; break;
      case 2: return machine_bitwise_operation; break;
      case 3: return machine_bitwise_shift; break;
   }
}

struct instruction decode_instruction(uint32_t word) {
   struct instruction inst;
   inst.high_level = word & 1; // Currently ignoring this flag.
   inst.instruction_width = (word & (0b1111 << 1)) >> 1;
   inst.condition_register_a = (word & (0b11111 << 5)) >> 5;
   inst.condition_register_b = (word & (0b11111 << 10)) >> 10;
   uint8_t condition_code = (word & (0b111 << 15)) >> 15;
   uint8_t instruction_code = (word & (0b11 << 18)) >> 18;
   inst.instruction_specific_data = (word & (0xfff << 20)) >> 20;
	
   inst.condition_operation = lookup_condition(condition_code);
   inst.fptr = lookup_instruction(instruction_code);
   return inst;
}

struct registers make_registers() {
   struct registers regs;
   regs.rp = 0;
   for(int i = 0; i < 31; i++) {
      regs.register_index[i] = &(regs.r[i]);
   }
   regs.register_index[31] = &(regs.rp);
   return regs;
}

struct machine make_machine(size_t word_count) {
   struct machine mach;
   mach.initialized = false;
   if(word_count > (1 << 31)) {
      return mach; // Return uninitialized.
   }
   mach.mem = calloc(word_count, sizeof(uint32_t));
   if(mach.mem == NULL) {
      return mach; // Return uninitialized.
   }
   mach.mem_size = word_count;
   mach.regs = make_registers();

   // After all initialization.
   mach.initialized = true;
   return mach;
}

void machine_exec_single(struct machine* mach) {
   uint32_t current_instruction = mach->mem[mach->regs.rp];
   // Set instruction pointer to next address.
   mach->regs.rp++;
   struct instruction inst = decode_instruction(current_instruction);
   if(check_condition(mach, inst)) {
      inst.fptr(mach, inst.instruction_specific_data);
   } else {
      mach->regs.rp++;
   }
}

// Let's hope it terminates, lol.
void machine_exec_all(struct machine* mach) {
   if(!mach->initialized) return;
   while(mach->regs.rp < mach->mem_size) {
      machine_exec_single(mach);
   }
}

void machine_exec_handled(struct machine* mach, machine_hook hook) {
   if(!mach->initialized) return;
   while(mach->regs.rp < mach->mem_size) {
      hook(mach);
      machine_exec_single(mach);
   }
}
