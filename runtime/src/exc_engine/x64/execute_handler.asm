;/////////////////////////////////////////////////////////////////////////////
;////    copyright (c) 2012-2017 project_ntke_cpprtl
;////    mailto:kt133a@seznam.cz
;////    license: the MIT license
;/////////////////////////////////////////////////////////////////////////////


include ksamd64.inc


EXCEPTION_UNWIND            equ  66h

ExceptionContinueExecution  equ  00h
ExceptionContinueSearch     equ  01h
ExceptionNestedException    equ  02h
ExceptionCollidedUnwind     equ  03h


EXCEPTION_RECORD struct
    ExceptionCode         dword ?
    ExceptionFlags        dword ?
    ExceptionRecord       qword ?
    ExceptionAddress      qword ?
    NumberParameters      dword ?
    __fill__              dword ?
    ExceptionInformation  qword 15 dup(?)
EXCEPTION_RECORD ends


DISPATCHER_CONTEXT struct
    ControlPc         qword ?
    ImageBase         qword ?
    FunctionEntry     qword ?
    EstablisherFrame  qword ?
    TargetIp          qword ?
    ContextRecord     qword ?
    LanguageHandler   qword ?
    HandlerData       qword ?
    HistoryTable      qword ?
    ScopeIndex        dword ?
    Fill0             dword ?
DISPATCHER_CONTEXT ends


EXECUTE_HANDLER_FRAME struct
    P1Home     qword ?
    P2Home     qword ?
    P3Home     qword ?
    P4Home     qword ?
    DspCtx     qword ?
    Unwind     qword ?
    __fill__   qword ?      ; complementary to the implicitly stack-pushed return address for the stack alignment requirements to be fitted in
EXECUTE_HANDLER_FRAME ends


    ;  rcx  EXCEPTION_RECORD*    -  a new exception record
    ;  rdx  void*                -  the frame this handler to be invoked for
    ;  r8   CONTEXT*             -  the context record (of the throwed function on dispatching, and of the function to be unwound while unwinding phase)
    ;  r9   DISPATCHER_CONTEXT*  -  the dispatcher context of the new dispatching loop
    LEAF_ENTRY exc_engine_execute_frame_handler, _TEXT

        cmp    [rdx].EXECUTE_HANDLER_FRAME.Unwind  ,  0                               ; check if the executor this handler responsible for was invoked while the unwinding phase
        jne    short EHunwind                                                         ; go to perform the preemption of the dispatcher context,
        mov    eax  ,  ExceptionContinueSearch                                        ; (anyway it's a default to return)
        test   [rcx].EXCEPTION_RECORD.ExceptionFlags  ,  EXCEPTION_UNWIND             ; else check if the current exception record is involved in the unwinding.
        jnz    short EHret                                                            ; if so, when we are just have to return the 'ContinueSearch'.

EHnested:                                                                             ; here to process the nested exception:
        mov    rax  ,  [rdx].EXECUTE_HANDLER_FRAME.DspCtx                             ; get the saved dispatcher context from the previous executor frame,
        mov    rax  ,  [rax].DISPATCHER_CONTEXT.EstablisherFrame                      ; load the frame the previous dispatching loop has been excepted on
        mov    [r9].DISPATCHER_CONTEXT.EstablisherFrame  ,  rax                       ; and tell it to the current dispatching loop,
        mov    eax  ,  ExceptionNestedException                                       ; so the disposition is the 'NestedException'.
        jmp    short EHret                                                            ; 

EHunwind:
        mov    rax  ,  [rdx].EXECUTE_HANDLER_FRAME.DspCtx           ; copy the previous loop dispatcher context into the current, excepting TargetIp
        mov    r10  ,  [rax].DISPATCHER_CONTEXT.ControlPc           ; ^
        mov    [r9].DISPATCHER_CONTEXT.ControlPc  ,  r10            ; ^
        mov    r10  ,  [rax].DISPATCHER_CONTEXT.ImageBase           ; ^
        mov    [r9].DISPATCHER_CONTEXT.ImageBase  ,  r10            ; ^
        mov    r10  ,  [rax].DISPATCHER_CONTEXT.FunctionEntry       ; ^
        mov    [r9].DISPATCHER_CONTEXT.FunctionEntry  ,  r10        ; ^
        mov    r10  ,  [rax].DISPATCHER_CONTEXT.EstablisherFrame    ; ^
        mov    [r9].DISPATCHER_CONTEXT.EstablisherFrame  ,  r10     ; ^
        mov    r10  ,  [rax].DISPATCHER_CONTEXT.ContextRecord       ; ^
        mov    [r9].DISPATCHER_CONTEXT.ContextRecord  ,  r10        ; ^
        mov    r10  ,  [rax].DISPATCHER_CONTEXT.LanguageHandler     ; ^
        mov    [r9].DISPATCHER_CONTEXT.LanguageHandler  ,  r10      ; ^
        mov    r10  ,  [rax].DISPATCHER_CONTEXT.HandlerData         ; ^
        mov    [r9].DISPATCHER_CONTEXT.HandlerData  ,  r10          ; ^
        mov    r10  ,  [rax].DISPATCHER_CONTEXT.HistoryTable        ; ^
        mov    [r9].DISPATCHER_CONTEXT.HistoryTable  ,  r10         ; ^
        mov    r10d  ,  [rax].DISPATCHER_CONTEXT.ScopeIndex         ; ^
        mov    [r9].DISPATCHER_CONTEXT.ScopeIndex  ,  r10d          ; ^
        mov    eax  ,  ExceptionCollidedUnwind                      ; the disposition is the 'CollidedUnwind'.

EHret:
        ret                                                     ; return the disposition at 'eax'.

    LEAF_END exc_engine_execute_frame_handler, _TEXT



    ;  rcx  EXCEPTION_RECORD*    -  the current exception record
    ;  rdx  void*                -  the frame the language-specific handler to be invoked for
    ;  r8   CONTEXT*             -  the context record (of the throwed function on dispatching, and of the function to be unwound while unwinding phase)
    ;  r9   DISPATCHER_CONTEXT*  -  the current dispatching loop context
    NESTED_ENTRY exc_engine_execute_handler, _TEXT, exc_engine_execute_frame_handler

        alloc_stack ( sizeof EXECUTE_HANDLER_FRAME )
        END_PROLOGUE

        mov    [rsp].EXECUTE_HANDLER_FRAME.DspCtx  ,  r9                    ; put the current dispatcher context on the stack to be accessed by the exception handler of this scope,
        mov    [rsp].EXECUTE_HANDLER_FRAME.Unwind  ,  0                     ; detect the executor is being
        test   [rcx].EXCEPTION_RECORD.ExceptionFlags  ,  EXCEPTION_UNWIND   ; invoked either at dispatching or unwinding phase
        jz     short EHexecute                                              ; and
        mov    [rsp].EXECUTE_HANDLER_FRAME.Unwind  ,  1                     ; save the flag on the current stack frame

EHexecute:                                                                  ; all the params for handler invoking are already in registers, so just
        call   [r9].DISPATCHER_CONTEXT.LanguageHandler                      ; invoke the language-specific handler supplied by the dispatching loop, 'rax' - the result to be returned
        nop                                                                 ; TODO RtlVirtualUnwind() requires this ?
        add    rsp  ,  sizeof EXECUTE_HANDLER_FRAME                         ; the invoke stack cleanup
        ret                                                                 ; return the disposition in the 'eax'.

    NESTED_END exc_engine_execute_handler, _TEXT


END

