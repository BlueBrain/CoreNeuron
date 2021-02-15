#ifndef LFP_H
#define LFP_H

#include <cmath>
#include <array>
#include <iostream>
#include <vector>

#include "coreneuron/nrnconf.h"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/mpi/nrnmpidec.h"

namespace coreneuron {

using F = double;
using Point3D = std::array<F, 3>;
using Point3Ds = std::vector<Point3D>;

F dot(const Point3D& p1, const Point3D& p2) {
    return p1[0] * p2[0] + p1[1] * p2[1] + p1[2] * p2[2];
}

F norm(const Point3D& p1) {
    return std::sqrt(dot(p1, p1));
}

Point3D barycenter(const Point3D& p1, const Point3D& p2) {
    return {0.5 * (p1[0] + p2[0]), 0.5 * (p1[1] + p2[1]), 0.5 * (p1[2] + p2[2])};
}

Point3D paxpy(const Point3D& p1, const F alpha, const Point3D& p2) {
    return {p1[0] + alpha * p2[0], p1[1] + alpha * p2[1], p1[2] + alpha * p2[2]};
}

/**
 *
 * \tparam Point3D Type of 3D point
 * \tparam F Floating point type
 * \param e_pos electrode position
 * \param seg_pos segment position
 * \param radius segment radius
 * \param f conductivity factor 1/([4 pi] * [conductivity])
 * \return Resistance of the medium from the segment to the electrode.
 */
F point_source_lfp_factor(const Point3D& e_pos, const Point3D& seg_pos, const F radius, const F f) {
    nrn_assert(radius >= 0.0);
    Point3D es = paxpy(e_pos, -1.0, seg_pos);
    return f / std::max(norm(es), radius);
}

/**
 *
 * \tparam Point3D Type of 3D point
 * \tparam F Floating point type
 * \param e_pos electrode position
 * \param seg_pos segment position
 * \param radius segment radius
 * \param f conductivity factor 1/([4 pi] * [conductivity])
 * \return Resistance of the medium from the segment to the electrode.
 */
F line_source_lfp_factor(const Point3D& e_pos,
                         const Point3D& seg_0,
                         const Point3D& seg_1,
                         const F radius,
                         const F f) {
    nrn_assert(radius >= 0.0);
    Point3D dx = paxpy(seg_1, -1.0, seg_0);
    Point3D de = paxpy(e_pos, -1.0, seg_0);
    F dx2(dot(dx, dx));
    F dxn(std::sqrt(dx2));
    if (dxn < std::numeric_limits<F>::epsilon()) {
        return point_source_lfp_factor(e_pos, seg_0, radius, f);
    }
    F de2(dot(de, de));
    F mu(dot(dx, de) / dx2);
    Point3D de_star(paxpy(de, -mu, dx));
    F de_star2(dot(de_star, de_star));
    F q2(de_star2 / dx2);

    F delta(mu * mu - (de2 - radius * radius) / dx2);
    F one_m_mu(1.0 - mu);
    auto log_integral = [&q2, &dxn](F a, F b) {
        if (q2 < std::numeric_limits<F>::epsilon()) {
            if (a * b <= 0) {
                throw std::invalid_argument("Log integral: invalid arguments " + std::to_string(b) +
                                            " " + std::to_string(a) +
                                            ". Likely electrode exactly on the segment and "
                                            "no flooring is present.");
            }
            return std::abs(std::log(b / a)) / dxn;
        } else {
            return std::log((b + std::sqrt(b * b + q2)) / (a + std::sqrt(a * a + q2))) / dxn;
        }
    };
    if (delta <= 0.0) {
        return f * log_integral(-mu, one_m_mu);
    } else {
        F sqr_delta(std::sqrt(delta));
        F d1(mu - sqr_delta);
        F d2(mu + sqr_delta);
        F parts = 0.0;
        if (d1 > 0.0) {
            F b(std::min(d1, 1.0) - mu);
            parts += log_integral(-mu, b);
        }
        if (d2 < 1.0) {
            F b(std::max(d2, 0.0) - mu);
            parts += log_integral(b, one_m_mu);
        };
        // complement
        F maxd1_0(std::max(d1, 0.0)), mind2_1(std::min(d2, 1.0));
        if (maxd1_0 < mind2_1) {
            parts += 1.0 / radius * (mind2_1 - maxd1_0);
        }
        return f * parts;
    };
}

enum LFPCalculatorType { LineSource, PointSource };

/**
 * \brief LFPCalculator allows calculation of LFP given membrane currents.
 */
template <LFPCalculatorType Ty, typename SegmentIdTy = int>
struct LFPCalculator {
    /**
     * LFP Calculator constructor
     * \tparam Point3Ds A vector of 3D points type
     * \tparam Vector A vector of floats type
     * \param comm MPI communicator
     * \param seg_start all segments start owned by the proc
     * \param seg_end all segments end owned by the proc
     * \param radius fence around the segment. Ensures electrode cannot be
     * arbitrarily close to the segment
     * \param electrodes positions of the electrodes
     * \param extra_cellular_conductivity conductivity of the extra-cellular
     * medium
     */
    LFPCalculator(const Point3Ds& seg_start,
                  const Point3Ds& seg_end,
                  const std::vector<double>& radius,
                  const std::vector<SegmentIdTy>& segment_ids,
                  const Point3Ds& electrodes,
                  double extra_cellular_conductivity)
        : segment_ids_(segment_ids) {
        if (seg_start.size() != seg_end.size()) {
            throw std::invalid_argument("Different number of segment starts and ends.");
        }
        if (seg_start.size() != radius.size()) {
            throw std::invalid_argument("Different number of segments and radii.");
        }
        double f(1.0 / (extra_cellular_conductivity * 4.0 * 3.141592653589));

        m.resize(electrodes.size());
        for (size_t k = 0; k < electrodes.size(); ++k) {
            auto& ms = m[k];
            ms.resize(seg_start.size());
            for (size_t l = 0; l < seg_start.size(); l++) {
                ms[l] = getFactor(electrodes[k], seg_start[l], seg_end[l], radius[l], f);
            }
        }
    }

