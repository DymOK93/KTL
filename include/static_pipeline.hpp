#pragma once
#include <nt_status.h>
#include <basic_types.h>
#include <type_traits.hpp>
#include <utility.hpp>
#include <memory_tools.hpp>

#include <functional>
#include <optional>

namespace ktl::worker {
namespace details {
template <class Worker, size_t N, class PassOn = NtSuccess>
class StaticPipeline {
 private:
  using byte = ktl::mm::byte;

 public:
  constexpr StaticPipeline() = default;
  constexpr StaticPipeline(const PassOn& pass_on) : m_pass_on(pass_on) {}
  constexpr StaticPipeline(PassOn&& pass_on) : m_pass_on(std::move(pass_on)) {}

  Worker& Attach(const Worker& worker) {
    return *mm::construct_at(addressof(get_worker(m_size++)), worker);
  }
  Worker& Attach(Worker&& worker) {
    return *mm::construct_at(addressof(get_worker(m_size++)), move(worker));
  }

  template <class Ty>
  class Deductor;

  template <typename... Types>
  auto Process(const Types&... args) const -> std::optional<
      remove_reference_t<std::invoke_result_t<Worker, Types...>>> {
    using result_t = remove_reference_t<std::invoke_result_t<Worker, Types...>>;
    std::optional<result_t> result;

    for (size_t idx = 0; idx < m_size; ++idx) {
      const auto& current_worker{get_worker(idx)};
      //ktl::debug::TypeDeductor<decltype(current_worker)> x;
      result = std::invoke(current_worker, args...);
      if (!m_pass_on(*result)) {
        break;
      }
    }
    return result;
  }

 private:
  Worker& get_worker(size_t idx) {
    return *(pointer_cast<Worker>(m_storage) + idx);
  }
  const Worker& get_worker(size_t idx) const {
    return *(pointer_cast<const Worker>(m_storage) + idx);
  }

 private:
  alignas(Worker) byte m_storage[sizeof(Worker) * N] = {};
  size_t m_size{0};
  PassOn m_pass_on = PassOn{};
};  // namespace details
}  // namespace details

template <class... Workers, class PassOn = NtSuccess>
constexpr auto MakeStaticPipeline(Workers&&... workers) {
  details::StaticPipeline<common_type_t<Workers...>, sizeof...(Workers), PassOn>
      pipeline;
  ((pipeline.Attach(forward<Workers>(workers))), ...);
  return pipeline;
}

}  // namespace ktl::worker
