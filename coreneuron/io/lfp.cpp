#include "coreneuron/io/lfp.hpp"

#include <cmath>
#include <limits>



namespace coreneuron {

    namespace lfputils {

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
 } // namespace utils
 
 using namespace lfputils;

 template <LFPCalculatorType Type, typename SegmentIdTy>
 LFPCalculator<Type, SegmentIdTy>::LFPCalculator(const Point3Ds& seg_start,
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

template <LFPCalculatorType Type, typename SegmentIdTy>
template <typename Vector>
    inline void LFPCalculator<Type, SegmentIdTy>::lfp(const Vector& membrane_current) {
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

template <>
lfputils::F LFPCalculator<LineSource>::getFactor(const lfputils::Point3D& e_pos,
                                       const lfputils::Point3D& seg_0,
                                       const lfputils::Point3D& seg_1,
                                       const lfputils::F radius,
                                       const lfputils::F f) const {
    return lfputils::line_source_lfp_factor(e_pos, seg_0, seg_1, radius, f);
}

template LFPCalculator<LineSource>::LFPCalculator(const lfputils::Point3Ds& seg_start,
                  const lfputils::Point3Ds& seg_end,
                  const std::vector<double>& radius,
                  const std::vector<int>& segment_ids,
                  const lfputils::Point3Ds& electrodes,
                  double extra_cellular_conductivity);
template LFPCalculator<PointSource>::LFPCalculator(const lfputils::Point3Ds& seg_start,
                  const lfputils::Point3Ds& seg_end,
                  const std::vector<double>& radius,
                  const std::vector<int>& segment_ids,
                  const lfputils::Point3Ds& electrodes,
                  double extra_cellular_conductivity);
template void  LFPCalculator<LineSource>::lfp(const Vec& membrane_current);
template void  LFPCalculator<PointSource>::lfp(const Vec& membrane_current);
template void  LFPCalculator<LineSource>::lfp(const std::vector<double>& membrane_current);
template void  LFPCalculator<PointSource>::lfp(const std::vector<double>& membrane_current);

} // namespace coreneuron
