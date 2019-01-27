#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ASSEMBLY_SIZE 2048   // maximum number of lines of assembly file
#define MAX_ASSEMBLY_LENGTH 128  // maximum length of a line
#define MAX_LABEL_SIZE 1024      // maximum number of labels
#define MAX_LABEL_LENGTH 30      // maximum length of label

enum {
    OP_HALT = 0x00,
    OP_NOP = 0x01,
    OP_ADDI = 0x02,
    OP_MOVEREG = 0x03,
    OP_MOVEI = 0x04,
    OP_LW = 0x05,
    OP_SW = 0x06,
    OP_BLEZ = 0x07,
    OP_LA = 0x08,
    OP_PUSH = 0x09,
    OP_POP = 0x0a,
    OP_ADD = 0x0b,
    OP_JMP = 0x0c,
    OP_IRET = 0x10,
    OP_PUT = 0x11,
    OP_NONE = 0xff,
};

void handle_line(char*, int*);
int is_symbol(char*);
int first_char(char*, int, char);
int first_whitespace(char*, int);
int first_non_whitespace(char*, int);
void remove_whitespace(char*);
void parse(char*, int, uint8_t*);
uint8_t parse_reg(char*, int);
int8_t parse_imm(char*);
int parse_label(char*);
int parse_sti(char*, uint8_t*, uint8_t*, int8_t*);
void throw_syntax_error(int);
void print_label_table();
void print_code();
char code[MAX_ASSEMBLY_SIZE][MAX_ASSEMBLY_LENGTH];  // code table
char label[MAX_LABEL_SIZE][MAX_LABEL_LENGTH];       // label table
int label_address[MAX_LABEL_SIZE];                  // label address table
int code_size = 0, label_size = 0;

int main(int argc, char** args) {
    if (argc != 3) {
        printf("Usage: %s assembly_prog executable_prog\n", args[0]);
        exit(EXIT_FAILURE);
    }

    /*
    Begin Phase 1: In phase one, the assembler read the asm file, build label
    table, store code and data in 'code', and remove redundant blank lines.
    */
    FILE* fp = fopen(args[1], "r");  // Open source file
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int address = 0;

    while ((read = getline(&line, &len, fp)) != -1)
        handle_line(line, &address);

    if (line)
        free(line);

    fclose(fp);

#ifdef DEBUG
    print_label_table();
    print_code();
#endif
    /* End Phase 1 */

    /*
    Begin Phase 2: In phase two, the assembler parse each line of code, substitute
    label with address, and encode into hexcode.
    */

    // binary code, one line of assembly code becomes four uint8_t
    uint8_t bin[MAX_ASSEMBLY_SIZE * 4];
    int code_index = 0;
    for (; code_index < code_size; ++code_index) {
        parse(code[code_index], code_index, bin + code_index * 4);
    }

    // write to binary file
    FILE* fp_out = fopen(args[2], "wb");  // Open binary file for output
    fwrite(bin, sizeof(uint8_t), code_size * 4, fp);
    fclose(fp_out);
    /* End Phase 2 */

    return 0;
}

void handle_line(char* line, int* p_address) {
    int i = first_non_whitespace(line, 0);
    if ((i == -1) || line[i] == ';')
        ;
    else {
        int s_index = is_symbol(line);
        if (s_index) {
            // This line is a symbol, add to symbol table
            strncpy(label[label_size], line + i, s_index - i);
            label_address[label_size] = *p_address;
            label_size++;
        } else {
            // the line processed is a code/data, increment address
            *p_address = *p_address + 1;
            // is assembly code/data, store for pass 2
            strcpy(code[code_size++], line + i);
        }
    }
}

