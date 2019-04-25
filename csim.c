#include "cachelab.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <math.h>

/**
 * Struct representing a location of data within the cache
 * @param set_id index of the set
 * @param tag_id tag of the line
 */
typedef struct location {
    int set_id;
    unsigned long long tag_id;
} location;

//Enum representing a cache hit, cold miss, or miss
enum HitOrMiss {HIT, COLD_MISS, MISS};

//Forward declare print_usage and get_and_set_tag
void print_usage();
void get_set_and_tag(location *loc, unsigned long long address, int tbits, int sbits);

/**
 * Struct to store the performance of the cache.
 * @param hits number of cache hits
 * @param misses number of cache misses
 * @param evictions number of cache evictions
 */
typedef struct cache_performance {
    int hits;
    int misses;
    int evictions;
} cache_performance;

//Forward declare the simulate_cache function
void simulate_cache();

/**
 * Struct representing a single line within a set in a cache
 * @param valid whether or not this line is caching data. 0 if the cache hasn't been fully warmed up
 * @param tag number representing the tag for this cache line
 */
typedef struct line {
    bool valid;
    unsigned long long tag; //ensure 64-bit address compatibility
} line;

/**
 * Struct representing a single set in the simulated cache
 * @param id index of the set in the cache's set array
 * @param lines list of lines within each specific set
 */
typedef struct set {
    int id;
    line *lines;
} set;

/**
 * Struct for a node in the linked lists managing the LRU eviction policy. Declared before the cache so that it knows
 * the type lru_node.
 * @param prev previous node in the LL
 * @param next subsequent node in the LL
 * @param idx index into the lines array for the set the linked list is responsible for
 */
typedef struct lru_node {
    struct lru_node *prev;
    struct lru_node *next;
    int idx; //Index into the array of lines in the set
} lru_node;

/**
 * Struct representing the cache to be simulated.
 * @param sets list of sets the cache will simulate (2^s)
 * @param lines_per_set how many lines there are per set (E)
 * @param bytes_per_line how many bytes each cache block will store (2^b)
 * @param sbits number of bits for the set id
 * @param tbits number of bits for the tag
 */
typedef struct cache {
    set *sets;
    int lines_per_set;
    int bytes_per_line;
    int sbits;
    int tbits;
    bool verbose;
    lru_node **lru_tracker;
} cache;

//Forward declare of functions requiring cache
void setup_cache(cache **sim_cache, int sbits, int lines_per_set, int bytes_per_line, int tbits);
void allocate_cache(cache **sim_cache);
enum HitOrMiss cache_scan(struct location *loc, cache *sim_cache);
void LRU_hit(cache *sim_cache, int set_id, unsigned long long tag_id, int z);
void LRU_cold(cache *sim_cache, int set_id, unsigned long long tag_id);
void LRU_miss(cache *sim_cache, int set_id, unsigned long long tag_id);

/**
 * Called on startup.
 * @param argc number of command line arguments
 * @param argv array of strings of the command line arguments
 * @return 0
 */
int main(int argc, char *argv[])
{
    //Initialize all command line parameters
    bool help_flag = false;
    bool verbose_flag = false;
    int s = -1;
    int lines_per_set = -1;
    int bytes_per_line = -1;
    char *trace_path = (char *) NULL;

    FILE *trace_file;

    //Allocate memory for the cache performance struct
    cache_performance *cp = (cache_performance *) malloc(sizeof(cache_performance));

    //Declare variables for the current command line argument, and p to pass into strtol
    int opt;
    char *p;

    //Loop through each command line argument, pull the data into the initialized variables
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch(opt) {
            case 'h':
                help_flag = true;
                break;
            case 'v':
                verbose_flag = true;
                break;
            case 's':
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
                break;
        }
    }

    //If one of the required parameters was not given, so inform user how parameters work then quit
    if(s == -1 || lines_per_set == -1 || bytes_per_line == -1 || trace_path == (char *) NULL || help_flag) {
        print_usage();
        exit(0);
    }

    //Open the trace file
    trace_file = fopen(trace_path, "r");

    //If the trace file doesn't exist, notify and quit
    if(trace_file == NULL) {
        printf("Invalid trace file path \"%s\".\n", trace_path);
        exit(0);
    }

    //Declare then allocate space needed for the cache
    cache *simulated_cache = NULL;
    setup_cache(&simulated_cache, s, lines_per_set, bytes_per_line, 64 - (s + bytes_per_line));
    simulated_cache->lru_tracker = (lru_node **) malloc(sizeof(lru_node *) * pow(2, simulated_cache->sbits));

    simulated_cache->verbose = verbose_flag;

    printf("RADARE RADARE RADARE RADARE\n");

    //Set up linked lists with length E for each set
    for(int i = 0; i < pow(2, s); i++) {
        lru_node *cur_node = (lru_node *) malloc(sizeof(lru_node));
        cur_node->idx = 0;
        cur_node->prev = NULL;
        simulated_cache->lru_tracker[i] = cur_node;

        for(int j = 0; j < lines_per_set + 1; j++) {
            lru_node *next_node = (lru_node *) malloc(sizeof(lru_node));
            next_node->idx = j + 1;
            cur_node->next = next_node;
            next_node->prev = cur_node;
            next_node->next = NULL;
            cur_node = cur_node->next;
        }
    }

    printf("Sentry idx of s0l0: %d\n", simulated_cache->lru_tracker[0]->idx);

    //Run the cache simulation with the trace file input
    simulate_cache(cp, simulated_cache, trace_file);

    location *loc = malloc(sizeof(location));
    loc->set_id = 0;
    loc->tag_id = 0;

    printSummary(cp->hits, cp->misses, cp->evictions);
    return 0;
}

