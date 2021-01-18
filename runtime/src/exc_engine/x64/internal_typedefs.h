#pragma once
#include <../exc_engine_interface.h>
#include <crt_attributes.h>

#ifndef EXCEPTION_UNWINDING
#define EXCEPTION_UNWINDING 0x2
#endif

#ifndef EXCEPTION_NESTED_CALL
#define EXCEPTION_NESTED_CALL 0x10
#endif

#ifndef EXCEPTION_TARGET_UNWIND
#define EXCEPTION_TARGET_UNWIND 0x20
#endif

#ifndef EXCEPTION_COLLIDED_UNWIND
#define EXCEPTION_COLLIDED_UNWIND 0x40
#endif

#ifndef EXCEPTION_UNWIND
#define EXCEPTION_UNWIND 0x66
#endif

#ifndef STATUS_UNWIND_CONSOLIDATE
#define STATUS_UNWIND_CONSOLIDATE 0x80000029L
#endif

#ifndef UNW_FLAG_NHANDLER
#define UNW_FLAG_NHANDLER 0x0
#endif

#ifndef UNW_FLAG_EHANDLER
#define UNW_FLAG_EHANDLER 0x1
#endif

#ifndef UNW_FLAG_UHANDLER
#define UNW_FLAG_UHANDLER 0x2
#endif

#ifndef UNWIND_HISTORY_TABLE_SIZE
#define UNWIND_HISTORY_TABLE_SIZE 12
#endif

struct RUNTIME_FUNCTION {
  uint32_t BeginAddress;
  uint32_t EndAddress;
  uint32_t UnwindData;
};

struct UNWIND_HISTORY_TABLE_ENTRY {
  uint64_t ImageBase;
  RUNTIME_FUNCTION* FunctionEntry;
};

struct UNWIND_HISTORY_TABLE {
  uint32_t Count;
  uint8_t LocalHint;
  uint8_t GlobalHint;
  uint8_t Search;
  uint8_t Once;
  uint64_t LowAddress;
  uint64_t HighAddress;
  UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
};

struct DISPATCHER_CONTEXT64 {
  uint64_t ControlPc;
  uint64_t ImageBase;
  RUNTIME_FUNCTION* FunctionEntry;
  uint64_t EstablisherFrame;
  uint64_t TargetIp;
  CONTEXT* ContextRecord;
  PEXCEPTION_ROUTINE LanguageHandler;
  void* HandlerData;
  UNWIND_HISTORY_TABLE* HistoryTable;
  uint32_t ScopeIndex;
  BOOLEAN ControlPcIsUnwound;
  unsigned char* NonVolatileRegisters;  // byte*
};

