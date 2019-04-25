#include "cachelab.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <math.h>

#define DEBUG 1

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
    lru_node **lru_tracker;
} cache;

//Forward declare of functions requiring cache
void setup_cache(cache *sim_cache, int sbits, int lines_per_set, int bytes_per_line, int tbits);
void allocate_cache(cache *sim_cache);
enum HitOrMiss cache_scan(struct location *loc, cache *sim_cache);
int set_cache(location *loc, cache *sim_cache);

/**
 * Called on startup.
 * @param argc number of command line arguments
 * @param argv array of strings of the command line arguments
 * @return 0
 */
int main(int argc, char *argv[])
{
    //Proof that get_and_set_tag works. TODO remove before submission
    location *l = malloc(sizeof(location));
    unsigned long long address = 18378908604322283520ULL;
    get_set_and_tag(l, address, 8, 8);

    printf("set index: %d\n", l->set_id);
    printf("tag: %llu\n", l->tag_id);
    //END REMOVE

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

    //If one of the required parameters was not given, so inform user how parameters work then quit
    if(s == -1 || lines_per_set == -1 || bytes_per_line == -1 || trace_path == (char *) NULL) {
        print_usage();
        exit(0);
    }

    //Open the trace file
    trace_file = fopen(trace_path, "r");

    //If the trace file doesn't exist, notify and quit
    if(trace_file == NULL) {
        printf("Invalid trace file path \"%s\".", trace_path);
        exit(0);
    }

    //If DEBUG or verbose was set, output the command line arguments
    if(DEBUG || verbose_flag) {
        printf("Help set: %s\n", help_flag ? "true" : "false");
        printf("Verbose flag set: %s\n", verbose_flag ? "true" : "false");
        printf("s: %d\n", s);
        printf("Lines per set: %d\n", lines_per_set);
        printf("Bytes per line: %d\n", bytes_per_line);
        printf("Trace file: %s\n", trace_path);
    }

    //Declare then allocate space needed for the cache
    cache *simulated_cache;
    setup_cache(simulated_cache, s, lines_per_set, bytes_per_line, 64 - (s + bytes_per_line));
    simulated_cache->lru_tracker = (lru_node **) malloc(sizeof(lru_node *) * pow(2, simulated_cache->sbits));

    //Set up linked lists with length E for each set
    for(int i = 0; i < pow(2, s); i++) {
        lru_node *cur_node = (lru_node *) malloc(sizeof(lru_node));
        cur_node->idx = 0;
        simulated_cache->lru_tracker[i] = cur_node;

        for(int j = 0; j < lines_per_set - 1; j++) {
            lru_node *next_node = (lru_node *) malloc(sizeof(lru_node));
            next_node->idx = j + 1;
            cur_node->next = next_node;
            next_node->prev = cur_node;
        }
    }

    //Run the cache simulation with the trace file input
    simulate_cache(cp, simulated_cache, trace_file);

    location *loc = malloc(sizeof(location));
    loc->set_id = 0;
    loc->tag_id = 0;

    printSummary(0, 0, 0);
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
void setup_cache(cache *sim_cache, int sbits, int lines_per_set, int bytes_per_line, int tbits) {
    sim_cache->sbits = sbits;
    sim_cache->lines_per_set = lines_per_set;
    sim_cache->bytes_per_line = bytes_per_line;
    sim_cache->tbits = 64 - (sbits + bytes_per_line);

    allocate_cache(sim_cache);
}

void allocate_cache(cache *sim_cache) {
    //Allocate memory to store the cache and sets
    sim_cache = (cache *) malloc(sizeof(cache));
    sim_cache->sets = (set *) malloc(sizeof(set) * pow(2, sim_cache->sbits));

    //For each set, allocate memory for the lines within
    for(int i = 0; i < pow(2, sim_cache->sbits); i++) {
        sim_cache->sets[i].lines = (line *) malloc(sizeof(line) * sim_cache->lines_per_set);
        sim_cache->sets[i].id = i;
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
                //Modify instruction. Performs a LOAD then a STORE. In our case, does it act the same as Load?
            case 'L':
                //Load instruction. If HIT, increment. If COLD_MISS, pull up new LRU node. If MISS, perform an eviction
                printf("Load, %x, %d\n", *address, *size);
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
            case 'S':
                //Store instruction. Will not update hits, misses, or evictions, simply updates the validity of the line
                printf("Store, %x, %d\n", *address, *size);
                set_cache(loc, sim_cache);
                break;
            case 'I':
                //Instruction instruction. Pass.
                printf("Instruction (pass)\n");
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
    //printf("Lines address: %p\n", lines); //To make sure I know what the fuck is going on (I don't)

    bool is_cache_full = true;

    //Loop through the lines in the set
    for (int i = 0; i < sim_cache->lines_per_set; i++) {
        //If we have a match, we have a hit. Return.
        if(lines[i].tag == tag_id) {
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
        //Perform eviction (overwrite LRU line)
        return MISS;
    } else {
        LRU_cold(sim_cache, set_id, tag_id);
        //Find an unused linked list element,
        return COLD_MISS;
    }
}

/**
 *
 * @param sim_cache makes sure that we still have access to the simulated cache
 * @param set_id the 0-indexed id of the set we are working in
 * @param tag_id the tag_id of whatever is being hit/missed in the cache
 * @param z on a cache hit, z is the position in the lines array of the matching line to the tag id
 */
void LRU_hit(cache *sim_cache, int set_id, unsigned long long tag_id, int z) {
    lru_node *current = sim_cache->lru_tracker[set_id];
    lru_node *front = sim_cache->lru_tracker[set_id];
    lru_node *hit;
    for (int i = 0; i < pow(2, sim_cache->sbits); i++) {
        if (current->idx = z) {
            //Move current to front, move front back one
            lru_node *previous = current->prev;
            lru_node *nextup = current->next;
            hit = current;
            hit->prev = null_ptr;
            hit->next = front;
            front->prev = hit;
            previous->next = nextup;
            nextup->prev = previous;
            *current = current->next;
        } else {
            *current = current->next;
        }
    }
}

/**
 *
 * @param sim_cache makes sure that we still have access to the simulated cache
 * @param set_id the 0-indexed id of the set we are working in
 * @param tag_id the tag_id of whatever is being hit/missed in the cache
 */
void LRU_cold(cache *sim_cache, int set_id, unsigned long long tag_id) {
    lru_node *current = sim_cache->lru_tracker[set_id];
    lru_node *front = sim_cache->lru_tracker[set_id];
    for (int i=0; i< pow(2, sim_cache->sbits); i++) {
        if(!sim_cache->sets[set_id].lines[current->idx].valid) {
            lru_node *previous = current->prev;
            lru_node *nextup = current->next;
            lru_node *empty = current;
            empty->prev = null_ptr;
            empty->next = front;
            front->prev = empty;
            previous->next = nextup;
            nextup->prev = previous;
            *current = current->next;
        } else {
            *current = current->next;
        }
    }
}




/**
 * Sets a cache line's validity bit to 1, based on set id and the tag.
 * @param loc set id and tag, uniquely identifying cache line
 * @param sim_cache cache to modify line of
 * @return 0 if successful, -1 if no line with that set id and tag was found.
 */
int set_cache(location *loc, cache *sim_cache) {
    //Pull out set that we are interested in
    set s = sim_cache->sets[loc->set_id];

    //Loop through lines in the set
    for(int i = 0; i < sim_cache->lines_per_set; i++) {
        //If we have a match, set the validity and return.
        if(s.lines[i].tag == loc->tag_id) {
            s.lines[i].valid = 1;
            return 0;
        }
    }

    //If we don't find a match, there was a problem. Return -1.
    return -1;
}

/**
 * Prints the command line usage of the executable. Used if the user did not correctly input parameters.
 */
void print_usage() {
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
}

void test_allocation();

bool test_load();
bool test_store();
bool test_modify();
bool test_instruction();

bool test_eviction();

/**
 * Runs tests to ensure the program is functional.
 * @return true if tests pass, false (or segfault) if they don't.
 */
bool run_tests() {
    test_allocation();

    test_load();
    test_store();
    test_modify();
    test_instruction();

    test_eviction();

    return true;
}

/**
 * Ensures that all structs were allocated correctly by attempting to write to every available parameter.
 * @return void, as the function will segfault if the write fails.
 */
void test_allocation() {
    //Write to all cache params
    //Write to all sets
    //Write to all lines
    //Write to all linked lists
}

/**
 * Ensures that the cache is correctly updated when a load instruction is called.
 * @return true if it loads correctly
 */
bool test_load() {
    return false;
}

/**
 * Ensures that the cache is correctly updated when a store instruction is called.
 * @return true if it stores correctly
 */
bool test_store() {
    return false;
}

/**
 * Ensures that the cache is correctly updated when a load instruction is called.
 * @return true if it modifies correctly
 */
bool test_modify() {
    return false;
}

/**
 * Ensures that the cache is correctly updated when a load instruction is called.
 * @return true if an instruction line is ignored
 */
bool test_instruction() {
    return false;
}

/**
 * Ensures that the LRU line is evicted when a miss (not cold miss) happens
 * @return true if an eviction is performed correctly
 */
bool test_eviction() {
    return false;
}
