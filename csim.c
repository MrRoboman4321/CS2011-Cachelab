#include "cachelab.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#define DEBUG 1

void print_usage();
int calc_tbits();

typedef struct cache_performance {
    int hits;
    int misses;
    int evictions;
} cache_performance;

int main(int argc, char *argv[])
{
    printf("What is happening");

    bool help_flag = false;
    bool verbose_flag = false;
    int sets = -1;
    int lines_per_set = -1;
    int bytes_per_line = -1;
    char *trace_file = (char *) NULL;

    int opt;
    char *p;

    printf("Before loop");

    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch(opt) {
            case 'h':
                help_flag = true;
                break;
            case 'v':
                verbose_flag = true;
                break;
            case 's':
                printf("parsing s...");
                sets = strtol(optarg, &p, 10);
                break;
            case 'E':
                lines_per_set = strtol(optarg, &p, 10);
                break;
            case 'b':
                bytes_per_line = strtol(optarg, &p, 10);
                break;
            case 't':
                trace_file = optarg;
                break;
            default:
                printf("other arg passed\n");
                break;
        }
    }

    printf("After arg parsing\n");

    /*if(sets == -1 || lines_per_set == -1 || bytes_per_line == -1 || trace_file == (char *) NULL) {
        print_usage();
        exit(0);
    }*/

    if(DEBUG) {
        printf("Help set: %s\n", help_flag ? "true" : "false");
        printf("Verbose flag set: %s\n", verbose_flag ? "true" : "false");
        printf("Sets: %d\n", sets);
        printf("Lines per set: %d\n", lines_per_set);
        printf("Bytes per line: %d\n", bytes_per_line);
        printf("Trace file: %s\n", trace_file);
    }

    printSummary(0, 0, 0);
    return 0;
}

int calc_tbits(char *trace) {
    char *string;
    unsigned long long int hexAddress;  //64-bit addresses
    int decBytes;
    unsigned long long int currentMax = 0;
    unsigned long long int currentMin = ~0;
    FILE *filepointer;
    filepointer = fopen(trace, "r");
    while(fscanf(filepointer, " %s %x,%d", string, hexAddress, decBytes) != -1) {
        if (hexAddress > currentMax) {
            currentMax = hexAddress;
        } else if (hexAddress < currentMin) {
            currentMin = hexAddress;
        }
    }
    return 0;
}

void print_usage() {
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
}
