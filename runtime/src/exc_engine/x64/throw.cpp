#include <throw.hpp>

namespace ktl::crt::exc_engine::x64 {
dispatcher_context make_context(symbol* cookie_,
                                throw_frame& frame_,
                                const frame_walk_pdata& pdata_) noexcept {
  return {cookie_, pdata_.image_base(), nullptr, &pdata_, &frame_};
}
}  // namespace ktl::crt::exc_engine::x64