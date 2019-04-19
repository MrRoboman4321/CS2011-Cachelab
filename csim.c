#include "cachelab.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <math.h>

#define DEBUG 1

void print_usage();
int calc_tbits();

typedef struct cache_performance {
    int hits;
    int misses;
    int evictions;
} cache_performance;

typedef struct line {
    bool valid;
    unsigned long long tag; //ensure 64-bit address compatibility
} line;


typedef struct set {
    int id;
    line *lines;
} set;

typedef struct cache {
    set *sets;
} cache;

typedef struct lru_node {
    struct lru_node *prev;
    struct lru_node *next;
    int idx; //Index into the array of lines in the set
} lru_node;

int main(int argc, char *argv[])
{
    bool help_flag = false;
    bool verbose_flag = false;
    int s = -1;
    int lines_per_set = -1;
    int bytes_per_line = -1;
    char *trace_file = (char *) NULL;

    int opt;
    char *p;

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
                s = strtol(optarg, &p, 10);
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

    if(s == -1 || lines_per_set == -1 || bytes_per_line == -1 || trace_file == (char *) NULL) {
        print_usage();
        exit(0);
    }

    if(DEBUG) {
        printf("Help set: %s\n", help_flag ? "true" : "false");
        printf("Verbose flag set: %s\n", verbose_flag ? "true" : "false");
        printf("s: %d\n", s);
        printf("Lines per set: %d\n", lines_per_set);
        printf("Bytes per line: %d\n", bytes_per_line);
        printf("Trace file: %s\n", trace_file);
    }

    lru_node *lru_tracker[(int) pow(2, s)];

    for(int i = 0; i < pow(2, s); i++) {
        lru_node *cur_node = (lru_node *) malloc(sizeof(lru_node));
        cur_node->idx = 0;
        lru_tracker[i] = cur_node;

        for(int j = 0; j < lines_per_set - 1; j++) {
            lru_node *next_node = (lru_node *) malloc(sizeof(lru_node));
            next_node->idx = j + 1;
            cur_node->next = next_node;
        }
    }

    cache *simulated_cache = (cache *) malloc(sizeof(cache));
    simulated_cache->sets  = (set *)   malloc(sizeof(set) * pow(2, s));

    for(int i = 0; i < pow(2, s); i++) {
        simulated_cache->sets[i].lines = (line *) malloc(sizeof(line) * lines_per_set);
    }

    printf("Using LRU tracker: %d\n", lru_tracker[0]->idx);

    printSummary(0, 0, 0);
    return 0;
}

/*
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
}*/

void print_usage() {
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
}
