#include <type_info.h>

type_info::type_info() noexcept
    : undecorated_name{nullptr}, decorated_name{""} {}

type_info::~type_info() {}