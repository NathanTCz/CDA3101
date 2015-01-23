/* instruction-level simulator for LC3101 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define NUMMEMORY 32 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

/* A mask with x least-significant bits set, possibly 0 or >=32 */
#define BIT_MASK(x) (((x) >= sizeof(unsigned) * CHAR_BIT) ? (unsigned) -1 : (1U << (x)) - 1)
#define log2(n) ( log(n) / log(2) )

int TIMESTAMP;

enum { add, nand, lw, sw, beq, cmov, halt, noop };
enum { fetch, store };
enum { false, true };
enum actionType
        {cacheToProcessor, processorToCache, memoryToCache, cacheToMemory,
        cacheToNowhere};

typedef struct cache_line_struct {
  int tag;
  int valid;
  int dirty;
  int lru;
  int access_timestamp;
  int mem_head;
  int *lines;
} cache_block;

typedef struct cache_set_struct {
  cache_block *blocks;
} cache_set;

typedef struct stateStruct {
  int pc;
  int mem[NUMMEMORY];
  int reg[NUMREGS];
  int numMemory;

  cache_set *CACHE;
} stateType;

int comp (const cache_block elem1, const cache_block elem2) {
    int f = elem1.access_timestamp;
    int s = elem2.access_timestamp;
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
}


/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *     cacheToProcessor: reading data from the cache to the processor
 *     processorToCache: writing data from the processor to the cache
 *     memoryToCache: reading data from the memory to the cache
 *     cacheToMemory: evicting cache data by writing it to the memory
 *     cacheToNowhere: evicting cache data by throwing it away
 */
void
printAction(int address, int size, enum actionType type)
{
    printf("@@@ transferring word [%d-%d] ", address, address + size - 1);
    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    } else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    } else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    } else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    } else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
}

int
cache_op(int op, int addr, int val, stateType *state, int n_sets, int b_size, int bps) {
  int block_offset = (addr & BIT_MASK( (int)log2(b_size) ) );
  int set_index = ( (addr >> (int)log2(b_size)) & BIT_MASK( (int)log2(n_sets) ) );
  int tag = (addr >> (int)( log2(b_size) + log2(n_sets) ) );

/*  printf("the address is %d\n the block_offset is %d\n the set_index is %d\n the tag is %d\n\n",
          addr, block_offset, set_index, tag);*/

  int data;
  data = 0;
  int i;
  int j;
  int cur_blk;
  int valid_found = false;

  int mem_block_head = get_block_head(addr, b_size);


  if ( (data = tag_exists(tag, state, set_index, block_offset, bps)) != -1 ) {
    //printf("tag exists\n");
    printAction(addr, 1, 0);
    return data;
  }

  else if ( (data = tag_exists(tag, state, set_index, block_offset, bps)) == -1 ) {
    // cache store
    while( !valid_found) {
      for (i = 0; i < bps; ++i) {
        if ( !state->CACHE[set_index].blocks[i].valid ) {
          valid_found = true;
          cur_blk = 0;
          state->CACHE[set_index].blocks[i].tag = tag;

          printAction(mem_block_head, b_size, 2);
          for (j = mem_block_head; j < (mem_block_head + b_size); ++j) {
            state->CACHE[set_index].blocks[i].lines[cur_blk] = state->mem[j];
            cur_blk++;
          }

          // Mark block valid
          state->CACHE[set_index].blocks[i].valid = true;
          state->CACHE[set_index].blocks[i].mem_head = mem_block_head;
          state->CACHE[set_index].blocks[i].access_timestamp = TIMESTAMP;

          if (op == 0) {
            printAction(addr, 1, 0);
            return state->CACHE[set_index].blocks[i].lines[block_offset];
          }
          else if (op == 1) {
            printAction(addr, 1, 1);
            state->CACHE[set_index].blocks[i].lines[block_offset] = val;
            state->CACHE[set_index].blocks[i].dirty = true;
            break;
          }
        }
      }
      if (valid_found == false) {
        kick_lru(b_size, set_index, bps, state);
      }
    }
  }

  TIMESTAMP++;
}

