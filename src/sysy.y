%code requires {
  #include <memory>
  #include <string>
  #include <vector>
  #include "ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ast.h"

// declare lexer function and error handling function
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

// definition of yyval
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST> > *vec_ast_val;
}

%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT
%token <int_val> INT_CONST
%token <str_val> REL_OPERATOR
%token <str_val> EQ_OPERATOR
%token <str_val> LAND_OPERATOR
%token <str_val> LOR_OPERATOR

%type <ast_val> CompUnit FuncDef FuncType Block Stmt 
%type <ast_val> Exp PrimaryExp Number UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal BlockItem LVal ConstExp
%type <ast_val> VarDecl VarDef InitVal
%type <ast_val> SimpleStmt OpenStmt ClosedStmt
%type <ast_val> FuncFParams FuncFParam FuncRParams
%type <vec_ast_val> VecConstDef VecBlockItem VecVarDef
%type <vec_ast_val> VecFuncFParam VecExp 
%type <vec_ast_val> VecConstArrayLen VecConstInitVal
%type <str_val> UnaryOp MulOp AddOp RelOp EqOp LAndOp LOrOp

%%

OriginCompUnit
  : CompUnit {
    auto origin_comp_unit = make_unique<OriginCompUnitAST>();
    origin_comp_unit->comp_unit = unique_ptr<BaseAST>($1);
    ast = move(origin_comp_unit);
  }

CompUnit
  : FuncDef {
    auto comp_unit = new CompUnitAST();
    comp_unit->comp_unit = nullptr;
    comp_unit->comp_unit_ptr = unique_ptr<BaseAST>($1);
    $$ = comp_unit;
  }
  | Decl {
    auto comp_unit = new CompUnitAST();
    comp_unit->comp_unit = nullptr;
    comp_unit->comp_unit_ptr = unique_ptr<BaseAST>($1);
    $$ = comp_unit;
  }
  | CompUnit FuncDef {
    auto comp_unit = new CompUnitAST();
    comp_unit->comp_unit = unique_ptr<BaseAST>($1);
    comp_unit->comp_unit_ptr = unique_ptr<BaseAST>($2);
    $$ = comp_unit;
  }
  | CompUnit Decl {
    auto comp_unit = new CompUnitAST();
    comp_unit->comp_unit = unique_ptr<BaseAST>($1);
    comp_unit->comp_unit_ptr = unique_ptr<BaseAST>($2);
    $$ = comp_unit; 
  }
  ;

Decl 
  : ConstDecl {
    auto decl_ast = new DeclAST();
    decl_ast->decl_ptr = unique_ptr<BaseAST>($1);
    $$ = decl_ast;
  }
  | VarDecl {
    auto decl_ast = new DeclAST();
    decl_ast->decl_ptr = unique_ptr<BaseAST>($1);
    $$ = decl_ast;
  }
  ;

ConstDecl
  : CONST FuncType ConstDef VecConstDef ';' {
    auto const_decl_ast = new ConstDeclAST();
    const_decl_ast->btype = unique_ptr<BaseAST>($2);
    const_decl_ast->const_def = unique_ptr<BaseAST>($3);
    const_decl_ast->vec_const_def = unique_ptr<vector<unique_ptr<BaseAST> > >($4);
    $$ = const_decl_ast;
  }
  | CONST FuncType ConstDef ';' {
    auto const_decl_ast = new ConstDeclAST();
    const_decl_ast->btype = unique_ptr<BaseAST>($2);
    const_decl_ast->const_def = unique_ptr<BaseAST>($3);
    const_decl_ast->vec_const_def = nullptr;
    $$ = const_decl_ast;
  }
  ;

