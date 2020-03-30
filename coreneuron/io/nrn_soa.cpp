#include "nrn_soa.hpp"

namespace coreneuron {

int nrn_soa_padded_size(int cnt, int layout) {
    return soa_padded_size<NRN_SOA_PAD>(cnt, layout);
}

size_t nrn_soa_byte_align(size_t i) {
    if (LAYOUT == 0) {
        size_t dbl_align = NRN_SOA_BYTE_ALIGN / sizeof(double);
        size_t rem = i % dbl_align;
        if (rem) {
            i += dbl_align - rem;
        }
        assert((i * sizeof(double)) % NRN_SOA_BYTE_ALIGN == 0);
    }
    return i;
}

int nrn_i_layout(int icnt, int cnt, int isz, int sz, int layout) {
    if (layout == 1) {
        return icnt * sz + isz;
    } else if (layout == 0) {
        int padded_cnt = nrn_soa_padded_size(cnt, layout);  // may want to factor out to save time
        return icnt + isz * padded_cnt;
    }
    assert(0);
    return 0;
}

// file data is AoS. ie.
// organized as cnt array instances of mtype each of size sz.
// So input index i refers to i_instance*sz + i_item offset
// Return the corresponding SoA index -- taking into account the
// alignment requirements. Ie. i_instance + i_item*align_cnt.

int nrn_param_layout(int i, int mtype, Memb_list* ml) {
    int layout = corenrn.get_mech_data_layout()[mtype];
    if (layout == 1) {
        return i;
    }
    assert(layout == 0);
    int sz = corenrn.get_prop_param_size()[mtype];
    return nrn_i_layout(i / sz, ml->nodecount, i % sz, sz, layout);
}
}  // namespace coreneuron

