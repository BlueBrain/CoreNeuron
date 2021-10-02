/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <functional>
#include <algorithm>
#include <queue>

#include "coreneuron/nrnconf.h"  // for size_t
#include "coreneuron/utils/lpt.hpp"
#include "coreneuron/utils/nrn_assert.h"

using P = std::pair<size_t, size_t>;

// lpt Least Processing Time algorithm.
// Largest piece goes into least size bag.
// in: number of bags, vector of sizes
// return: a new vector of bag indices parallel to the vector of sizes.

std::vector<size_t> lpt(size_t nbag, std::vector<size_t>& pieces, double* bal) {
    nrn_assert(nbag > 0);
    nrn_assert(!pieces.empty());

    std::vector<P> pvec;
    for (size_t i = 0; i < pieces.size(); ++i) {
        pvec.push_back(P(i, pieces[i]));
    }

    auto P_comp = [](const P& a, const P& b) { return a.second > b.second; };

    std::sort(pvec.begin(), pvec.end(), P_comp);

    std::vector<size_t> bagindices(pieces.size());

    std::priority_queue<P, std::vector<P>, decltype(P_comp)> bagq(P_comp);
    for (size_t i = 0; i < nbag; ++i) {
        bagq.push(P(i, 0));
    }

    for (const auto& p: pvec) {
        P bagqitem = bagq.top();
        bagq.pop();
        bagindices[p.first] = bagqitem.first;
        bagqitem.second += p.second;
        bagq.push(bagqitem);
    }

    // load balance average/max (1.0 is perfect)
    std::vector<size_t> v(bagq.size());
    for (auto& item: v) {
        item = bagq.top().second;
        bagq.pop();
    }
    double b = load_balance(v);
    if (bal) {
        *bal = b;
    }

    return bagindices;
}

double load_balance(std::vector<size_t>& v) {
    size_t sum = 0;
    size_t max = 1;
    for (size_t i = 0; i < v.size(); ++i) {
        size_t val = v[i];
        sum += val;
        max = std::max(max, val);
    }
    return (double(sum) / v.size()) / max;
}
