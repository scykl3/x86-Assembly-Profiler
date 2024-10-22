#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

unsigned short *frequency_tracker_memory = NULL;
size_t start_address;
long frequency_entries;
char *executable_name = NULL;

extern int __ehdr_start;
extern int __data_start;

void initialize_tracker() {
    start_address = (size_t)&__ehdr_start;
    size_t end_address = (size_t)&__data_start;
    frequency_entries = (long)(end_address - start_address);

    char path[1024];
    ssize_t path_len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (path_len == -1) {
        perror("Failed to retrieve executable path");
        exit(1);
    }
    path[path_len] = '\0';
    executable_name = strdup(path);

    unsigned short *frequency_table = (unsigned short *)calloc(frequency_entries, sizeof(unsigned short));
    if (!frequency_table) {
        perror("Memory allocation failed for tracker");
        exit(1);
    }

    frequency_tracker_memory = frequency_table;

    unsigned int scale_factor = 65536;
    profil(frequency_table, frequency_entries * sizeof(unsigned short), start_address, scale_factor);
}

void output_tracker() {
    if (!frequency_tracker_memory) {
        fprintf(stderr, "No tracker data available\n");
        return;
    }

    char output_filename[1024];
    snprintf(output_filename, sizeof(output_filename), "%s_freq_table.txt", executable_name);

    FILE *output_file = fopen(output_filename, "w");
    if (!output_file) {
        perror("Failed to open output file");
        return;
    }

    for (int i = 0; i < frequency_entries; ++i) {
        if (frequency_tracker_memory[i] != 0) {
            fprintf(output_file, "0x%lx\t%4dms\n", start_address + 2 * i, frequency_tracker_memory[i] * 10);
        }
    }

    fclose(output_file);
    free(frequency_tracker_memory);
    frequency_tracker_memory = NULL;
}
