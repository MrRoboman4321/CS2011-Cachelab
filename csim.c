#include "cachelab.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 1

void print_usage();

typedef struct cache_performance {
    int hits;
    int misses;
    int evictions;
};

int main(int argc, char *argv[])
{
    struct cache_performance *data;

    bool help_flag = false;
    bool verbose_flag = false;
    int sets = -1;
    int lines_per_set = -1;
    int bytes_per_line = -1;
    char *trace_file = (char *) NULL;

    int opt;

    while((opt = getopt(argc, argv, ":hvsEbt")) != -1) {
        switch(opt) {
            case 'h':
                help_flag = true;
                break;
            case 'v':
                verbose_flag = true;
                break;
            case 's':
                sets = strtol(optarg, (char **) NULL, 10);
                break;
            case 'E':
                lines_per_set = strtol(optarg, (char **) NULL, 10);
                break;
            case 'b':
                bytes_per_line = strtol(optarg, (char **) NULL, 10);
                break;
            case 't':
                trace_file = optarg;
                break;
        }
    }

    if(sets == -1 || lines_per_set == -1 || bytes_per_line == -1 || trace_file == (char *) NULL) {
        print_usage();
        exit(0);
    }

    if(DEBUG) {
        printf("Help set: %s\n", help_flag ? "true" : "false");
        printf("Verbose flag set: %s\n", verbose_flag ? "true" : "false");
        printf("Sets: %d", sets);
        printf("Lines per set: %d", lines_per_set);
        printf("Bytes per line: %d", bytes_per_line);
        printf("Trace file: %s", trace_file);
    }

    printSummary(0, 0, 0);
    return 0;
}

void print_usage() {
    printf("Usage: ./csmin [-hv] -s <s> -E <E> -b <b> -t <tracefile>");
}