VecConstDef
  : ',' ConstDef {
    auto vec_const_def = new vector<unique_ptr<BaseAST> >();
    auto const_def = unique_ptr<BaseAST>($2);
    vec_const_def->push_back(move(const_def));
    $$ = vec_const_def;
  }
  | VecConstDef ',' ConstDef {
    auto vec_const_def = $1;
    auto const_def = unique_ptr<BaseAST>($3);
    vec_const_def->push_back(move(const_def));
    $$ = vec_const_def;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto const_def_ast = new ConstDefAST();
    const_def_ast->ident = *unique_ptr<string>($1);
    const_def_ast->vec_const_exp = nullptr;
    const_def_ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = const_def_ast;
  } 
  | IDENT VecConstArrayLen '=' ConstInitVal {
    auto const_def_ast = new ConstDefAST();
    const_def_ast->ident = *unique_ptr<string>($1);
    const_def_ast->vec_const_exp = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    const_def_ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = const_def_ast;
  }
  ;

VecConstArrayLen
  : '[' ConstExp ']' {
    auto vec_const_arr_len = new vector<unique_ptr<BaseAST> >();
    auto const_exp= unique_ptr<BaseAST>($2);
    vec_const_arr_len->insert(vec_const_arr_len->begin(), move(const_exp));
    $$ = vec_const_arr_len;
  }
  | VecConstArrayLen '[' ConstExp ']' {
    auto vec_const_arr_len = $1;
    auto const_exp= unique_ptr<BaseAST>($3);
    vec_const_arr_len->insert(vec_const_arr_len->begin(), move(const_exp));
    $$ = vec_const_arr_len;
  }
  ;

ConstInitVal
  : ConstExp {
    auto const_init_val_ast = new ConstInitValAST();
    const_init_val_ast->const_exp = unique_ptr<BaseAST>($1);
    const_init_val_ast->vec_const_init_val = nullptr;
    $$ = const_init_val_ast;
  }
  | '{' '}' {
    auto const_init_val_ast = new ConstInitValAST();
    const_init_val_ast->const_exp = nullptr;
    const_init_val_ast->vec_const_init_val = make_unique<vector<unique_ptr<BaseAST> > >();
    $$ = const_init_val_ast;
  }
  | '{' ConstInitVal '}' {
    auto const_init_val_ast = new ConstInitValAST();
    const_init_val_ast->const_exp = nullptr;
    const_init_val_ast->vec_const_init_val = make_unique<vector<unique_ptr<BaseAST> > >();
    auto const_init = unique_ptr<BaseAST>($2);
    (const_init_val_ast->vec_const_init_val)->push_back(move(const_init));
    $$ = const_init_val_ast;
  }
  | '{' ConstInitVal VecConstInitVal '}' {
    auto const_init_val_ast = new ConstInitValAST();
    const_init_val_ast->const_exp = nullptr;
    const_init_val_ast->vec_const_init_val = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    auto const_init = unique_ptr<BaseAST>($2);
    (const_init_val_ast->vec_const_init_val)->insert((const_init_val_ast->vec_const_init_val)->begin(), move(const_init));
    $$ = const_init_val_ast;
  }
  ;

VecConstInitVal
  : ',' ConstInitVal {
    auto vec_const_init_val = new vector<unique_ptr<BaseAST> >();
    auto const_init_val= unique_ptr<BaseAST>($2);
    vec_const_init_val->push_back(move(const_init_val));
    $$ = vec_const_init_val;
  }
  | VecConstInitVal ',' ConstInitVal {
    auto vec_const_init_val = $1;
    auto const_init_val= unique_ptr<BaseAST>($3);
    vec_const_init_val->push_back(move(const_init_val));
    $$ = vec_const_init_val;
  }
  ;

VarDecl
  : FuncType VarDef VecVarDef ';' {
    auto var_decl_ast = new VarDeclAST();
    var_decl_ast->btype = unique_ptr<BaseAST>($1);
    var_decl_ast->var_def = unique_ptr<BaseAST>($2);
    var_decl_ast->vec_var_def = unique_ptr<vector<unique_ptr<BaseAST> > >($3);
    $$ = var_decl_ast;
  }
  | FuncType VarDef ';' {
    auto var_decl_ast = new VarDeclAST();
    var_decl_ast->btype = unique_ptr<BaseAST>($1);
    var_decl_ast->var_def = unique_ptr<BaseAST>($2);
    var_decl_ast->vec_var_def = nullptr;
    $$ = var_decl_ast;
  }
  ;

