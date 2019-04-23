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
void get_set_and_tag(location *loc, unsigned long long address, int tbits, int sbits);

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
    int bytes_per_line;
    int sbits;
    int tbits;
    lru_node *lru_tracker[];
} cache;

enum HitOrMiss cache_scan(struct location *loc, cache *sim_cache);

typedef struct lru_node {
    struct lru_node *prev;
    struct lru_node *next;
    int idx; //Index into the array of lines in the set
} lru_node;

int main(int argc, char *argv[])
{
    location *l = malloc(sizeof(location));
    unsigned long long address = 18378908604322283520ULL;
    get_set_and_tag(l, address, 8, 8);

    printf("set index: %d\n", l->set_id);
    printf("tag: %llu\n", l->tag_id);

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

    //Allocate memory to store the cache and sets
    cache *simulated_cache = (cache *) malloc(sizeof(cache));
    simulated_cache->sets  = (set *)   malloc(sizeof(set) * pow(2, s));

    //For each set, allocate memory for the lines within
    for(int i = 0; i < pow(2, s); i++) {
        simulated_cache->sets[i].lines = (line *) malloc(sizeof(line) * lines_per_set);
    }

    simulated_cache->sbits = s;
    simulated_cache->lines_per_set = lines_per_set;
    simulated_cache->bytes_per_line = bytes_per_line;
    simulated_cache->tbits = 64 - (s + bytes_per_line);

    simulated_cache->lru_tracker[(int) pow(2, s)];

    //Set up linked lists with length E for each set
    for(int i = 0; i < pow(2, s); i++) {
        simulated_cache->lru_tracker *cur_node = (simulated_cache->lru_tracker *) malloc(sizeof(simulated_cache->lru_tracker));
        cur_node->idx = 0;
        simulated_cache->lru_tracker[i] = cur_node;

        for(int j = 0; j < lines_per_set - 1; j++) {
            simulated_cache->lru_tracker *next_node = (simulated_cache->lru_tracker *) malloc(sizeof(simulated_cache->lru_tracker));
            next_node->idx = j + 1;
            cur_node->next = next_node;
        }
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

void get_set_and_tag(location *loc, unsigned long long address, int tbits, int sbits) {
    unsigned long long tag_mask = ~0 << (64 - tbits);
    unsigned long long set_mask = ~0 << (64 - sbits);
    set_mask >>= tbits;

    loc->tag_id = (tag_mask & address) >> (64 - tbits);
    loc->set_id = (set_mask & address) >> ((64 - tbits) - sbits);
}

cache_performance *simulate_cache(cache_performance *cp, cache *sim_cache, FILE *trace_file) {
    char *type = (char *) calloc(sizeof(char), 1);
    unsigned int *address = (unsigned int *) calloc(sizeof(unsigned int), 1);
    int *size = (int *) calloc(sizeof(int), 1);

    location *loc = malloc(sizeof(location));
    loc->set_id = 0;
    loc->tag_id = 0;

    //Loop through each line in the trace file
    while(fscanf(trace_file, " %c %x,%d\n", type, address, size) != -1) {
        get_set_and_tag(loc, *address, sim_cache->tbits, sim_cache->sbits);
        switch(*type) {
            case 'L':
                printf("Load, %x, %d\n", *address, *size);
                int result = cache_scan(loc, sim_cache);
                if(result == 0) {
                    cp->hits++;
                } else if(result == 1 || result == 2) {
                    cp->misses++;
                    if (result == 2) {
                        cp->evictions++;
                    };
                }
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
    line *lines = sim_cache->sets[set_id].lines;
    printf("Lines address: %p\n", lines);                          //To make sure I know what the fuck is going on (I don't)
    bool empty_cache = false;
    for (int i = 0; i < sim_cache->lines_per_set; i++){
        if(lines[i].tag == tag_id) {
            LRU_manipulate(sim_cache, set_id, tag_id, i, 0);
            return HIT;
        }
        if(!lines[i].valid) {
            empty_cache = true;
        }
    }
    if(empty_cache) {
        LRU_manipulate(sim_cache, 1);
        return COLD_MISS;
    } else {
        LRU_manipulate(sim_cache, 2);
        return MISS;
    }
}

/**
 *
 * @param sim_cache makes sure that we still have access to the simulated cache
 * @param set_id the 0-indexed id of the set we are working in
 * @param tag_id the tag_id of whatever is being hit/missed in the cache
 * @param z on a cache hit, z is the position in the lines array of the matching line to the tag id
 * @param result 0 is a cache hit, 1 is a cold miss, 2 is a miss. Dictates which LRU manipulation happens. TODO: split into 3 functions instead of having a result parameter.
 */
void LRU_manipulate(cache *sim_cache, int set_id, unsigned long long tag_id, int z, int result) {
    lru_node *current = sim_cache->lru_tracker[set_id];
    if (result == 0) {
        for(int i = 0; i < pow(2, sim_cache->sbits); i++) {
            if(current->idx = z) {
                //Move current to front, move front back one
            } else {
                //*current = current->next;
            }
        }
    } else if (result == 1) {

    }else if (result == 2) {

    }
}

void print_usage() {
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
}
