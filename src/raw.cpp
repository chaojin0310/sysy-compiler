#include <iostream>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "koopa.h"
#include "raw.h"
using namespace std;

using veci = vector<int>;

// mapping of reg_id and reg_name
string temp_regs[17] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
                        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
                        "x0", "ra"};
// use condition of regs
int reg_use[15];
// called saved regs offset to sp
int reg_offset[NUM_REGS];
// ra save
bool ra_save = false;
// ra offset
int ra_offset;
// id of x0
const int x0_id = 15;
// id of ra
const int ra_id = 16;
// Total bytes allocated for one stack frame
unsigned int num_bytes = 0;
// The minimum valid label id
int min_label_id = 0;
// The id of the block containing ret instruction
int end_label_id = 0;
// current function
koopa_raw_function_t curr_func;

struct myCompare {
    bool operator()(const koopa_raw_value_t &v1, const koopa_raw_value_t &v2) const {
        return reinterpret_cast<long long int>(v1) < reinterpret_cast<long long int>(v2);
    }
};
// mapping of instruction and where (reg_id) it stored its result
map<koopa_raw_value_t, int, myCompare> reg_map;
// mapping of instruction and where (offset to sp) is stored its result
map<koopa_raw_value_t, int, myCompare> offset_map;

struct myBlockCompare {
    bool operator()(const koopa_raw_basic_block_t &b1, 
                    const koopa_raw_basic_block_t &b2) const {
        return reinterpret_cast<long long int>(b1) < reinterpret_cast<long long int>(b2);
    }
};
map<koopa_raw_basic_block_t, int, myBlockCompare> label_map;


/* Initialization */
void init() {
    memset(reg_use, 0, sizeof(reg_use));
    memset(reg_offset, 0, sizeof(reg_offset));
    ra_save = false;
} 

/* Find next available reg */
int find_next_reg() {
    int j = 0;
    for (int s = 0; s < NUM_REGS; ++s) {
        if (!reg_use[j])
            break;
        j = (j + 1) % NUM_REGS;
    }
    return j;
}

void use_reg(int reg_id) {
    reg_use[reg_id] = 1;
}

void free_reg(int reg_id) {
    reg_use[reg_id] = 0;
}

/* Allocate vars on the stack in a block */
void alloc_block_local_var(const koopa_raw_basic_block_t &block) {
    for (size_t i = 0; i < block->insts.len; ++i) {
        auto value = reinterpret_cast<koopa_raw_value_t>(block->insts.buffer[i]);
        const auto &kind = value->kind;
        if (kind.tag == KOOPA_RVT_ALLOC) {
            auto ptr = value->ty->data.pointer.base;
            if (ptr->tag == KOOPA_RTT_ARRAY) {  // array
                offset_map[value] = num_bytes;
                int total_len = 4;
                while (ptr->data.array.base && ptr->tag == KOOPA_RTT_ARRAY) {
                    int dim = ptr->data.array.len;
                    if (dim == 0)
                        break;
                    total_len *= dim;
                    ptr = ptr->data.array.base;
                }
                num_bytes += total_len;
            } else {  // pointer or int
                offset_map[value] = num_bytes;
                num_bytes += 4;
            }
        } else if (value->ty->tag != KOOPA_RTT_UNIT) {
            offset_map[value] = num_bytes;
            num_bytes += 4;
        }
    }
}

/* Allocate params space for function call */
bool alloc_params(const koopa_raw_function_t &func) {
    unsigned int param_bytes = 0;
    bool func_call = false;
    for (size_t i = 0; i < func->bbs.len; ++i) {
        auto block = func->bbs.buffer[i];
        koopa_raw_basic_block_t blk = reinterpret_cast<koopa_raw_basic_block_t>(block);
        for (size_t j = 0; j < blk->insts.len; ++j) {
            auto value = reinterpret_cast<koopa_raw_value_t>(blk->insts.buffer[j]);
            const auto &kind = value->kind;
            if (kind.tag == KOOPA_RVT_CALL) {
                func_call = true;
                unsigned int num_params = kind.data.call.args.len;
                if (num_params > 8)
                    num_params -= 8;
                else
                    num_params = 0;
                if (param_bytes < num_params)
                    param_bytes = num_params;
            }
        }
    }
    num_bytes += (param_bytes * 4);
    return func_call;
}

