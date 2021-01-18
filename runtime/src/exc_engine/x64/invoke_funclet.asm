;/////////////////////////////////////////////////////////////////////////////
;////    copyright (c) 2012-2017 project_ntke_cpprtl
;////    mailto:kt133a@seznam.cz
;////    license: the MIT license
;/////////////////////////////////////////////////////////////////////////////

include ksamd64.inc

INVOKE_FUNCLET_FRAME struct
    RCX_shadow   qword ?
    RDX_shadow   qword ?
    R8_shadow    qword ?
    R9_shadow    qword ?
    __fill__     qword ?   ; complementary to the implicitly stack-pushed return address for the stack alignment requirements to be fitted in
INVOKE_FUNCLET_FRAME ends


    ;  rcx  void* funclet_entry  -  entry point.
    ;  rdx  void* frame_ptr      -  funclet frame
    NESTED_ENTRY exc_engine_invoke_funclet, _TEXT

        alloc_stack ( sizeof INVOKE_FUNCLET_FRAME )     ;  allocate the params shadow space
        END_PROLOGUE

        call    rcx                                     ;  invoke the funclet
        add     rsp , sizeof INVOKE_FUNCLET_FRAME       ;  deallocate the local stack space
        ret                                             ;  'rax' - the continuation address if any

    NESTED_END exc_engine_invoke_funclet, _TEXT

END
