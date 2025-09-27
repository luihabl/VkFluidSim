#pragma once

#include "gfx/common.h"
#include "models/model.h"
namespace vfs {
/*
 * This model is based on the work by M. Becker and M. Teschner, “Weakly compressible SPH for free
 * surface flows,” in Proceedings of the 2007 ACM SIGGRAPH/eurographics symposium on computer
 * animation, in Sca ’07. San Diego, California: Eurographics Association, 2007, pp. 209–217.
 */
class WCSPHModel : public SPHModel {
public:
    struct Parameters {
        float stiffness;
        float expoent;
    };

    WCSPHModel(const SPHModel::Parameters* sph_parameters, const Parameters* parameters);

    void Init(const gfx::CoreCtx& ctx) override;
    void Step(const gfx::CoreCtx& ctx, VkCommandBuffer cmd) override;
    void Clear(const gfx::CoreCtx& ctx) override;

private:
    Parameters parameters;
};

}  // namespace vfs
