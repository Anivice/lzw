#ifndef REPEATOR_H
#define REPEATOR_H

#include <vector>
#include <ranges>
#include <algorithm>

namespace repeator {
    constexpr uint8_t none = 0;
    constexpr uint8_t trimmed = 0x4F;

class repeator {
private:
    std::vector < uint8_t > & input_;
    std::vector < uint8_t > & output_;

    static void encode(std::vector<uint8_t> & input, std::vector<uint8_t> & output);
public:
    repeator(std::vector<uint8_t> & input, std::vector<uint8_t> & output)
        : input_(input), output_(output) {}

    void encode();
    void decode();
};

}

#endif //REPEATOR_H
