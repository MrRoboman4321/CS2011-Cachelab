#include "cachelab.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <math.h>

#define DEBUG 1

typedef struct location {
    int set_id;
    unsigned long long tag_id;
} location;

//
enum HitOrMiss {HIT, COLD_MISS, MISS};    //I bet they never miss yuhh

void print_usage();
int calc_tbits();
void get_and_set_tag(struct location *loc, unsigned int address, int tbits, int sbits);

typedef struct cache_performance {
    int hits;
    int misses;
    int evictions;
} cache_performance;

cache_performance *simulate_cache();

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
    int lines_per_set;
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
    char *trace_path = (char *) NULL;

    FILE *trace_file;

    cache_performance *cp = (cache_performance *) malloc(sizeof(cache_performance));

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
                cache.lines_per_set = lines_per_set;
                break;
            case 'b':
                bytes_per_line = strtol(optarg, &p, 10);
                break;
            case 't':
                trace_path = optarg;
                break;
            default:
                printf("other arg passed\n");
                break;
        }
    }

    if(s == -1 || lines_per_set == -1 || bytes_per_line == -1 || trace_path == (char *) NULL) {
        print_usage();
        exit(0);
    }

    trace_file = fopen(trace_path, "r");

    if(trace_file == NULL) {
        printf("Invalid trace file path \"%s\".", trace_path);
        exit(0);
    }

    if(DEBUG || verbose_flag) {
        printf("Help set: %s\n", help_flag ? "true" : "false");
        printf("Verbose flag set: %s\n", verbose_flag ? "true" : "false");
        printf("s: %d\n", s);
        printf("Lines per set: %d\n", lines_per_set);
        printf("Bytes per line: %d\n", bytes_per_line);
        printf("Trace file: %s\n", trace_path);
    }

    lru_node *lru_tracker[(int) pow(2, s)];

    //Set up linked lists with length E for each set
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

    //Allocate memory to store the cache and sets
    cache *simulated_cache = (cache *) malloc(sizeof(cache));
    simulated_cache->sets  = (set *)   malloc(sizeof(set) * pow(2, s));

    //For each set, allocate memory for the lines within
    for(int i = 0; i < pow(2, s); i++) {
        simulated_cache->sets[i].lines = (line *) malloc(sizeof(line) * lines_per_set);
    }

    //Run the cache simulation with the trace file input
    simulate_cache(cp, simulated_cache, trace_file);

    //TODO: remove, just to ignore "unused variable lru_tracker" on compilation
    printf("Use LRU tracker: %d", lru_tracker[0]->idx);

    location *loc = malloc(sizeof(location));
    loc->set_id = 0;
    loc->tag_id = 0;

    //get_and_set_tag(loc, 0, 1, 1);

    printSummary(0, 0, 0);
    return 0;
}

cache_performance *simulate_cache(cache_performance *cp, cache *sim_cache, FILE *trace_file) {
    char *type = (char *) calloc(sizeof(char), 1);
    unsigned int *address = (unsigned int *) calloc(sizeof(unsigned int), 1);
    int *size = (int *) calloc(sizeof(int), 1);

    //Loop through each line in the trace file
    while(fscanf(trace_file, " %c %x,%d\n", type, address, size) != -1) {
        switch(*type) {
            case 'L':
                printf("Load, %x, %d\n", *address, *size);
                break;
            case 'S':
                printf("Store, %x, %d\n", *address, *size);
                break;
            case 'I':
                printf("Instruction (pass)\n");
                break;
            default:
                break;
        }
    }

    return cp;
}

void get_set_and_tag(location *loc, unsigned int address, int tbits, int sbits) {
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

enum HitOrMiss cache_scan(location *loc, cache *sim_cache) {
    int set_id = loc->set_id;
    unsigned long long tag_id = loc->tag_id;
    line *lines = sim_cache->sets[set_id]->lines;
    printf(lines);                          //To make sure I know what the fuck is going on (I don't)//
    bool empty_cache = false;
    for (var i = 0; i < sim_cache->lines_per_set; i++){
        if(lines[i] == tag_id) {
            return HIT;
        }
        if(!lines[i]->valid) {
            empty_cache = true;
        }
    }
    if(empty_cache) {
        return COLD_MISS;
    } else {
        return MISS;
    }


}

void print_usage() {
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
}