VecVarDef
  : ',' VarDef {
    auto vec_var_def = new vector<unique_ptr<BaseAST> >();
    auto var_def = unique_ptr<BaseAST>($2);
    vec_var_def->push_back(move(var_def));
    $$ = vec_var_def;
  }
  | VecVarDef ',' VarDef {
    auto vec_var_def = $1;
    auto var_def = unique_ptr<BaseAST>($3);
    vec_var_def->push_back(move(var_def));
    $$ = vec_var_def;
  }
  ;

VarDef 
  : IDENT {
    auto var_def_ast = new VarDefAST();
    auto var_uninit_ast = new VarUnInitAST();
    var_uninit_ast->ident = *unique_ptr<string>($1);
    var_def_ast->var_def_ptr = unique_ptr<BaseAST>(var_uninit_ast);
    $$ = var_def_ast;
  }
  | IDENT '=' InitVal {
    auto var_def_ast = new VarDefAST();
    auto var_init_ast = new VarInitAST();
    var_init_ast->ident = *unique_ptr<string>($1);
    var_init_ast->init_val = unique_ptr<BaseAST>($3);
    var_def_ast->var_def_ptr = unique_ptr<BaseAST>(var_init_ast);
    $$ = var_def_ast;
  }
  | IDENT VecConstArrayLen {
    auto var_def_ast = new VarDefAST();
    auto var_arr_ast = new VarArrayAST();
    var_arr_ast->ident = *unique_ptr<string>($1);
    var_arr_ast->vec_const_exp = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    var_arr_ast->init_val = nullptr;
    var_def_ast->var_def_ptr = unique_ptr<BaseAST>(var_arr_ast);
    $$ = var_def_ast;
  }
  | IDENT VecConstArrayLen '=' ConstInitVal {
    auto var_def_ast = new VarDefAST();
    auto var_arr_ast = new VarArrayAST();
    var_arr_ast->ident = *unique_ptr<string>($1);
    var_arr_ast->vec_const_exp = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    var_arr_ast->init_val = unique_ptr<BaseAST>($4);
    var_def_ast->var_def_ptr = unique_ptr<BaseAST>(var_arr_ast);
    $$ = var_def_ast;
  }
  ;

InitVal
  : Exp {
    auto init_val_ast = new InitValAST();
    init_val_ast->exp = unique_ptr<BaseAST>($1);
    $$ = init_val_ast;
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto func_def_ast = new FuncDefAST();
    func_def_ast->func_type = unique_ptr<BaseAST>($1);
    func_def_ast->ident = *unique_ptr<string>($2);
    func_def_ast->func_fparams = nullptr;
    func_def_ast->block = unique_ptr<BaseAST>($5);
    $$ = func_def_ast;
  }
  | FuncType IDENT '(' FuncFParams ')' Block {
    auto func_def_ast = new FuncDefAST();
    func_def_ast->func_type = unique_ptr<BaseAST>($1);
    func_def_ast->ident = *unique_ptr<string>($2);
    func_def_ast->func_fparams = unique_ptr<BaseAST>($4);
    func_def_ast->block = unique_ptr<BaseAST>($6);
    $$ = func_def_ast;
  }
  ;

FuncType
  : INT {
    auto func_type_ast = new FuncTypeAST();
    func_type_ast->func_type = string("int");
    $$ = func_type_ast;
  }
  | VOID {
    auto func_type_ast = new FuncTypeAST();
    func_type_ast->func_type = string("void");
    $$ = func_type_ast;
  }
  ;

