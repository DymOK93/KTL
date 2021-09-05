#include <throw.hpp>

namespace ktl::crt::exc_engine::x64 {
dispatcher_context make_context(symbol* cookie,
                                throw_frame& frame,
                                const frame_walk_pdata& pdata) noexcept {
  dispatcher_context ctx{};
  ctx.image_base = pdata.image_base();
  ctx.pdata = &pdata;
  ctx.throw_frame = &frame;
  ctx.cookie = cookie;
  return ctx;
}
}  // namespace ktl::crt::exc_engine::x64