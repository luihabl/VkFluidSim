#include "kernel.h"

#include <cmath>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace vfs {

CubicSplineKernel::CubicSplineKernel(double r) {
    SetRadius(r);
}
void CubicSplineKernel::SetRadius(double r) {
    radius = r;
    factor = 8.0 / (M_PI * r * r * r);
    grad_factor = 48.0 / (M_PI * r * r * r);
    w_zero = W(0.0);
}

double CubicSplineKernel::W(double r) {
    double res = 0.0;
    const double q = r / radius;
    if (q <= 1.0) {
        if (q <= 0.5) {
            const double q2 = q * q;
            const double q3 = q2 * q;
            res = factor * (6.0 * q3 - 6.0 * q2 + 1.0);
        } else {
            res = factor * (2.0 * pow(1.0 - q, 3.0));
        }
    }

    return res;
}

glm::dvec3 CubicSplineKernel::GradW(const glm::dvec3& r) {
    glm::dvec3 res;
    const double rl = glm::length(r);
    const double q = rl / radius;
    if ((rl > 1.0e-9) && (q <= 1.0)) {
        glm::dvec3 gradq = r / rl;
        gradq /= radius;
        if (q <= 0.5) {
            res = grad_factor * q * (3.0 * q - 2.0) * gradq;
        } else {
            const double k = 1.0 - q;
            res = grad_factor * (-k * k) * gradq;
        }
    } else
        res = glm::zero<glm::dvec3>();

    return res;
}

}  // namespace vfs