int
kick_lru(int b_size, int s_index, int bps, stateType *state) {
  qsort (state->CACHE[s_index].blocks, bps, sizeof(cache_block), comp);
  state->CACHE[s_index].blocks[0].lru = true;

  int i;
  int j;
  int m_head;
  int cur_blk;

  for (i = 0; i < bps; ++i) {
    if (state->CACHE[s_index].blocks[i].lru) {
      m_head = state->CACHE[s_index].blocks[i].mem_head;
      state->CACHE[s_index].blocks[i].valid = false;
      state->CACHE[s_index].blocks[i].lru = false;
      state->CACHE[s_index].blocks[i].access_timestamp = 999;
      state->CACHE[s_index].blocks[i].tag = 999;

      if (state->CACHE[s_index].blocks[i].dirty == true) {
        cur_blk = 0;

        // re-init cache block
        state->CACHE[s_index].blocks[i].dirty = false;

        //printf("the cache block [%d-%d] was dirty\n", m_head, m_head + (b_size -1));

        printAction(m_head, b_size, 3);

        for (j = m_head; j < (m_head + b_size); ++j) {
          state->mem[j] = state->CACHE[s_index].blocks[i].lines[cur_blk];
        }
        return 0;
      }
      else {
        printAction(m_head, b_size, 4);
        return 0;
      }
    }
  }
  
}

int
tag_exists(int tag, stateType *state, int set_i, int b_off, int bps) {

  //printf("tag check:: tag: %d, set_i: %d, b_off: %d\n", tag, set_i, b_off);

  int i;
  for (i = 0; i < bps; ++i) {
    if ( state->CACHE[set_i].blocks[i].tag == tag ) {
      return state->CACHE[set_i].blocks[i].lines[b_off];
    }
  }

  return -1;
}

int
get_block_head(int addr, int b_size) {
  return ( (addr / b_size) * b_size );
}

void
printState(stateType *statePtr)
{
  int i;
  printf("\n@@@\nstate:\n");
  printf("\tpc %d\n", statePtr->pc);
  printf("\tmemory:\n");

  for (i=0; i< NUMMEMORY; i++) {
    printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
  }
  printf("\tregisters:\n");

  for (i=0; i<NUMREGS; i++) {
    printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
  }
  printf("end state\n");
}

void
printCache(stateType *state, int num_sets, int b_size, int bps) {
  printf("cache\n");
  int i;
  int j;
  int k;
  for (i = 0; i < num_sets; ++i) {
    printf("\tcache set %d\n", i);
    for (k = 0; k < bps; ++k) {
      printf("\t\tdirty: %d  block #%d\n", state->CACHE[i].blocks[k].dirty, k);
      for (j = 0; j < b_size; ++j) {
        printf("\t\t\tblock");
        printf("[%d] = %d\n", j, state->CACHE[i].blocks[k].lines[j]);
      }
    }
  }
  printf("\n\n");
}

int
convertNum(int num)
{
  /* convert a 16-bit number into a 32-bit Sun integer */
  if (num & ( 1 << 15 ) ) {
    num -= ( 1 << 16 );
  } 
  return(num);
}

