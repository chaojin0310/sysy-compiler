#ifndef AST_H
#define AST_H

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <map>
using namespace std;

// #define MAX_KOOPAIR_SIZE 65536

/*
CompUnit      ::= [CompUnit] (Decl | FuncDef);

Decl          ::= ConstDecl | VarDecl;
ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
BType         ::= "int";
ConstDef      ::= IDENT "=" ConstInitVal;
ConstInitVal  ::= ConstExp;
VarDecl       ::= BType VarDef {"," VarDef} ";";
VarDef        ::= IDENT | IDENT "=" InitVal;
InitVal       ::= Exp;

FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;
FuncType      ::= "void" | "int";
FuncFParams   ::= FuncFParam {"," FuncFParam};
FuncFParam    ::= BType IDENT;

Block         ::= "{" {BlockItem} "}";
BlockItem     ::= Decl | Stmt;
Stmt          ::= LVal "=" Exp ";"
                | [Exp] ";"
                | Block
                | "if" "(" Exp ")" Stmt ["else" Stmt]
                | "while" "(" Exp ")" Stmt
                | "break" ";"
                | "continue" ";"
                | "return" [Exp] ";";

Exp           ::= LOrExp;
LVal          ::= IDENT;
PrimaryExp    ::= "(" Exp ")" | LVal | Number;
Number        ::= INT_CONST;
UnaryExp      ::= PrimaryExp | IDENT "(" [FuncRParams] ")" | UnaryOp UnaryExp;
UnaryOp       ::= "+" | "-" | "!";
FuncRParams   ::= Exp {"," Exp};
MulExp        ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
AddExp        ::= MulExp | AddExp ("+" | "-") MulExp;
RelExp        ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
EqExp         ::= RelExp | EqExp ("==" | "!=") RelExp;
LAndExp       ::= EqExp | LAndExp "&&" EqExp;
LOrExp        ::= LAndExp | LOrExp "||" LAndExp;
ConstExp      ::= Exp;
*/

using veci=vector<int>;

// The most minimum valid id of temporary variables of the KoopaIR program
extern int min_temp_id;
// The operand that will be used during current dump
extern string op_num;
// The function parameters when calling a function
extern vector<string> params;
// The minimum valid id to help to distinguish identifiers with the same name
extern int var_id;
// The minimum valid block id to build branches
extern int block_id;
// Tagged Union to represent a symbol
enum class Tag { Constant, Variable, Function, Array, Pointer };
struct Symbol {
    Tag tag;
    union Value {
        int const_val;
        struct VarSymbol {
            bool init;
            int val;
            int aux_id;
        } var_sym;
        int func_type;
        struct ArrInfo {
            int arr_aux_id;
            int dim;
        } arr_info;
        struct PtrInfo {
            int ptr_aux_id;
            int dim;
        } ptr_info;
    } value;
};

// check if there's an empty block at the end
void check_empty_block(string &s, int func_type);

// add sysy library function
void import_sysy_lib(string &s);

// Linked list of function block's symbol table
class BlockSymTab {
public:
    unique_ptr<map<string, Symbol> > local_symtab;
    BlockSymTab *prev_block_symtab;
    BlockSymTab *next_block_symtab;

    BlockSymTab() {
        local_symtab = unique_ptr<map<string, Symbol> >(new map<string, Symbol>());
        prev_block_symtab = nullptr;
        next_block_symtab = nullptr;
    }
};

class FuncSymTab {
public:
    BlockSymTab *first_block_symtab;
    BlockSymTab *curr_block_symtab;

    FuncSymTab() {
        first_block_symtab = nullptr;
        curr_block_symtab = nullptr;
    }
    void insert_block_symtab() {  // append actually
        BlockSymTab *new_block_symtab = new BlockSymTab();
        if (!curr_block_symtab) {
            first_block_symtab = new_block_symtab;
            curr_block_symtab = new_block_symtab;
        } else {
            curr_block_symtab->next_block_symtab = new_block_symtab;
            (curr_block_symtab->next_block_symtab)->prev_block_symtab = curr_block_symtab;
            curr_block_symtab = curr_block_symtab->next_block_symtab;
        }
    }
    void delete_curr_block_symtab() {  // Only delete the tail block symtab
        if (!curr_block_symtab) {
            cerr << "error: no block symtab to delete!" << endl;
            return;
        }
        if (curr_block_symtab->prev_block_symtab == nullptr) {
            delete curr_block_symtab;
            curr_block_symtab = nullptr;
            first_block_symtab = nullptr;
        } else {
            curr_block_symtab = curr_block_symtab->prev_block_symtab;
            delete curr_block_symtab->next_block_symtab;
            curr_block_symtab->next_block_symtab = nullptr;
        }
    }
    // find the local symbol
    bool find_local_symbol(Symbol &symb, string &ident) {
        BlockSymTab *ptr = curr_block_symtab;
        if (!ptr) 
            return false;
        map<string, Symbol>::iterator it;
        while (ptr) {
            it = ptr->local_symtab->find(ident);
            if (it != ptr->local_symtab->end()) {
                symb = it->second;
                if (symb.tag == Tag::Variable) {
                    ident = ident + "_" + string(to_string(symb.value.var_sym.aux_id));
                } else if (symb.tag == Tag::Array) {
                    ident = ident + "_" + string(to_string(symb.value.arr_info.arr_aux_id));
                } else if (symb.tag == Tag::Pointer) {
                    ident = ident + "_" + string(to_string(symb.value.ptr_info.ptr_aux_id));
                }
                return true;
            }
            ptr = ptr->prev_block_symtab;
        }
        return false;
    }
};

// Linked list of program's symbol table
class ProgSymTab {
public:
    unique_ptr<map<string, Symbol> > global_symtab;
    FuncSymTab *curr_func_symtab;

    ProgSymTab() {
        global_symtab = unique_ptr<map<string, Symbol> >(new map<string, Symbol>());;
        curr_func_symtab = nullptr;
    }
    void create_func_symtab() {
        if (!curr_func_symtab) {
            curr_func_symtab = new FuncSymTab();
        } else {
            cerr << "error: func symtab not clean!" << endl;
        }
    }
    void delete_func_symtab() {
        if (!curr_func_symtab) {
            cerr << "error: null func symtab!" << endl;
        } else {
            // Assume that all block symtab are deleted
            delete curr_func_symtab;
            curr_func_symtab = nullptr;
        }
    }
    bool find_symbol(Symbol &symb, string &ident) {
        if (curr_func_symtab) {
            if (curr_func_symtab->find_local_symbol(symb, ident))
                return true;
        }
        map<string, Symbol>::iterator it;
        it = global_symtab->find(ident);
        if (it != global_symtab->end()) {
            symb = it->second;
            return true;
        }
        return false;
    }
    bool find_global_symbol(Symbol &symb, string &ident) {
        map<string, Symbol>::iterator it;
        it = global_symtab->find(ident);
        if (it != global_symtab->end()) {
            symb = it->second;
            return true;
        }
        return false;
    }
};

// Record information of a while loop
struct WhileInfo {
    int entry_blk_id;
    int end_blk_id;
};

// Symbol Table
extern ProgSymTab prog_symtab;
// To indicate what current instruction is
enum class INSTR_TYPE { NONE, LOAD, STORE };
extern enum INSTR_TYPE curr_instr;
// To indicate a call instruction
extern bool is_call;
// Is there an end instr (ret, jump or br) in current koopaIR block?
extern bool ret_in_blk;
// Is there an jump instr (jump, br) in current koopaIR block?
extern bool jump_in_blk;
// while info vector
extern vector<WhileInfo> vec_while;
// To indicate a gloabl var or a Local var
enum class Domain { Global, Local };
extern enum Domain curr_domain;

