#include <throw.hpp>

namespace ktl::crt::exc_engine::x64 {
dispatcher_context make_context(symbol* cookie,
                                throw_frame& frame,
                                const frame_walk_pdata& pdata) noexcept {
  dispatcher_context ctx{frame.mach.rip, pdata.image_base(), nullptr, &pdata, &frame};
  ctx.cookie = cookie;
  return ctx;
}
}  // namespace ktl::crt::exc_engine::x64