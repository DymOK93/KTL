#include <type_info.hpp>

type_info::type_info() noexcept
    : m_undecorated_name{nullptr}, m_decorated_name{""} {}

type_info::~type_info() = default;