/* Compute total space which should be allocated on the stack and build offset_map */
void alloc_func(const koopa_raw_function_t &func) {
    num_bytes = 0;
    unsigned int ra_bytes = 0;
    // Allocate params
    ra_save = alloc_params(func);
    if (ra_save) {
        ra_bytes = 4;
    }
    // Allocate caller saved reg space, make sure that reg offset < 2048
    if (ra_save) {
        for (int i = 0; i < NUM_REGS; ++i) {
            reg_offset[i] = num_bytes;
            num_bytes += 4;
        }
    }
    // Allocate local vars
    for (size_t i = 0; i < func->bbs.len; ++i) {
        auto block = func->bbs.buffer[i];
        alloc_block_local_var(reinterpret_cast<koopa_raw_basic_block_t>(block)); 
    }
    // Allocate space for ra
    if (ra_save) {
        ra_offset = num_bytes;
        num_bytes += ra_bytes;
    }
    // Round up to multiples of 16
    num_bytes = (((num_bytes - 1) >> 4) + 1) << 4; 
}

/* Allocate label ids for each block in a function */
void alloc_labels(const koopa_raw_function_t &func) {
    for (size_t i = 0; i < func->bbs.len; ++i) {
        auto block = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        int new_label_id = min_label_id;
        min_label_id++;
        label_map[block] = new_label_id;
    }
}

void koopa2riscv(const char *str, string &s) {
    // KoopaIR to raw program
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(str, &program);
    assert(ret == KOOPA_EC_SUCCESS);

    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);

    // Delete KoopaIR program
    koopa_delete_program(program);

    // Handle raw program
    traverse(raw, s);
    
    // Delete raw program
    koopa_delete_raw_program_builder(builder);
}

/* Traverse raw program */
void traverse(const koopa_raw_program_t &program, string &s) {
    // Traverse all global variables
    traverse(program.values, s);
    // Traverse all functions
    traverse(program.funcs, s);
}

/* Traverse raw slice */
void traverse(const koopa_raw_slice_t &slice, string &s) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                curr_func = reinterpret_cast<koopa_raw_function_t>(ptr);
                traverse(reinterpret_cast<koopa_raw_function_t>(ptr), s);
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                traverse(reinterpret_cast<koopa_raw_basic_block_t>(ptr), s);
                break;
            case KOOPA_RSIK_VALUE:
                traverse(reinterpret_cast<koopa_raw_value_t>(ptr), s);
                break;
            default:
                break;
        }
    }
}

/* Traverse functions */
void traverse(const koopa_raw_function_t &func, string &s) {
    if (func->bbs.len == 0)
        return;
    init();
    s += "  .text\n";
    s += "  .globl ";
    string func_name = string(func->name);
    func_name.erase(0, 1);
    s = s + func_name + "\n" + func_name + ":\n";
    // Prologue
    alloc_func(func);
    if (num_bytes > 0) {
        if (num_bytes <= 2048) { // Rember to substract, not to increase num_bytes
            s = s + "  addi sp, sp, -" + string(to_string(num_bytes)) + "\n";
        } else {
            s = s + "  li t0, -" + string(to_string(num_bytes)) + "\n";
            s = s + "  add sp, sp, t0\n";
        }
    }
    if (ra_save) {
        // s = s + "  sw ra, " + string(to_string(ra_offset)) + "(sp)\n\n";
        visit_stack(ra_id, ra_offset, 1, s);
    }

    alloc_labels(func);
    traverse(func->bbs, s);

    // Epilogue
    s = s + "end" + string(to_string(end_label_id)) + ":\n";
    end_label_id++;
    if (ra_save) {
        // s = s + "  lw ra, " + string(to_string(ra_offset)) + "(sp)\n";
        visit_stack(ra_id, ra_offset, 0, s);
    }
    if (num_bytes > 0) {
        if (num_bytes < 2048) { // Add num_bytes back
            s = s + "  addi sp, sp, " + string(to_string(num_bytes)) + "\n";
        } else {
            s = s + "  li t0, " + string(to_string(num_bytes)) + "\n";
            s = s + "  add sp, sp, t0\n";
        }
    }
    s += "  ret\n\n";
}

