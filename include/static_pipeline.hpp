#pragma once
#include <general.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
#include <memory_tools.hpp>

#include <functional>
#include <optional>

namespace winapi::kernel::worker {
	namespace details {
		template <class Worker, size_t N, class PassOn = NtSuccess>
		class StaticPipeline {
		private:
			using byte = winapi::kernel::mm::byte;
		public:
			constexpr StaticPipeline() = default;
			constexpr StaticPipeline(const PassOn& pass_on) : m_pass_on(pass_on) {}
			constexpr StaticPipeline(PassOn&& pass_on) : m_pass_on(std::move(pass_on)) {}

			Worker& Attach(const Worker& worker) {
				return *mm::construct_at(get_worker_by_ptr(m_size++), worker);
			}
			Worker& Attach(Worker&& worker) {
				return *mm::construct_at(get_worker_by_ptr(m_size++), std::move(worker));
			}

			template <typename... Types>
			auto Process(const Types&... args) const {
				using result_t = remove_reference_t<std::invoke_result_t<Worker, Types...>>;
				std::optional<result_t> result;

				for (size_t idx = 0; idx < m_size; ++idx) {
					result = std::invoke(*get_worker_by_ptr(idx), args...);
					if (!m_pass_on(std::forward<result_t>(*result))) {
						break;
					}
				}
				return *result;             //UB if empty
			}

		private:
			Worker* get_worker_by_ptr(size_t idx) {
				return reinterpret_cast<Worker*>(m_storage[idx]);
			}
			const Worker* get_worker_by_ptr(size_t idx) const {
				return reinterpret_cast<const Worker*>(m_storage[idx]);
			}

		private:
			alignas(Worker) byte m_storage[sizeof(Worker) * N] = {};
			size_t m_size{ 0 };
			PassOn m_pass_on = PassOn{};
		};
	}  // namespace details

	template <class... Workers, class PassOn = NtSuccess>
	constexpr auto MakeStaticPipeline(Workers&&... workers) {
		details::StaticPipeline<std::common_type_t<Workers...>, sizeof...(Workers),
			PassOn>
			pipeline;
		((pipeline.Attach(std::forward<Workers>(workers))), ...);
		return pipeline;
	}

}  // namespace winapi::kernel::worker