// The base class for all ASTs 
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void dump2str(string &s) const = 0;
    virtual void insert2symtab() const = 0;
    virtual int cal_val() const = 0;
    virtual void supdump(string &s) const = 0;  // Supplementary dump
    virtual unique_ptr<veci> aggr_init(veci dim) const = 0;  // aggregated init
};

class OriginCompUnitAST : public BaseAST {
public:
    unique_ptr<BaseAST> comp_unit;

    void dump2str(string &s) const override {
        comp_unit->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class CompUnitAST : public BaseAST {
public:
    unique_ptr<BaseAST> comp_unit;
    unique_ptr<BaseAST> comp_unit_ptr;

    void dump2str(string &s) const override {
        curr_domain = Domain::Global;
        if (comp_unit)
            comp_unit->dump2str(s);
        curr_domain = Domain::Global;
        comp_unit_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class DeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> decl_ptr;

    void dump2str(string &s) const override {
        decl_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ConstDeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> btype;
    unique_ptr<BaseAST> const_def;
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_const_def;

    void dump2str(string &s) const override {
        const_def->dump2str(s);
        if (!vec_const_def)
            return;
        for (int i = 0; i < vec_const_def->size(); ++i) {
            (*vec_const_def)[i]->dump2str(s);
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ConstDefAST : public BaseAST {
public:
    string ident; 
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_const_exp;
    unique_ptr<BaseAST> const_init_val;

    void dump2str(string &s) const override {
        if (!vec_const_exp) {  // const
            insert2symtab();
        } else {  // const array
            insert2symtab();
            // compute dim
            veci dim;
            for (int i = 0; i < vec_const_exp->size(); ++i) {
                int l = (*vec_const_exp)[i]->cal_val();
                dim.push_back(l);
            }
            // alloc
            string instr("");
            string name = ident;
            struct Symbol symb;
            if (curr_domain == Domain::Local) {
                if (prog_symtab.curr_func_symtab->find_local_symbol(symb, name)) {
                    instr = instr + "@" + name + " = alloc ";
                }
            } else {
                if (prog_symtab.find_global_symbol(symb, name)) {
                    instr = instr + "global @" + name + " = alloc ";
                }
            }
            string len("");
            for (int i = 0; i < dim.size(); ++i) {
                if (i > 0) {
                    len = "[" + len + ", " + string(to_string(dim[i])) + "]";
                } else {
                    len = "[i32, " + string(to_string(dim[i])) + "]";
                }
            }
            instr += len;
            s += instr;
            // init
            unique_ptr<veci> init_val = const_init_val->aggr_init(dim);
            if (curr_domain == Domain::Local) {
                // local stored init
                s += "\n";
                vector<veci> vpos;
                int total_num = 1;
                for (int j = 0; j < dim.size(); ++j)
                    total_num *= dim[j];
                int repeat_num = 1;
                for (int j = 0; j < dim.size(); ++j) {
                    veci vp;
                    vp.clear();
                    int out_repeat = total_num / (dim[j] * repeat_num);
                    for (int t = 0; t < out_repeat; ++t) {
                        for (int k = 0; k < dim[j]; ++k) {
                            for (int p = 0; p < repeat_num; ++p) {
                                vp.push_back(k);
                            }
                        }
                    }
                    repeat_num *= dim[j];
                    vpos.push_back(vp);
                }
                for (int j = 0; j < total_num; ++j) {
                    veci pos;  // one pos
                    for (int k = dim.size() - 1; k >= 0; --k) {
                        pos.push_back(vpos[k][j]);
                    }
                    struct Symbol symb;
                    string name = ident;
                    if (!(prog_symtab.curr_func_symtab->find_local_symbol(symb, name)))
                        cerr << "cannot find arr symbol\n";
                    for (int k = 0; k < pos.size(); ++k) {
                        string new_op_num = "%" + string(to_string(min_temp_id));
                        min_temp_id++;
                        string src_ptr("");
                        if (k == 0)
                            src_ptr = "@" + name;
                        else
                            src_ptr = op_num;
                        s = s + new_op_num + " = getelemptr " + src_ptr + ", " + 
                            string(to_string(pos[k])) + "\n";
                        op_num = new_op_num;
                    }
                    s = s + "store " + string(to_string((*init_val)[j])) + ", " + 
                        op_num + "\n";
                }
            } else {
                // global aggregated init
                s += ", ";
                vector<string> sv;
                for (int j = 0; j < init_val->size(); ++j) {
                    sv.push_back(string(to_string((*init_val)[j])));
                }
                vector<string> tsv; // temp string vector
                for (int j = 0; j < dim.size(); ++j) {
                    tsv.clear();
                    int k = 0;
                    while (k < sv.size()) {
                        string temp_s("");
                        temp_s += "{";
                        for (int p = k; p < k + dim[j]; ++p) {
                            temp_s += sv[p];
                            if (p < k + dim[j] - 1)
                                temp_s += ", ";
                        }
                        temp_s += "}";
                        k += dim[j];
                        tsv.push_back(temp_s);
                    }
                    sv.clear();
                    for (int t = 0; t < tsv.size(); ++t) {
                        sv.push_back(tsv[t]);
                    }
                }
                s += sv[0] + "\n";
            }
        }
    }
    void insert2symtab() const override {
        if (!vec_const_exp) {
            struct Symbol symb;
            symb.tag = Tag::Constant;
            symb.value.const_val = const_init_val->cal_val();
            if (curr_domain == Domain::Local) {
                (*(prog_symtab.curr_func_symtab->curr_block_symtab->local_symtab))[ident] = symb;
            } else {
                (*(prog_symtab.global_symtab))[ident] = symb;
            }
        } else {
            struct Symbol symb;
            symb.tag = Tag::Array;
            symb.value.arr_info.arr_aux_id = var_id;
            var_id++;
            symb.value.arr_info.dim = vec_const_exp->size();
            if (curr_domain == Domain::Local) {
                (*(prog_symtab.curr_func_symtab->curr_block_symtab->local_symtab))[ident] = symb;
            } else {
                (*(prog_symtab.global_symtab))[ident] = symb;
            }
        }
    }
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ConstInitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> const_exp;
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_const_init_val;

    void dump2str(string &s) const override {}
    void insert2symtab() const override {}
    int cal_val() const override { 
        if (const_exp && !vec_const_init_val) {
            return const_exp->cal_val(); 
        }
        return 0;
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { 
        unique_ptr<veci> res = make_unique<veci>();
        int num_ele = 0;
        if (vec_const_init_val) {
            for (int i = 0; i < vec_const_init_val->size(); ++i) {
                ConstInitValAST *const_init = 
                    reinterpret_cast<ConstInitValAST *>((*vec_const_init_val)[i].get());
                if (const_init->const_exp) {
                    int val = const_init->cal_val();
                    res->push_back(val);
                    num_ele++;
                    continue;
                }
                veci new_dim;
                new_dim.clear();
                int m = 1, j = 0;
                if (num_ele == 0) {
                    for (int k = 0; k < dim.size() - 1; ++k) {
                        new_dim.push_back(dim[k]);
                    }
                } else {
                    while (j < dim.size()) {
                        if (num_ele % m != 0)
                            break;
                        m *= dim[j];
                        j++;
                    }
                    for (int k = 0; k < j - 1; ++k)
                        new_dim.push_back(dim[k]);
                }
                if (new_dim.empty()) {  // corner case: [2][3][4] = {1, 2, {3}, ...}
                    new_dim.push_back(1);
                }
                unique_ptr<veci> temp = (*vec_const_init_val)[i]->aggr_init(new_dim);
                res->insert(res->end(), temp->begin(), temp->end());
                num_ele = res->size();
            }
        }
        int num_total = 1;
        for (int i = 0; i < dim.size(); ++i)
            num_total *= dim[i];
        if (num_ele < num_total) {
            for (int k = num_ele; k < num_total; ++k)
                res->push_back(0);
        }
        return res;
    }
};

class VarDeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> btype;
    unique_ptr<BaseAST> var_def;
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_var_def;

    void dump2str(string &s) const override {
        var_def->dump2str(s);
        if (!vec_var_def)
            return;
        for (int i = 0; i < vec_var_def->size(); ++i) {
            (*vec_var_def)[i]->dump2str(s);
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class VarDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> var_def_ptr;

    void dump2str(string &s) const override {
        var_def_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class VarUnInitAST : public BaseAST {
public:
    string ident;

    void dump2str(string &s) const override {
        insert2symtab();
        string instr("");
        string name = ident;
        struct Symbol symb;
        if (curr_domain == Domain::Local) {
            if (prog_symtab.curr_func_symtab->find_local_symbol(symb, name)) {
                instr = instr + "@" + name + " = alloc i32\n";
                s += instr;
            }
        } else {
            if (prog_symtab.find_global_symbol(symb, name)) {
                instr = instr + "global @" + name + " = alloc i32, ";
                int val = symb.value.var_sym.val;
                instr = instr + string(to_string(val)) + "\n";
                instr += "\n";
                s += instr;
            }
        }
    }
    void insert2symtab() const override {
        struct Symbol symb;
        symb.tag = Tag::Variable;
        symb.value.var_sym.init = false;
        symb.value.var_sym.val = 0;
        symb.value.var_sym.aux_id = var_id;
        var_id++;
        if (curr_domain == Domain::Local) {
            (*(prog_symtab.curr_func_symtab->curr_block_symtab->local_symtab))[ident] = symb;
        } else {
            (*(prog_symtab.global_symtab))[ident] = symb;
        }
    }
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class VarInitAST : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> init_val;

    void dump2str(string &s) const override {
        insert2symtab();
        string name = ident;
        struct Symbol symb;
        if (curr_domain == Domain::Local) {
            if (prog_symtab.curr_func_symtab->find_local_symbol(symb, name)) {
                s = s + "@" + name + " = alloc i32\n";
                curr_instr = INSTR_TYPE::LOAD;
                init_val->dump2str(s);
                curr_instr = INSTR_TYPE::STORE;
                s = s + "store " + op_num + ", @" + name + "\n";
                curr_instr = INSTR_TYPE::NONE;
            }
        } else {
            if (prog_symtab.find_global_symbol(symb, name)) {
                s = s + "global @" + name + " = alloc i32, ";
                int val = symb.value.var_sym.val;
                s = s + string(to_string(val)) + "\n";
                s += "\n";
            }
        }
    }
    void insert2symtab() const override {
        struct Symbol symb;
        symb.tag = Tag::Variable;
        symb.value.var_sym.init = true;
        symb.value.var_sym.aux_id = var_id;
        var_id++; 
        // note that it is a run-time value actually
        if (curr_domain == Domain::Local) {
            symb.value.var_sym.val = 0;
            (*(prog_symtab.curr_func_symtab->curr_block_symtab->local_symtab))[ident] = symb;
        } else {
            symb.value.var_sym.val = init_val->cal_val();
            (*(prog_symtab.global_symtab))[ident] = symb;
        }
    }
    int cal_val() const override { return 0; } 
    void supdump(string &s) const override {} 
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class VarArrayAST : public BaseAST {
public:
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_const_exp;
    unique_ptr<BaseAST> init_val;

    void dump2str(string &s) const override {
        // var array
        insert2symtab();
        // compute dim
        veci dim;
        for (int i = 0; i < vec_const_exp->size(); ++i) {
            int l = (*vec_const_exp)[i]->cal_val();
            dim.push_back(l);
        }
        // alloc
        string instr("");
        string name = ident;
        struct Symbol symb;
        if (curr_domain == Domain::Local) {
            if (prog_symtab.curr_func_symtab->find_local_symbol(symb, name)) {
                instr = instr + "@" + name + " = alloc ";
            }
        } else {
            if (prog_symtab.find_global_symbol(symb, name)) {
                instr = instr + "global @" + name + " = alloc ";
            }
        }
        string len("");
        for (int i = 0; i < dim.size(); ++i) {
            if (i > 0) {
                len = "[" + len + ", " + string(to_string(dim[i])) + "]";
            } else {
                len = "[i32, " + string(to_string(dim[i])) + "]";
            }
        }
        instr += len;
        s += instr;
        // init
        if (init_val) {
            unique_ptr<veci> init_v = init_val->aggr_init(dim);
            if (curr_domain == Domain::Local) {
                // local stored init
                s += "\n";
                vector<veci> vpos;
                int total_num = 1;
                for (int j = 0; j < dim.size(); ++j)
                    total_num *= dim[j];
                int repeat_num = 1;
                for (int j = 0; j < dim.size(); ++j) {
                    veci vp;
                    vp.clear();
                    int out_repeat = total_num / (dim[j] * repeat_num);
                    for (int t = 0; t < out_repeat; ++t) {
                        for (int k = 0; k < dim[j]; ++k) {
                            for (int p = 0; p < repeat_num; ++p) {
                                vp.push_back(k);
                            }
                        }
                    }
                    repeat_num *= dim[j];
                    vpos.push_back(vp);
                }
                for (int j = 0; j < total_num; ++j) {
                    veci pos;  // one pos
                    for (int k = dim.size() - 1; k >= 0; --k) {
                        pos.push_back(vpos[k][j]);
                    }
                    struct Symbol symb;
                    string name = ident;
                    if (!(prog_symtab.curr_func_symtab->find_local_symbol(symb, name)))
                        cerr << "cannot find arr symbol\n";
                    for (int k = 0; k < pos.size(); ++k) {
                        string new_op_num = "%" + string(to_string(min_temp_id));
                        min_temp_id++;
                        string src_ptr("");
                        if (k == 0)
                            src_ptr = "@" + name;
                        else
                            src_ptr = op_num;
                        s = s + new_op_num + " = getelemptr " + src_ptr + ", " + 
                            string(to_string(pos[k])) + "\n";
                        op_num = new_op_num;
                    }
                    s = s + "store " + string(to_string((*init_v)[j])) + ", " + 
                        op_num + "\n";
                }
            } else {
                // global aggregated init
                s += ", ";
                vector<string> sv;
                for (int j = 0; j < init_v->size(); ++j) {
                    sv.push_back(string(to_string((*init_v)[j])));
                }
                vector<string> tsv; // temp string vector
                for (int j = 0; j < dim.size(); ++j) {
                    tsv.clear();
                    int k = 0;
                    while (k < sv.size()) {
                        string temp_s("");
                        temp_s += "{";
                        for (int p = k; p < k + dim[j]; ++p) {
                            temp_s += sv[p];
                            if (p < k + dim[j] - 1)
                                temp_s += ", ";
                        }
                        temp_s += "}";
                        k += dim[j];
                        tsv.push_back(temp_s);
                    }
                    sv.clear();
                    for (int t = 0; t < tsv.size(); ++t) {
                        sv.push_back(tsv[t]);
                    }
                }
                s += sv[0] + "\n";
            }
        } else {
            if (curr_domain == Domain::Global) {
                // // global aggregated init
                // veci init_v;
                // int total_num = 1;
                // for (int i = 0; i < dim.size(); ++i) {
                //     total_num *= dim[i];
                // }
                // for (int i = 0; i < total_num; ++i) 
                //     init_v.push_back(0);
                // s += ", ";
                // vector<string> sv;
                // for (int j = 0; j < init_v.size(); ++j) {
                //     sv.push_back(string(to_string(init_v[j])));
                // }
                // vector<string> tsv; // temp string vector
                // for (int j = 0; j < dim.size(); ++j) {
                //     tsv.clear();
                //     int k = 0;
                //     while (k < sv.size()) {
                //         string temp_s("");
                //         temp_s += "{";
                //         for (int p = k; p < k + dim[j]; ++p) {
                //             temp_s += sv[p];
                //             if (p < k + dim[j] - 1)
                //                 temp_s += ", ";
                //         }
                //         temp_s += "}";
                //         k += dim[j];
                //         tsv.push_back(temp_s);
                //     }
                //     sv.clear();
                //     for (int t = 0; t < tsv.size(); ++t) {
                //         sv.push_back(tsv[t]);
                //     }
                // }
                // s += sv[0] + "\n";
                s += ", zeroinit\n";
            } else {
                s += "\n";
            }
        }
    }
    void insert2symtab() const override {
        struct Symbol symb;
        symb.tag = Tag::Array;
        symb.value.arr_info.arr_aux_id = var_id;
        var_id++;
        symb.value.arr_info.dim = vec_const_exp->size();
        if (curr_domain == Domain::Local) {
            (*(prog_symtab.curr_func_symtab->curr_block_symtab->local_symtab))[ident] = symb;
        } else {
            (*(prog_symtab.global_symtab))[ident] = symb;
        }
    }
    int cal_val() const override { return 0; } 
    void supdump(string &s) const override {} 
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class InitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void dump2str(string &s) const override {
        curr_instr = INSTR_TYPE::LOAD;
        exp->dump2str(s);
        curr_instr = INSTR_TYPE::NONE;
    }
    void insert2symtab() const override {}
    int cal_val() const override { 
        return exp->cal_val(); // note that it is a run-time value actually
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> func_fparams;
    unique_ptr<BaseAST> block;

    void dump2str(string & s) const override {
        insert2symtab();
        curr_domain = Domain::Local;
        ret_in_blk = false;
        jump_in_blk = false;
        prog_symtab.create_func_symtab();
        s += "fun @";
        s += ident;
        s += "(";
        if (func_fparams) {
            prog_symtab.curr_func_symtab->insert_block_symtab();
            func_fparams->dump2str(s);
        }
        s += ")";
        func_type->dump2str(s);
        s += "{\n";
        s += "%";
        s += "entry" + string(to_string(block_id)) + ":\n";
        block_id++;
        if (func_fparams) {
            func_fparams->supdump(s);
        }
        block->dump2str(s);
        check_empty_block(s, func_type->cal_val());  // check if there's an empty block at the end
        s += "}\n\n";
        if (func_fparams) {
            prog_symtab.curr_func_symtab->delete_curr_block_symtab();
        }
        prog_symtab.delete_func_symtab();
    }
    void insert2symtab() const override {
        struct Symbol symb;
        symb.tag = Tag::Function;
        symb.value.func_type = func_type->cal_val();
        (*(prog_symtab.global_symtab))[ident] = symb;
    }
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class FuncTypeAST : public BaseAST {
public:
    string func_type;

    void dump2str(string &s) const override {
        if (func_type == "int") {
            s += ": i32 ";
        } else {
            s += " ";
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { 
        if (func_type == "int")
            return 1;
        else 
            return 0;
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class FuncFParamsAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_fparam;
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_func_fparam;

    void dump2str(string &s) const override {
        func_fparam->dump2str(s);
        if (!vec_func_fparam)
            return;
        if (vec_func_fparam->size() > 0)
            s = s + ", ";
        for (int i = 0; i < vec_func_fparam->size(); ++i) {
            (*vec_func_fparam)[i]->dump2str(s);
            if (i < vec_func_fparam->size() - 1)
                s = s + ", ";
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {
        func_fparam->supdump(s);
        if (!vec_func_fparam)
            return;
        for (int i = 0; i < vec_func_fparam->size(); ++i) {
            (*vec_func_fparam)[i]->supdump(s);
        }
    }
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class FuncFParamAST : public BaseAST {
public:
    unique_ptr<BaseAST> btype;
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_const_exp;

    void dump2str(string &s) const override {
        insert2symtab();
        string name = ident;
        struct Symbol symb;
        if (prog_symtab.curr_func_symtab->find_local_symbol(symb, name)) {
            if (symb.tag == Tag::Variable) {
                s = s + "@" + name + ": i32";
            } else {  // ptr, arr param
                s = s + "@" + name + ": ";
                if (vec_const_exp->size() == 0) {  // arr[]
                    s = s + "*i32";
                } else {  // arr[][2]...
                    veci dim;
                    dim.clear();
                    for (int i = 0; i < vec_const_exp->size(); ++i) {
                        int l = (*vec_const_exp)[i]->cal_val();
                        dim.push_back(l);
                    }
                    string len("");
                    for (int i = 0; i < dim.size(); ++i) {
                        if (i > 0) {
                            len = "[" + len + ", " + string(to_string(dim[i])) + "]";
                        } else {
                            len = "[i32, " + string(to_string(dim[i])) + "]";
                        }
                    }
                    s = s + "*" + len;
                }
            }
        }
    }
    void insert2symtab() const override {
        struct Symbol symb;
        if (!vec_const_exp) {
            symb.tag = Tag::Variable;
            symb.value.var_sym.init = false;
            symb.value.var_sym.val = 0;
            symb.value.var_sym.aux_id = var_id;
            var_id++;
        } else {
            symb.tag = Tag::Pointer;
            symb.value.ptr_info.ptr_aux_id = var_id;
            var_id++;
            symb.value.ptr_info.dim = vec_const_exp->size();
        }
        (*(prog_symtab.curr_func_symtab->curr_block_symtab->local_symtab))[ident] = symb;
    }
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {
        string name = ident;
        struct Symbol symb;
        if (prog_symtab.curr_func_symtab->find_local_symbol(symb, name)) {
            insert2symtab();
            struct Symbol new_symb;
            string new_name = ident;
            if (prog_symtab.curr_func_symtab->find_local_symbol(new_symb, new_name)) {
                if (new_symb.tag == Tag::Variable) {
                    s = s + "@" + new_name + " = alloc i32\n";
                    curr_instr = INSTR_TYPE::STORE;
                    s = s + "store @" + name + ", @" + new_name + "\n";
                    curr_instr = INSTR_TYPE::NONE;
                } else {
                    s = s + "@" + new_name + " = alloc ";
                    if (vec_const_exp->size() == 0) {
                        s = s + "*i32\n";
                    } else {
                        veci dim;
                        dim.clear();
                        for (int i = 0; i < vec_const_exp->size(); ++i) {
                            int l = (*vec_const_exp)[i]->cal_val();
                            dim.push_back(l);
                        }
                        string len("");
                        for (int i = 0; i < dim.size(); ++i) {
                            if (i > 0) {
                                len = "[" + len + ", " + string(to_string(dim[i])) + "]";
                            } else {
                                len = "[i32, " + string(to_string(dim[i])) + "]";
                            }
                        }
                        s = s + "*" + len + "\n";
                    }
                    curr_instr = INSTR_TYPE::STORE;
                    s = s + "store @" + name + ", @" + new_name + "\n";
                    curr_instr = INSTR_TYPE::NONE;
                }
            }
        }
    }
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class BlockAST : public BaseAST {
public:
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_block_item;

    void dump2str(string &s) const override {
        if (!vec_block_item)
            return;
        prog_symtab.curr_func_symtab->insert_block_symtab();
        for (int i = 0; i < vec_block_item->size(); ++i) {
            (*vec_block_item)[i]->dump2str(s);
        }
        prog_symtab.curr_func_symtab->delete_curr_block_symtab();
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class BlockItemAST : public BaseAST {
public:
    unique_ptr<BaseAST> block_item_ptr;

    void dump2str(string &s) const override {
        if (ret_in_blk || jump_in_blk)
            return;
        block_item_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class StmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> stmt_ptr;

    void dump2str(string &s) const override {
        stmt_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class OpenStmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> open_stmt_ptr;

    void dump2str(string &s) const override {
        open_stmt_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class OpenIfStmtAST : public BaseAST {
public:  // if (Exp) Stmt
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmt;

    void dump2str(string &s) const override {
        int then_blk_id = block_id;
        int end_blk_id = block_id + 1;
        block_id += 2;
        curr_instr = INSTR_TYPE::LOAD;
        exp->dump2str(s);
        curr_instr = INSTR_TYPE::NONE;
        s = s + "br " + op_num + ", %block" + string(to_string(then_blk_id)) + 
            ", %block" + string(to_string(end_blk_id)) + "\n";
        
        s = s + "\n%block" + string(to_string(then_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
        stmt->dump2str(s);
        if (!ret_in_blk && !jump_in_blk)
            s = s + "jump %block" + string(to_string(end_blk_id)) + "\n";
        
        s = s + "\n%block" + string(to_string(end_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class OpenIfElseStmtAST : public BaseAST {
public:  // if (Exp) ClosedStmt else OpenStmt
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> closed_stmt;
    unique_ptr<BaseAST> open_stmt;

    void dump2str(string &s) const override {
        int then_blk_id = block_id, else_blk_id = block_id + 1;
        int end_blk_id = block_id + 2;
        block_id += 3;
        curr_instr = INSTR_TYPE::LOAD;
        exp->dump2str(s);
        curr_instr = INSTR_TYPE::NONE;
        s = s + "br " + op_num + ", %block" + string(to_string(then_blk_id)) + 
            ", %block" + string(to_string(else_blk_id)) + "\n";
        
        s = s + "\n%block" + string(to_string(then_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
        closed_stmt->dump2str(s);
        if (!ret_in_blk && !jump_in_blk)
            s = s + "jump %block" + string(to_string(end_blk_id)) + "\n";
        int ret_in_then = ret_in_blk;
        
        s = s + "\n%block" + string(to_string(else_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
        open_stmt->dump2str(s);
        if (!ret_in_blk && !jump_in_blk)
            s = s + "jump %block" + string(to_string(end_blk_id)) + "\n";
        int ret_in_else = ret_in_blk;

        if (ret_in_then && ret_in_else)
            return;
        s = s + "\n%block" + string(to_string(end_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ClosedStmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> closed_stmt_ptr;

    void dump2str(string &s) const override {
        closed_stmt_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ClosedIfElseStmtAST : public BaseAST {
public:  // if (Exp) ClosedStmt else OpenStmt
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> closed_stmt_if;
    unique_ptr<BaseAST> closed_stmt_else;

    void dump2str(string &s) const override {
        int then_blk_id = block_id, else_blk_id = block_id + 1;
        int end_blk_id = block_id + 2;
        block_id += 3;
        curr_instr = INSTR_TYPE::LOAD;
        exp->dump2str(s);
        curr_instr = INSTR_TYPE::NONE;
        s = s + "br " + op_num + ", %block" + string(to_string(then_blk_id)) + 
            ", %block" + string(to_string(else_blk_id)) + "\n";
        
        s = s + "\n%block" + string(to_string(then_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
        closed_stmt_if->dump2str(s);
        if (!ret_in_blk && !jump_in_blk)
            s = s + "jump %block" + string(to_string(end_blk_id)) + "\n";
        int ret_in_then = ret_in_blk;
        
        s = s + "\n%block" + string(to_string(else_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
        closed_stmt_else->dump2str(s);
        if (!ret_in_blk && !jump_in_blk)
            s = s + "jump %block" + string(to_string(end_blk_id)) + "\n";
        int ret_in_else = ret_in_blk;
        
        if (ret_in_then && ret_in_else)
            return;
        s = s + "\n%block" + string(to_string(end_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class WhileStmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmt;

    void dump2str(string &s) const override {
        int entry_blk_id = block_id, body_blk_id = block_id + 1;
        int end_blk_id = block_id + 2;
        block_id += 3;
        // Build and insert this while loop's info
        struct WhileInfo while_info;
        while_info.entry_blk_id = entry_blk_id;
        while_info.end_blk_id = end_blk_id;
        vec_while.push_back(while_info);

        s = s + "jump %block" + string(to_string(entry_blk_id)) + "\n";

        s = s + "\n%block" + string(to_string(entry_blk_id)) + ":\n";
        curr_instr = INSTR_TYPE::LOAD;
        exp->dump2str(s);
        curr_instr = INSTR_TYPE::NONE;
        s = s + "br " + op_num + ", %block" + string(to_string(body_blk_id)) + 
            ", %block" + string(to_string(end_blk_id)) + "\n";
        
        s = s + "\n%block" + string(to_string(body_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
        stmt->dump2str(s);
        if (!ret_in_blk && !jump_in_blk)
            s = s + "jump %block" + string(to_string(entry_blk_id)) + "\n";
        vec_while.pop_back();
        
        s = s + "\n%block" + string(to_string(end_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class SimpleStmtAST : public BaseAST {
public:  // non-if statement
    unique_ptr<BaseAST> simple_stmt_ptr;

    void dump2str(string &s) const override {
        simple_stmt_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class BreakStmtAST : public BaseAST {
public:
    string break_stmt;

    void dump2str(string &s) const override {
        if (vec_while.empty())
            return;
        struct WhileInfo while_info = vec_while[vec_while.size() - 1];
        int end_blk_id = while_info.end_blk_id;
        if (!ret_in_blk && !jump_in_blk) {
            s = s + "jump %block" + string(to_string(end_blk_id)) + "\n";
            jump_in_blk = true;
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ContinueStmtAST : public BaseAST {
public:
    string continue_stmt;

    void dump2str(string &s) const override {
        if (vec_while.empty())
            return;
        struct WhileInfo while_info = vec_while[vec_while.size() - 1];
        int entry_blk_id = while_info.entry_blk_id;
        if (!ret_in_blk && !jump_in_blk) {
            s = s + "jump %block" + string(to_string(entry_blk_id)) + "\n";
            jump_in_blk = true;
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class AssignStmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> lval;
    unique_ptr<BaseAST> exp;

    void dump2str(string &s) const override {
        curr_instr = INSTR_TYPE::LOAD;
        exp->dump2str(s);
        curr_instr = INSTR_TYPE::STORE;
        lval->dump2str(s);
        curr_instr = INSTR_TYPE::NONE;
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ExpStmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void dump2str(string &s) const override {
        if (exp) {
            curr_instr = INSTR_TYPE::LOAD;
            exp->dump2str(s);
            curr_instr = INSTR_TYPE::NONE;
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class RetStmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void dump2str(string &s) const override {
        ret_in_blk = true;
        if (exp) {
            curr_instr = INSTR_TYPE::LOAD;
            exp->dump2str(s);
            ret_in_blk = true; // Reclaim "ret", it may be changed by exp->dump2str(s)
            s += "ret ";
            s += op_num;
            s += "\n";
            curr_instr = INSTR_TYPE::NONE;
        } else {
            s += "ret\n";
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lor_exp;

    void dump2str(string &s) const override {
        lor_exp->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return lor_exp->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class LValAST : public BaseAST {
public:
    string ident;
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_exp;

    void dump2str(string &s) const override {
        struct Symbol symb;
        string name = ident;
        if (!prog_symtab.find_symbol(symb, name))
            cerr << "No such lval\n"; 
        if (symb.tag != Tag::Array && symb.tag != Tag::Pointer) {
            if (symb.tag == Tag::Constant) {
                op_num = string(to_string(symb.value.const_val));
            } else if (curr_instr == INSTR_TYPE::LOAD) {
                string load_op_num("");
                load_op_num = load_op_num + "%" + string(to_string(min_temp_id));
                min_temp_id++;
                op_num = load_op_num;
                s = s + load_op_num + " = load " + "@" + name + "\n";
            } else if (curr_instr == INSTR_TYPE::STORE) {
                string store_op_num = op_num;
                s = s + "store " + store_op_num + ", @" + name + "\n";
            }
        } else {
            if (symb.tag == Tag::Array) {
                if (curr_instr == INSTR_TYPE::LOAD) {
                    if (!is_call) {
                        if (vec_exp && vec_exp->size() > 0) {
                            for (int i = vec_exp->size() - 1; i >= 0; --i) {
                                string src_ptr("");
                                if (i == vec_exp->size() - 1)
                                    src_ptr = "@" + name;
                                else
                                    src_ptr = op_num;
                                ((*vec_exp)[i])->dump2str(s);
                                string offset_num = op_num;
                                string new_op_num = "%" + string(to_string(min_temp_id));
                                min_temp_id++;
                                s = s + new_op_num + " = getelemptr " + src_ptr + ", " + 
                                    offset_num + "\n";
                                op_num = new_op_num;
                            }
                        }
                        string load_op_num("");
                        load_op_num = "%" + string(to_string(min_temp_id));
                        min_temp_id++;
                        s = s + load_op_num + " = load " + op_num + "\n";
                        op_num = load_op_num;
                    } else {
                        if (vec_exp && vec_exp->size() > 0) {
                            for (int i = vec_exp->size() - 1; i >= 0; --i) {
                                string src_ptr("");
                                if (i == vec_exp->size() - 1)
                                    src_ptr = "@" + name;
                                else
                                    src_ptr = op_num;
                                is_call = false;
                                ((*vec_exp)[i])->dump2str(s);
                                is_call = true;
                                string offset_num = op_num;
                                string new_op_num = "%" + string(to_string(min_temp_id));
                                min_temp_id++;
                                s = s + new_op_num + " = getelemptr " + src_ptr + ", " + 
                                    offset_num + "\n";
                                op_num = new_op_num;
                            }
                            if (int(vec_exp->size()) < symb.value.arr_info.dim) {
                                string new_op_num = "%" + string(to_string(min_temp_id));
                                min_temp_id++;
                                s = s + new_op_num + " = getelemptr " + op_num + ", 0\n";
                                op_num = new_op_num;
                            } else {
                                string new_op_num = "%" + string(to_string(min_temp_id));
                                min_temp_id++;
                                s = s + new_op_num + " = load " + op_num + "\n";
                                op_num = new_op_num;
                            }
                        } else if (!vec_exp) {
                            string new_op_num = "%" + string(to_string(min_temp_id));
                            min_temp_id++;
                            s = s + new_op_num + " = getelemptr @" + name + ", 0\n";
                            op_num = new_op_num;
                        }
                    }
                } else if (curr_instr == INSTR_TYPE::STORE) {
                    string store_op_num = op_num;
                    if (vec_exp->size() > 0) {
                        for (int i = vec_exp->size() - 1; i >= 0; --i) {
                            string src_ptr("");
                            if (i == vec_exp->size() - 1)
                                src_ptr = "@" + name;
                            else
                                src_ptr = op_num;
                            curr_instr = INSTR_TYPE::LOAD;
                            ((*vec_exp)[i])->dump2str(s);
                            curr_instr = INSTR_TYPE::STORE;
                            string offset_num = op_num;
                            string new_op_num = "%" + string(to_string(min_temp_id));
                            min_temp_id++;
                            s = s + new_op_num + " = getelemptr " + src_ptr + ", " + 
                                offset_num + "\n";
                            op_num = new_op_num;
                        }
                    }
                    s = s + "store " + store_op_num + ", " + op_num + "\n";
                }
            } else if (symb.tag == Tag::Pointer) {
                if (curr_instr == INSTR_TYPE::LOAD) {
                    if (!is_call) {
                        string first_ptr("");
                        first_ptr = "%" + string(to_string(min_temp_id));
                        min_temp_id++;
                        s = s + first_ptr + " = load @" + name + "\n";
                        op_num = first_ptr;
                        string second_ptr("");
                        second_ptr = "%" + string(to_string(min_temp_id));
                        min_temp_id++;
                        if (vec_exp && vec_exp->size() > 0) {
                            string first_offset("");
                            (*vec_exp)[vec_exp->size() - 1]->dump2str(s);
                            first_offset = op_num;
                            s = s + second_ptr + " = getptr " + first_ptr + ", " + 
                                first_offset + "\n";
                            op_num = second_ptr;
                            if (vec_exp->size() > 1) {
                                for (int i = vec_exp->size() - 2; i >= 0; --i) {
                                    string src_ptr = op_num;
                                    ((*vec_exp)[i])->dump2str(s);
                                    string offset_num = op_num;
                                    string new_op_num = "%" + string(to_string(min_temp_id));
                                    min_temp_id++;
                                    s = s + new_op_num + " = getelemptr " + src_ptr + 
                                        ", " + offset_num + "\n";
                                    op_num = new_op_num;
                                }
                            }
                        } else {
                            s = s + second_ptr + " = getptr " + first_ptr + ", 0\n";
                            op_num = second_ptr;
                        }
                        string load_op_num("");
                        load_op_num = "%" + string(to_string(min_temp_id));
                        min_temp_id++;
                        s = s + load_op_num + " = load " + op_num + "\n";
                        op_num = load_op_num;
                    } else {
                        string first_ptr("");
                        first_ptr = "%" + string(to_string(min_temp_id));
                        min_temp_id++;
                        s = s + first_ptr + " = load @" + name + "\n";
                        op_num = first_ptr;
                        string second_ptr("");
                        second_ptr = "%" + string(to_string(min_temp_id));
                        min_temp_id++;
                        if (vec_exp && vec_exp->size() > 0) {
                            string first_offset("");
                            is_call = false;
                            (*vec_exp)[vec_exp->size() - 1]->dump2str(s);
                            is_call = true;
                            first_offset = op_num;
                            s = s + second_ptr + " = getptr " + first_ptr + ", " + 
                                first_offset + "\n";
                            op_num = second_ptr;
                            if (vec_exp->size() > 1) {
                                for (int i = vec_exp->size() - 2; i >= 0; --i) {
                                    string src_ptr = op_num;
                                    is_call = false;
                                    ((*vec_exp)[i])->dump2str(s);
                                    is_call = true;
                                    string offset_num = op_num;
                                    string new_op_num = "%" + string(to_string(min_temp_id));
                                    min_temp_id++;
                                    s = s + new_op_num + " = getelemptr " + src_ptr + 
                                        ", " + offset_num + "\n";
                                    op_num = new_op_num;
                                }
                            }
                            if (int(vec_exp->size()) == symb.value.ptr_info.dim + 1) {
                                string new_op_num = "%" + string(to_string(min_temp_id));
                                min_temp_id++;
                                s = s + new_op_num + " = load " + op_num + "\n";
                                op_num = new_op_num;
                            } else {
                                string new_op_num = "%" + string(to_string(min_temp_id));
                                min_temp_id++;
                                s = s + new_op_num + " = getelemptr " + op_num + ", 0\n";
                                op_num = new_op_num;
                            }
                        } else {
                            s = s + second_ptr + " = getptr " + first_ptr + ", 0\n";
                            op_num = second_ptr;
                        }
                    }
                } else if (curr_instr == INSTR_TYPE::STORE) {
                    string store_op_num = op_num;
                    string first_ptr("");
                    first_ptr = "%" + string(to_string(min_temp_id));
                    min_temp_id++;
                    s = s + first_ptr + " = load @" + name + "\n";
                    op_num = first_ptr;
                    string second_ptr("");
                    second_ptr = "%" + string(to_string(min_temp_id));
                    min_temp_id++;
                    if (vec_exp && vec_exp->size() > 0) {
                        string first_offset("");
                        curr_instr = INSTR_TYPE::LOAD;
                        (*vec_exp)[vec_exp->size() - 1]->dump2str(s);
                        curr_instr = INSTR_TYPE::STORE;
                        first_offset = op_num;
                        s = s + second_ptr + " = getptr " + first_ptr + ", " + 
                            first_offset + "\n";
                        op_num = second_ptr;
                        if (vec_exp->size() > 1) {
                            for (int i = vec_exp->size() - 2; i >= 0; --i) {
                                string src_ptr = op_num;
                                curr_instr = INSTR_TYPE::LOAD;
                                ((*vec_exp)[i])->dump2str(s);
                                curr_instr = INSTR_TYPE::STORE;
                                string offset_num = op_num;
                                string new_op_num = "%" + string(to_string(min_temp_id));
                                min_temp_id++;
                                s = s + new_op_num + " = getelemptr " + src_ptr + 
                                    ", " + offset_num + "\n";
                                op_num = new_op_num;
                            }
                        }
                    } else {
                        s = s + second_ptr + " = getptr " + first_ptr + ", 0\n";
                        op_num = second_ptr;
                    }
                    s = s + "store " + store_op_num + ", " + op_num + "\n";
                }
            }
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        struct Symbol symb;
        string name = ident;
        if (prog_symtab.find_symbol(symb, name)) {
            if (symb.tag == Tag::Constant) {
                return symb.value.const_val;
            } else {
                return symb.value.var_sym.val;
            } 
        } else {
            cerr << "error: undefined lval!" << endl;
        }
        return 0;
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class PrimaryExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> primary_ptr;

    void dump2str(string &s) const override {
        primary_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { 
        return primary_ptr->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class NumberAST : public BaseAST {
public:
    int number;

    void dump2str(string &s) const override {
        op_num = string(to_string(number));
    }
    void insert2symtab() const override {}
    int cal_val() const override { 
        return number;
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class UnaryExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> unary_ptr;

    void dump2str(string &s) const override {
        unary_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { 
        return unary_ptr->cal_val(); 
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class FunctionCallAST : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> func_rparams;

    void dump2str(string &s) const override {
        is_call = true;
        int num_params = 0;
        if (func_rparams) {
            func_rparams->dump2str(s);
            num_params = func_rparams->cal_val();
        }
        struct Symbol symb;
        string name = ident;
        if (prog_symtab.find_global_symbol(symb, name)) {
            if (symb.value.func_type == 1) {
                string new_op_num("");
                new_op_num += "%" + string(to_string(min_temp_id));
                min_temp_id++; 
                s = s + new_op_num + " = ";
                op_num = new_op_num;
            }
            s = s + "call @" + name + "(";
            if (num_params > 0) {
                for (int i = params.size() - num_params; i < params.size(); ++i) {
                    s = s + params[i];
                    if (i < params.size() - 1)
                        s = s + ", ";
                }
                for (int i = 0; i < num_params; ++i)
                    params.pop_back();
            }
            s = s + ")\n";
        }
        is_call = false;
    }
    void insert2symtab() const override {}
    int cal_val() const override { return 0; }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class FuncRParamsAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<vector<unique_ptr<BaseAST> > > vec_exp;

    void dump2str(string &s) const override {
        string param("");
        exp->dump2str(s);
        param = op_num;
        params.push_back(param);
        if (!vec_exp)
            return;
        for (int i = 0; i < vec_exp->size(); ++i) {
            param = "";
            (*vec_exp)[i]->dump2str(s);
            param = op_num;
            params.push_back(param);
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override { 
        int n = 1;  // exp
        if (vec_exp)  // vec_exp 
            n += int(vec_exp->size());
        return n;
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class UnaryOpExpAST : public BaseAST {
public:
    string unary_op;
    unique_ptr<BaseAST> unary_exp;

    void dump2str(string &s) const override {
        unary_exp->dump2str(s);
        string op_exp("");
        string new_op_num("");
        switch (unary_op[0]) {
            case '+':
                break;
            case '-':
                new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
                min_temp_id++;
                op_exp = op_exp + new_op_num + " = ";
                op_exp = op_exp + "sub 0, " + op_num + "\n";
                op_num = new_op_num;
                s += op_exp;
                break;
            case '!':
                new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
                min_temp_id++;
                op_exp = op_exp + new_op_num + " = ";
                op_exp = op_exp + "eq " + op_num + ", " + "0\n";
                op_num = new_op_num;
                s += op_exp;
                break;
            default:
                break;
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        switch (unary_op[0]) {
            case '+':
                return unary_exp->cal_val();
            case '-':
                return -(unary_exp->cal_val());
            case '!':
                return !(unary_exp->cal_val());
            default:
                cerr << "error: illegal unary operator!" << endl;
                break;
        }
        return 0;
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class MulExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> mul_ptr;

    void dump2str(string &s) const override {
        mul_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override { 
        return mul_ptr->cal_val(); 
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class MulOpExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> mul_exp;
    string mul_op;
    unique_ptr<BaseAST> unary_exp;

    void dump2str(string &s) const override {
        mul_exp->dump2str(s);
        string op_num1 = op_num;
        unary_exp->dump2str(s);
        string op_num2 = op_num;
        string op_exp("");
        string new_op_num("");
        switch (mul_op[0]) {
            case '*':
                new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
                min_temp_id++;
                op_exp = op_exp + new_op_num + " = ";
                op_exp = op_exp + "mul " + op_num1 + ", " + op_num2 + "\n";
                op_num = new_op_num;
                s += op_exp;
                break;
            case '/':
                new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
                min_temp_id++;
                op_exp = op_exp + new_op_num + " = ";
                op_exp = op_exp + "div " + op_num1 + ", " + op_num2 + "\n";
                op_num = new_op_num;
                s += op_exp;
                break;
            case '%':
                new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
                min_temp_id++;
                op_exp = op_exp + new_op_num + " = ";
                op_exp = op_exp + "mod " + op_num1 + ", " + op_num2 + "\n";
                op_num = new_op_num;
                s += op_exp;
                break;
            default:
                break;
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        switch (mul_op[0]) {
            case '*':
                return mul_exp->cal_val() * unary_exp->cal_val();
            case '/':
                return mul_exp->cal_val() / unary_exp->cal_val();;
            case '%':
                return mul_exp->cal_val() % unary_exp->cal_val();
            default:
                cerr << "error: illegal binary operator!" << endl;
                break;
        }
        return 0;
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class AddExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> add_ptr;
    
    void dump2str(string &s) const override {
        add_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return add_ptr->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class AddOpExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> add_exp;
    string add_op;
    unique_ptr<BaseAST> mul_exp;

    void dump2str(string &s) const override {
        add_exp->dump2str(s);
        string op_num1 = op_num;
        mul_exp->dump2str(s);
        string op_num2 = op_num;
        string op_exp("");
        string new_op_num("");
        switch (add_op[0]) {
            case '+':
                new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
                min_temp_id++;
                op_exp = op_exp + new_op_num + " = ";
                op_exp = op_exp + "add " + op_num1 + ", " + op_num2 + "\n";
                op_num = new_op_num;
                s += op_exp;                
                break;
            case '-':
                new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
                min_temp_id++;
                op_exp = op_exp + new_op_num + " = ";
                op_exp = op_exp + "sub " + op_num1 + ", " + op_num2 + "\n";
                op_num = new_op_num;
                s += op_exp;                
                break;
            default:
                break;
        }
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        if (add_op[0] == '+') {
            return add_exp->cal_val() + mul_exp->cal_val();
        } else {
            return add_exp->cal_val() - mul_exp->cal_val();
        }
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class RelExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> rel_ptr;

    void dump2str(string &s) const override {
        rel_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return rel_ptr->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class RelOpExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> rel_exp;
    string rel_op;
    unique_ptr<BaseAST> add_exp;

    void dump2str(string &s) const override {
        rel_exp->dump2str(s);
        string op_num1 = op_num;
        add_exp->dump2str(s);
        string op_num2 = op_num;
        string op_exp("");
        string new_op_num("");
        string instr("");
        if (rel_op == "<") {
            instr = string("lt ");
        } else if (rel_op == "<=") {
            instr = string("le ");
        } else if (rel_op == ">") {
            instr = string("gt ");
        } else {
            instr = string("ge ");
        }
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        op_exp = op_exp + new_op_num + " = ";
        op_exp = op_exp + instr + op_num1 + ", " + op_num2 + "\n";
        op_num = new_op_num;
        s += op_exp;
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        if (rel_op == "<") {
            return rel_exp->cal_val() < add_exp->cal_val();
        } else if (rel_op == "<=") {
            return rel_exp->cal_val() <= add_exp->cal_val();
        } else if (rel_op == ">") {
            return rel_exp->cal_val() > add_exp->cal_val();
        } else {
            return rel_exp->cal_val() >= add_exp->cal_val();
        }
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class EqExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> eq_ptr;

    void dump2str(string &s) const override {
        eq_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return eq_ptr->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class EqOpExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> eq_exp;
    string eq_op;
    unique_ptr<BaseAST> rel_exp;

    void dump2str(string &s) const override {
        eq_exp->dump2str(s);
        string op_num1 = op_num;
        rel_exp->dump2str(s);
        string op_num2 = op_num;
        string op_exp("");
        string new_op_num("");
        string instr("");
        if (eq_op == "==") {
            instr = string("eq ");
        } else {
            instr = string("ne ");
        }
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        op_exp = op_exp + new_op_num + " = ";
        op_exp = op_exp + instr + op_num1 + ", " + op_num2 + "\n";
        op_num = new_op_num;
        s += op_exp;
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        if (eq_op == "==") {
            return eq_exp->cal_val() == rel_exp->cal_val();
        } else {
            return eq_exp->cal_val() != rel_exp->cal_val();
        }
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class LAndExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> land_ptr;

    void dump2str(string &s) const override {
        land_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return land_ptr->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class LAndOpExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> land_exp;
    string land_op;
    unique_ptr<BaseAST> eq_exp;

    void dump2str(string &s) const override {
        // use "B = ne A, 0" to transfer integer value A to 
        // corresponding logical value B
        string op_exp("");
        string new_op_num("");
        string instr("and ");

        // lhs computation
        // int result = 0;
        int then_blk_id = block_id;
        int end_blk_id = block_id + 1;
        block_id += 2;
        new_op_num = "";
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        s = s + new_op_num + " = alloc i32\n";
        s = s + "store 0, "+ new_op_num + "\n";
        string res = new_op_num;
        // if (op_num1 != 0)
        land_exp->dump2str(s);
        string op_num1 = op_num;
        new_op_num = "";
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        s = s + new_op_num + " = ";
        s = s + "ne " + op_num1 + ", 0\n";
        op_num = new_op_num;
        s = s + "br " + op_num + ", %block" + string(to_string(then_blk_id)) + 
            ", %block" + string(to_string(end_blk_id)) + "\n";
        // result = op_num2 != 0;
        s = s + "\n%block" + string(to_string(then_blk_id)) + ":\n";
        eq_exp->dump2str(s);       
        string op_num2 = op_num;
        new_op_num = "";
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        s = s + new_op_num + " = ";
        s = s + "ne " + op_num2 + ", 0\n";
        op_num = new_op_num;
        s = s + "store " + op_num + ", " + res + "\n";
        s = s + "jump %block" + string(to_string(end_blk_id)) + "\n";
        
        s = s + "\n%block" + string(to_string(end_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
        new_op_num = "";
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        s = s + new_op_num + " = load " + res + "\n";
        op_num = new_op_num;

        // land_exp->dump2str(s);
        // new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        // min_temp_id++;
        // op_exp = op_exp + new_op_num + " = ";
        // op_exp = op_exp + "ne " + op_num + ", 0\n";
        // op_num = new_op_num;
        // string op_num1 = new_op_num;
        // s += op_exp;

        // op_exp = "";
        // new_op_num = "";
        // eq_exp->dump2str(s);
        // new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        // min_temp_id++;
        // op_exp = op_exp + new_op_num + " = ";
        // op_exp = op_exp + "ne " + op_num + ", 0\n";
        // op_num = new_op_num;       
        // string op_num2 = new_op_num;
        // s += op_exp;
        
        // op_exp = "";
        // new_op_num = "";
        // new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        // min_temp_id++;
        // op_exp = op_exp + new_op_num + " = ";
        // op_exp = op_exp + instr + op_num1 + ", " + op_num2 + "\n";
        // op_num = new_op_num;
        // s += op_exp;
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return land_exp->cal_val() && eq_exp->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class LOrExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lor_ptr;

    void dump2str(string &s) const override {
        lor_ptr->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return lor_ptr->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class LOrOpExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> lor_exp;
    string lor_op;
    unique_ptr<BaseAST> land_exp;

    void dump2str(string &s) const override {
        string op_exp("");
        string new_op_num("");
        string instr("or ");

        // lhs computation
        // int result = 1;
        int then_blk_id = block_id;
        int end_blk_id = block_id + 1;
        block_id += 2;
        new_op_num = "";
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        s = s + new_op_num + " = alloc i32\n";
        s = s + "store 1, "+ new_op_num + "\n";
        string res = new_op_num;
        // if (op_num1 == 0)
        lor_exp->dump2str(s);
        string op_num1 = op_num;
        new_op_num = "";
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        s = s + new_op_num + " = ";
        s = s + "eq " + op_num1 + ", 0\n";
        op_num = new_op_num;
        s = s + "br " + op_num + ", %block" + string(to_string(then_blk_id)) + 
            ", %block" + string(to_string(end_blk_id)) + "\n";
        // result = op_num2 != 0;
        s = s + "\n%block" + string(to_string(then_blk_id)) + ":\n";
        land_exp->dump2str(s);       
        string op_num2 = op_num;
        new_op_num = "";
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        s = s + new_op_num + " = " + "ne " + op_num2 + ", 0\n";
        op_num = new_op_num;
        s = s + "store " + op_num + ", " + res + "\n";
        s = s + "jump %block" + string(to_string(end_blk_id)) + "\n";
        
        s = s + "\n%block" + string(to_string(end_blk_id)) + ":\n";
        ret_in_blk = false;
        jump_in_blk = false;
        new_op_num = "";
        new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        min_temp_id++;
        s = s + new_op_num + " = load " + res + "\n";
        op_num = new_op_num;

        // lor_exp->dump2str(s);
        // new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        // min_temp_id++;
        // op_exp = op_exp + new_op_num + " = ";
        // op_exp = op_exp + "ne " + op_num + ", 0\n";
        // op_num = new_op_num;
        // string op_num1 = new_op_num;
        // s += op_exp;

        // op_exp = "";
        // new_op_num = "";
        // land_exp->dump2str(s);
        // new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        // min_temp_id++;
        // op_exp = op_exp + new_op_num + " = ";
        // op_exp = op_exp + "ne " + op_num + ", 0\n";
        // op_num = new_op_num;       
        // string op_num2 = new_op_num;
        // s += op_exp;
        
        // op_exp = "";
        // new_op_num = "";
        // new_op_num = new_op_num + "%" + string(to_string(min_temp_id));
        // min_temp_id++;
        // op_exp = op_exp + new_op_num + " = ";
        // op_exp = op_exp + instr + op_num1 + ", " + op_num2 + "\n";
        // op_num = new_op_num;
        // s += op_exp;
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return lor_exp->cal_val() || land_exp->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

class ConstExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void dump2str(string &s) const override {
        exp->dump2str(s);
    }
    void insert2symtab() const override {}
    int cal_val() const override {
        return exp->cal_val();
    }
    void supdump(string &s) const override {}
    unique_ptr<veci> aggr_init(veci dim) const override { unique_ptr<veci> v = make_unique<veci>(); return v; }
};

#endif