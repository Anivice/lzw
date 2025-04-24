#include "arithmetic.h"
#include <algorithm>
#include <stdexcept>
#include "log.hpp"

arithmetic::float_t arithmetic::renormalization_within_range(const float_t data, const float_t low, const float_t csr)
{
    constexpr float_t raw_scale = 1;
    return ((data * raw_scale) - (low * raw_scale)) / (csr * raw_scale);
}

void arithmetic::form_symbol_pool_from_input_data()
{
    for (const auto & src : input_) {
        symbol_pool_[src]++;
    }

    data_size = input_.size();
}

void arithmetic::form_probability_table_from_existing_symbol_pool()
{
    for (const auto &[symbol, occurrence] : symbol_pool_)
    {
        probability_table_[symbol] =
            static_cast<float_t>(occurrence) / static_cast<float_t>(data_size);
    }
}

void arithmetic::form_probability_distribution_from_existing_probability_table()
{
    float_t lower_bond = 0.00f;
    uint8_t last_symbol = 0;

    for (const auto & [symbol, probability] : probability_table_)
    {
        probability_distribution_table_.emplace(std::make_pair(symbol,
            std::make_pair(lower_bond, lower_bond + probability)));
        lower_bond += probability;
        last_symbol = symbol;
    }

    // should never be used, but Linux **might** fuck this up
    if (!probability_distribution_table_.empty() && probability_distribution_table_[last_symbol].second != 1) {
        probability_distribution_table_[last_symbol].second = 1;
    }
}

void arithmetic::progressive_symbolization()
{
    debug::log(debug::to_stderr, debug::debug_log, "---> Table <---\n", probability_distribution_table_, "\n");
    float_t Low = 0;
    float_t High = 1;

    for (const auto & src : input_) {
        const auto CumulatedLow = probability_distribution_table_[src].first;
        const auto CumulatedHigh = probability_distribution_table_[src].second;
        const auto CurrentSymbolRange = High - Low;


        debug::log(debug::to_stderr, debug::debug_log,
            "progressive_symbolization(): "
            "High = ", High, ", Low = ", Low, "\n"
            "CumulatedHigh = ", std::fixed, std::setprecision(8), CumulatedHigh, "\n"
            "CumulatedLow = ", std::fixed, std::setprecision(8), CumulatedLow, "\n"
            "CurrentSymbolRange = ", std::fixed, std::setprecision(8), CurrentSymbolRange, "\n");

        High = Low + CurrentSymbolRange * CumulatedHigh;
        Low = Low + CurrentSymbolRange * CumulatedLow;
        debug::log(debug::to_stderr, debug::debug_log,
            "progressive_symbolization(): High = ", std::fixed, std::setprecision(8), High,
            ", Low = ", std::fixed, std::setprecision(8), Low, "\n");
    }

    const auto FinalLowerBound = Low;
    const auto FinalHighBound = High;

    // !! Floating-Point Precision Can Be a Problem !!
    const auto float_represent = (FinalLowerBound + FinalHighBound) / 2;
    auto is_in_range = [&](const uint64_t representing, const uint64_t scale)->bool
    {
        const auto scale_10_based = static_cast<uint64_t>(pow(10, static_cast<double>(scale)));;
        const auto FinalLowerBoundInt = static_cast<uint64_t>(FinalLowerBound * static_cast<float_t>(scale_10_based));
        const auto FinalHighBoundInt = static_cast<uint64_t>(FinalHighBound * static_cast<float_t>(scale_10_based));
        if (FinalLowerBoundInt == FinalHighBoundInt) {
            return false; // scale too small
        }

        return (representing > FinalLowerBoundInt) && (representing < FinalHighBoundInt);
    };

    scale = 1;
    represent_arithmetic = static_cast<uint64_t>(float_represent * static_cast<float_t>(10));
    while (!is_in_range(represent_arithmetic, scale))
    {
        ++scale;
        const auto unpacked_scale = static_cast<float_t>(pow(10, static_cast<double>(scale)));
        const auto new_float_represent = float_represent * unpacked_scale;
        represent_arithmetic = static_cast<uint64_t>(new_float_represent);
    }

    represent_float_arithmetic_data = float_represent;
    debug::log(debug::to_stderr, debug::debug_log, "progressive_symbolization(): represent_float_arithmetic_data: ",
        represent_float_arithmetic_data, "\n");
}

void arithmetic::progressive_decoding()
{
    debug::log(debug::to_stderr, debug::debug_log, "---> Table <---\n", probability_distribution_table_, "\n");

    const auto unpacked_scale = static_cast<float_t>(pow(10, static_cast<double>(scale)));
    represent_float_arithmetic_data = static_cast<float_t>(represent_arithmetic) / unpacked_scale; // code
    float_t Low = 0;
    float_t High = 1;

    // find symbol within given range
    auto find_symbol_within_given_range = [&](const float_t data, uint8_t & result)->bool
    {
        for (const auto & [symbol, range]
            : probability_distribution_table_)
        {
            const auto upper_bond = range.second;
            const auto lower_bond = range.first;

            if ((data >= lower_bond) && (data <= upper_bond)) {
                result = symbol;
                return true;
            }
        }

        result = 0;
        return false;
    };

    std::vector<uint8_t> result_stack;
    result_stack.reserve(data_size);

    for (uint64_t i = 0; i < data_size; i++)
    {
        debug::log(debug::to_stderr, debug::debug_log,
            "progressive_decoding(): High = ", std::fixed, std::setprecision(8), High,
            ", Low = ", std::fixed, std::setprecision(8), Low,
            ", represent_float_arithmetic_data = ", represent_float_arithmetic_data, "\n");
        const float_t csr = High - Low;
        represent_float_arithmetic_data = renormalization_within_range(represent_float_arithmetic_data, Low, csr);
        uint8_t symbol = 0;
        if (!find_symbol_within_given_range(represent_float_arithmetic_data, symbol)) {
            throw std::invalid_argument("arithmetic::progressive_decoding(): Unresolvable symbol");
        }
        result_stack.push_back(symbol);
        High = probability_distribution_table_[symbol].second;
        Low = probability_distribution_table_[symbol].first;
    }

    debug::log(debug::to_stderr, debug::debug_log,
        "progressive_decoding(): High = ", std::fixed, std::setprecision(8), High,
        ", Low = ", std::fixed, std::setprecision(8), Low,
        ", represent_float_arithmetic_data = ", represent_float_arithmetic_data, "\n");

    // std::ranges::sort(result_stack);
    // std::ranges::reverse(result_stack);
    output_.reserve(data_size + output_.size());
    output_.insert(output_.end(), result_stack.begin(), result_stack.end());
}