int
main(int argc, char *argv[])
{
  char line[MAXLINELENGTH];
  stateType state;
  FILE *filePtr;

  TIMESTAMP = 0;

  /*
   * Cache parameters
  */
  int cache_size;
  int block_size;
  int blocks_per_set;
  int number_sets;
  int tag_size;

  /*
   * Instruction Info
  */
  int opcode;
  int regA;
  int regB;
  int destR;
  int offset;
  int num_instr;
  int is_halt;

  if (argc != 5) {
    printf("error: usage: %s <machine-code file> blockSizeInWords numberOfSets blocksPerSet\n", argv[0]);
    exit(1);
  }

  filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
      printf("error: can't open file %s", argv[1]);
      perror("fopen");
      exit(1);
      }

  /*
   * Initialise the state of the machine. Initialise all of
   * the mem to 0;
   */
  int i;
  for (i = 0; i < NUMMEMORY; ++i) {
    state.mem[i] = 0;
  }

  /* read in the entire machine-code file into memory */
  for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
    state.numMemory++) {
    if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
        printf("error in reading address %d\n", state.numMemory);
        exit(1);
    }
    //printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
  }

  /*
   * Initialise the state of the machine. Initialise all of
   * the registers to 0;
   */
  for (i = 0; i < NUMREGS; ++i) {
    state.reg[i] = 0;
  }


  /*
   * CACHE INIT
  */
  block_size = atoi(argv[2]);
  number_sets = atoi(argv[3]);
  blocks_per_set = atoi(argv[4]);
  cache_size = ( block_size * number_sets * blocks_per_set );

  state.CACHE = malloc( number_sets * sizeof(cache_set *) );

  for (i = 0; i < number_sets; ++i) {
    cache_set new_cache_set;

    new_cache_set.blocks = malloc ( blocks_per_set * sizeof(cache_block) );

    int j;
    int k;
    for (j = 0; j < blocks_per_set; ++j) {
      new_cache_set.blocks[j].lines = malloc ( block_size * sizeof(int) );
      new_cache_set.blocks[j].tag = 999;
      new_cache_set.blocks[j].valid = false;
      new_cache_set.blocks[j].dirty = false;
      new_cache_set.blocks[j].lru = false;
      new_cache_set.blocks[j].access_timestamp = 9999;

      for (k = 0; k < block_size; ++k)
        new_cache_set.blocks[j].lines[k] = 0;
    }

    state.CACHE[i] = new_cache_set;
  }


  num_instr = 0;
  state.pc = 0;
  is_halt = false;
  int mem_data;

  while ( !is_halt ) {
    mem_data = 999;
    //printState(&state);
    //printCache(&state, number_sets, block_size, blocks_per_set);

    

    int instr = cache_op( 0, state.pc, 0, &state, number_sets, block_size, blocks_per_set );
    //int instr = state.mem[state.pc];

    opcode = ( (instr >> 22) & 7 );

    regA = ( (instr >> 19) & 7 );
    regB = ( (instr >> 16) & 7 );

    destR = ( (instr >> 0) & 7 );

    offset = convertNum( (instr >> 0) & 65535 );

    //printf("pc is %d instr is %d, opcode is %d\n",state.pc, instr, opcode);

    switch (opcode) {
      case add:
        if ( destR != 0)
          state.reg[destR] = ( state.reg[regA] + state.reg[regB] );
        else
          exit(1);

        state.pc++;
        break;

      case nand:
        if ( destR != 0)
          state.reg[destR] = ~( state.reg[regA] & state.reg[regB] );
        else
          exit(1);

        state.pc++;
        break;

      case lw:
        if ( destR != 0) {
          //state.reg[regB] = state.mem[ (state.reg[regA] + offset) ];
          mem_data = cache_op( 0, (state.reg[regA] + offset), 0, &state, number_sets, block_size, blocks_per_set );
          state.reg[regB] = mem_data;
        }
        else
          exit(1);

        state.pc++;
        break;

      case sw:
        //state.mem[ (state.reg[regA] + offset) ] = state.reg[regB];
        cache_op( 1, (state.reg[regA] + offset), state.reg[regB], &state, number_sets, block_size, blocks_per_set );
        state.pc++;
        break;

      case beq:
        if ( state.reg[regA] == state.reg[regB] )
          state.pc = (state.pc + 1 + offset);
        else
          state.pc++;
        break;

      case cmov:
        if ( destR != 0)
          if ( state.reg[regB] != 0 )
            state.reg[destR] = state.reg[regA];
        else
          exit(1);

        state.pc++;
        break;

      case halt:
        is_halt = true;

        state.pc++;
        break;

      case noop:
        state.pc++;
        break;
    }

    num_instr++;
  }

/*  printf("machine halted\n");
  printf("total of %d instructions executed\n", num_instr);
  printf("final state of machine:\n");
  printState(&state);*/

  return(0);
}