FuncFParams
  : FuncFParam VecFuncFParam {
    auto func_fparams_ast = new FuncFParamsAST();
    func_fparams_ast->func_fparam = unique_ptr<BaseAST>($1);
    func_fparams_ast->vec_func_fparam = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = func_fparams_ast;
  }
  | FuncFParam {
    auto func_fparams_ast = new FuncFParamsAST();
    func_fparams_ast->func_fparam = unique_ptr<BaseAST>($1);
    func_fparams_ast->vec_func_fparam = nullptr;
    $$ = func_fparams_ast;
  }
  ;

VecFuncFParam
  : ',' FuncFParam {
    auto vec_func_fparam = new vector<unique_ptr<BaseAST> >();
    auto func_fparam = unique_ptr<BaseAST>($2);
    vec_func_fparam->push_back(move(func_fparam));
    $$ = vec_func_fparam;
  }
  | VecFuncFParam ',' FuncFParam {
    auto vec_func_fparam = $1;
    auto func_fparam = unique_ptr<BaseAST>($3);
    vec_func_fparam->push_back(move(func_fparam));
    $$ = vec_func_fparam;
  }
  ;

FuncFParam
  : FuncType IDENT {
    auto func_fparam_ast = new FuncFParamAST();
    func_fparam_ast->btype = unique_ptr<BaseAST>($1);
    func_fparam_ast->ident = *unique_ptr<string>($2);
    func_fparam_ast->vec_const_exp = nullptr;
    $$ = func_fparam_ast;
  }
  | FuncType IDENT '[' ']' {
    auto func_fparam_ast = new FuncFParamAST();
    func_fparam_ast->btype = unique_ptr<BaseAST>($1);
    func_fparam_ast->ident = *unique_ptr<string>($2);
    func_fparam_ast->vec_const_exp = make_unique<vector<unique_ptr<BaseAST> > >();
    $$ = func_fparam_ast;
  }
  | FuncType IDENT '[' ']' VecConstArrayLen {
    auto func_fparam_ast = new FuncFParamAST();
    func_fparam_ast->btype = unique_ptr<BaseAST>($1);
    func_fparam_ast->ident = *unique_ptr<string>($2);
    func_fparam_ast->vec_const_exp = unique_ptr<vector<unique_ptr<BaseAST> > >($5);
    $$ = func_fparam_ast;
  }
  ;

Block
  : '{' VecBlockItem '}' {
    auto block_ast = new BlockAST();
    block_ast->vec_block_item = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = block_ast;
  }
  | '{' '}' {
    auto block_ast = new BlockAST();
    block_ast->vec_block_item = nullptr;
    $$ = block_ast;
  }
  ;

VecBlockItem 
  : BlockItem {
    auto vec_block_item = new vector<unique_ptr<BaseAST> >();
    auto block_item = unique_ptr<BaseAST>($1);
    vec_block_item->push_back(move(block_item));
    $$ = vec_block_item;
  }
  | VecBlockItem BlockItem {
    auto vec_block_item = $1;
    auto block_item = unique_ptr<BaseAST>($2);
    vec_block_item->push_back(move(block_item));
    $$ = vec_block_item;
  }
  ;

BlockItem 
  : Decl {
    auto block_item_ast = new BlockItemAST();
    block_item_ast->block_item_ptr = unique_ptr<BaseAST>($1);
    $$ = block_item_ast;
  }
  | Stmt {
    auto block_item_ast = new BlockItemAST();
    block_item_ast->block_item_ptr = unique_ptr<BaseAST>($1);
    $$ = block_item_ast; 
  }
  ;

Stmt 
  : OpenStmt {
    auto stmt_ast = new StmtAST();
    stmt_ast->stmt_ptr = unique_ptr<BaseAST>($1);
    $$ = stmt_ast;
  }
  | ClosedStmt {
    auto stmt_ast = new StmtAST();
    stmt_ast->stmt_ptr = unique_ptr<BaseAST>($1);
    $$ = stmt_ast;
  }
  ;