namespace ktl::crt::exc_engine {
using dispatcher_context_t = DISPATCHER_CONTEXT64;
using fwd_compatibility_handler_t = disposition_t(CRTCALL*)(...);
using constructor_t = void(CRTCALL*)(void*);
using constructor_with_vt_t = void(CRTCALL*)(void*, int);  // vt - Virtual Table
using destructor_t = void(CRTCALL*)();

using unwind_handler_t = void (*)(void*, void*);
using catch_handler_t = void* (*)(void*, void*);

struct type_descriptor {
  void const* vtable_ptr;  // vtable of type_info class
  char* unmangled_name;  // used by msvcrt to keep a demangled name returned by
                         // type_info::name()
  char name;  // the actual type is char[], keeps a zero-terminated mangled type
              // name, e.g. ".H" = "int", ".?AUA@@" = "struct A", ".?AVA@@" =
              // "class A"
};

struct subtype_cast_info {
  int subtype_offset;      // member offset
  int vbase_table_offset;  // offset of the vbtable (-1 if not a virtual base)
  int vbase_disp_offset;  // offset to the displacement value inside the vbtable
};

struct catchable_type {
  unsigned int
      attributes;  // 0x01: simple type (can be copied by memmove), 0x02: can be
                   // caught by reference only, 0x04: has virtual bases
  type_descriptor* type_descr;  // type info
  subtype_cast_info cast_info;  // how to cast the thrown object to this type
  unsigned int size;            // object size
  union {
    constructor_t cctor;             // copy constructor address
    constructor_with_vt_t cctor_vb;  // copy constructor address
  };
};

struct catchable_type_table {
  unsigned int type_array_size;  // number of entries in the type array
  catchable_type* catchable_type_array[1];  // [1] - just a fake size
};

struct exception_descriptor {
  unsigned int attributes;                     // 0x01: const, 0x02: volatile
  destructor_t dtor;                           // exception object destructor
  fwd_compatibility_handler_t forward_compat;  // forward compatibility handler
  catchable_type_table*
      catchable_types;  // list of types that can catch this exception (the
                        // actual type and all its ancestors)
};

////////////////////////////////////////////////////
//// function scope stuff (try-catch and unwind information)

struct unwind_descriptor {
  int prev_state;           // target state
  unwind_handler_t action;  // action to perform (unwind funclet address)
};

struct catch_descriptor {
  unsigned int attributes;  // 0x01: const, 0x02: volatile, 0x08: reference
  type_descriptor*
      type_descr;  // RTTI descriptor of the exception type, 0=(...)
  int exc_offset;  // ebp-based offset of the exception object in the function
                   // stack, 0 means no object (catch by type)
  catch_handler_t handler_address;  // address of the catch handler code
                                    // returning a continuation address
  unsigned int frame_offset;
};

struct try_descriptor {
  int low_level;    // this try{} covers states ranging from low_level
  int high_level;   // to high_level
  int catch_level;  // highest state inside catch handlers of this try
  unsigned int catch_array_size;  // number of catch handlers
  catch_descriptor* catch_array;  // catch handlers table
};

struct function_descriptor {
  unsigned int
      magic_number;  // compiler version. 0x19930520: up to msvc6, 0x19930521:
                     // msvc2002 msvc2003, 0x19930522: msvc2005
  unsigned int unwind_array_size;    // number of entries in unwind table
  unwind_descriptor* unwind_array;   // table of unwind destructors
  unsigned int try_array_size;       // number of try blocks in the function
  try_descriptor* try_array;         // mapping of catch blocks to try blocks
  unsigned int ip2state_array_size;  // not used on x86
  void* ip2state_array;              // not used on x86
  void* expected_exceptions_table;   // >=msvc2002, expected exceptions list
                                    // (function "throw" specifier). not enabled
                                    // in MSVC by default - use /d1ESrt to
                                    // enable (-->void*)
  unsigned int frame_ptrs; 
  unsigned int
      flags;  // >=msvc2005, bit 0 set if function was compiled with /EHs
};

struct frame_pointers {
  int current_state;
  int reserved;
};

struct ip2state_descriptor {
  uint64_t ip;
  int state;
};

enum {
  EXCEPTION_OPCODE_THROW = 'ehop',
  EXCEPTION_OPCODE_STACK_CONSOLIDATE,
  EXCEPTION_OPCODE_NO_EXC_OBJ,
  EXCEPTION_OPCODE_STACKWALKER_UNWIND
};

//// exception flags residing at the ExceptionInformation[EXCPTR_FLAGS]
enum {
  EXCEPTION_FLAG_TRANSLATED = 1 << 0,
  EXCEPTION_FLAG_OBJECT_RETHROWED = 1 << 1,
  EXCEPTION_FLAG_STACKWALKER_UNWIND = 1 << 2
};

////  ExceptionInformation common layout for exceptions generated by this lib
enum {
  EXCPTR_RESERVED  // because of EXCEPTION_OPCODE_STACK_CONSOLIDATE's the [0] is
                   // mandatory held by callback address
  ,
  EXCPTR_OPCODE  // EXCEPTION_OPCODE_xxx
  ,
  EXCPTR_FLAGS  // EXCEPTION_FLAG_xxx
  ,
  EXCPTR_COMMON_LAST
};

////  ExceptionInformation layout for opcode EXCEPTION_OPCODE_THROW
enum {
  EXCPTR_THR_THROWOBJECT = EXCPTR_COMMON_LAST,
  EXCPTR_THR_THROWINFO,
  EXCPTR_THR_IMAGEBASE,
  EXCPTR_THR_PREV_EXCEPTION,
  EXCPTR_THR_UNWIND_EXCREC  // the pointer to the eh-dispatching routine's
                            // c++-specific context (the unwind exception
                            // record)
  ,
  ARRAYSIZE_EXCPTR_THROW
};

////  ExceptionInformation layout for opcode EXCEPTION_OPCODE_STACK_CONSOLIDATE
enum {
  EXCPTR_UNW_CALLBACK_ADDR  // target frame unwind callback address should be at
                            // [0],  void* (*)(EXCEPTION_RECORD*)
  ,
  EXCPTR_UNW_FUNCTION_FRAME = EXCPTR_COMMON_LAST  // funcframe_ptr_t
  ,
  EXCPTR_UNW_HANDLER_ADDR  // catch_handler_ft
  ,
  EXCPTR_UNW_TARGET_STATE  // int
  ,
  EXCPTR_UNW_TARGET_FRAME  // frame_ptr_t
  ,
  EXCPTR_UNW_CONTEXT  // CONTEXT*
  ,
  EXCPTR_UNW_FUNC_DESCRIPTOR  // func_descriptor*
  ,
  EXCPTR_UNW_CURRENT_EXCEPTION  // EXCEPTION_RECORD*
  ,
  EXCPTR_UNW_ARM_NV_CONTEXT  // _M_ARM specific: void* (pointer to the {r4-r11}
                             // array filled by RtlUnwindEx(), carefully keep it
                             // at [10])
  ,
  ARRAYSIZE_EXCPTR_UNW
};

////  ExceptionInformation layout for opcode EXCEPTION_OPCODE_NO_EXC_OBJ
enum { EXCPTR_NOOBJ_EXCREC_PTR = EXCPTR_COMMON_LAST, ARRAYSIZE_EXCPTR_NOOBJ };
}  // namespace ktl::crt::exc_engine
