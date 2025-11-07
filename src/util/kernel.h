#pragma once

#include <glm/glm.hpp>

namespace vfs {

class CubicSplineKernel {
public:
    CubicSplineKernel() = default;
    CubicSplineKernel(double radius);
    void SetRadius(double radius);

    double W(double r);
    double WZero() { return w_zero; }
    glm::dvec3 GradW(const glm::dvec3& r);
    double Factor() { return factor; }
    double GradFactor() { return grad_factor; }

private:
    double w_zero{0.0};
    double radius{0.0};
    double grad_factor{0.0};
    double factor{0.0};
};

}  // namespace vfs
