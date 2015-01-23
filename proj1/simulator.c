/* instruction-level simulator for LC3101 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

enum { add, nand, lw, sw, beq, cmov, halt, noop };
enum { false, true };

typedef struct stateStruct {
  int pc;
  int mem[NUMMEMORY];
  int reg[NUMREGS];
  int numMemory;
} stateType;

void printState(stateType *);

int
main(int argc, char *argv[])
{
  char line[MAXLINELENGTH];
  stateType state;
  FILE *filePtr;

  int opcode;
  int regA;
  int regB;
  int destR;
  int offset;
  int num_instr;
  int is_halt;

  if (argc != 2) {
    printf("error: usage: %s <machine-code file>\n", argv[0]);
    exit(1);
  }

  filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
      printf("error: can't open file %s", argv[1]);
      perror("fopen");
      exit(1);
      }

  /* read in the entire machine-code file into memory */
  for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
    state.numMemory++) {
    if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
        printf("error in reading address %d\n", state.numMemory);
        exit(1);
    }
    printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
  }

  /*
   * Initialise the state of the machine. Initialise all of
   * the registers to 0;
   */
  int i;
  for (i = 0; i < NUMREGS; ++i) {
    state.reg[i] = 0;
  }

  num_instr = 0;
  state.pc = 0;
  is_halt = false;

  while ( !is_halt ) {
    printState(&state);

    opcode = ( (state.mem[state.pc] >> 22) & 7 );

    regA = ( (state.mem[state.pc] >> 19) & 7 );
    regB = ( (state.mem[state.pc] >> 16) & 7 );

    destR = ( (state.mem[state.pc] >> 0) & 7 );

    offset = convertNum( (state.mem[state.pc] >> 0) & 65535 );

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
        if ( destR != 0)
          state.reg[regB] = ( state.mem[ (state.reg[regA] + offset) ] );
        else
          exit(1);

        state.pc++;
        break;

      case sw:
        state.mem[ (state.reg[regA] + offset) ] = state.reg[regB];

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

  printf("machine halted\n");
  printf("total of %d instructions executed\n", num_instr);
  printf("final state of machine:\n");
  printState(&state);


  return(0);
}

void
printState(stateType *statePtr)
{
  int i;
  printf("\n@@@\nstate:\n");
  printf("\tpc %d\n", statePtr->pc);
  printf("\tmemory:\n");

  for (i=0; i<statePtr->numMemory; i++) {
    printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
  }
  printf("\tregisters:\n");

  for (i=0; i<NUMREGS; i++) {
    printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
  }
  printf("end state\n");
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