OpenStmt
  : IF '(' Exp ')' Stmt {
    auto open_stmt_ast = new OpenStmtAST();
    auto open_if_stmt_ast = new OpenIfStmtAST();
    open_if_stmt_ast->exp = unique_ptr<BaseAST>($3);
    open_if_stmt_ast->stmt = unique_ptr<BaseAST>($5);
    open_stmt_ast->open_stmt_ptr = unique_ptr<BaseAST>(open_if_stmt_ast);
    $$ = open_stmt_ast;
  }
  | IF '(' Exp ')' ClosedStmt ELSE OpenStmt {
    auto open_stmt_ast = new OpenStmtAST();
    auto open_ifelse_stmt_ast = new OpenIfElseStmtAST();
    open_ifelse_stmt_ast->exp = unique_ptr<BaseAST>($3);
    open_ifelse_stmt_ast->closed_stmt = unique_ptr<BaseAST>($5);
    open_ifelse_stmt_ast->open_stmt = unique_ptr<BaseAST>($7);
    open_stmt_ast->open_stmt_ptr = unique_ptr<BaseAST>(open_ifelse_stmt_ast);
    $$ = open_stmt_ast;
  }
  | WHILE '(' Exp ')' OpenStmt {
    auto open_stmt_ast = new OpenStmtAST();
    auto while_stmt_ast = new WhileStmtAST();
    while_stmt_ast->exp = unique_ptr<BaseAST>($3);
    while_stmt_ast->stmt = unique_ptr<BaseAST>($5);
    open_stmt_ast->open_stmt_ptr = unique_ptr<BaseAST>(while_stmt_ast);
    $$ = open_stmt_ast;
  }
  ;

ClosedStmt
  : SimpleStmt {
    auto closed_stmt_ast = new ClosedStmtAST();
    closed_stmt_ast->closed_stmt_ptr = unique_ptr<BaseAST>($1);
    $$ = closed_stmt_ast;
  }
  | IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
    auto closed_stmt_ast = new ClosedStmtAST();
    auto closed_ifelse_stmt_ast = new ClosedIfElseStmtAST();
    closed_ifelse_stmt_ast->exp = unique_ptr<BaseAST>($3);
    closed_ifelse_stmt_ast->closed_stmt_if = unique_ptr<BaseAST>($5);
    closed_ifelse_stmt_ast->closed_stmt_else = unique_ptr<BaseAST>($7);
    closed_stmt_ast->closed_stmt_ptr = unique_ptr<BaseAST>(closed_ifelse_stmt_ast);
    $$ = closed_stmt_ast;
  }
  | WHILE '(' Exp ')' ClosedStmt {
    auto closed_stmt_ast = new ClosedStmtAST();
    auto while_stmt_ast = new WhileStmtAST();
    while_stmt_ast->exp = unique_ptr<BaseAST>($3);
    while_stmt_ast->stmt = unique_ptr<BaseAST>($5);
    closed_stmt_ast->closed_stmt_ptr = unique_ptr<BaseAST>(while_stmt_ast);
    $$ = closed_stmt_ast;
  }
  ;

