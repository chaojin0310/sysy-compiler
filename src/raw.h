#ifndef RAW_H
#define RAW_H

#include <cassert>
#include <cstring>
#include <string>
#include <map>
#include "koopa.h"
using namespace std;


#define NUM_REGS 3
extern string temp_regs[17];     // mapping of reg_id and reg_name
extern int reg_use[15];    // use of regs
extern const int x0_id;
extern string koopa_ir;

/* 
 * Functions to traverse the raw program and 
 * generate proper riscv instructions stored in string s. 
 */
int get_total_var(const string &s, string::size_type start_pos);
void traverse(const koopa_raw_program_t &program, string &s);
void traverse(const koopa_raw_slice_t &slice, string &s);
void traverse(const koopa_raw_function_t &func, string &s);
void traverse(const koopa_raw_basic_block_t &bb, string &s);
void traverse(const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_return_t &ret, string &s);
void traverse(const koopa_raw_integer_t &i, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_binary_t &b, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_load_t &lw, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_store_t & sw, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_branch_t & br, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_jump_t & j, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_global_alloc_t & glb, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_call_t & call, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_get_elem_ptr_t &get_ep, const koopa_raw_value_t &value, string &s);
void traverse(const koopa_raw_get_ptr_t &get_p, const koopa_raw_value_t &value, string &s);
int find_next_reg();
void use_reg(int reg_id);
void free_reg(int reg_id);
void koopa2riscv(const char *str, string &s);
void visit_stack(int dst_reg, int dst_offset, int mode, string &instr);
void visit_heap(int dst_reg, const koopa_raw_value_t &value, int mode, string &s);

#endif