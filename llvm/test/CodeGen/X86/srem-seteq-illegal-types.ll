; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=i686-unknown-linux-gnu < %s | FileCheck %s --check-prefixes=X86
; RUN: llc -mtriple=x86_64-unknown-linux-gnu < %s | FileCheck %s --check-prefixes=X64
; RUN: llc -mtriple=x86_64-unknown-linux-gnu -mattr=+sse2 < %s | FileCheck %s --check-prefixes=X64,SSE2
; RUN: llc -mtriple=x86_64-unknown-linux-gnu -mattr=+sse4.1 < %s | FileCheck %s --check-prefixes=X64,SSE41
; RUN: llc -mtriple=x86_64-unknown-linux-gnu -mattr=+avx < %s | FileCheck %s --check-prefixes=X64,AVX1
; RUN: llc -mtriple=x86_64-unknown-linux-gnu -mattr=+avx2 < %s | FileCheck %s --check-prefixes=X64,AVX2
; RUN: llc -mtriple=x86_64-unknown-linux-gnu -mattr=+avx512f,+avx512vl < %s | FileCheck %s --check-prefixes=X64,AVX512VL

define i1 @test_srem_odd(i29 %X) nounwind {
; X86-LABEL: test_srem_odd:
; X86:       # %bb.0:
; X86-NEXT:    imull $526025035, {{[0-9]+}}(%esp), %eax # imm = 0x1F5A814B
; X86-NEXT:    addl $2711469, %eax # imm = 0x295FAD
; X86-NEXT:    andl $536870911, %eax # imm = 0x1FFFFFFF
; X86-NEXT:    cmpl $5422939, %eax # imm = 0x52BF5B
; X86-NEXT:    setb %al
; X86-NEXT:    retl
;
; X64-LABEL: test_srem_odd:
; X64:       # %bb.0:
; X64-NEXT:    imull $526025035, %edi, %eax # imm = 0x1F5A814B
; X64-NEXT:    addl $2711469, %eax # imm = 0x295FAD
; X64-NEXT:    andl $536870911, %eax # imm = 0x1FFFFFFF
; X64-NEXT:    cmpl $5422939, %eax # imm = 0x52BF5B
; X64-NEXT:    setb %al
; X64-NEXT:    retq
  %srem = srem i29 %X, 99
  %cmp = icmp eq i29 %srem, 0
  ret i1 %cmp
}

define i1 @test_srem_even(i4 %X) nounwind {
; X86-LABEL: test_srem_even:
; X86:       # %bb.0:
; X86-NEXT:    movb {{[0-9]+}}(%esp), %al
; X86-NEXT:    shlb $4, %al
; X86-NEXT:    sarb $4, %al
; X86-NEXT:    movsbl %al, %ecx
; X86-NEXT:    imull $43, %ecx, %ecx
; X86-NEXT:    movzwl %cx, %ecx
; X86-NEXT:    movl %ecx, %edx
; X86-NEXT:    shrl $15, %edx
; X86-NEXT:    addb %ch, %dl
; X86-NEXT:    movzbl %dl, %ecx
; X86-NEXT:    addl %ecx, %ecx
; X86-NEXT:    leal (%ecx,%ecx,2), %ecx
; X86-NEXT:    subb %cl, %al
; X86-NEXT:    cmpb $1, %al
; X86-NEXT:    sete %al
; X86-NEXT:    retl
;
; X64-LABEL: test_srem_even:
; X64:       # %bb.0:
; X64-NEXT:    shlb $4, %dil
; X64-NEXT:    sarb $4, %dil
; X64-NEXT:    movsbl %dil, %eax
; X64-NEXT:    imull $43, %eax, %ecx
; X64-NEXT:    movzwl %cx, %ecx
; X64-NEXT:    movl %ecx, %edx
; X64-NEXT:    shrl $15, %edx
; X64-NEXT:    shrl $8, %ecx
; X64-NEXT:    addb %dl, %cl
; X64-NEXT:    movzbl %cl, %ecx
; X64-NEXT:    addl %ecx, %ecx
; X64-NEXT:    leal (%rcx,%rcx,2), %ecx
; X64-NEXT:    subb %cl, %al
; X64-NEXT:    cmpb $1, %al
; X64-NEXT:    sete %al
; X64-NEXT:    retq
  %srem = srem i4 %X, 6
  %cmp = icmp eq i4 %srem, 1
  ret i1 %cmp
}

