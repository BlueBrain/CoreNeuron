#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "spdlog/fmt/ostr.h"

struct CoreneuronUserdefinedLog
{
    int i;
    template<typename OStream>
    friend OStream &operator<<(OStream &os, const my_type &c)
    {
        return os << "[coreneuron_userdefined_log i=" << c.i << "]";
    }
};

