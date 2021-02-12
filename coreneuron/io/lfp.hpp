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

template <typename T>
using Array3 = std::array<T, 3>;

template <typename Point3D, typename F>
F dot(const Point3D& p1, const Point3D& p2) {
    return p1[0] * p2[0] + p1[1] * p2[1] + p1[2] * p2[2];
}

template <typename Point3D, typename F>
F norm(const Point3D& p1) {
    return std::sqrt(dot<Point3D, F>(p1, p1));
}

template <typename Point3D, typename F>
Array3<F> barycenter(const Point3D& p1, const Point3D& p2) {
    return {0.5 * (p1[0] + p2[0]), 0.5 * (p1[1] + p2[1]), 0.5 * (p1[2] + p2[2])};
}

template <typename Point3D, typename F>
Array3<F> axp(const Point3D& p1, const F alpha, const Point3D& p2) {
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
template <typename Point3D, typename F>
F point_source_lfp_factor(const Point3D& e_pos, const Point3D& seg_pos, const F radius, const F f) {
    nrn_assert(radius >= 0.0);
    Array3<F> es = axp(e_pos, -1.0, seg_pos);
    return f / std::max(norm<Point3D, F>(es), radius);
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
template <typename Point3D, typename F>
F line_source_lfp_factor(const Point3D& e_pos,
                         const Point3D& seg_0,
                         const Point3D& seg_1,
                         const F radius,
                         const F f) {
    nrn_assert(radius >= 0.0);
    Array3<F> dx = axp(seg_1, -1.0, seg_0);
    Array3<F> de = axp(e_pos, -1.0, seg_0);
    F dx2(dot<Array3<F>, F>(dx, dx));
    F dxn(std::sqrt(dx2));
    if (dxn < 1.0e-20) {
        return point_source_lfp_factor(e_pos, seg_0, radius, f);
    }
    F de2(dot<Array3<F>, F>(de, de));
    F mu(dot<Array3<F>, F>(dx, de) / dx2);
    Array3<F> de_star(axp(de, -mu, dx));
    F de_star2(dot<Array3<F>, F>(de_star, de_star));
    F q2(de_star2 / dx2);

    F delta(mu * mu - (de2 - radius * radius) / dx2);
    F one_m_mu(1.0 - mu);
    auto log_integral = [&q2, &dxn](F a, F b) {
        if (std::abs(q2) < std::numeric_limits<F>::epsilon()) {
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
    template <typename Point3Ds, typename Vector>
    LFPCalculator(const Point3Ds& seg_start,
                  const Point3Ds& seg_end,
                  const Vector& radius,
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
        double f(1.0 / (extra_cellular_conductivity * 4.0 * pi));

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
    template <typename Point3D, typename F>
    inline F getFactor(const Point3D& e_pos,
                       const Point3D& seg_0,
                       const Point3D& seg_1,
                       const F radius,
                       const F f) const;

    std::vector<std::vector<double>> m;
    const std::vector<SegmentIdTy>& segment_ids_;
};

template <>
template <typename Point3D, typename F>
F LFPCalculator<LineSource>::getFactor(const Point3D& e_pos,
                                       const Point3D& seg_0,
                                       const Point3D& seg_1,
                                       const F radius,
                                       const F f) const {
    return line_source_lfp_factor(e_pos, seg_0, seg_1, radius, f);
}

template <>
template <typename Point3D, typename F>
F LFPCalculator<PointSource>::getFactor(const Point3D& e_pos,
                                        const Point3D& seg_0,
                                        const Point3D& seg_1,
                                        const F radius,
                                        const F f) const {
    return point_source_lfp_factor(e_pos, barycenter<Point3D, F>(seg_0, seg_1), radius, f);
}
};  // namespace coreneuron

#endif  // AREA_LFP_H