define i1 @test_srem_pow2_setne(i6 %X) nounwind {
; X86-LABEL: test_srem_pow2_setne:
; X86:       # %bb.0:
; X86-NEXT:    movb {{[0-9]+}}(%esp), %al
; X86-NEXT:    movl %eax, %ecx
; X86-NEXT:    shlb $2, %cl
; X86-NEXT:    sarb $5, %cl
; X86-NEXT:    shrb $4, %cl
; X86-NEXT:    andb $3, %cl
; X86-NEXT:    addb %al, %cl
; X86-NEXT:    andb $60, %cl
; X86-NEXT:    subb %cl, %al
; X86-NEXT:    testb $63, %al
; X86-NEXT:    setne %al
; X86-NEXT:    retl
;
; X64-LABEL: test_srem_pow2_setne:
; X64:       # %bb.0:
; X64-NEXT:    # kill: def $edi killed $edi def $rdi
; X64-NEXT:    leal (,%rdi,4), %eax
; X64-NEXT:    sarb $5, %al
; X64-NEXT:    shrb $4, %al
; X64-NEXT:    andb $3, %al
; X64-NEXT:    addb %dil, %al
; X64-NEXT:    andb $60, %al
; X64-NEXT:    subb %al, %dil
; X64-NEXT:    testb $63, %dil
; X64-NEXT:    setne %al
; X64-NEXT:    retq
  %srem = srem i6 %X, 4
  %cmp = icmp ne i6 %srem, 0
  ret i1 %cmp
}

