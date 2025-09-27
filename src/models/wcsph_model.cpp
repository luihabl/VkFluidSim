#include "wcsph_model.h"

#include "models/model.h"

namespace vfs {
WCSPHModel::WCSPHModel(const SPHModel::Parameters* base_par, const Parameters* par)
    : SPHModel(base_par) {
    if (par) {
        parameters = *par;
    } else {
        parameters = {
            .stiffness = 2000,
            .expoent = 7,
        };
    }
}

void WCSPHModel::Init(const gfx::CoreCtx& ctx) {
    SPHModel::Init(ctx);
}

void WCSPHModel::Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) {}

void WCSPHModel::Clear(const gfx::CoreCtx& ctx) {}

void WCSPHModel::DrawDebugUI() {}
}  // namespace vfs
