#include "transformer.h"
#include "log.hpp"

int main()
{
    debug::set_log_level(debug::L_DEBUG_FG);
    std::vector<uint8_t> text(1024 * 128, 'C');
    text.append_range(std::vector<uint8_t>{'A', 'B', 'A', 'B'});
    auto [bwt, primary] = transformer::forward(text);
    auto original = transformer::inverse(bwt, primary);
    assert(std::equal(original.begin(), original.end(), text.begin()));
}