/* Traverse basic blocks */
void traverse(const koopa_raw_basic_block_t &bb, string &s) {
    int blk_id = label_map[bb];
    koopa_raw_basic_block_t bb1 = 
        reinterpret_cast<koopa_raw_basic_block_t>(curr_func->bbs.buffer[0]);
    int blk1_id = label_map[bb1];
    if (blk_id != blk1_id)
        s = s + "label" + string(to_string(blk_id)) + ":\n";
    traverse(bb->insts, s);
}

/* Traverse values */
void traverse(const koopa_raw_value_t &value, string &s) {
    const auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            // return instruction
            traverse(kind.data.ret, s);
            break;
        case KOOPA_RVT_INTEGER:
            // integer
            traverse(kind.data.integer, value, s);
            break;
        case KOOPA_RVT_BINARY:
            // binary instruction
            traverse(kind.data.binary, value, s);
            break;
        case KOOPA_RVT_ALLOC:
            break;
        case KOOPA_RVT_LOAD:
            traverse(kind.data.load, value, s);
            break;
        case KOOPA_RVT_STORE:
            traverse(kind.data.store, value, s);
            break;
        case KOOPA_RVT_BRANCH:
            traverse(kind.data.branch, value, s);
            break;
        case KOOPA_RVT_JUMP:
            traverse(kind.data.jump, value, s);
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            traverse(kind.data.global_alloc, value, s);
            break;
        case KOOPA_RVT_CALL:
            traverse(kind.data.call, value, s);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
            traverse(kind.data.get_elem_ptr, value, s);
            break;
        case KOOPA_RVT_GET_PTR:
            traverse(kind.data.get_ptr, value, s);
            break;
        default:
            break;
    }
}

/* Traverse return */
void traverse(const koopa_raw_return_t &ret, string &s) {
    if (ret.value) {
        const auto &kind = ret.value->kind;
        int reg_id = 0;
        int src_offset = 0;
        switch (kind.tag) {
            case KOOPA_RVT_INTEGER:
                traverse(kind.data.integer, ret.value, s);
                reg_id = reg_map[ret.value];
                s = s + "  mv a0, " + temp_regs[reg_id] + "\n";
                reg_map.erase(ret.value);
                free_reg(reg_id);
                break;
            default:
                src_offset = offset_map[ret.value];
                visit_stack(7, src_offset, 0, s);
        }
    }
    s = s + "  j end" + string(to_string(end_label_id)) + "\n\n";
}

/* Traverse integer */
void traverse(const koopa_raw_integer_t &i, const koopa_raw_value_t &value, string &s) {
    string instr("");
    // if (i.value == 0) {
    //     reg_map[value] = x0_id;   // use x0 for convenience
    // } else {
    //     int reg_id = find_next_reg();
    //     instr = instr + "  li " + temp_regs[reg_id] + ", " + 
    //             string(to_string(i.value)) + "\n";
    //     use_reg(reg_id);
    //     reg_map[value] = reg_id;
    //     s += instr;
    // }
    int reg_id = find_next_reg();
    instr = instr + "  li " + temp_regs[reg_id] + ", " + 
            string(to_string(i.value)) + "\n";
    use_reg(reg_id);
    reg_map[value] = reg_id;
    s += instr;
}   

