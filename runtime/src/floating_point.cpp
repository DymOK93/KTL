#include <crt_attributes.hpp>

/**
 * @var extern "C" const int _fltused
 * @brief A dummy variable allowing floating point operations
 * It should be a single underscore since the double one is the mangled name
 */
EXTERN_C const int _fltused{0};