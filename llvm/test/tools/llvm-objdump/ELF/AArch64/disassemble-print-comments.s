## Check that 'llvm-objdump -d' prints comments generated by the disassembler.

# RUN: llvm-mc -filetype=obj -triple=aarch64 -mattr=+sve %s -o %t
# RUN: llvm-objdump -d --mattr=+sve --no-show-raw-insn %t | FileCheck %s

# CHECK:      0000000000000000 <foo>:
# CHECK-NEXT:        0:   add x0, x2, #2, lsl #12    // =8192
# CHECK-NEXT:        4:   add z31.d, z31.d, #65280   // =0xff00

## Check that comments and locations of variables can be printed together.
# RUN: llvm-objdump -d --mattr=+sve --debug-vars --no-show-raw-insn %t | \
# RUN:   FileCheck %s --check-prefix=DBGVARS

# DBGVARS:      0000000000000000 <foo>:
# DBGVARS-NEXT:                                                     ┠─ bar = W1
# DBGVARS-NEXT:        0:   add x0, x2, #2, lsl #12    // =8192     ┃
# DBGVARS-NEXT:        4:   add z31.d, z31.d, #65280   // =0xff00   ┻

    .text
foo:
    add x0, x2, 8192
    add z31.d, z31.d, #65280
.LFooEnd:

    .section .debug_abbrev,"",@progbits
    .uleb128 1                      // Abbreviation Code
    .uleb128 0x11                   // DW_TAG_compile_unit
    .byte 1                         // DW_CHILDREN_yes
    .byte 0                         // EOM(1)
    .byte 0                         // EOM(2)
    .uleb128 2                      // Abbreviation Code
    .uleb128 0x2e                   // DW_TAG_subprogram
    .byte 1                         // DW_CHILDREN_yes
    .uleb128 0x11                   // DW_AT_low_pc
    .uleb128 0x01                   // DW_FORM_addr
    .uleb128 0x12                   // DW_AT_high_pc
    .uleb128 0x06                   // DW_FORM_data4
    .byte 0                         // EOM(1)
    .byte 0                         // EOM(2)
    .uleb128 3                      // Abbreviation Code
    .uleb128 0x34                   // DW_TAG_variable
    .byte 0                         // DW_CHILDREN_no
    .uleb128 0x02                   // DW_AT_location
    .uleb128 0x18                   // DW_FORM_exprloc
    .uleb128 0x03                   // DW_AT_name
    .uleb128 0x08                   // DW_FORM_string
    .byte 0                         // EOM(1)
    .byte 0                         // EOM(2)
    .byte 0                         // EOM(3)

    .section .debug_info,"",@progbits
    .long .LCuEnd-.LCuBegin         // Length of Unit
.LCuBegin:
    .short 4                        // DWARF version number
    .long .debug_abbrev             // Offset Into Abbrev. Section
    .byte 8                         // Address Size
    .uleb128 1                      // Abbrev [1] DW_TAG_compile_unit
    .uleb128 2                      // Abbrev [2] DW_TAG_subprogram
    .quad foo                       // DW_AT_low_pc
    .long .LFooEnd-foo              // DW_AT_high_pc
    .uleb128 3                      // Abbrev [3] DW_TAG_variable
    .byte .LLocEnd-.LLocBegin       // DW_AT_location
.LLocBegin:
    .byte 0x51                      // DW_OP_reg1
.LLocEnd:
    .asciz "bar"                    // DW_FORM_string
    .byte 0                         // End Of Children Mark
    .byte 0                         // End Of Children Mark
.LCuEnd:
