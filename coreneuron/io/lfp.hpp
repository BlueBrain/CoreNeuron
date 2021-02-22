#pragma once

#include <array>
#include <vector>

#include "coreneuron/nrnconf.h"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/mpi/nrnmpidec.h"

namespace coreneuron {

namespace lfputils {

using F = double;
using Point3D = std::array<F, 3>;
using Point3Ds = std::vector<Point3D>;
using Vec = double*;

inline F dot(const Point3D& p1, const Point3D& p2) {
    return p1[0] * p2[0] + p1[1] * p2[1] + p1[2] * p2[2];
}

inline F norm(const Point3D& p1) {
    return std::sqrt(dot(p1, p1));
}

inline Point3D barycenter(const Point3D& p1, const Point3D& p2) {
    return {0.5 * (p1[0] + p2[0]), 0.5 * (p1[1] + p2[1]), 0.5 * (p1[2] + p2[2])};
}

inline Point3D paxpy(const Point3D& p1, const F alpha, const Point3D& p2) {
    return {p1[0] + alpha * p2[0], p1[1] + alpha * p2[1], p1[2] + alpha * p2[2]};
}

/**
 *
 * \param e_pos electrode position
 * \param seg_pos segment position
 * \param radius segment radius
 * \param f conductivity factor 1/([4 pi] * [conductivity])
 * \return Resistance of the medium from the segment to the electrode.
 */
inline F point_source_lfp_factor(const Point3D& e_pos, const Point3D& seg_pos, const F radius, const F f) {
    nrn_assert(radius >= 0.0);
    Point3D es = paxpy(e_pos, -1.0, seg_pos);
    return f / std::max(norm(es), radius);
}

/**
 *
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
                         const F f);
} // namespace utils

enum LFPCalculatorType { LineSource, PointSource };

/**
 * \brief LFPCalculator allows calculation of LFP given membrane currents.
 */
template <LFPCalculatorType Ty, typename SegmentIdTy = int>
struct LFPCalculator {
    /**
     * LFP Calculator constructor
     * \param comm MPI communicator
     * \param seg_start all segments start owned by the proc
     * \param seg_end all segments end owned by the proc
     * \param radius fence around the segment. Ensures electrode cannot be
     * arbitrarily close to the segment
     * \param electrodes positions of the electrodes
     * \param extra_cellular_conductivity conductivity of the extra-cellular
     * medium
     */
    LFPCalculator(const lfputils::Point3Ds& seg_start,
                  const lfputils::Point3Ds& seg_end,
                  const std::vector<double>& radius,
                  const std::vector<SegmentIdTy>& segment_ids,
                  const lfputils::Point3Ds& electrodes,
                  double extra_cellular_conductivity);

    template <typename Vector>
     void lfp(const Vector& membrane_current);

    std::vector<double> lfp_values;

  private:
    inline lfputils::F getFactor(const lfputils::Point3D& e_pos,
                       const lfputils::Point3D& seg_0,
                       const lfputils::Point3D& seg_1,
                       const lfputils::F radius,
                       const lfputils::F f) const;

    std::vector<std::vector<double>> m;
    const std::vector<SegmentIdTy>& segment_ids_;
};

template <>
lfputils::F LFPCalculator<LineSource>::getFactor(const lfputils::Point3D& e_pos,
                                       const lfputils::Point3D& seg_0,
                                       const lfputils::Point3D& seg_1,
                                       const lfputils::F radius,
                                       const lfputils::F f) const {
    return lfputils::line_source_lfp_factor(e_pos, seg_0, seg_1, radius, f);
}

template <>
lfputils::F LFPCalculator<PointSource>::getFactor(const lfputils::Point3D& e_pos,
                                        const lfputils::Point3D& seg_0,
                                        const lfputils::Point3D& seg_1,
                                        const lfputils::F radius,
                                        const lfputils::F f) const {
    return lfputils::point_source_lfp_factor(e_pos, lfputils::barycenter(seg_0, seg_1), radius, f);
}

extern template LFPCalculator<LineSource>::LFPCalculator(const lfputils::Point3Ds& seg_start,
                  const lfputils::Point3Ds& seg_end,
                  const std::vector<double>& radius,
                  const std::vector<int>& segment_ids,
                  const lfputils::Point3Ds& electrodes,
                  double extra_cellular_conductivity);
extern template LFPCalculator<PointSource>::LFPCalculator(const lfputils::Point3Ds& seg_start,
                  const lfputils::Point3Ds& seg_end,
                  const std::vector<double>& radius,
                  const std::vector<int>& segment_ids,
                  const lfputils::Point3Ds& electrodes,
                  double extra_cellular_conductivity);
extern template void LFPCalculator<LineSource>::lfp(const lfputils::Vec& membrane_current);
extern template void LFPCalculator<PointSource>::lfp(const lfputils::Vec& membrane_current);
extern template void LFPCalculator<LineSource>::lfp(const std::vector<double>& membrane_current);
extern template void LFPCalculator<PointSource>::lfp(const std::vector<double>& membrane_current);
};  // namespace coreneuron

