// Yusuf Dogukan Dogan 251ADB239
// (optional) https://github.com/dogukandgn0/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
/* 
Resources used:
Gemini AI
Recursive Descent Parser: https://share.gemini.google/lKlWNrSENySk
W3schools Guides
Recursion: https://www.w3schools.com/c/c_functions_recursion.php
C Reallocate Memory: https://www.w3schools.com/c/c_memory_reallocate.php
C math: https://www.w3schools.com/c/c_ref_math.php
C Structures: https://www.w3schools.com/c/c_structs.php
C Enumeration: https://www.w3schools.com/c/c_enums.php
*/ 
// Recursive descent parser, float handling, directory scanning and exact position rules implemented.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

const char *STUDENT_INFO = "yusufdogukan_dogan_251239";

//TOKENIZER & STRUCTURES

typedef enum {
    TOKEN_NUMBER, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV, TOKEN_POW,
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_NEWLINE, TOKEN_EOF, TOKEN_ERROR
} TokenType;

typedef struct {
    int is_float; 
    long long i_val;
    double f_val;
} Value;

typedef struct {
    TokenType type;  
    Value val; 
    size_t position; 
} Token;

const char *text_buffer;
size_t current_index = 0;
size_t global_pos = 1;
int is_start_of_line = 1; 

Token get_next_token() {
    Token token;
    while (text_buffer[current_index] != '\0') {
        char c = text_buffer[current_index];
        
        if (c == ' ' || c == '\t' || c == '\r') { current_index++; global_pos++; continue; }
        
        if (c == '#') {
            if (is_start_of_line) {
                while (text_buffer[current_index] != '\n' && text_buffer[current_index] != '\0') {
                    current_index++; global_pos++;
                }
                continue; 
            } else { 
                token.type = TOKEN_ERROR; token.position = global_pos; 
                current_index++; global_pos++; 
                return token; 
            }
        }
        
        if (c == '\n') {
            token.type = TOKEN_NEWLINE; token.position = global_pos;
            current_index++; global_pos++; is_start_of_line = 1; return token;
        }
        
        is_start_of_line = 0;
        token.position = global_pos; 
        
        if (c == '+') { token.type = TOKEN_PLUS; current_index++; global_pos++; return token; }
        if (c == '-') { token.type = TOKEN_MINUS; current_index++; global_pos++; return token; }
        if (c == '/') { token.type = TOKEN_DIV; current_index++; global_pos++; return token; }
        if (c == '(') { token.type = TOKEN_LPAREN; current_index++; global_pos++; return token; }
        if (c == ')') { token.type = TOKEN_RPAREN; current_index++; global_pos++; return token; }
        
        if (c == '*') {
            if (text_buffer[current_index + 1] == '*') {
                token.type = TOKEN_POW; current_index += 2; global_pos += 2; return token;
            } else { token.type = TOKEN_MUL; current_index++; global_pos++; return token; }
        }
        
        if ((c >= '0' && c <= '9') || c == '.') {
            token.type = TOKEN_NUMBER;
            token.val.is_float = 0;
            size_t start_idx = current_index;
            
            if (c == '.') { 
                token.type = TOKEN_ERROR; 
                current_index++; global_pos++; 
                return token; 
            }
            
            while ((text_buffer[current_index] >= '0' && text_buffer[current_index] <= '9') || text_buffer[current_index] == '.') {
                if (text_buffer[current_index] == '.') {
                    if (token.val.is_float) { 
                        token.type = TOKEN_ERROR; token.position = global_pos; 
                        current_index++; global_pos++; 
                        return token; 
                    }
                    token.val.is_float = 1;
                    if (text_buffer[current_index+1] < '0' || text_buffer[current_index+1] > '9') {
                        token.type = TOKEN_ERROR; token.position = global_pos; 
                        current_index++; global_pos++; 
                        return token; 
                    }
                }
                current_index++; global_pos++;
            }
            
            if (text_buffer[current_index] == 'e' || text_buffer[current_index] == 'E') {
                token.type = TOKEN_ERROR; token.position = global_pos; 
                current_index++; global_pos++; 
                return token; 
            }
            
            char buffer[64];
            size_t len = current_index - start_idx;
            if (len >= sizeof(buffer)) len = sizeof(buffer) - 1;
            strncpy(buffer, &text_buffer[start_idx], len);
            buffer[len] = '\0';
            
            if (token.val.is_float) token.val.f_val = atof(buffer);
            else token.val.i_val = atoll(buffer);
            
            return token;
        }
        
        token.type = TOKEN_ERROR; 
        current_index++; global_pos++; 
        return token;
    }
    token.type = TOKEN_EOF; token.position = global_pos; return token;
}


//PARSER

Token current_token;
int parse_error = 0;
size_t error_pos = 0;

void advance() { current_token = get_next_token(); }
void set_error(size_t pos) { if (!parse_error) { parse_error = 1; error_pos = pos; } }

Value parse_expr(); 

Value parse_primary() {
    Value res = {0, 0, 0.0};
    if (current_token.type == TOKEN_NUMBER) {
        res = current_token.val; advance(); return res;
    } else if (current_token.type == TOKEN_LPAREN) {
        advance(); res = parse_expr();
        if (current_token.type == TOKEN_RPAREN) advance();
        else set_error(current_token.position);
        return res;
    }
    set_error(current_token.position); return res;
}

Value parse_power() {
    Value left = parse_primary();
    if (parse_error) return left;
    
    if (current_token.type == TOKEN_POW) {
        size_t op_pos = current_token.position; advance();
        size_t right_pos = current_token.position;
        Value right = parse_power();
        if (parse_error) return left;
        
        Value res;
        res.is_float = left.is_float || right.is_float;
        if (!res.is_float) {
            if (right.i_val < 0) { set_error(right_pos); return res; }
            long long p = 1; for(long long i=0; i<right.i_val; i++) p *= left.i_val;
            res.i_val = p;
        } else {
            double l = left.is_float ? left.f_val : (double)left.i_val;
            double r = right.is_float ? right.f_val : (double)right.i_val;
            res.f_val = pow(l, r);
            if (isnan(res.f_val) || isinf(res.f_val)) set_error(op_pos);
        }
        return res;
    }
    return left;
}

Value parse_term() {
    Value left = parse_power();
    while (!parse_error && (current_token.type == TOKEN_MUL || current_token.type == TOKEN_DIV)) {
        TokenType op = current_token.type;
        size_t op_pos = current_token.position; advance();
        size_t right_pos = current_token.position;
        Value right = parse_power();
        if (parse_error) break;
        
        Value res;
        res.is_float = left.is_float || right.is_float;
        
        if (!res.is_float) {
            if (op == TOKEN_MUL) res.i_val = left.i_val * right.i_val;
            else {
                if (right.i_val == 0) { set_error(right_pos); break; }
                res.i_val = left.i_val / right.i_val;
            }
        } else {
            double l = left.is_float ? left.f_val : (double)left.i_val;
            double r = right.is_float ? right.f_val : (double)right.i_val;
            if (op == TOKEN_MUL) res.f_val = l * r;
            else {
                if (r == 0.0) { set_error(right_pos); break; }
                res.f_val = l / r;
            }
            if (isnan(res.f_val) || isinf(res.f_val)) { set_error(op_pos); break; }
        }
        left = res;
    }
    return left;
}

Value parse_expr() {
    Value left = parse_term();
    while (!parse_error && (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS)) {
        TokenType op = current_token.type; advance();
        Value right = parse_term();
        if (parse_error) break;
        
        Value res;
        res.is_float = left.is_float || right.is_float;
        if (!res.is_float) {
            if (op == TOKEN_PLUS) res.i_val = left.i_val + right.i_val;
            else res.i_val = left.i_val - right.i_val;
        } else {
            double l = left.is_float ? left.f_val : (double)left.i_val;
            double r = right.is_float ? right.f_val : (double)right.i_val;
            if (op == TOKEN_PLUS) res.f_val = l + r;
            else res.f_val = l - r;
        }
        left = res;
    }
    return left;
}

