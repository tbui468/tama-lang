#ifndef TMD_COMPILER_H
#define TMD_COMPILER_H

#include "token.h"
#include "byte_array.h"
#include "ast.h"

struct VarData {
    struct Token var;
    struct Token type;
    int bp_offset;
};


struct VarDataArray {
    struct VarData *vds;
    int count;
    int max_count;
    struct VarDataArray* next;
};


struct Compiler {
    struct ByteArray text;
    struct ByteArray data;
    int data_offset; //in bytes TODO: Not using this, are we?
    struct VarDataArray *head;
    unsigned conditional_label_id; //used to create unique ids for labels in assembly code
};


void vda_init(struct VarDataArray *vda);
void vda_free(struct VarDataArray *vda);
void vda_add(struct VarDataArray *vda, struct VarData vd);
struct VarData* vda_get_local(struct VarDataArray *vda, struct Token var);

void compiler_init(struct Compiler *c);
void compiler_free(struct Compiler *c);
void compiler_decl_local(struct Compiler *c, struct Token var, struct Token type);
struct VarData* compiler_get_local(struct Compiler* c, struct Token var);
void compiler_begin_scope(struct Compiler *c);
int compiler_end_scope(struct Compiler *c);
void compiler_output_assembly(struct Compiler *c);
void write_op(struct Compiler *c, const char* format, ...);
enum TokenType compiler_compile(struct Compiler *c, struct Node *n);


#endif //TMD_COMPILER_H
