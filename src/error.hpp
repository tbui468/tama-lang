#ifndef TMD_ERROR_H
#define TMD_ERROR_H


struct Error {
    char* msg;
    int line;
};


struct ErrorMsgs {
    struct Error *errors;
    int count;
    int max_count;
};


extern struct ErrorMsgs ems;

void ems_init(struct ErrorMsgs *ems);
void ems_free(struct ErrorMsgs *ems);
void ems_add(struct ErrorMsgs *ems, int line, char* format, ...);
void ems_sort(struct ErrorMsgs *ems); //this can be static
void ems_print(struct ErrorMsgs *ems);

#endif //TMD_ERROR_H