SimpleStmt
  : LVal '=' Exp ';' {
    auto simple_stmt_ast = new SimpleStmtAST();
    auto assign_stmt_ast = new AssignStmtAST();
    assign_stmt_ast->lval = unique_ptr<BaseAST>($1);
    assign_stmt_ast->exp = unique_ptr<BaseAST>($3);
    simple_stmt_ast->simple_stmt_ptr = unique_ptr<BaseAST>(assign_stmt_ast);
    $$ = simple_stmt_ast;
  }
  | Exp ';' {
    auto simple_stmt_ast = new SimpleStmtAST();
    auto exp_stmt_ast = new ExpStmtAST();
    exp_stmt_ast->exp = unique_ptr<BaseAST>($1);
    simple_stmt_ast->simple_stmt_ptr = unique_ptr<BaseAST>(exp_stmt_ast);
    $$ = simple_stmt_ast;
  }
  | ';' {
    auto simple_stmt_ast = new SimpleStmtAST();
    auto exp_stmt_ast = new ExpStmtAST();
    exp_stmt_ast->exp = nullptr;
    simple_stmt_ast->simple_stmt_ptr = unique_ptr<BaseAST>(exp_stmt_ast);
    $$ = simple_stmt_ast;
  }
  | RETURN Exp ';' {
    auto simple_stmt_ast = new SimpleStmtAST();
    auto ret_stmt_ast = new RetStmtAST();
    ret_stmt_ast->exp = unique_ptr<BaseAST>($2);
    simple_stmt_ast->simple_stmt_ptr = unique_ptr<BaseAST>(ret_stmt_ast);
    $$ = simple_stmt_ast;
  }
  | Block {
    auto simple_stmt_ast = new SimpleStmtAST();
    simple_stmt_ast->simple_stmt_ptr = unique_ptr<BaseAST>($1);
    $$ = simple_stmt_ast;
  }
  | RETURN ';' {
    auto simple_stmt_ast = new SimpleStmtAST();
    auto ret_stmt_ast = new RetStmtAST();
    ret_stmt_ast->exp = nullptr;
    simple_stmt_ast->simple_stmt_ptr = unique_ptr<BaseAST>(ret_stmt_ast);
    $$ = simple_stmt_ast;
  }
  | BREAK ';' {
    auto simple_stmt_ast = new SimpleStmtAST();
    auto break_stmt_ast = new BreakStmtAST();
    break_stmt_ast->break_stmt = string("break");
    simple_stmt_ast->simple_stmt_ptr = unique_ptr<BaseAST>(break_stmt_ast);
    $$ = simple_stmt_ast;
  }
  | CONTINUE ';' {
    auto simple_stmt_ast = new SimpleStmtAST();
    auto continue_stmt_ast = new ContinueStmtAST();
    continue_stmt_ast->continue_stmt = string("continue");
    simple_stmt_ast->simple_stmt_ptr = unique_ptr<BaseAST>(continue_stmt_ast);
    $$ = simple_stmt_ast;
  }
  ;

Exp
  : LOrExp {
    auto exp_ast = new ExpAST();
    exp_ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = exp_ast;
  }
  ;

LVal
  : IDENT {
    auto lval_ast = new LValAST();
    lval_ast->ident = *unique_ptr<string>($1);
    lval_ast->vec_exp = nullptr;
    $$ = lval_ast;
  }
  | IDENT VecConstArrayLen {
    auto lval_ast = new LValAST();
    lval_ast->ident = *unique_ptr<string>($1);
    lval_ast->vec_exp = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = lval_ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto primaryexp_ast = new PrimaryExpAST();
    primaryexp_ast->primary_ptr = unique_ptr<BaseAST>($2);
    $$ = primaryexp_ast;
  }
  | LVal {
    auto primaryexp_ast = new PrimaryExpAST();
    primaryexp_ast->primary_ptr = unique_ptr<BaseAST>($1);
    $$ = primaryexp_ast;
  }
  | Number {
    auto primaryexp_ast = new PrimaryExpAST();
    primaryexp_ast->primary_ptr = unique_ptr<BaseAST>($1);
    $$ = primaryexp_ast;
  }
  ;