define <3 x i1> @test_srem_vec(<3 x i33> %X) nounwind {
; X86-LABEL: test_srem_vec:
; X86:       # %bb.0:
; X86-NEXT:    pushl %ebp
; X86-NEXT:    pushl %ebx
; X86-NEXT:    pushl %edi
; X86-NEXT:    pushl %esi
; X86-NEXT:    subl $12, %esp
; X86-NEXT:    movl {{[0-9]+}}(%esp), %edi
; X86-NEXT:    andl $1, %edi
; X86-NEXT:    negl %edi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %ebx
; X86-NEXT:    movl {{[0-9]+}}(%esp), %ebp
; X86-NEXT:    andl $1, %ebp
; X86-NEXT:    negl %ebp
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    andl $1, %eax
; X86-NEXT:    negl %eax
; X86-NEXT:    pushl $-1
; X86-NEXT:    pushl $-9
; X86-NEXT:    pushl %eax
; X86-NEXT:    pushl {{[0-9]+}}(%esp)
; X86-NEXT:    calll __moddi3
; X86-NEXT:    addl $16, %esp
; X86-NEXT:    movl %eax, %esi
; X86-NEXT:    movl %edx, {{[-0-9]+}}(%e{{[sb]}}p) # 4-byte Spill
; X86-NEXT:    pushl $0
; X86-NEXT:    pushl $9
; X86-NEXT:    pushl %ebp
; X86-NEXT:    pushl %ebx
; X86-NEXT:    calll __moddi3
; X86-NEXT:    addl $16, %esp
; X86-NEXT:    movl %eax, %ebx
; X86-NEXT:    movl %edx, %ebp
; X86-NEXT:    notl %ebp
; X86-NEXT:    pushl $0
; X86-NEXT:    pushl $9
; X86-NEXT:    pushl %edi
; X86-NEXT:    pushl {{[0-9]+}}(%esp)
; X86-NEXT:    calll __moddi3
; X86-NEXT:    addl $16, %esp
; X86-NEXT:    xorl $3, %eax
; X86-NEXT:    orl %edx, %eax
; X86-NEXT:    setne %al
; X86-NEXT:    xorl $3, %esi
; X86-NEXT:    orl {{[-0-9]+}}(%e{{[sb]}}p), %esi # 4-byte Folded Reload
; X86-NEXT:    setne %cl
; X86-NEXT:    xorl $-3, %ebx
; X86-NEXT:    andl $1, %ebp
; X86-NEXT:    orl %ebx, %ebp
; X86-NEXT:    setne %dl
; X86-NEXT:    addl $12, %esp
; X86-NEXT:    popl %esi
; X86-NEXT:    popl %edi
; X86-NEXT:    popl %ebx
; X86-NEXT:    popl %ebp
; X86-NEXT:    retl
;
; SSE2-LABEL: test_srem_vec:
; SSE2:       # %bb.0:
; SSE2-NEXT:    movq %rdx, %rcx
; SSE2-NEXT:    shlq $31, %rcx
; SSE2-NEXT:    sarq $31, %rcx
; SSE2-NEXT:    shlq $31, %rdi
; SSE2-NEXT:    sarq $31, %rdi
; SSE2-NEXT:    shlq $31, %rsi
; SSE2-NEXT:    sarq $31, %rsi
; SSE2-NEXT:    movabsq $2049638230412172402, %r8 # imm = 0x1C71C71C71C71C72
; SSE2-NEXT:    movq %rsi, %rax
; SSE2-NEXT:    imulq %r8
; SSE2-NEXT:    movq %rdx, %rax
; SSE2-NEXT:    shrq $63, %rax
; SSE2-NEXT:    addq %rdx, %rax
; SSE2-NEXT:    leaq (%rax,%rax,8), %rax
; SSE2-NEXT:    subq %rax, %rsi
; SSE2-NEXT:    movq %rsi, %xmm1
; SSE2-NEXT:    movq %rdi, %rax
; SSE2-NEXT:    imulq %r8
; SSE2-NEXT:    movq %rdx, %rax
; SSE2-NEXT:    shrq $63, %rax
; SSE2-NEXT:    addq %rdx, %rax
; SSE2-NEXT:    leaq (%rax,%rax,8), %rax
; SSE2-NEXT:    subq %rax, %rdi
; SSE2-NEXT:    movq %rdi, %xmm0
; SSE2-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    movdqa {{.*#+}} xmm1 = [8589934591,8589934591]
; SSE2-NEXT:    pand %xmm1, %xmm0
; SSE2-NEXT:    movabsq $2049638230412172401, %rdx # imm = 0x1C71C71C71C71C71
; SSE2-NEXT:    movq %rcx, %rax
; SSE2-NEXT:    imulq %rdx
; SSE2-NEXT:    subq %rcx, %rdx
; SSE2-NEXT:    movq %rdx, %rax
; SSE2-NEXT:    shrq $63, %rax
; SSE2-NEXT:    sarq $3, %rdx
; SSE2-NEXT:    addq %rax, %rdx
; SSE2-NEXT:    leaq (%rdx,%rdx,8), %rax
; SSE2-NEXT:    addq %rcx, %rax
; SSE2-NEXT:    movq %rax, %xmm2
; SSE2-NEXT:    pand %xmm1, %xmm2
; SSE2-NEXT:    pcmpeqd {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %xmm0
; SSE2-NEXT:    pcmpeqd {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %xmm2
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    shufps {{.*#+}} xmm1 = xmm1[1,3],xmm2[1,2]
; SSE2-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,2],xmm2[0,3]
; SSE2-NEXT:    andps %xmm1, %xmm0
; SSE2-NEXT:    pcmpeqd %xmm1, %xmm1
; SSE2-NEXT:    pxor %xmm0, %xmm1
; SSE2-NEXT:    movdqa %xmm1, -{{[0-9]+}}(%rsp)
; SSE2-NEXT:    movb -{{[0-9]+}}(%rsp), %al
; SSE2-NEXT:    movb -{{[0-9]+}}(%rsp), %dl
; SSE2-NEXT:    movb -{{[0-9]+}}(%rsp), %cl
; SSE2-NEXT:    retq
;
; SSE41-LABEL: test_srem_vec:
; SSE41:       # %bb.0:
; SSE41-NEXT:    movq %rdx, %rcx
; SSE41-NEXT:    shlq $31, %rcx
; SSE41-NEXT:    sarq $31, %rcx
; SSE41-NEXT:    shlq $31, %rdi
; SSE41-NEXT:    sarq $31, %rdi
; SSE41-NEXT:    shlq $31, %rsi
; SSE41-NEXT:    sarq $31, %rsi
; SSE41-NEXT:    movabsq $2049638230412172402, %r8 # imm = 0x1C71C71C71C71C72
; SSE41-NEXT:    movq %rsi, %rax
; SSE41-NEXT:    imulq %r8
; SSE41-NEXT:    movq %rdx, %rax
; SSE41-NEXT:    shrq $63, %rax
; SSE41-NEXT:    addq %rdx, %rax
; SSE41-NEXT:    leaq (%rax,%rax,8), %rax
; SSE41-NEXT:    subq %rax, %rsi
; SSE41-NEXT:    movq %rsi, %xmm1
; SSE41-NEXT:    movq %rdi, %rax
; SSE41-NEXT:    imulq %r8
; SSE41-NEXT:    movq %rdx, %rax
; SSE41-NEXT:    shrq $63, %rax
; SSE41-NEXT:    addq %rdx, %rax
; SSE41-NEXT:    leaq (%rax,%rax,8), %rax
; SSE41-NEXT:    subq %rax, %rdi
; SSE41-NEXT:    movq %rdi, %xmm0
; SSE41-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE41-NEXT:    movdqa {{.*#+}} xmm1 = [8589934591,8589934591]
; SSE41-NEXT:    pand %xmm1, %xmm0
; SSE41-NEXT:    movabsq $2049638230412172401, %rdx # imm = 0x1C71C71C71C71C71
; SSE41-NEXT:    movq %rcx, %rax
; SSE41-NEXT:    imulq %rdx
; SSE41-NEXT:    subq %rcx, %rdx
; SSE41-NEXT:    movq %rdx, %rax
; SSE41-NEXT:    shrq $63, %rax
; SSE41-NEXT:    sarq $3, %rdx
; SSE41-NEXT:    addq %rax, %rdx
; SSE41-NEXT:    leaq (%rdx,%rdx,8), %rax
; SSE41-NEXT:    addq %rcx, %rax
; SSE41-NEXT:    movq %rax, %xmm2
; SSE41-NEXT:    pand %xmm1, %xmm2
; SSE41-NEXT:    pcmpeqq {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %xmm0
; SSE41-NEXT:    pcmpeqd %xmm1, %xmm1
; SSE41-NEXT:    pxor %xmm1, %xmm0
; SSE41-NEXT:    pcmpeqq {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %xmm2
; SSE41-NEXT:    pxor %xmm1, %xmm2
; SSE41-NEXT:    pextrb $0, %xmm0, %eax
; SSE41-NEXT:    pextrb $8, %xmm0, %edx
; SSE41-NEXT:    pextrb $0, %xmm2, %ecx
; SSE41-NEXT:    # kill: def $al killed $al killed $eax
; SSE41-NEXT:    # kill: def $dl killed $dl killed $edx
; SSE41-NEXT:    # kill: def $cl killed $cl killed $ecx
; SSE41-NEXT:    retq
;
; AVX1-LABEL: test_srem_vec:
; AVX1:       # %bb.0:
; AVX1-NEXT:    movq %rdx, %rcx
; AVX1-NEXT:    shlq $31, %rcx
; AVX1-NEXT:    sarq $31, %rcx
; AVX1-NEXT:    shlq $31, %rdi
; AVX1-NEXT:    sarq $31, %rdi
; AVX1-NEXT:    shlq $31, %rsi
; AVX1-NEXT:    sarq $31, %rsi
; AVX1-NEXT:    movabsq $2049638230412172402, %r8 # imm = 0x1C71C71C71C71C72
; AVX1-NEXT:    movq %rsi, %rax
; AVX1-NEXT:    imulq %r8
; AVX1-NEXT:    movq %rdx, %rax
; AVX1-NEXT:    shrq $63, %rax
; AVX1-NEXT:    addq %rdx, %rax
; AVX1-NEXT:    leaq (%rax,%rax,8), %rax
; AVX1-NEXT:    subq %rax, %rsi
; AVX1-NEXT:    vmovq %rsi, %xmm0
; AVX1-NEXT:    movq %rdi, %rax
; AVX1-NEXT:    imulq %r8
; AVX1-NEXT:    movq %rdx, %rax
; AVX1-NEXT:    shrq $63, %rax
; AVX1-NEXT:    addq %rdx, %rax
; AVX1-NEXT:    leaq (%rax,%rax,8), %rax
; AVX1-NEXT:    subq %rax, %rdi
; AVX1-NEXT:    vmovq %rdi, %xmm1
; AVX1-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; AVX1-NEXT:    movabsq $2049638230412172401, %rdx # imm = 0x1C71C71C71C71C71
; AVX1-NEXT:    movq %rcx, %rax
; AVX1-NEXT:    imulq %rdx
; AVX1-NEXT:    subq %rcx, %rdx
; AVX1-NEXT:    movq %rdx, %rax
; AVX1-NEXT:    shrq $63, %rax
; AVX1-NEXT:    sarq $3, %rdx
; AVX1-NEXT:    addq %rax, %rdx
; AVX1-NEXT:    leaq (%rdx,%rdx,8), %rax
; AVX1-NEXT:    addq %rcx, %rax
; AVX1-NEXT:    vmovq %rax, %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    vandps {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %ymm0, %ymm0
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vpcmpeqq {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %xmm1, %xmm1
; AVX1-NEXT:    vpcmpeqd %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpxor %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vpcmpeqq {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %xmm0, %xmm0
; AVX1-NEXT:    vpxor %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    vpextrb $0, %xmm0, %eax
; AVX1-NEXT:    vpextrb $8, %xmm0, %edx
; AVX1-NEXT:    vpextrb $0, %xmm1, %ecx
; AVX1-NEXT:    # kill: def $al killed $al killed $eax
; AVX1-NEXT:    # kill: def $dl killed $dl killed $edx
; AVX1-NEXT:    # kill: def $cl killed $cl killed $ecx
; AVX1-NEXT:    vzeroupper
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_srem_vec:
; AVX2:       # %bb.0:
; AVX2-NEXT:    movq %rdx, %rcx
; AVX2-NEXT:    shlq $31, %rcx
; AVX2-NEXT:    sarq $31, %rcx
; AVX2-NEXT:    shlq $31, %rdi
; AVX2-NEXT:    sarq $31, %rdi
; AVX2-NEXT:    shlq $31, %rsi
; AVX2-NEXT:    sarq $31, %rsi
; AVX2-NEXT:    movabsq $2049638230412172402, %r8 # imm = 0x1C71C71C71C71C72
; AVX2-NEXT:    movq %rsi, %rax
; AVX2-NEXT:    imulq %r8
; AVX2-NEXT:    movq %rdx, %rax
; AVX2-NEXT:    shrq $63, %rax
; AVX2-NEXT:    addq %rdx, %rax
; AVX2-NEXT:    leaq (%rax,%rax,8), %rax
; AVX2-NEXT:    subq %rax, %rsi
; AVX2-NEXT:    vmovq %rsi, %xmm0
; AVX2-NEXT:    movq %rdi, %rax
; AVX2-NEXT:    imulq %r8
; AVX2-NEXT:    movq %rdx, %rax
; AVX2-NEXT:    shrq $63, %rax
; AVX2-NEXT:    addq %rdx, %rax
; AVX2-NEXT:    leaq (%rax,%rax,8), %rax
; AVX2-NEXT:    subq %rax, %rdi
; AVX2-NEXT:    vmovq %rdi, %xmm1
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; AVX2-NEXT:    movabsq $2049638230412172401, %rdx # imm = 0x1C71C71C71C71C71
; AVX2-NEXT:    movq %rcx, %rax
; AVX2-NEXT:    imulq %rdx
; AVX2-NEXT:    subq %rcx, %rdx
; AVX2-NEXT:    movq %rdx, %rax
; AVX2-NEXT:    shrq $63, %rax
; AVX2-NEXT:    sarq $3, %rdx
; AVX2-NEXT:    addq %rax, %rdx
; AVX2-NEXT:    leaq (%rdx,%rdx,8), %rax
; AVX2-NEXT:    addq %rcx, %rax
; AVX2-NEXT:    vmovq %rax, %xmm1
; AVX2-NEXT:    vinserti128 $1, %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    vpbroadcastq {{.*#+}} ymm1 = [8589934591,8589934591,8589934591,8589934591]
; AVX2-NEXT:    vpand %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    vpcmpeqq {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %ymm0, %ymm0
; AVX2-NEXT:    vpcmpeqd %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vpxor %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    vextracti128 $1, %ymm0, %xmm1
; AVX2-NEXT:    vpextrb $0, %xmm0, %eax
; AVX2-NEXT:    vpextrb $8, %xmm0, %edx
; AVX2-NEXT:    vpextrb $0, %xmm1, %ecx
; AVX2-NEXT:    # kill: def $al killed $al killed $eax
; AVX2-NEXT:    # kill: def $dl killed $dl killed $edx
; AVX2-NEXT:    # kill: def $cl killed $cl killed $ecx
; AVX2-NEXT:    vzeroupper
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: test_srem_vec:
; AVX512VL:       # %bb.0:
; AVX512VL-NEXT:    movq %rdx, %rcx
; AVX512VL-NEXT:    shlq $31, %rcx
; AVX512VL-NEXT:    sarq $31, %rcx
; AVX512VL-NEXT:    shlq $31, %rdi
; AVX512VL-NEXT:    sarq $31, %rdi
; AVX512VL-NEXT:    shlq $31, %rsi
; AVX512VL-NEXT:    sarq $31, %rsi
; AVX512VL-NEXT:    movabsq $2049638230412172402, %r8 # imm = 0x1C71C71C71C71C72
; AVX512VL-NEXT:    movq %rsi, %rax
; AVX512VL-NEXT:    imulq %r8
; AVX512VL-NEXT:    movq %rdx, %rax
; AVX512VL-NEXT:    shrq $63, %rax
; AVX512VL-NEXT:    addq %rdx, %rax
; AVX512VL-NEXT:    leaq (%rax,%rax,8), %rax
; AVX512VL-NEXT:    subq %rax, %rsi
; AVX512VL-NEXT:    vmovq %rsi, %xmm0
; AVX512VL-NEXT:    movq %rdi, %rax
; AVX512VL-NEXT:    imulq %r8
; AVX512VL-NEXT:    movq %rdx, %rax
; AVX512VL-NEXT:    shrq $63, %rax
; AVX512VL-NEXT:    addq %rdx, %rax
; AVX512VL-NEXT:    leaq (%rax,%rax,8), %rax
; AVX512VL-NEXT:    subq %rax, %rdi
; AVX512VL-NEXT:    vmovq %rdi, %xmm1
; AVX512VL-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; AVX512VL-NEXT:    movabsq $2049638230412172401, %rdx # imm = 0x1C71C71C71C71C71
; AVX512VL-NEXT:    movq %rcx, %rax
; AVX512VL-NEXT:    imulq %rdx
; AVX512VL-NEXT:    subq %rcx, %rdx
; AVX512VL-NEXT:    movq %rdx, %rax
; AVX512VL-NEXT:    shrq $63, %rax
; AVX512VL-NEXT:    sarq $3, %rdx
; AVX512VL-NEXT:    addq %rax, %rdx
; AVX512VL-NEXT:    leaq (%rdx,%rdx,8), %rax
; AVX512VL-NEXT:    addq %rcx, %rax
; AVX512VL-NEXT:    vmovq %rax, %xmm1
; AVX512VL-NEXT:    vinserti128 $1, %xmm1, %ymm0, %ymm0
; AVX512VL-NEXT:    vpandq {{\.?LCPI[0-9]+_[0-9]+}}(%rip){1to4}, %ymm0, %ymm0
; AVX512VL-NEXT:    vpcmpneqq {{\.?LCPI[0-9]+_[0-9]+}}(%rip), %ymm0, %k0
; AVX512VL-NEXT:    kshiftrw $1, %k0, %k1
; AVX512VL-NEXT:    kmovw %k1, %edx
; AVX512VL-NEXT:    kshiftrw $2, %k0, %k1
; AVX512VL-NEXT:    kmovw %k1, %ecx
; AVX512VL-NEXT:    kmovw %k0, %eax
; AVX512VL-NEXT:    # kill: def $al killed $al killed $eax
; AVX512VL-NEXT:    # kill: def $dl killed $dl killed $edx
; AVX512VL-NEXT:    # kill: def $cl killed $cl killed $ecx
; AVX512VL-NEXT:    vzeroupper
; AVX512VL-NEXT:    retq
  %srem = srem <3 x i33> %X, <i33 9, i33 9, i33 -9>
  %cmp = icmp ne <3 x i33> %srem, <i33 3, i33 -3, i33 3>
  ret <3 x i1> %cmp
}
