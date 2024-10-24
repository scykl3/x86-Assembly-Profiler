#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t pc;  // counter
    int time;
} FreqEntry;

typedef struct {
    char func_name[256];  // func name
    size_t start_addr;
    size_t end_addr;
    int total_time;
} FuncEntry;

void parse_tracker(const char *filename, FreqEntry **table, int *table_size);
void parse_nm_output(const char *prog_name, FuncEntry **funcs, int *func_count);
void accumulate_function_times(FreqEntry *table, int table_size, FuncEntry *funcs, int func_count);
void sort_functions_by_time(FuncEntry *funcs, int func_count);
void print_top_functions(FuncEntry *funcs, int func_count);
void print_assembly_times(const char *prog_name, FuncEntry *funcs, int func_count, FreqEntry *table, int table_size);

int compare_func(const void *a, const void *b) {
    FuncEntry *func_a = (FuncEntry *)a;
    FuncEntry *func_b = (FuncEntry *)b;
    return func_b->total_time - func_a->total_time;  // desc?
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Not enough inputs");
        return 1;
    }
    printf("Profile: %s\n", argv[1]);

    const char *prog_name = argv[1];
    FreqEntry *table = NULL;
    int table_size = 0;

    FuncEntry *funcs = NULL;
    int func_count = 0;

    char tracker_filename[1024];
    snprintf(tracker_filename, sizeof(tracker_filename), "%s_freq_table.txt", prog_name);

    parse_tracker(tracker_filename, &table, &table_size);

    parse_nm_output(prog_name, &funcs, &func_count);

    accumulate_function_times(table, table_size, funcs, func_count);

    sort_functions_by_time(funcs, func_count);

    print_top_functions(funcs, func_count);

    print_assembly_times(prog_name, funcs, func_count, table, table_size);

    free(table);
    free(funcs);

    return 0;
}

void parse_tracker(const char *filename, FreqEntry **table, int *table_size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("frequency file failed to open");
        exit(1);
    }

    int capacity = 100;
    *table = malloc(capacity * sizeof(FreqEntry));
    *table_size = 0;

    char line[256];
        size_t pc;
    int time;

        while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "0x%lx %dms", &pc, &time) == 2) {
            if (*table_size >= capacity) {
                capacity *= 2;
                *table = realloc(*table, capacity * sizeof(FreqEntry));
            }
            (*table)[*table_size].pc = pc;
            (*table)[*table_size].time = time;
            (*table_size)++;
        }
    }

    fclose(file);
}

void parse_nm_output(const char *prog_name, FuncEntry **funcs, int *func_count) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "nm -v %s", prog_name);
    FILE *nm_pipe = popen(cmd, "r");

    if (!nm_pipe) {
        perror("nm command failed");
        exit(1);
    }

    int capacity = 100;
    *funcs = malloc(capacity * sizeof(FuncEntry));
    *func_count = 0;

    size_t addr;
    char type;
    char name[256];

    while (fscanf(nm_pipe, "%lx %c %255s", &addr, &type, name) == 3) {
        if (type == 'T' || type == 't') {  // text symbols only
            if (*func_count >= capacity) {
                capacity *= 2;
                *funcs = realloc(*funcs, capacity * sizeof(FuncEntry));
            }
            (*funcs)[*func_count].start_addr = addr;
            strcpy((*funcs)[*func_count].func_name, name);
            (*funcs)[*func_count].total_time = 0;
            (*func_count)++;
        }
    }

    pclose(nm_pipe);

    for (int i = 0; i < *func_count - 1; i++) {
        (*funcs)[i].end_addr = (*funcs)[i + 1].start_addr - 1;
    }
    (*funcs)[*func_count - 1].end_addr = (size_t)-1;
}

void accumulate_function_times(FreqEntry *table, int table_size, FuncEntry *funcs, int func_count) {
    for (int i = 0; i < table_size; i++) {
        size_t pc = table[i].pc;
        int time = table[i].time;

        for (int j = 0; j < func_count; j++) {
            if (pc >= funcs[j].start_addr && pc < funcs[j].end_addr) {
                funcs[j].total_time += time;
                break;
            }
        }
    }
}

void sort_functions_by_time(FuncEntry *funcs, int func_count) {
    qsort(funcs, func_count, sizeof(FuncEntry), compare_func);
}

void print_top_functions(FuncEntry *funcs, int func_count) {
    printf("Top 10 functions:\n");
    printf("%-5s %-20s %-10s %-10s\n", "ith", "Function", "Time(ms)", "(%)");

    int total_time = 0;
    for (int i = 0; i < func_count; i++) {
        total_time += funcs[i].total_time;
    }

    for (int i = 0; i < 10 && i < func_count; i++) {
        float percent = (float)funcs[i].total_time / total_time * 100;
        printf("%-5d: %-20s %-10d %.2f%%\n", i + 1, funcs[i].func_name, funcs[i].total_time, percent);
    }
}

void print_assembly_times(const char *prog_name, FuncEntry *funcs, int func_count, FreqEntry *table, int table_size) {
    printf("\nTop 10 functions Assembly:\n");

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "objdump -d %s", prog_name);

        int total_time = 0;
        for (int i = 0; i < func_count; i++) {
                total_time += funcs[i].total_time;
        }

    for (int i = 0; i < 10 && i < func_count; i++) {
                float percent = (float)funcs[i].total_time / total_time * 100;
        printf("\n%d: %s %10dms %10.2f%%\n", i + 1, funcs[i].func_name, funcs[i].total_time, percent);
        size_t current_func_start = funcs[i].start_addr;
        size_t current_func_end = funcs[i].end_addr;

        printf("\nAssembly Instructions for function %s:\n", funcs[i].func_name);

        // reopen dump
        FILE *objdump_pipe = popen(cmd, "r");
        if (!objdump_pipe) {
            perror("objdump command failed");
            exit(1);
        }

        char line[1024];
        while (fgets(line, sizeof(line), objdump_pipe)) {
            size_t addr;
                        char hex[256];
            char instr[256];

            if (sscanf(line, " %lx:\t%255[^\t] %255[^\n]", &addr, hex, instr) == 3) {
                if (addr >= current_func_start && addr < current_func_end) {
                    // find time
                    int instruction_time = 0;
                    for (int j = 0; j < table_size; j++) {
                        if (table[j].pc == addr) {
                            instruction_time = table[j].time;
                            break;
                        }
                    }
                    printf("0x%lx:\t%-20s %-70s\t %dms\n", addr, hex, instr, instruction_time);
                }
            }
        }

        pclose(objdump_pipe);
    }
}