Number
  : INT_CONST {
    auto number_ast = new NumberAST();
    number_ast->number = int($1);
    $$ = number_ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto unaryexp_ast = new UnaryExpAST();
    unaryexp_ast->unary_ptr = unique_ptr<BaseAST>($1);
    $$ = unaryexp_ast;
  }
  | UnaryOp UnaryExp {
    auto unaryexp_ast = new UnaryExpAST();
    auto unaryopexp_ast = new UnaryOpExpAST();
    unaryopexp_ast->unary_op = *unique_ptr<string>($1);
    unaryopexp_ast->unary_exp = unique_ptr<BaseAST>($2);
    unaryexp_ast->unary_ptr = unique_ptr<BaseAST>(unaryopexp_ast);
    $$ = unaryexp_ast;
  }
  | IDENT '(' ')' {
    auto unaryexp_ast = new UnaryExpAST();
    auto func_call_ast = new FunctionCallAST();
    func_call_ast->ident = *unique_ptr<string>($1);
    func_call_ast->func_rparams = nullptr;
    unaryexp_ast->unary_ptr = unique_ptr<BaseAST>(func_call_ast);
    $$ = unaryexp_ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto unaryexp_ast = new UnaryExpAST();
    auto func_call_ast = new FunctionCallAST();
    func_call_ast->ident = *unique_ptr<string>($1);
    func_call_ast->func_rparams = unique_ptr<BaseAST>($3);
    unaryexp_ast->unary_ptr = unique_ptr<BaseAST>(func_call_ast);
    $$ = unaryexp_ast;
  }
  ;

FuncRParams
  : Exp VecExp {
    auto func_rparams_ast = new FuncRParamsAST();
    func_rparams_ast->exp = unique_ptr<BaseAST>($1);
    func_rparams_ast->vec_exp = unique_ptr<vector<unique_ptr<BaseAST> > >($2);
    $$ = func_rparams_ast;
  }
  | Exp {
    auto func_rparams_ast = new FuncRParamsAST();
    func_rparams_ast->exp = unique_ptr<BaseAST>($1);
    func_rparams_ast->vec_exp = nullptr;
    $$ = func_rparams_ast;
  }
  ;

VecExp
  : ',' Exp {
    auto vec_exp = new vector<unique_ptr<BaseAST> >();
    auto exp = unique_ptr<BaseAST>($2);
    vec_exp->push_back(move(exp));
    $$ = vec_exp;
  }
  | VecExp ',' Exp {
    auto vec_exp = $1;
    auto exp = unique_ptr<BaseAST>($3);
    vec_exp->push_back(move(exp));
    $$ = vec_exp;
  }
  ;

UnaryOp
  : '+' {
    $$ = new string("+");
  }
  | '-' {
    $$ = new string("-");
  }
  | '!' {
    $$ = new string("!");
  }
  ;

MulExp
  : UnaryExp {
    auto mulexp_ast = new MulExpAST();
    mulexp_ast->mul_ptr = unique_ptr<BaseAST>($1);
    $$ = mulexp_ast;
  }
  | MulExp MulOp UnaryExp {
    auto mulexp_ast = new MulExpAST();
    auto mulopexp_ast = new MulOpExpAST();
    mulopexp_ast->mul_exp = unique_ptr<BaseAST>($1);
    mulopexp_ast->mul_op = *unique_ptr<string>($2);
    mulopexp_ast->unary_exp = unique_ptr<BaseAST>($3);
    mulexp_ast->mul_ptr = unique_ptr<BaseAST>(mulopexp_ast);
    $$ = mulexp_ast;
  }
  ;

MulOp
  : '*' {
    $$ = new string("*");
  }
  | '/' {
    $$ = new string("/");
  }
  | '%' {
    $$ = new string("%");
  }
  ;

AddExp
  : MulExp {
    auto addexp_ast = new AddExpAST();
    addexp_ast->add_ptr = unique_ptr<BaseAST>($1);
    $$ = addexp_ast;
  }
  | AddExp AddOp MulExp {
    auto addexp_ast = new AddExpAST();
    auto addopexp_ast = new AddOpExpAST();
    addopexp_ast->add_exp = unique_ptr<BaseAST>($1);
    addopexp_ast->add_op = *unique_ptr<string>($2);
    addopexp_ast->mul_exp = unique_ptr<BaseAST>($3);
    addexp_ast->add_ptr = unique_ptr<BaseAST>(addopexp_ast);
    $$ = addexp_ast;
  }
  ;