/* Traverse binary operation */
void traverse(const koopa_raw_binary_t &b, const koopa_raw_value_t &value, string &s) {
    string instr("");
    int lhs_id, rhs_id;              // two operator reg id
    int lhs_offset = 0, rhs_offset = 0;
    const auto &kind1 = b.lhs->kind;
    switch (kind1.tag) {
        case KOOPA_RVT_INTEGER:
            traverse(kind1.data.integer, b.lhs, s);
            lhs_id = reg_map[b.lhs];
            break;
        default:
            lhs_offset = offset_map[b.lhs];
            lhs_id = find_next_reg();
            use_reg(lhs_id);
            visit_stack(lhs_id, lhs_offset, 0, s);
            reg_map[b.lhs] = lhs_id;
    }
    const auto &kind2 = b.rhs->kind;
    switch (kind2.tag) {
        case KOOPA_RVT_INTEGER:
            traverse(kind2.data.integer, b.rhs, s);
            rhs_id = reg_map[b.rhs];
            break;
        default:
            rhs_offset = offset_map[b.rhs];
            rhs_id = find_next_reg();
            use_reg(rhs_id);
            visit_stack(rhs_id, rhs_offset, 0, s);
            reg_map[b.rhs] = rhs_id;
    }
    reg_map.erase(b.lhs);
    reg_map.erase(b.rhs);
    free_reg(lhs_id);
    free_reg(rhs_id);
    int reg_id = find_next_reg();    // dst reg_id of this binary op
    switch (b.op) {
        case KOOPA_RBO_NOT_EQ:
            instr = instr + "  xor " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            instr = instr + "  snez " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[reg_id] + "\n";
            break;
        case KOOPA_RBO_EQ:
            instr = instr + "  xor " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            instr = instr + "  seqz " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[reg_id] + "\n";
            break;
        case KOOPA_RBO_GT:
            instr = instr + "  sgt " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_LT:
            instr = instr + "  slt " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_GE:
            instr = instr + "  slt " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            instr = instr + "  seqz " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[reg_id] + "\n";
            break;
        case KOOPA_RBO_LE:
            instr = instr + "  sgt " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            instr = instr + "  seqz " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[reg_id] + "\n";
            break;
        case KOOPA_RBO_ADD:
            instr = instr + "  add " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_SUB:
            instr = instr + "  sub " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_MUL:
            instr = instr + "  mul " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_DIV:
            instr = instr + "  div " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_MOD:
            instr = instr + "  rem " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_AND:
            instr = instr + "  and " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_OR:
            instr = instr + "  or " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_XOR:
            instr = instr + "  xor " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_SHL:
            instr = instr + "  sll " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_SHR:
            instr = instr + "  srl " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        case KOOPA_RBO_SAR:
            instr = instr + "  sra " + temp_regs[reg_id] + ", ";
            instr = instr + temp_regs[lhs_id] + ", " + temp_regs[rhs_id] + "\n";
            break;
        default:
            break;        
    }
    use_reg(reg_id);
    int dst_offset = offset_map[value];
    visit_stack(reg_id, dst_offset, 1, instr);
    s += instr;
    free_reg(reg_id);
}

/* To get an address on the stack */
void get_stack_addr(int dst_reg, int dst_offset, string &instr) {
    if (dst_offset >= -2048 && dst_offset < 2048) {
        instr = instr + "  addi " + temp_regs[dst_reg] + ", sp, " + 
                string(to_string(dst_offset)) + "\n";
    } else {
        int med_reg = 3;  // t3
        instr = instr + "  li " + temp_regs[med_reg] + ", " + 
                string(to_string(dst_offset)) + "\n";
        instr = instr + "  add " + temp_regs[dst_reg] + ", " + 
                temp_regs[med_reg] + ", sp\n";
    }
}

/* To visit imm(sp) with load or store command */
void visit_stack(int dst_reg, int dst_offset, int mode, string &instr) {
    string cmd("");
    if (mode == 0) // mode == 0 represents LOAD
        cmd = "  lw ";
    else           // mode == 1 represents STORE
        cmd = "  sw ";
    if (dst_offset >= -2048 && dst_offset < 2048) {
        instr = instr + cmd + temp_regs[dst_reg] + ", " + 
                string(to_string(dst_offset)) + "(sp)\n";
    } else {
        int med_reg = 3;  // t3
        instr = instr + "  li " + temp_regs[med_reg] + ", " + 
                string(to_string(dst_offset)) + "\n";
        instr = instr + "  add " + temp_regs[med_reg] + ", " + 
                temp_regs[med_reg] + ", sp\n";
        instr = instr + cmd + temp_regs[dst_reg] + ", 0(" + 
                temp_regs[med_reg] + ")\n";
    }
}

