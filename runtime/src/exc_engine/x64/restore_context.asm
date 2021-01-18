;/////////////////////////////////////////////////////////////////////////////
;////    copyright (c) 2012-2017 project_ntke_cpprtl
;////    mailto:kt133a@seznam.cz
;////    license: the MIT license
;/////////////////////////////////////////////////////////////////////////////


include ksamd64.inc


M128A struct                       ; the 'OWORD' replacement, because the 'ddk3790'-ml64 is got collapsed seeing 'oword'
    Qword_1        qword ?
    Qword_2        qword ?
M128A ends


AUX_CONTEXT struct  ;  ::aux_::context look-alike
    _Rax      qword ?
    _Rcx      qword ?
    _Rdx      qword ?
    _Rbx      qword ?
    _Rsp      qword ?
    _Rbp      qword ?
    _Rsi      qword ?
    _Rdi      qword ?
    _R8       qword ?
    _R9       qword ?
    _R10      qword ?
    _R11      qword ?
    _R12      qword ?
    _R13      qword ?
    _R14      qword ?
    _R15      qword ?
    _Xmm0     M128A <>
    _Xmm1     M128A <>
    _Xmm2     M128A <>
    _Xmm3     M128A <>
    _Xmm4     M128A <>
    _Xmm5     M128A <>
    _Xmm6     M128A <>
    _Xmm7     M128A <>
    _Xmm8     M128A <>
    _Xmm9     M128A <>
    _Xmm10    M128A <>
    _Xmm11    M128A <>
    _Xmm12    M128A <>
    _Xmm13    M128A <>
    _Xmm14    M128A <>
    _Xmm15    M128A <>
    _Rip      qword ?
    MxCsr     dword ?
    SegCS     word  ?
    SegDS     word  ?
    SegES     word  ?
    SegFS     word  ?
    SegGS     word  ?
    SegSS     word  ?
    EFlags    dword ?
    __fill__  dword ?
AUX_CONTEXT ends


RESTORE_CTX_FRAME struct  ;  'iret'-frame layout
    _Rip        qword ?
    SegCS       qword ?
    EFlags      qword ?
    _Rsp        qword ?
    SegSS       qword ?
    __fill__    qword ?   ;  to achieve a proper 'iret'-frame alignment
RESTORE_CTX_FRAME ends


    ;  rcx  AUX_CONTEXT* ctx  -  the context to be restored, the 'ctx.Rip' content is expected to point the continuation address
    NESTED_ENTRY exc_engine_restore_context, _TEXT

        alloc_stack ( sizeof RESTORE_CTX_FRAME )
      ; the non-volatile registers are not saved 'caz this scope is not expected to be unwound or returned
        END_PROLOGUE

        movdqa    xmm0   ,  [rcx].AUX_CONTEXT._Xmm0    ; restore XMM-registers
        movdqa    xmm1   ,  [rcx].AUX_CONTEXT._Xmm1    ; ^
        movdqa    xmm2   ,  [rcx].AUX_CONTEXT._Xmm2    ; ^
        movdqa    xmm3   ,  [rcx].AUX_CONTEXT._Xmm3    ; ^
        movdqa    xmm4   ,  [rcx].AUX_CONTEXT._Xmm4    ; ^
        movdqa    xmm5   ,  [rcx].AUX_CONTEXT._Xmm5    ; ^
        movdqa    xmm6   ,  [rcx].AUX_CONTEXT._Xmm6    ; ^
        movdqa    xmm7   ,  [rcx].AUX_CONTEXT._Xmm7    ; ^
        movdqa    xmm8   ,  [rcx].AUX_CONTEXT._Xmm8    ; ^
        movdqa    xmm9   ,  [rcx].AUX_CONTEXT._Xmm9    ; ^
        movdqa    xmm10  ,  [rcx].AUX_CONTEXT._Xmm10   ; ^
        movdqa    xmm11  ,  [rcx].AUX_CONTEXT._Xmm11   ; ^
        movdqa    xmm12  ,  [rcx].AUX_CONTEXT._Xmm12   ; ^
        movdqa    xmm13  ,  [rcx].AUX_CONTEXT._Xmm13   ; ^
        movdqa    xmm14  ,  [rcx].AUX_CONTEXT._Xmm14   ; ^
        movdqa    xmm15  ,  [rcx].AUX_CONTEXT._Xmm15   ; ^
        ldmxcsr   [rcx].AUX_CONTEXT.MxCsr              ; restore 'MXCSR' register

        cli                           ; turn off interrupts, a page fault isn't expected while accessing the 'AUX_CONTEXT' because it's expected to be placed on the stack
                                                               ; preparing the 'iret'-frame
        movzx     eax  ,  [rcx].AUX_CONTEXT.SegSS              ; push 'SS'
        mov       [rsp].RESTORE_CTX_FRAME.SegSS   ,  rax       ; ^
        mov       rax  ,  [rcx].AUX_CONTEXT._Rsp               ; push 'RSP'
        mov       [rsp].RESTORE_CTX_FRAME._Rsp    ,  rax       ; ^
        mov       eax  ,  [rcx].AUX_CONTEXT.EFlags             ; push EFlags
        mov       [rsp].RESTORE_CTX_FRAME.EFlags  ,  rax       ; ^
        movzx     eax  ,  [rcx].AUX_CONTEXT.SegCS              ; push 'CS'
        mov       [rsp].RESTORE_CTX_FRAME.SegCS   ,  rax       ; ^
        mov       rax  ,  [rcx].AUX_CONTEXT._Rip               ; push 'RIP'
        mov       [rsp].RESTORE_CTX_FRAME._Rip    ,  rax       ; ^


        mov     rax  ,  [rcx].AUX_CONTEXT._Rax         ; restore volatile registers
        mov     rdx  ,  [rcx].AUX_CONTEXT._Rdx         ; ^
        mov     r8   ,  [rcx].AUX_CONTEXT._R8          ; ^
        mov     r9   ,  [rcx].AUX_CONTEXT._R9          ; ^
        mov     r10  ,  [rcx].AUX_CONTEXT._R10         ; ^
        mov     r11  ,  [rcx].AUX_CONTEXT._R11         ; ^
                                                         
        mov     rbx  ,  [rcx].AUX_CONTEXT._Rbx         ; restore nonvolatile integer registers
        mov     rsi  ,  [rcx].AUX_CONTEXT._Rsi         ; ^
        mov     rdi  ,  [rcx].AUX_CONTEXT._Rdi         ; ^
        mov     rbp  ,  [rcx].AUX_CONTEXT._Rbp         ; ^
        mov     r12  ,  [rcx].AUX_CONTEXT._R12         ; ^
        mov     r13  ,  [rcx].AUX_CONTEXT._R13         ; ^
        mov     r14  ,  [rcx].AUX_CONTEXT._R14         ; ^
        mov     r15  ,  [rcx].AUX_CONTEXT._R15         ; ^
        mov     rcx  ,  [rcx].AUX_CONTEXT._Rcx         ; ^ 'rcx' is the last to be restored 'cos it formerly keeps the previous context

        iretq                                      ; return into the restored context, it's supposed the interrupt flag is to be enabled by the 'EFlags' popped from the 'iret'-frame

    NESTED_END exc_engine_restore_context, _TEXT

END