AddOp
  : '+' {
    $$ = new string("+");
  }
  | '-' {
    $$ = new string("-");
  }
  ;

RelExp
  : AddExp {
    auto relexp_ast = new RelExpAST();
    relexp_ast->rel_ptr = unique_ptr<BaseAST>($1);
    $$ = relexp_ast;
  }
  | RelExp RelOp AddExp {
    auto relexp_ast = new RelExpAST();
    auto relopexp_ast = new RelOpExpAST();
    relopexp_ast->rel_exp = unique_ptr<BaseAST>($1);
    relopexp_ast->rel_op = *unique_ptr<string>($2);
    relopexp_ast->add_exp = unique_ptr<BaseAST>($3);
    relexp_ast->rel_ptr = unique_ptr<BaseAST>(relopexp_ast);
    $$ = relexp_ast;
  }
  ;

RelOp
  : '<' {
    $$ = new string("<");
  }
  | '>' {
    $$ = new string(">");
  }
  | REL_OPERATOR {
    $$ = $1;
  }
  ;

EqExp
  : RelExp {
    auto eqexp_ast = new EqExpAST();
    eqexp_ast->eq_ptr = unique_ptr<BaseAST>($1);
    $$ = eqexp_ast;
  }
  | EqExp EqOp RelExp {
    auto eqexp_ast = new EqExpAST();
    auto eqopexp_ast = new EqOpExpAST();
    eqopexp_ast->eq_exp = unique_ptr<BaseAST>($1);
    eqopexp_ast->eq_op = *unique_ptr<string>($2);
    eqopexp_ast->rel_exp = unique_ptr<BaseAST>($3);
    eqexp_ast->eq_ptr = unique_ptr<BaseAST>(eqopexp_ast);
    $$ = eqexp_ast;
  }
  ;

EqOp
  : EQ_OPERATOR {
    $$ = $1;
  }

LAndExp
  : EqExp {
    auto landexp_ast = new LAndExpAST();
    landexp_ast->land_ptr = unique_ptr<BaseAST>($1);
    $$ = landexp_ast;
  }
  | LAndExp LAndOp EqExp {
    auto landexp_ast = new LAndExpAST();
    auto landopexp_ast = new LAndOpExpAST();
    landopexp_ast->land_exp = unique_ptr<BaseAST>($1);
    landopexp_ast->land_op = *unique_ptr<string>($2);
    landopexp_ast->eq_exp = unique_ptr<BaseAST>($3);
    landexp_ast->land_ptr = unique_ptr<BaseAST>(landopexp_ast);
    $$ = landexp_ast;
  }
  ;

LAndOp
  : LAND_OPERATOR {
    $$ = $1;
  }
  ;

LOrExp
  : LAndExp {
    auto lorexp_ast = new LOrExpAST();
    lorexp_ast->lor_ptr = unique_ptr<BaseAST>($1);
    $$ = lorexp_ast;
  }
  | LOrExp LOrOp LAndExp {
    auto lorexp_ast = new LOrExpAST();
    auto loropexp_ast = new LOrOpExpAST();
    loropexp_ast->lor_exp = unique_ptr<BaseAST>($1);
    loropexp_ast->lor_op = *unique_ptr<string>($2);
    loropexp_ast->land_exp = unique_ptr<BaseAST>($3);
    lorexp_ast->lor_ptr = unique_ptr<BaseAST>(loropexp_ast);
    $$ = lorexp_ast;
  }
  ;

LOrOp
  : LOR_OPERATOR {
    $$ = $1;
  }
  ;

ConstExp
  : Exp {
    auto const_exp_ast = new ConstExpAST();
    const_exp_ast->exp = unique_ptr<BaseAST>($1);
    $$ = const_exp_ast;
  }
  ;

%%

void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