int is_symbol(char* line) {
    int i = first_non_whitespace(line, 0);
    if (line[i] == '.')
        return 0;  // Not a label if it starts with "."

    int code_end_idx = first_char(line, 0, ';');
    if (code_end_idx == -1)
        code_end_idx = strlen(line);
    while (isspace(line[code_end_idx - 1]))
        code_end_idx--;

    code_end_idx--;
    if (line[code_end_idx] == ':') {
        if (code_end_idx - i > MAX_LABEL_LENGTH) {
            // Exit failure if label is too long
            // Since we will exit failure, we can edit this line
            line[code_end_idx] = '\0';
            printf("Syntax Error: label %s is too long", line + i);
            exit(EXIT_FAILURE);
        } else {
            // check for validity of label
            while (i < code_end_idx) {
                if (!isalnum(line[i]) && line[i] != '_') {
                    line[code_end_idx] = '\0';
                    printf("Syntax Error: label %s is invalid", line + i);
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            // return the ending index of label
            return code_end_idx;
        }
    } else
        return 0;  // Did not find a label, this instruction is assembly code/data
}

/*
Look for the first char c starting from index i
*/
int first_char(char* str, int i, char c) {
    int len = strlen(str), flag = 0;
    for (; i < len; i++)
        if (str[i] == c) {
            flag = 1;
            break;
        }
    return (flag) ? i : -1;
}

int first_whitespace(char* str, int i) {
    int len = strlen(str), flag = 0;
    for (; i < len; i++)
        if (isspace(str[i])) {
            flag = 1;
            break;
        }
    return (flag) ? i : -1;
}

int first_non_whitespace(char* str, int i) {
    int len = strlen(str), flag = 0;
    for (; i < len; i++)
        if (!isspace(str[i])) {
            flag = 1;
            break;
        }
    return (flag) ? i : -1;
}

void remove_whitespace(char* line) {
    while (*line != '\0') {
        if (isspace(*line)) {
            for (int i = 0; i < strlen(line); ++i)
                line[i] = line[i + 1];
        } else
            line++;
    }
}

// TODO parser
void parse(char* line, int idx, uint8_t* bin) {
    // Since we omit preceeding whitespace in phase 1, we can assert that the
    // first character must be a valid one

    if (line[0] == '.') {
        // Parse .word
        if (!strncmp(line + 1, "word", 4)) {
            // starting with .word
            if (isspace(line[5])) {
                // whitespace after .word, correct syntax
                int word_end_idx, word_begin_idx = 6;
                // locate the first non-whitespace character
                while (isspace(line[word_begin_idx]))
                    word_begin_idx++;
                if ((line[word_begin_idx] != '-') && !isdigit(line[word_begin_idx])) {
                    throw_syntax_error(idx);
                } else {
                    word_end_idx = word_begin_idx + 1;
                    // if the first non-whitespace character is valid, locate the ending
                    while (isdigit(line[word_end_idx])) {
                        word_end_idx++;
                    }
                    // if the ending is not whitespace, then throw error
                    if (!isspace(line[word_end_idx]) && line[word_end_idx] != ';') {
                        throw_syntax_error(idx);
                    } else {
                        int last_idx = first_non_whitespace(line, word_end_idx);
                        if (last_idx == -1 || line[last_idx] == '\0' || line[last_idx] == ';')
                            ;
                        else {
                            throw_syntax_error(idx);
                        }
                    }
                }
                line[word_end_idx] = '\0';  // set the ending of word
                uint32_t value = strtol(line + word_begin_idx, NULL, 10);
                uint8_t* p_u8_value = (uint8_t*) &value;
                *bin = *(p_u8_value);
                *(bin + 1) = *(p_u8_value + 1);
                *(bin + 2) = *(p_u8_value + 2);
                *(bin + 3) = *(p_u8_value + 3);
            } else {
                // incorrect syntax
                throw_syntax_error(idx);
            }
        } else {
            throw_syntax_error(idx);
        }
    } else {
        // Parse opcode
        // remove comments
        int code_end_idx = first_char(line, 0, ';');
        if (code_end_idx == -1)
            code_end_idx = strlen(line);
        while (isspace(line[code_end_idx - 1]))
            code_end_idx--;
        line[code_end_idx] = '\0';

        switch (code_end_idx) {
        case 3:
            // must be NOP, or syntax error
            if (!strcmp(line, "NOP")) {
                *bin = *(bin + 1) = *(bin + 2) = 0x00;
                *(bin + 3) = OP_NOP;
            } else
                throw_syntax_error(idx);
            return;

        case 4:
            // must be halt or iret, or syntax error
            if (!strcmp(line, "halt")) {
                *bin = *(bin + 1) = *(bin + 2) = 0x00;
                *(bin + 3) = OP_HALT;
            } else if (!strcmp(line, "iret")) {
                *bin = *(bin + 1) = *(bin + 2) = 0x00;
                *(bin + 3) = OP_IRET;
            } else
                throw_syntax_error(idx);
            return;
        }

        // not among the simple three, more complicated logic needed
        int op_end_idx = first_whitespace(line, 0);
        // remove all whitespace from the character after the first whitespace
        remove_whitespace(line + op_end_idx + 1);
        uint8_t sreg, treg;
        int8_t imm;
        switch (op_end_idx) {
        case 2:
            if (!strncmp(line, "lw", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, &sreg, &treg, &imm) == -1)
                    throw_syntax_error(idx);
                *bin = imm;
                *(bin + 1) = treg;
                *(bin + 2) = sreg;
                *(bin + 3) = OP_LW;
            } else if (!strncmp(line, "sw", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, &sreg, &treg, &imm) == -1)
                    throw_syntax_error(idx);
                *bin = imm;
                *(bin + 1) = treg;
                *(bin + 2) = sreg;
                *(bin + 3) = OP_SW;
            } else if (!strncmp(line, "la", op_end_idx)) {
                int comma_sep_idx = first_char(line + op_end_idx + 1, 0, ',');
                if (comma_sep_idx == -1)
                    throw_syntax_error(idx);
                int addr = parse_label(line + op_end_idx + 1 + comma_sep_idx + 1);
                if (addr == -1)
                    throw_syntax_error(idx);
                else
                    imm = addr - idx - 1;
                treg = parse_reg(line + op_end_idx + 1, comma_sep_idx);
                *bin = imm;
                *(bin + 1) = treg;
                *(bin + 2) = 0x00;
                *(bin + 3) = OP_LA;
            } else
                throw_syntax_error(idx);
            return;

        case 3:
            if (!strncmp(line, "jmp", op_end_idx)) {
                int addr = parse_label(line + op_end_idx + 1);
                if (addr == -1)
                    throw_syntax_error(idx);
                else
                    imm = addr - idx - 1;

                *bin = imm;
                *(bin + 1) = 0x00;
                *(bin + 2) = 0x00;
                *(bin + 3) = OP_JMP;
            } else if (!strncmp(line, "pop", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, NULL, &treg, NULL) == -1)
                    throw_syntax_error(idx);
                *bin = 0x00;
                *(bin + 1) = treg;
                *(bin + 2) = 0x00;
                *(bin + 3) = OP_POP;
            } else if (!strncmp(line, "put", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, &sreg, NULL, NULL) == -1)
                    throw_syntax_error(idx);
                *bin = 0x00;
                *(bin + 1) = 0x00;
                *(bin + 2) = sreg;
                *(bin + 3) = OP_PUT;
            } else if (!strncmp(line, "add", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, &sreg, &treg, NULL) == -1)
                    throw_syntax_error(idx);
                *bin = 0x00;
                *(bin + 1) = treg;
                *(bin + 2) = sreg;
                *(bin + 3) = OP_ADD;
            } else
                throw_syntax_error(idx);
            return;

        case 4:
            if (!strncmp(line, "push", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, &sreg, NULL, NULL) == -1)
                    throw_syntax_error(idx);
                *bin = 0x00;
                *(bin + 1) = 0x00;
                *(bin + 2) = sreg;
                *(bin + 3) = OP_PUSH;
            } else if (!strncmp(line, "blez", op_end_idx)) {
                int comma_sep_idx = first_char(line + op_end_idx + 1, 0, ',');
                if (comma_sep_idx == -1)
                    throw_syntax_error(idx);
                int addr = parse_label(line + op_end_idx + 1 + comma_sep_idx + 1);
                if (addr == -1)
                    throw_syntax_error(idx);
                else
                    imm = addr - idx - 1;
                sreg = parse_reg(line + op_end_idx + 1, comma_sep_idx);
                *bin = imm;
                *(bin + 1) = 0x00;
                *(bin + 2) = sreg;
                *(bin + 3) = OP_BLEZ;
            } else if (!strncmp(line, "addi", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, &sreg, &treg, &imm) == -1)
                    throw_syntax_error(idx);
                *bin = imm;
                *(bin + 1) = treg;
                *(bin + 2) = sreg;
                *(bin + 3) = OP_ADDI;
            } else
                throw_syntax_error(idx);
            return;

        case 5:
            if (!strncmp(line, "movei", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, NULL, &treg, &imm) == -1)
                    throw_syntax_error(idx);
                *bin = imm;
                *(bin + 1) = treg;
                *(bin + 2) = 0x00;
                *(bin + 3) = OP_MOVEI;
            } else
                throw_syntax_error(idx);
            return;

        case 8:
            if (!strncmp(line, "move_reg", op_end_idx)) {
                if (parse_sti(line + op_end_idx + 1, &sreg, &treg, NULL) == -1)
                    throw_syntax_error(idx);
                *bin = 0x00;
                *(bin + 1) = treg;
                *(bin + 2) = sreg;
                *(bin + 3) = OP_MOVEREG;
            } else
                throw_syntax_error(idx);
            return;
        }
        throw_syntax_error(idx);
    }
}

