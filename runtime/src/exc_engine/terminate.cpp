#include <bugcheck.h>

EXTERN_C [[noreturn]] void __std_terminate() {
  terminate();
}