/**
 * Allocates the entire cache, including sets and lines.
 * @param sim_cache
 * @param sbits
 * @param lines_per_set
 * @param bytes_per_line
 * @param tbits
 */
void setup_cache(cache **sim_cache, int sbits, int lines_per_set, int bytes_per_line, int tbits) {
    *sim_cache = (cache *) malloc(sizeof(cache));

    (*sim_cache)->sbits = sbits;
    (*sim_cache)->lines_per_set = lines_per_set;
    (*sim_cache)->bytes_per_line = bytes_per_line;
    (*sim_cache)->tbits = 64 - (sbits + bytes_per_line);

    allocate_cache(sim_cache);
}

void allocate_cache(cache **sim_cache) {
    //Allocate memory to store the cache and sets
    (*sim_cache)->sets = (set *) malloc(sizeof(set) * pow(2, (*sim_cache)->sbits));

    //For each set, allocate memory for the lines within
    for(int i = 0; i < pow(2, (*sim_cache)->sbits); i++) {
        (*sim_cache)->sets[i].lines = (line *) malloc(sizeof(line) * (*sim_cache)->lines_per_set);
        (*sim_cache)->sets[i].id = i;
        for(int j = 0; j < (*sim_cache)->lines_per_set; j++) {
            (*sim_cache)->sets[i].lines[j].valid = 0;
        }
    }
}

/**
 * Separate the tag and set from an address, given s and t to determine set and tag sizes
 * @param loc location struct to fill in with the result
 * @param address location in memory to parse into tag and set
 * @param tbits number of bits the tag takes up
 * @param sbits number of bits the set takes up
 * @return fills in the loc struct with the tag and set id
 */
void get_set_and_tag(location *loc, unsigned long long address, int tbits, int sbits) {
    //Shift a block of 1s up to the top of the number, size determined by tbits and sbits
    unsigned long long tag_mask = ~0 << (64 - tbits);
    unsigned long long set_mask = ~0 << (64 - sbits);

    //Since the set block comes after tbits, shift the mask back down to line it up
    set_mask >>= tbits;

    //By &'ing the mask against the address, we get the bits representing the set and tag.
    //By shifting those back down, we can get the value without 0s between it and the bottom bit.
    loc->tag_id = (tag_mask & address) >> (64 - tbits);
    loc->set_id = (set_mask & address) >> ((64 - tbits) - sbits);
}

/**
 * Simulates a cache based on trace file output from Valgrind. Counts hits, misses, and evictions.
 * @param cp struct to fill in, specifying hit, miss, and eviction count.
 * @param sim_cache allocated cache to perform operations on
 * @param trace_file file handler for the specified trace file
 * @return fills in the cp variable with the hit, miss, and eviction count
 */
void simulate_cache(cache_performance *cp, cache *sim_cache, FILE *trace_file) {

    //Allocate (with initialization) the 3 parameters of any trace line
    char *type = (char *) calloc(sizeof(char), 1);
    unsigned int *address = (unsigned int *) calloc(sizeof(unsigned int), 1);
    int *size = (int *) calloc(sizeof(int), 1);

    //Allocate for the location, initialize set and tag id's
    location *loc = malloc(sizeof(location));
    loc->set_id = 0;
    loc->tag_id = 0;

    //Loop through each line in the trace file
    while(fscanf(trace_file, " %c %x,%d\n", type, address, size) != -1) {
        get_set_and_tag(loc, *address, sim_cache->tbits, sim_cache->sbits);
        switch(*type) {
            case 'M':
                cp->hits++;
            case 'S':
            case 'L':
                ;
                //Load instruction. If HIT, increment. If COLD_MISS, pull up new LRU node. If MISS, perform an eviction
                int result = cache_scan(loc, sim_cache);
                if(result == HIT) {
                    cp->hits++;
                } else if(result == COLD_MISS || result == MISS) {
                    cp->misses++;
                    if (result == MISS) {
                        cp->evictions++;
                    };
                }
                break;
            case 'I':
                //Instruction instruction. Pass.
                break;
            default:
                break;
        }
    }
}

