#include "thirdparty/tbv/tbv.hpp"
#include <cmath>
#include <span>

unsigned herbceptionEmulationSqrt(std::span<double> values, unsigned repeat);
unsigned herbceptionEmulationFib(unsigned n, unsigned maxDepth);

unsigned herbceptionsSqrt(std::span<double> values, unsigned repeat) {
   // The emulation is good enough here, the call overhead is negligible
   return herbceptionEmulationSqrt(values, repeat);
}

#if defined(__x86_64__) && defined(__linux__)

static unsigned doFib(unsigned n, unsigned maxDepth) __attribute__((naked));
static unsigned doFib(unsigned /*n*/, unsigned /*maxDepth*/) {
   asm(R"(
.LBfib:
        pushq   %rbp
        pushq   %r14
        pushq   %rbx
        testl   %esi, %esi
        je      .LBB0_1
        movl    %edi, %r14d
        movl    $1, %eax
        cmpl    $3, %edi
        jb      .LBB0_4
        movl    %esi, %ebx
        leal    -2(%r14), %edi
        decl    %ebx
        movl    %ebx, %esi
        callq   .LBfib
        jc      .LBB0_4
        movl    %eax, %ebp
        decl    %r14d
        movl    %r14d, %edi
        movl    %ebx, %esi
        callq   .LBfib
        jc      .LBB0_4
        addl    %ebp, %eax
        jmp     .LBB0_4
.LBB0_1:
	xor     %eax, %eax
        stc
        jmp     .LBB0_5
.LBB0_4:
        clc
.LBB0_5:
        popq    %rbx
        popq    %r14
        popq    %rbp
        retq
   )");
}

unsigned herbceptionsFib(unsigned n, unsigned maxDepth) {
   return doFib(n, maxDepth);
}

#else
#warning No herbception implementation provided for this platform, falling back to emulation

unsigned herbceptionsFib(unsigned n, unsigned maxDepth) { return herbceptionEmulation(n, maxDepth); }
#endif

