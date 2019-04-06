#ifndef BREADMISC_H
#define BREADMISC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct registers {
	uint32_t* register_index[32]; // Convenience.
	uint32_t r[31]; // General Purpose Registers
	uint32_t rp; // Instruction Pointer
};

struct machine {
	struct registers regs;
	uint32_t* mem; // Max 16 GiB
	size_t mem_size;
	bool initialized; // If this is false, the struct is unusable.
};

// 2^(5 + (2^4 - 1))
// 2^(5 + 15)
// 2^(20)
// Word size will always be expressible by a uint32_t

typedef bool (*predicate)(uint32_t, uint32_t);
typedef void (*iptr)(struct machine*, uint16_t);

struct instruction {
	bool high_level; // 1 if for >16 bit processor, 0 otherwise.
	uint32_t instruction_width; // Bit length of the instruction.
	uint8_t condition_register_a; // First register for condition check
	uint8_t condition_register_b; // Second register for condition check
	predicate condition_operation;
	iptr fptr;
	uint16_t instruction_specific_data;
};

// Specified Instructions
void machine_add(struct machine*, uint16_t);
void machine_move(struct machine*, uint16_t);
void machine_bitwise_operation(struct machine*, uint16_t);
void machine_bitwise_shift(struct machine*, uint16_t);

// Machine handling utilities
struct instruction decode_instruction(uint32_t);
void machine_exec_instruction(struct machine*);
void machine_exec_all(struct machine*);
void machine_debug(struct machine*);

#endif