/* To visit global variable with load or store command */
void visit_heap(int dst_reg, const koopa_raw_value_t &value, int mode, string &s) {
    string cmd("");
    if (mode == 0) 
        cmd = "  lw ";
    else 
        cmd = "  sw ";
    string var_name = string(value->name);
    var_name.erase(0, 1);
    int med_reg = 3;
    s = s + "  la " + temp_regs[med_reg] + ", " + var_name + "\n";
    s = s + cmd + temp_regs[dst_reg] + ", 0(" + temp_regs[med_reg] + ")\n";
}

/* Traverse load */
void traverse(const koopa_raw_load_t &lw, const koopa_raw_value_t &value, string &s) {
    int reg_id = find_next_reg();
    use_reg(reg_id);
    const auto &kind = lw.src->kind;
    int dst_offset = offset_map[value];
    int src_offset = 0;
    int reg_med;
    string var_name;
    switch (kind.tag) {
        case KOOPA_RVT_GLOBAL_ALLOC:
            var_name = string(lw.src->name);
            var_name.erase(0, 1);
            visit_heap(reg_id, lw.src, 0, s);
            visit_stack(reg_id, dst_offset, 1, s);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
        case KOOPA_RVT_GET_PTR:
            reg_med = find_next_reg();
            use_reg(reg_med);
            src_offset = offset_map[lw.src];
            visit_stack(reg_med, src_offset, 0, s);
            s = s + "  lw " + temp_regs[reg_id] + ", 0(" + temp_regs[reg_med] + ")\n";
            free_reg(reg_med);
            visit_stack(reg_id, dst_offset, 1, s);
            break;
        default:
            src_offset = offset_map[lw.src];
            visit_stack(reg_id, src_offset, 0, s);
            visit_stack(reg_id, dst_offset, 1, s);
    }
    free_reg(reg_id);
}

