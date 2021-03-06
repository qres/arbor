#include <functional>
#include <iosfwd>
#include <vector>

#include <arbor/spike.hpp>

namespace aux {

struct spike_emitter {
    std::reference_wrapper<std::ostream> out;

    spike_emitter(std::ostream& out);
    void operator()(const std::vector<arb::spike>&);
};

} // namespace aux
