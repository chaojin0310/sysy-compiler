#include <string>
#include <memory>
#include <map>
#include "ast.h"
using namespace std;

// The minimum valid id of temporary variables of the KoopaIR program
int min_temp_id = 0;
// The operand that will be used during current dump
string op_num("");
// The function parameters when calling a function
vector<string> params;
// Symbol Table
ProgSymTab prog_symtab;
// To indicate what current instruction is
enum INSTR_TYPE curr_instr = INSTR_TYPE::NONE;
// Is there an return instr (ret) in current koopaIR block?
bool ret_in_blk = false;
// Is there an jump instr (jump, br) in current koopaIR block?
bool jump_in_blk = false;
// The minimum valid id to help to distinguish identifiers with the same name
int var_id = 0;
// The minimum valid block id to build branches
int block_id = 0;
// while info vector
vector<WhileInfo> vec_while;
// To indicate a gloabl var or a Local var
enum Domain curr_domain = Domain::Global;
// To indicate a call instruction
bool is_call = false;

// check if there's an empty block at the end
void check_empty_block(string &s, int func_type) {
    if (!ret_in_blk && !jump_in_blk) {
    // if (s[s.size() - 2] == ':') {  // an empty block
        if (func_type == 1) {  // int
            s = s + "ret 0\n";
        } else if (func_type == 0) {  // void
            s = s + "ret\n";
        }
    }
}

void import_sysy_lib(string &s) {
    s += "decl @getint(): i32\n";
    s += "decl @getch(): i32\n";
    s += "decl @getarray(*i32): i32\n";
    s += "decl @putint(i32)\n";
    s += "decl @putch(i32)\n";
    s += "decl @putarray(i32, *i32)\n";
    s += "decl @starttime()\n";
    s += "decl @stoptime()\n\n";

    {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = 1;
        (*(prog_symtab.global_symtab))["getint"] = symb;
    }
    {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = 1;
        (*(prog_symtab.global_symtab))["getch"] = symb;
    }
    {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = 1;
        (*(prog_symtab.global_symtab))["getarray"] = symb;
    }
    {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = 0;
        (*(prog_symtab.global_symtab))["putint"] = symb;
    }
    {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = 0;
        (*(prog_symtab.global_symtab))["putch"] = symb;
    }
    {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = 0;
        (*(prog_symtab.global_symtab))["putarray"] = symb;
    }
    {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = 0;
        (*(prog_symtab.global_symtab))["starttime"] = symb;
    }
    {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = 0;
        (*(prog_symtab.global_symtab))["stoptime"] = symb;
    }
}