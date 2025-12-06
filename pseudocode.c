//initial parameters and Address decomposition 
static inline unsigned long get_set_index(unsigned long addr, int s, int b) {
unsigned long mask = ((1UL << s) - 1);
return (addr >> b) & mask;
}
static inline unsigned long get_tag(unsigned long addr, int s, int b) {
return addr >> (s + b);
}

//cache line 
typedef struct {
int valid; // 0 o 1
unsigned long tag; // tag
unsigned long lru; // LRU counter  O
unsigned long last_touch; //  timestamp of last access
} CacheLine;

//LRU constraints 
On every access:
    cache.time_counter++
    line.last_touch = cache.time_counter

On hit:
    update last_touch as above; hits++

On miss:
    misses++
    if there is an invalid line in the set:
        use it
    else:
     victim = argmin(set.lines, by last_touch)  // LRU
        evictions++
    victim.valid = 1
    victim.tag = tag
    victim.last_touch = ++cache.time_counter

//cache sets 
typedef struct {
CacheLine *lines; // array size of E
} CacheSet;

typedef struct {
    int s, E, b; // parameters
    int S; // 1 << s
    CacheSet *sets; // array of size S
    //  global counters
    unsigned long hits, misses, evictions;
    unsigned long time_counter; // for LRU
    int verbose; // flag -v
} return Cache;

//valgrind traces
open(tracefile)
for each line:
    if line starts with 'I': continue
    parse op, address (hex), size (int)
    if op in {L, S}: access(address)
    if op == 'M': access(address); access(address)

//global function 
ACCESS(address):
    set_index = get_set_index(address, s, b)
    tag = get_tag(address, s, b)
    set = cache.sets[set_index]
 // look for a hit
    for line in set.lines:
    if line.valid && line.tag == tag:
    cache.hits++
    line.last_touch = ++cache.time_counter
    if cache.verbose: print("hit")
    return
 // Miss
    cache.misses++
    if cache.verbose: print("miss")
 // look for empty 
    empty = first line with valid==0 in set.lines (if any)
    if empty exists:
    line = empty
    else:
 // Evict LRU
 victim = argmin(set.lines, by last_touch)
 cache.evictions++
 if cache.verbose: print("eviction")
 line = victim
 // load new line 
 line.valid = 1
 line.tag = tag
 line.last_touch = ++cache.time_counter

// final pseudocode 
INIT(s,E,b):
 S = 1 << s
 allocate cache.sets[S]
 for i in [0..S-1]:
 allocate sets[i].lines[E]; init valid=0, tag=0, last_touch=0
 cache.hits=cache.misses=cache.evictions=0; cache.time_counter=0
ACCESS(addr):
 set_idx = (addr >> b) & ((1UL<<s)-1)
 tag = addr >> (s+b)
 set = sets[set_idx]
 // hit?
 for line in set.lines:
 if line.valid && line.tag==tag:
 hits++; line.last_touch=++time_counter; if verbose print(" hit");
return
 // miss
 misses++; if verbose print(" miss")
 // empty?
 target = first line with valid==0 else LRU line (min last_touch)
 if target.valid==1: evictions++; if verbose print(" eviction")
 target.valid=1; target.tag=tag; target.last_touch=++time_counter
RUN(trace):
 for each line:
 if op=='I': continue
 parse addr
 if op in {L,S}: ACCESS(addr)
 if op=='M': ACCESS(addr); ACCESS(addr)
MAIN: parse args; INIT; RUN; print results; free


parse_args_(): s, E, b, tracefile, verbose
init_cache(s, E, b)
open(tracefile)
for each trace line:
 if line begins with 'I': continue
 parse op, addr_hex, size
 if op == 'L': ACCESS(addr)
 if op == 'S': ACCESS(addr)
 if op == 'M': ACCESS(addr); ACCESS(addr)
print("hits:%lu misses:%lu evictions:%lu", ...)
free_cache()