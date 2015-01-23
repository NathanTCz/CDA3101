  lw  0 2 mplier load reg1 with mcand
  lw  0 3 mcand  load reg2 with mplier
  lw  0 4 one load value 1 for a mutator into reg3
  lw  0 5 counter this value is used as a counter
  lw  0 6 negOne
loop  beq 0 5 done
  nand 2 4 7
  nand 7 7 7
  beq 7 4 adder
  add 4 4 4
  add 3 3 3
  add 5 6 5
  beq 0 0 loop
adder add 3 1 1
  add 4 4 4
  add 3 3 3
  add 5 6 5
  beq 0 0 loop
done halt
mcand .fill 32766
mplier .fill 10383
one .fill 1
negOne .fill -1
counter .fill 15