/**
 * Scans the cache for the location provided. Returns whether that line resulted in a cache hit, cold miss, or miss.
 * @param loc location to search for
 * @param sim_cache cache to search through
 * @return HIT, COLD_MISS, or MISS depending on the cache
 */
enum HitOrMiss cache_scan(location *loc, cache *sim_cache) {
    //Pull out set and tag ids from location
    int set_id = loc->set_id;
    unsigned long long tag_id = loc->tag_id;

    //Get the list of lines from the set we want to look at
    line *lines = sim_cache->sets[set_id].lines;

    bool is_cache_full = true;

    //Loop through the lines in the set
    for (int i = 0; i < sim_cache->lines_per_set; i++) {
        //If we have a match, we have a hit. Return.
        if(lines[i].tag == tag_id && lines[i].valid) {
            LRU_hit(sim_cache, set_id, tag_id, i);
            return HIT;
        }

        //If ANY validity bit isn't set, our cache is not empty.
        if(!lines[i].valid) {
            is_cache_full = false;
        }
    }

    //If we don't get a hit and the cache is full, perform an eviction then return. Otherwise, just return.
    if(is_cache_full) {
        LRU_miss(sim_cache, set_id, tag_id);
        return MISS;
    } else {
        LRU_cold(sim_cache, set_id, tag_id);
        return COLD_MISS;
    }
}

/**
 * LRU_hit keeps track of LRU logic in the lru_tracker linked list on a cache hit
 * @param sim_cache makes sure that we still have access to the simulated cache
 * @param set_id the 0-indexed id of the set we are working in
 * @param tag_id the tag_id of whatever is being hit/missed in the cache
 * @param z on a cache hit, z is the position in the lines array of the matching line to the tag id
 */
void LRU_hit(cache *sim_cache, int set_id, unsigned long long tag_id, int z) {
    lru_node *current = sim_cache->lru_tracker[set_id];
    current = current->next;
    lru_node *front = sim_cache->lru_tracker[set_id];

    for (int i = 0; i < sim_cache->lines_per_set; i++) {

        if (current->idx-1 == z) {
            //Move current to front, move front back one
            lru_node *previous = current->prev;
            previous->next = current;
            lru_node *nextup = current->next;
            nextup->prev = current;
            lru_node *hit = current;

            hit->prev = front;
            if(front->next != current) {
                hit->next = front->next;
                front->next->prev = hit;
                previous->next = nextup;
                nextup->prev = previous;
            }

            front->next = hit;

            sim_cache->sets[set_id].lines[current->idx-1].tag = tag_id;
            return;
        } else {
            current = current->next;
        }
    }
}

/**
 * LRU_cold keeps track of LRU logic in the lru_tracker linked list on a cold miss
 * @param sim_cache makes sure that we still have access to the simulated cache
 * @param set_id the 0-indexed id of the set we are working in
 * @param tag_id the tag_id of whatever is being hit/missed in the cache
 */
void LRU_cold(cache *sim_cache, int set_id, unsigned long long tag_id) {
    lru_node *current = sim_cache->lru_tracker[set_id];

    current = current->next;

    lru_node *front = sim_cache->lru_tracker[set_id];

    for (int i = 0; i < sim_cache->lines_per_set; i++) {


        if(!sim_cache->sets[set_id].lines[current->idx-1].valid) {
            lru_node *previous = current->prev;
            previous->next = current;

            lru_node *nextup = current->next;
            nextup->prev = current;
            lru_node *empty = current;

            empty->prev = front;
            if(front->next != current) {
                empty->next = front->next;
                front->next->prev = empty;
                previous->next = nextup;
                nextup->prev = previous;
            }

            front->next = empty;

            sim_cache->sets[set_id].lines[current->idx-1].tag = tag_id;
            sim_cache->sets[set_id].lines[current->idx-1].valid = true;
            return;
        } else {
            current = current->next;
        }
    }
}

/**
 * LRU_miss keeps track of LRU logic in the lru_tracker linked list on a cache miss
 * @param sim_cache makes sure the function has access to the simulated cache
 * @param set_id passes in the ID of the set where the miss occurs
 * @param tag_id passes in the tag id that needs to be added to the simulated cache
 */
void LRU_miss(cache *sim_cache, int set_id, unsigned long long tag_id) {
    lru_node *current = sim_cache->lru_tracker[set_id];
    current = current->next;
    lru_node *front = sim_cache->lru_tracker[set_id];

    if(sim_cache->lines_per_set > 1) {
        while(current->next != NULL) {
            current = current->next;
        }

        current = current->prev;

        current->prev->next = current->next;
        current->next->prev = current->prev;
        front->next->prev = current;
        current->next = front->next;
        current->prev = front;
        front->next = current;
    }
    sim_cache->sets[set_id].lines[current->idx-1].tag = tag_id;

}

/**
 * Prints the command line usage of the executable. Used if the user did not correctly input parameters.
 */
void print_usage() {
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
}
