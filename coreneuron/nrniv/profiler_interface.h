#pragma once

namespace coreneuron {

void start_profile();
void stop_profile();

enum class InstrumentorType {Empty, Caliper};

struct Instrumentor {
    template<InstrumentorType type>
    static void phase_begin(const char*);

    template<InstrumentorType type>
    static void phase_end(const char*);
};

#ifdef CALIPER
#define INSTRUMENTOR InstrumentorType::Caliper
#else
#define INSTRUMENTOR InstrumentorType::Empty
#endif


}  // namespace coreneuron