    template <typename Vector>
    void lfp(const Vector& membrane_current) {
        std::vector<double> res(m.size());
        for (size_t k = 0; k < m.size(); ++k) {
            res[k] = 0.0;
            auto& ms = m[k];
            for (size_t l = 0; l < ms.size(); l++) {
                res[k] += ms[l] * membrane_current[segment_ids_[l]];
            }
        }
        std::vector<double> res_reduced(res.size());
#if NRNMPI
        int mpi_sum{1};
        nrnmpi_dbl_allreduce_vec(res.data(), res_reduced.data(), res.size(), mpi_sum);
#else
        res_reduced = res;
#endif
        lfp_values = res_reduced;
    }

    std::vector<double> lfp_values;

  private:
    inline F getFactor(const Point3D& e_pos,
                       const Point3D& seg_0,
                       const Point3D& seg_1,
                       const F radius,
                       const F f) const;

    std::vector<std::vector<double>> m;
    const std::vector<SegmentIdTy>& segment_ids_;
};

template <>
F LFPCalculator<LineSource>::getFactor(const Point3D& e_pos,
                                       const Point3D& seg_0,
                                       const Point3D& seg_1,
                                       const F radius,
                                       const F f) const {
    return line_source_lfp_factor(e_pos, seg_0, seg_1, radius, f);
}

template <>
F LFPCalculator<PointSource>::getFactor(const Point3D& e_pos,
                                        const Point3D& seg_0,
                                        const Point3D& seg_1,
                                        const F radius,
                                        const F f) const {
    return point_source_lfp_factor(e_pos, barycenter(seg_0, seg_1), radius, f);
}

};  // namespace coreneuron

#endif  // AREA_LFP_H