//FILE IO
char *read_entire_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) return NULL;
    size_t capacity = 4096, size = 0, bytes_read;
    char *buffer = malloc(capacity);
    while (buffer && (bytes_read = fread(buffer + size, 1, capacity - size - 1, file)) > 0) {
        size += bytes_read;
        if (size + 1 == capacity) { capacity *= 2; buffer = realloc(buffer, capacity); }
    }
    fclose(file);
    if (buffer) buffer[size] = '\0';
    return buffer;
}

void process_file(const char *input_path, const char *outdir) {
    char *content = read_entire_file(input_path);
    if (!content) { fprintf(stderr, "Error: Cannot read %s\n", input_path); exit(1); }
    
    const char *slash = strrchr(input_path, '/');
    const char *filename = slash ? slash + 1 : input_path;
    const char *dot = strrchr(filename, '.');
    size_t baselen = dot ? (size_t)(dot - filename) : strlen(filename);
    
    char out_path[1024];
    snprintf(out_path, sizeof(out_path), "%s/%.*s_%s.txt", outdir, (int)baselen, filename, STUDENT_INFO);
    
    FILE *fout = fopen(out_path, "w");
    if (!fout) { free(content); fprintf(stderr, "Error: Cannot write to %s\n", out_path); exit(1); }
    
    text_buffer = content;
    current_index = 0; global_pos = 1; is_start_of_line = 1;
    advance();
    
    int file_has_error = 0;
    size_t first_error_pos = 0;
    
    while (current_token.type != TOKEN_EOF) {
        if (current_token.type == TOKEN_NEWLINE) { advance(); continue; }
        parse_error = 0;
        Value result = parse_expr();
        
        if (!parse_error && current_token.type != TOKEN_NEWLINE && current_token.type != TOKEN_EOF) set_error(current_token.position);
        
        if (parse_error) {
            file_has_error = 1;
            first_error_pos = error_pos;
            break;
        } 
        
        if (result.is_float) fprintf(fout, "%.15g\n", result.f_val); 
        else fprintf(fout, "%lld\n", result.i_val);
        
        while (current_token.type != TOKEN_NEWLINE && current_token.type != TOKEN_EOF) advance();
        if (current_token.type == TOKEN_NEWLINE) advance();
    }

    if (file_has_error) {
        fclose(fout);
        fout = fopen(out_path, "w");
        fprintf(fout, "ERROR:%zu\n", first_error_pos);
    }
    
    fclose(fout); free(content);
}

//MAIN

int main(int argc, char *argv[]) {
    char *input = NULL, *dir = NULL, *outdir = NULL;
    
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) && i + 1 < argc) dir = argv[++i];
        else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) && i + 1 < argc) outdir = argv[++i];
        else if (!input) input = argv[i];
        else { fprintf(stderr, "Unknown option.\n"); return 1; }
    }
    
    if (!input && !dir) { fprintf(stderr, "Missing input.\n"); return 1; }
    
    char def_outdir[512];
    if (!outdir) {
        const char *base = dir ? dir : input;
        const char *slash = strrchr(base, '/');
        const char *name = slash ? slash + 1 : base;
        const char *dot = strrchr(name, '.');
        size_t len = dot ? (size_t)(dot - name) : strlen(name);
        snprintf(def_outdir, sizeof(def_outdir), "%.*s_%s", (int)len, name, STUDENT_INFO);
        outdir = def_outdir;
    }
    
    mkdir(outdir, 0777); 
    
    if (dir) {
        DIR *d = opendir(dir);
        if (!d) { fprintf(stderr, "Cannot open dir.\n"); return 1; }
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            size_t len = strlen(ent->d_name);
            if (len > 4 && strcmp(ent->d_name + len - 4, ".txt") == 0) {
                char in_path[1024];
                snprintf(in_path, sizeof(in_path), "%s/%s", dir, ent->d_name);
                process_file(in_path, outdir);
            }
        }
        closedir(d);
    } else {
        process_file(input, outdir);
    }
    
    return 0;
}