  lw 0 1 anotherone   load value 1 into register 1
  lw 0 2 twothree     load value 23 into register 2
  add 1 2 3
  beq 0 0 done        UNDECLARED LABEL: done
  noop
  halt
anotherone .fill 1
twothree .fill 23
