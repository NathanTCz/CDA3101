/* Assembler code fragment for LC3101 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLINELENGTH 1000
#define MAXINSTR 1000

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

struct instr {
    int addr;
    char *lbl;
};

enum { add, nand, lw, sw, beq, cmov, halt, noop };
enum { false, true };

int
main(int argc, char *argv[])
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label  [MAXLINELENGTH],
         opcode [MAXLINELENGTH], 
         arg0   [MAXLINELENGTH],
         arg1   [MAXLINELENGTH], 
         arg2   [MAXLINELENGTH];

    struct instr *labels;
    labels = malloc( sizeof(struct instr) * MAXINSTR );

    int line_number = 0;
    int mc = 0;
    int count = 0;
    int label_found = false;

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
            argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }
    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    /*
     * Make the first pass through to grab the labels and store the
     * addresses.
     */
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2) ) {
        labels[line_number].addr = line_number;

        if ( label == '\0' )
            strcpy(labels[line_number].lbl, '\0');
        else {
            labels[line_number].lbl = malloc( sizeof(label) );
            strcpy(labels[line_number].lbl, label);
        }

        line_number++;
    }

    /*
     * ERROR CHECK: make sure there are no duplicate labels
     */
    int i;
    int j;
    for ( i = 0; i < line_number; ++i ) {
        if ( strcmp(labels[i].lbl, "") ) {
            for ( j = 0; j < line_number; ++j) {
                if ( j != i )
                    if ( !strcmp(labels[i].lbl, labels[j].lbl) )
                        exit(1);
            }
        }
    }

    rewind(inFilePtr);

    /*
     * Make the second pass, this time 
     */
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2) ) {
        /*
         * R-Type instructions (add, nand)
         */
        if ( !strcmp(opcode, "add") || !strcmp(opcode, "nand") || !strcmp(opcode, "cmov")) {
            if ( !strcmp(opcode, "add") )
                mc = (add << 22);
            else if ( !strcmp(opcode, "nand") )
                mc = (nand << 22);
            else
                mc = (cmov << 22);

            /* Get arg0 and add to machine code as regA */
            if ( isNumber(arg0) )
                mc += ( atoi(arg0) << 19 );
            else
                exit(1);

            /* Get arg1 and add to machine code as regB */
            if ( isNumber(arg1) )
                mc += ( atoi(arg1) << 16 );
            else
                exit(1);

            /* Get destR and add to machine code as destR */
            if ( isNumber(arg2) )
                mc += ( atoi(arg2) << 0 );
            else
                exit(1);

            fprintf(outFilePtr, "%i\n", mc);
        }

        /*
         * I-Type instructions (lw, sw, beq)
         */
        else if ( !strcmp(opcode, "lw") || !strcmp(opcode, "sw") ) {
            if ( !strcmp(opcode, "lw") )
                mc = (lw << 22);
            else
                mc = (sw << 22);

            /* Get arg0 and add to machine code as regA */
            if ( isNumber(arg0) )
                mc += ( atoi(arg0) << 19 );
            else
                exit(1);

            /* Get arg1 and add to machine code as regB */
            if ( isNumber(arg1) )
                mc += ( atoi(arg1) << 16 );
            else
                exit(1);

            /* Get arg2 and add to machine code as offset */
            if ( isNumber(arg2) )
                mc += ( atoi(arg2) );
            else {
                int k;
                for ( k = 0; k < line_number; ++k ) {
                    if ( !strcmp(labels[k].lbl, arg2) ) {
                        mc += labels[k].addr << 0;

                        /* set label_found to true for error handling*/
                        label_found = true;
                        break;
                    }
                }
                /* exit with error if no matching label found */
                if ( !label_found )
                    exit(1);
            }

            fprintf(outFilePtr, "%i\n", mc);
        }

        else if ( !strcmp(opcode, "beq") ) {
            mc = (beq << 22);

            /* Get arg0 and add to machine code as regA */
            if ( isNumber(arg0) )
                mc += ( atoi(arg0) << 19 );
            else
                exit(1);

            /* Get arg1 and add to machine code as regB */
            if ( isNumber(arg1) )
                mc += ( atoi(arg1) << 16 );
            else
                exit(1);

            /* Get arg2 and add to machine code as offset */
            if ( isNumber(arg2) )
                mc += ( atoi(arg2) );
            else {
                int k;
                for ( k = 0; k < line_number; ++k ) {
                    if ( !strcmp(labels[k].lbl, arg2) ) {
                        int num = labels[k].addr - count - 1;

                        /* set label_found to true for error handling*/
                        label_found = true;

                        if (num < 0) {

                            /*
                             * numbers can on range from -32768 to 32767.
                             * total of 65535. XOR to get handle negative
                             * offsets
                             */
                            num = 65535 ^ -num;

                            num += 1;
                            mc += num;
                        }
                        else
                            mc += num;
                        
                        break;
                    }
                }
                /* exit with error if no matching label found */
                if ( !label_found )
                    exit(1);
            }

            fprintf(outFilePtr, "%i\n", mc);
        }

        /*
         * O-Type instructions (noop, halt)
         */
        else if ( !strcmp(opcode, "noop") || !strcmp(opcode, "halt") ) {
            if ( !strcmp(opcode, "noop") )
                mc = (noop << 22);
            else
                mc = (halt << 22);

            fprintf(outFilePtr, "%i\n", mc);
        }

        /*
         * Assembler directive .fill handler
         */
        else if ( !strcmp(opcode, ".fill") ) {
            /* Get arg0 and add to machine code as address or value */
            if ( isNumber(arg0) )
                mc = ( atoi(arg0) );
            else {
                int k;
                for ( k = 0; k < line_number; ++k ) {
                    if ( !strcmp(labels[k].lbl, arg0) ) {
                        mc = labels[k].addr << 0;

                        /* set label_found to true for error handling*/
                        label_found = true;
                        break;
                    }
                }
                /* exit with error if no matching label found */
                if ( !label_found )
                    exit(1);
            }

            fprintf(outFilePtr, "%i\n", mc);
        }
        /*
         * ELSE this means there is an unsupported opcode
         * and we should exit(1) with an error
         */
        else
            exit(1);

        count++;
    }

    return(0);
}

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int
readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
    char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
  /* reached end of file */
        return(0);
    }

    /* check for line too long (by looking for a \n) */
    if (strchr(line, '\n') == NULL) {
        /* line too long */
  printf("error: line too long\n");
  exit(1);
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label)) {
  /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n ]%[^\t\n ]%*[\t\n ]%[^\t\n ]%*[\t\n ]%[^\t\n ]%*[\t\n ]%[^\t\n ]",
        opcode, arg0, arg1, arg2);

    return(1);
}

int
isNumber(char *string)
{
    /* return 1 if string is a number */
    int i;
    return( (sscanf(string, "%d", &i)) == 1);
}