/* Traverse store */
void traverse(const koopa_raw_store_t & sw, const koopa_raw_value_t &value, string &s) {
    const auto &kind = sw.value->kind;
    int reg_id = 0;
    int src_offset = 0;
    int dst_offset = offset_map[sw.dest];
    map<koopa_raw_value_t, int, myCompare>::iterator it;
    size_t i = 0;
    bool found = false;
    switch (kind.tag) {
        case KOOPA_RVT_INTEGER:  // store 2, @x
            traverse(kind.data.integer, sw.value, s);
            reg_id = reg_map[sw.value];
            reg_map.erase(sw.value);
            break;
        default:  // store %1, @x
            it = offset_map.find(sw.value);
            if (it != offset_map.end()) {
                src_offset = offset_map[sw.value];
                reg_id = find_next_reg();
                use_reg(reg_id);
                visit_stack(reg_id, src_offset, 0, s);
            } else {  // store @x_0, @x_1, while @x_0 is a fparam
                for (; i < curr_func->params.len; ++i) {
                    auto ptr = curr_func->params.buffer[i];
                    koopa_raw_value_t val = reinterpret_cast<koopa_raw_value_t>(ptr);
                    if (sw.value == val) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    cerr << "No such value!\n";
                if (i < 8) {
                    reg_id = 7 + i;
                } else {
                    src_offset = (i - 8) * 4 + num_bytes;  // in caller's frame
                    reg_id = find_next_reg();
                    use_reg(reg_id);
                    visit_stack(reg_id, src_offset, 0, s);
                }
            }
    }
    const auto &dst_kind = sw.dest->kind;
    int ptr_offset = 0;
    int reg_med = 0;
    switch (dst_kind.tag) {
        case KOOPA_RVT_GLOBAL_ALLOC:  // store at a global var
            visit_heap(reg_id, sw.dest, 1, s);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
        case KOOPA_RVT_GET_PTR:
            ptr_offset = offset_map[sw.dest];
            reg_med = find_next_reg();
            use_reg(reg_med);
            visit_stack(reg_med, ptr_offset, 0, s);
            s = s + "  sw " + temp_regs[reg_id] + ", 0(" + temp_regs[reg_med] + ")\n";
            free_reg(reg_med);
            break;
        default:
            visit_stack(reg_id, dst_offset, 1, s);
    }
    free_reg(reg_id);
}

/* Traverse branch */
void traverse(const koopa_raw_branch_t & br, const koopa_raw_value_t &value, string &s) {
    int cond_id;              // condition reg id
    int cond_offset = 0;
    const auto &kind = br.cond->kind;
    switch (kind.tag) {
        case KOOPA_RVT_INTEGER:
            traverse(kind.data.integer, br.cond, s);
            cond_id = reg_map[br.cond];
            reg_map.erase(br.cond);
            free_reg(cond_id);
            break;
        default:
            cond_offset = offset_map[br.cond];
            cond_id = find_next_reg();
            use_reg(cond_id);
            visit_stack(cond_id, cond_offset, 0, s);
            free_reg(cond_id);
    }
    int then_blk_id = label_map[br.true_bb];
    int else_blk_id = label_map[br.false_bb];
    // use jr to exceed 2048 bytes' limitation
    int med_label_id = min_label_id;
    min_label_id++;
    s = s + "  bnez " + temp_regs[cond_id] + ", label" + 
        string(to_string(med_label_id)) + "\n";
    s = s + "  la t4, label" + string(to_string(else_blk_id)) + "\n";
    s = s + "  jr t4\n\n";
    s = s + "label" + string(to_string(med_label_id)) + ":\n";
    s = s + "  la t4, label" + string(to_string(then_blk_id)) + "\n";
    s = s + "  jr t4\n\n";
}

/* Traverse jump */
void traverse(const koopa_raw_jump_t & j, const koopa_raw_value_t &value, string &s) {
    int jump_blk_id = label_map[j.target];
    // use jr to exceed 2048 bytes' limitation
    s = s + "  la t4, label" + string(to_string(jump_blk_id)) + "\n";
    s = s + "  jr t4\n\n";
}

/* Get array aggregated init value */
veci get_init_val(const koopa_raw_aggregate_t &aggr) {
    veci res;
    res.clear();
    for (int i = 0; i < aggr.elems.len; ++i) {
        auto ptr = aggr.elems.buffer[i];
        koopa_raw_value_t val = reinterpret_cast<koopa_raw_value_t>(ptr);
        const auto kind = val->kind;
        switch (kind.tag) {
            case KOOPA_RVT_INTEGER:
                res.push_back(kind.data.integer.value);
                break;
            default:
                veci temp = get_init_val(kind.data.aggregate);
                res.insert(res.end(), temp.begin(), temp.end());
        }
    }
    return res;
}

/* Get zero init value */
void get_zero_init(const koopa_raw_value_t &value, string &s) {  // array zero init
    int total_len = 4;
    auto ptr = value->ty->data.pointer.base;
    if (ptr->tag == KOOPA_RTT_ARRAY) {  // array
        while (ptr->data.array.base && ptr->tag == KOOPA_RTT_ARRAY) {
            int dim = ptr->data.array.len;
            if (dim == 0)
                break;
            total_len *= dim;
            ptr = ptr->data.array.base;
        }
    }
    s = s + "  .zero " + string(to_string(total_len)) + "\n";
}

/* Traverse global alloc */
void traverse(const koopa_raw_global_alloc_t & glb, const koopa_raw_value_t &value, string &s) {
    string glbvar_name = string(value->name);
    glbvar_name.erase(0, 1);
    s = s + "  .data\n.globl " + glbvar_name + "\n";
    s = s + glbvar_name + ":\n";
    veci init_val;
    int i;
    const auto &kind = glb.init->kind;
    switch (kind.tag) {
        case KOOPA_RVT_INTEGER:
            s = s + "  .word " + string(to_string(int(kind.data.integer.value))) + "\n";
            break;
        case KOOPA_RVT_ZERO_INIT:  // array zero init
            // s = s + "  .zero 4\n";
            get_zero_init(value, s);
            break;
        case KOOPA_RVT_AGGREGATE:
            init_val = get_init_val(kind.data.aggregate);
            for (i = 0; i < init_val.size(); ++i) {
                s = s + "  .word " + string(to_string(init_val[i])) + "\n";
            }
            break;
        default:
            break;
    }
    s += "\n";
}

void put_params(const koopa_raw_slice_t &slice, string &s) {
    size_t len = slice.len < 8 ? slice.len : 8;
    int reg_id = 7;
    int med_id = 0;
    int src_offset = 0, dst_offset = 0;
    for (size_t i = 0; i < len; ++i) {
        auto ptr = slice.buffer[i];
        koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
        const auto kind = value->kind;
        switch (kind.tag) {
            case KOOPA_RVT_INTEGER:
                s = s + "  li " + temp_regs[reg_id] + ", " + 
                    string(to_string(kind.data.integer.value)) + "\n";
                break;
            default:
                src_offset = offset_map[value];
                visit_stack(reg_id, src_offset, 0, s);
        }
        reg_id++;
    }
    if (slice.len > 8) {
        for (size_t i = 8; i < slice.len; ++i) {
            auto ptr = slice.buffer[i];
            koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
            const auto kind = value->kind;
            switch (kind.tag) {
                case KOOPA_RVT_INTEGER:
                    traverse(kind.data.integer, value, s);
                    med_id = reg_map[value];
                    reg_map.erase(value);
                    break;
                default:
                    src_offset = offset_map[value];
                    med_id = find_next_reg();
                    use_reg(med_id);
                    visit_stack(med_id, src_offset, 0, s);
            }
            dst_offset = (i - 8) * 4;
            visit_stack(med_id, dst_offset, 1, s);
            free_reg(med_id);
        }
    }
    
}

void traverse(const koopa_raw_call_t & call, const koopa_raw_value_t &value, string &s) {
    // store reg value onto the stack
    for (int i = 0; i < NUM_REGS; ++i) {
        s = s + "  sw " + temp_regs[i] + ", " + 
            string(to_string(reg_offset[i])) + "(sp)\n";
    }
    // put params in regs and stack
    put_params(call.args, s);
    // call
    string callee_name = string(call.callee->name);
    callee_name.erase(0, 1);
    s = s + "  call " + callee_name + "\n";
    // store ret value
    if (value->ty->tag != KOOPA_RTT_UNIT) {
        int dst_offset = offset_map[value];
        visit_stack(7, dst_offset, 1, s);
    }
    // restore reg value
    for (int i = 0; i < NUM_REGS; ++i) {
        s = s + "  lw " + temp_regs[i] + ", " + 
            string(to_string(reg_offset[i])) + "(sp)\n";
    }
    s += "\n";
}

int cal_base(const koopa_raw_get_elem_ptr_t &get_ep) {
    int total_len = 4;
    int count = 0;
    auto ptr = get_ep.src->ty->data.pointer.base;
    while (ptr->data.array.base && ptr->tag == KOOPA_RTT_ARRAY) {
        int dim = ptr->data.array.len;
        if (dim == 0)
            break;
        if (count > 0)
            total_len *= dim;
        ptr = ptr->data.array.base;
        count++;
    }
    return total_len;
}

int cal_base(const koopa_raw_get_ptr_t &get_p) {
    int total_len = 4;
    auto ptr = get_p.src->ty->data.pointer.base;
    while (ptr->data.array.base && ptr->tag == KOOPA_RTT_ARRAY) {
        int dim = ptr->data.array.len;
        if (dim == 0)
            break;
        total_len *= dim;
        ptr = ptr->data.array.base;
    }
    return total_len;
}

void traverse(const koopa_raw_get_elem_ptr_t &get_ep, const koopa_raw_value_t &value, string &s) {
    auto src = get_ep.src;
    auto index = get_ep.index;
    const auto idx_kind = index->kind;
    int reg_idx = 0;
    int idx_offset = 0;
    switch (idx_kind.tag) {
        case KOOPA_RVT_INTEGER:
            traverse(idx_kind.data.integer, index, s);
            reg_idx = reg_map[index];
            reg_map.erase(index);
            break;
        default:
            idx_offset = offset_map[index];
            reg_idx = find_next_reg();
            use_reg(reg_idx);
            visit_stack(reg_idx, idx_offset, 0, s);
    }
    int reg_src = 0;
    reg_src = find_next_reg();
    use_reg(reg_src);
    int src_offset = 0;
    const auto &src_kind = src->kind;
    if (src_kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        string var_name = string(src->name);
        var_name.erase(0, 1);
        s = s + "  la " + temp_regs[reg_src] + ", " + var_name + "\n";
    } else if (src_kind.tag == KOOPA_RVT_ALLOC) {
        src_offset = offset_map[src];
        get_stack_addr(reg_src, src_offset, s);
    } else if (src_kind.tag == KOOPA_RVT_GET_ELEM_PTR || 
               src_kind.tag == KOOPA_RVT_GET_PTR || 
               src_kind.tag == KOOPA_RVT_LOAD) {
        src_offset = offset_map[src];
        visit_stack(reg_src, src_offset, 0, s);
    } else {
        cout << "no\n";
    }
    int reg_base = 0;
    reg_base = find_next_reg();
    int base_num = cal_base(get_ep);
    s = s + "  li " + temp_regs[reg_base] + ", " + string(to_string(base_num)) + "\n";
    s = s + "  mul " + temp_regs[reg_base] + ", " + temp_regs[reg_base] + ", " + 
        temp_regs[reg_idx] + "\n";
    s = s + "  add " + temp_regs[reg_src] + ", " + temp_regs[reg_src] + ", " + 
        temp_regs[reg_base] + "\n";
    free_reg(reg_idx);
    free_reg(reg_base);
    int dst_offset = offset_map[value];
    visit_stack(reg_src, dst_offset, 1, s);
    free_reg(reg_src);
}

void traverse(const koopa_raw_get_ptr_t &get_p, const koopa_raw_value_t &value, string &s) {
    auto src = get_p.src;
    auto index = get_p.index;
    const auto idx_kind = index->kind;
    int reg_idx = 0;
    int idx_offset = 0;
    switch (idx_kind.tag) {
        case KOOPA_RVT_INTEGER:
            traverse(idx_kind.data.integer, index, s);
            reg_idx = reg_map[index];
            reg_map.erase(index);
            break;
        default:
            idx_offset = offset_map[index];
            reg_idx = find_next_reg();
            use_reg(reg_idx);
            visit_stack(reg_idx, idx_offset, 0, s);
    }
    int reg_src = 0;
    reg_src = find_next_reg();
    use_reg(reg_src);
    int src_offset = 0;
    const auto &src_kind = src->kind;
    if (src_kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        string var_name = string(src->name);
        var_name.erase(0, 1);
        s = s + "  la " + temp_regs[reg_src] + ", " + var_name + "\n";
    } else if (src_kind.tag == KOOPA_RVT_ALLOC) {
        src_offset = offset_map[src];
        get_stack_addr(reg_src, src_offset, s);
    } else if (src_kind.tag == KOOPA_RVT_GET_ELEM_PTR || 
               src_kind.tag == KOOPA_RVT_GET_PTR || 
               src_kind.tag == KOOPA_RVT_LOAD) {
        src_offset = offset_map[src];
        visit_stack(reg_src, src_offset, 0, s);
    } else {
        cout << "no\n";
    }
    int reg_base = 0;
    reg_base = find_next_reg();
    int base_num = cal_base(get_p);
    s = s + "  li " + temp_regs[reg_base] + ", " + string(to_string(base_num)) + "\n";
    s = s + "  mul " + temp_regs[reg_base] + ", " + temp_regs[reg_base] + ", " + 
        temp_regs[reg_idx] + "\n";
    s = s + "  add " + temp_regs[reg_src] + ", " + temp_regs[reg_src] + ", " + 
        temp_regs[reg_base] + "\n";
    free_reg(reg_idx);
    free_reg(reg_base);
    int dst_offset = offset_map[value];
    visit_stack(reg_src, dst_offset, 1, s);
    free_reg(reg_src);
}