#pragma once
#include <type_traits_impl.hpp>

#include <ntddk.h>

namespace ktl {
class preload_initializer  // NOLINT(cppcoreguidelines-special-member-functions)
    : public non_relocatable {
 public:
  preload_initializer() noexcept;

  virtual NTSTATUS run(DRIVER_OBJECT& driver_object,
                       UNICODE_STRING& registry_path) noexcept = 0;

 protected:
  virtual ~preload_initializer() = default;
};

// clang-format off
class preload_initializer_registry  // NOLINT(cppcoreguidelines-special-member-functions)
    : public non_relocatable {
  // clang-format on

  static constexpr size_t MAX_INITIALIZER_COUNT{32};

 public:
  static preload_initializer_registry& get_instance() noexcept;

  preload_initializer_registry(const preload_initializer_registry&) = delete;
  preload_initializer_registry& operator=(const preload_initializer_registry&) =
      delete;

  bool add(preload_initializer& obj) noexcept;
  NTSTATUS run_all(DRIVER_OBJECT& driver_object,
                   UNICODE_STRING& registry_path) noexcept;

 private:
  preload_initializer_registry() noexcept = default;

 private:
  preload_initializer* m_initializers[MAX_INITIALIZER_COUNT]{nullptr};
  size_t m_size{0};
  bool m_fired{false};
};
}  // namespace ktl
