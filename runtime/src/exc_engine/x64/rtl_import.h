#include <crt_attributes.h>
#include <internal_typedefs.h>

EXTERN_C RUNTIME_FUNCTION* RtlLookupFunctionEntry(
    uint64_t ControlPc,
    uint64_t* ImageBase,
    UNWIND_HISTORY_TABLE* HistoryTable);

EXTERN_C void RtlUnwindEx(void* TargetFrame,
                          void* TargetIp,
                          EXCEPTION_RECORD* ExceptionRecord,
                          void* ReturnValue,
                          CONTEXT* ContextRecord,
                          UNWIND_HISTORY_TABLE* HistoryTable);

EXTERN_C void* RtlPcToFileHeader(void* PcValue, void** BaseOfImage);

EXTERN_C void RtlRaiseException(EXCEPTION_RECORD* ExceptionRecord);

EXTERN_C void RtlCaptureContext(CONTEXT* ContextRecord);

EXTERN_C PEXCEPTION_ROUTINE
RtlVirtualUnwind(uint32_t HandlerType,
                 uint64_t ImageBase,
                 uint64_t ControlPc,
                 RUNTIME_FUNCTION* FunctionEntry,
                 CONTEXT* ContextRecord,
                 void** HandlerData,
                 uint64_t* EstablisherFrame,
                 struct KNONVOLATILE_CONTEXT_POINTERS* ContextPointers);

EXTERN_C void RtlRestoreContext(CONTEXT* ContextRecord,
                                EXCEPTION_RECORD* ExceptionRecord);