/* Lua's error path calls setjmp, which this glibc header turns into _setjmp.
 * The buffer is the system layout: eight words, then the signal mask Lua
 * does not use. Pointers are stored plain. Both sides are this file.
 */

__attribute__((naked)) int _setjmp(void *)
{
    __asm__ volatile(
        "movq %rbx, 0(%rdi)\n\t"
        "movq %rbp, 8(%rdi)\n\t"
        "movq %r12, 16(%rdi)\n\t"
        "movq %r13, 24(%rdi)\n\t"
        "movq %r14, 32(%rdi)\n\t"
        "movq %r15, 40(%rdi)\n\t"
        "leaq 8(%rsp), %rax\n\t"
        "movq %rax, 48(%rdi)\n\t"
        "movq (%rsp), %rax\n\t"
        "movq %rax, 56(%rdi)\n\t"
        "xorl %eax, %eax\n\t"
        "ret\n\t"
    );
}

__attribute__((naked)) void longjmp(void *, int)
{
    __asm__ volatile(
        "movl %esi, %eax\n\t"
        "testl %eax, %eax\n\t"
        "jnz 1f\n\t"
        "movl $1, %eax\n\t"
        "1:\n\t"
        "movq 0(%rdi), %rbx\n\t"
        "movq 8(%rdi), %rbp\n\t"
        "movq 16(%rdi), %r12\n\t"
        "movq 24(%rdi), %r13\n\t"
        "movq 32(%rdi), %r14\n\t"
        "movq 40(%rdi), %r15\n\t"
        "movq 48(%rdi), %rsp\n\t"
        "jmp *56(%rdi)\n\t"
    );
}
