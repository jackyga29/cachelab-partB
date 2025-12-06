/*----------------
    JAQUELINE GALLEGOS ALCALA (101924540)
    JGALLEGOSALCALA29@UNM.EDU
    CS 341L -  Introduction to Computer Architecture and Organization
 */


#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/**
 * This struct represents a single cache line. It stores the tag, valid bit, and a usage time stamp
 * I will later use this to replace places following the LRU rule.
 */
typedef struct {
    int valid;
    unsigned long long tag; /* for LRU: larger means more recent */
    unsigned long long last_used; 
} CacheLine;

/**
 * struct to represent a single cache set, which contains the number of lines in the set. 
 */
typedef struct {
    //pointer to an array of lines E in the set.
    CacheLine *lines; 
} CacheSet;

/**
 * This represents the whole cache, declaring each parameter needed 
 * incuiding the pointer to the array of sets.
 */
typedef struct {
    int s; //set index bits
    int E; // lines per set (associativity) 
    int b; // number of block bits 
    int S; // number of sets = 2^s 
    CacheSet *sets; //poniter to the sets in the cache
} Cache;

/* Global counters */
static int verbose = 0;
static int hitCount = 0;
static int missCount = 0;
static int evictionCounter = 0;

/* Global counter to keep track of the LRU */
static unsigned long long globalCounter  = 0ULL;

/**
 * function that prints the message with all the options to use the simulator
 */
static void print_usage(const char *prog) {
    printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", prog);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <s>     Number of set index bits.\n");
    printf("  -E <E>     Number of lines per set (associativity).\n");
    printf("  -b <b>     Number of block bits.\n");
    printf("  -t <file>  Name of the valgrind trace to replay.\n");
    printf("\nExamples:\n");
    printf("  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", prog);
    printf("  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", prog);
}

/**
 * This functions starts the cache data structure and allocates all sets and lines
 */
static Cache init_cache(int s, int E, int b) {
    Cache cache;
    cache.s = s;
    cache.E = E;
    cache.b = b;
    cache.S = 1 << s;
    cache.sets = (CacheSet *)malloc((size_t)cache.S * sizeof(CacheSet));
    if (!cache.sets) {
        fprintf(stderr, "Error allocating space for sets\n");
        exit(1);
    }

    //For each cache set, allocate E cache lines and initialize them
    for (int i = 0; i < cache.S; i++) {
        cache.sets[i].lines = (CacheLine *)malloc((size_t)E * sizeof(CacheLine));
        if (!cache.sets[i].lines) {
            fprintf(stderr, "Error in malloc for lines\n");
            exit(1);
        }
        for (int j = 0; j < E; j++) {
            cache.sets[i].lines[j].valid = 0;
            cache.sets[i].lines[j].tag = 0ULL;
            cache.sets[i].lines[j].last_used = 0ULL;
        }
    }
    return cache;
}

/**
 * this function frees all memory used by the cache data structure,
 * It is called at the end of the program to release all the sets and lines
 */
static void free_cache(Cache *cache) {
    if (!cache || !cache->sets) return;
    for (int i = 0; i < cache->S; i++) {
        free(cache->sets[i].lines);
        cache->sets[i].lines = NULL;
    }
    free(cache->sets);
    cache->sets = NULL;
}

/**
 * This is the principal function that determines either a hit or miss,
 * when you need an eviction it uses the LRU rule to do the eviction.
 * it also updates all the global counters and recods the outcome in verbose.
 */
static void access_cache(Cache *cache, unsigned long long addr, char *verbose_out, size_t verbose_out_cap) {
    /* Compute set index and tag */
    unsigned long long set_index = (addr >> cache->b) & ((1ULL << cache->s) - 1ULL);
    unsigned long long tag = addr >> (cache->s + cache->b);

    CacheSet *set = &cache->sets[set_index];

    /* 1) Check for hit */
    int hit = 0;
    for (int i = 0; i < cache->E; i++) {
        CacheLine *line = &set->lines[i];
        if (line->valid && line->tag == tag) {
            hit = 1;
            hitCount++;
            line->last_used = ++globalCounter;
            if (verbose && verbose_out) strncat(verbose_out, " hit", verbose_out_cap - strlen(verbose_out) - 1);
            break;
        }
    }
    if (hit) return;

    /* 2) Miss */
    missCount++;
    if (verbose && verbose_out) strncat(verbose_out, " miss", verbose_out_cap - strlen(verbose_out) - 1);

    /* 3) Try to fill empty line */
    for (int i = 0; i < cache->E; i++) {
        CacheLine *line = &set->lines[i];
        if (!line->valid) {
            line->valid = 1;
            line->tag = tag;
            line->last_used = ++globalCounter;
            return;
        }
    }

    /* 4) Evict LRU */
    evictionCounter++;
    if (verbose && verbose_out) strncat(verbose_out, " eviction", verbose_out_cap - strlen(verbose_out) - 1);

    int lru_index = 0;
    unsigned long long lru_time = set->lines[0].last_used;
    for (int i = 1; i < cache->E; i++) {
        if (set->lines[i].last_used < lru_time) {
            lru_time = set->lines[i].last_used;
            lru_index = i;
        }
    }
    CacheLine *victim = &set->lines[lru_index];
    victim->tag = tag;
    victim->last_used = ++ globalCounter;
}

/**
 * main function it first sets up the cache simulator, then it repasses the trace file, 
 * at the end it returns the final statistics 
 */
int main(int argc, char **argv) {
    int opt;
    int s = -1, E = -1, b = -1;
    char *tracefile = NULL;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                tracefile = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (s < 0 || E < 1 || b < 0 || tracefile == NULL) {
        fprintf(stderr, "Missing required command line argument\n");
        print_usage(argv[0]);
        return 1;
    }

    Cache cache = init_cache(s, E, b);

    FILE *fp = fopen(tracefile, "r");
    if (!fp) {
        fprintf(stderr, "Could not open trace file %s: %s\n", tracefile, strerror(errno));
        free_cache(&cache);
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Lines look like: (I 0400d7d4,8), (M 20,1), (L 22,1), (S 18,1)
        char op = 0;
        unsigned long long addr = 0ULL;
        int size = 0;

        if (sscanf(line, " %c %llx,%d", &op, &addr, &size) != 3) {
            // If it doesn't match, maybe it's an instruction line or blank; the checks for an I 
            if (line[0] == 'I') {
                continue; //ignore instruction fetches 
            } else {
                continue; // skip anything malformed 
            }
        }

        if (op == 'I') {
            continue; // ignore instruction accesses 
        }

        if (verbose) {
            // Build the left side exactly like csim-ref prints 
            // It prints the original operation, address and size as seen in the trace 
            printf("%c %llx,%d", op, addr, size);
        }

        char vbuf[64]; vbuf[0] = '\0';

        switch (op) {
            case 'L':
                access_cache(&cache, addr, vbuf, sizeof(vbuf));
                break;
            case 'S':
                access_cache(&cache, addr, vbuf, sizeof(vbuf));
                break;
            case 'M':
                access_cache(&cache, addr, vbuf, sizeof(vbuf));
                access_cache(&cache, addr, vbuf, sizeof(vbuf));
                break;
            default:
                //Unknown operation, breask 
                break;
        }

        if (verbose) {
            //Prints the outcomes accumulated in vbuf 
            printf("%s\n", vbuf);
        }
    }

    fclose(fp);

    printSummary(hitCount, missCount, evictionCounter);

    free_cache(&cache);
    return 0;
}