/*
Parse register number
return -1 if invalid, otherwise return the register number
R0-R63 is general purpose register and sp (stack pointer) is R64
*/
uint8_t parse_reg(char* r, int l) {
    if (l == 2 && r[0] == 's' && r[1] == 'p')
        return 64;
    if (*r != 'R' || l > 3 || l < 2)
        return -1;
    else {
        char s[3];  // make a copy of r
        s[0] = r[1];
        if (l == 2)
            s[1] = '\0';
        else {  // l == 3
            s[1] = r[2];
            s[2] = '\0';
        }
        int reg_n = strtol(s, NULL, 10);
        if (reg_n < 0 || reg_n > 63)
            return -1;
        else
            return (uint8_t) reg_n;
    }
}

/*
Parse immediate number
*/
int8_t parse_imm(char* r) {
    int8_t value = (int8_t) strtol(r, NULL, 10);  // force convert to int8
    return value;
}

/*
Loop up label tabel then return address if found
Otherwise return -1
*/
int parse_label(char* lab) {
    for (int i = 0; i < label_size; ++i) {
        if (strcmp(lab, label[i]) == 0) {
            return label_address[i];
        }
    }
    return -1;
}

/*
The input line is guaranteed to be a string without whitespace,
separated by comma if needed.
Returns -1 when syntax error happed
*/
int parse_sti(char* line, uint8_t* p_sreg, uint8_t* p_treg, int8_t* p_imm) {
    if (p_sreg != NULL && p_treg != NULL && p_imm != NULL) {
        // lw, sw, addi
        int first_comma_sep_idx = first_char(line, 0, ',');
        if (first_comma_sep_idx == -1)
            return -1;
        int second_comma_sep_idx = first_char(line, first_comma_sep_idx + 1, ',');
        if (second_comma_sep_idx == -1)
            return -1;
        *p_sreg = parse_reg(line, first_comma_sep_idx - 0);
        *p_treg = parse_reg(line + first_comma_sep_idx + 1, second_comma_sep_idx - first_comma_sep_idx - 1);
        *p_imm = parse_imm(line + second_comma_sep_idx + 1);
    }

    if (p_sreg == NULL && p_treg != NULL && p_imm != NULL) {
        // movei
        int first_comma_sep_idx = first_char(line, 0, ',');
        if (first_comma_sep_idx == -1)
            return -1;
        *p_treg = parse_reg(line, first_comma_sep_idx - 0);
        *p_imm = parse_imm(line + first_comma_sep_idx + 1);
    }

    if (p_sreg != NULL && p_treg == NULL && p_imm == NULL) {
        // push, put
        *p_sreg = parse_reg(line, strlen(line));
    }

    if (p_sreg != NULL && p_treg != NULL && p_imm == NULL) {
        // add, move_reg
        int first_comma_sep_idx = first_char(line, 0, ',');
        if (first_comma_sep_idx == -1)
            return -1;
        *p_sreg = parse_reg(line, first_comma_sep_idx - 0);
        *p_treg = parse_reg(line + first_comma_sep_idx + 1, strlen(line) - first_comma_sep_idx - 1);
    }

    if (p_sreg == NULL && p_treg != NULL && p_imm == NULL) {
        // pop
        *p_treg = parse_reg(line, strlen(line));
    }
}

void throw_syntax_error(int idx) {
    printf("Syntax Error: in line %d", idx);
    exit(EXIT_FAILURE);
}

void print_label_table() {
    printf("--------LABEL TABLE--------\n");
    for (int i = 0; i < label_size; i++) {
        printf("Label: %s \t Address: %d\n", label[i], label_address[i]);
    }
}

void print_code() {
    printf("--------CODE--------\n");
    for (int i = 0; i < code_size; i++) {
        printf("%s", code[i]);
    }
}