#include "arithmetic.h"
#include <algorithm>
#include <ranges>
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

    data_size_ = input_.size();
}

void arithmetic::form_probability_table_from_existing_symbol_pool()
{
    for (const auto &[symbol, occurrence] : symbol_pool_)
    {
        probability_table_[symbol] =
            static_cast<float_t>(occurrence) / static_cast<float_t>(data_size_);
    }
}

void arithmetic::form_probability_distribution_from_existing_probability_table(float_t stage_min, const float_t stage_max)
{
    probability_distribution_table_.clear();

    for (const auto & [symbol, probability] : probability_table_)
    {
        const auto domain_length = stage_max - stage_min;
        const auto cumulated_probability = domain_length * probability + stage_min;
        probability_distribution_table_.emplace(std::make_pair(symbol,
            std::make_pair(stage_min, cumulated_probability)));
        stage_min = cumulated_probability;
    }
}

void arithmetic::progressive_symbolization()
{
    float_t Low = 0;
    float_t High = 1;
    output_bits_.clear();

    for (const auto & src : input_) {
        form_probability_distribution_from_existing_probability_table(Low, High);
        Low = probability_distribution_table_[src].first;
        High = probability_distribution_table_[src].second;

        debug::log(debug::to_stderr, debug::debug_log,
            "progressive_symbolization(): Redistributed interval: [", Low, ", ", High, ")\n");

        // Renormalize interval if necessary
        // while (true) {
        //     if (High < 0.5) {
        //         // Output 0 and scale interval
        //         output_bits_.push_back(false);
        //         Low *= 2;
        //         High *= 2;
        //     } else if (Low >= 0.5) {
        //         // Output 1 and scale interval
        //         output_bits_.push_back(true);
        //         Low = 2 * (Low - 0.5);
        //         High = 2 * (High - 0.5);
        //     } else if (Low >= 0.25 && High < 0.75) {
        //         // Apply E3 scaling
        //         Low = 2 * (Low - 0.25);
        //         High = 2 * (High - 0.25);
        //         output_bits_.push_back(false); // Output 0
        //         output_bits_.push_back(true);  // Followed by 1 (increment)
        //     } else {
        //         break; // No more renormalization needed
        //     }
        // }
    }

    form_probability_distribution_from_existing_probability_table(Low, High);

    std::vector<float_t> last_interation_probabilities;
    last_interation_probabilities.reserve(probability_distribution_table_.size() * 2);
    for (const auto &[lower, higher]:
         probability_distribution_table_ | std::views::values)
    {
        last_interation_probabilities.push_back(lower);
        last_interation_probabilities.push_back(higher);
    }

    std::ranges::sort(last_interation_probabilities, std::less());
    const auto FinalLowerBound = last_interation_probabilities.front();
    const auto FinalHigherBound = last_interation_probabilities.back();
    represent_float_arithmetic_data_ = (FinalLowerBound + FinalHigherBound) / 2;

    debug::log(debug::to_stderr,
        debug::debug_log, "progressive_symbolization(): Redistributed interval: [", Low, ", ", High, ")\n",
        debug::debug_log, "represent_float_arithmetic_data = ", represent_float_arithmetic_data_, "\n");
}

void arithmetic::progressive_decoding()
{
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
    result_stack.reserve(data_size_);
    float_t Low = 0;
    float_t High = 1;
    float_t code = represent_float_arithmetic_data_;
    // for (const bool bit : output_bits_) {
    //     code = code * 2.0 + (bit ? 1.0 : 0.0);
    // }
    // const float_t scale = 1.0 / static_cast<float_t>(1ULL << output_bits_.size());
    // code += represent_float_arithmetic_data_ * scale;
    debug::log(debug::to_stderr, debug::debug_log, "progressive_decoding(): Rescaled code: ", code, "\n");

    for (uint64_t i = 0; i < data_size_; i++)
    {
        form_probability_distribution_from_existing_probability_table(Low, High);
        debug::log(debug::to_stderr, debug::debug_log,
            "progressive_decoding(): Redistributed interval: [", Low, ", ", High, ")\n");
        uint8_t symbol = 0;
        if (!find_symbol_within_given_range(represent_float_arithmetic_data_, symbol)) {
            throw std::invalid_argument("arithmetic::progressive_decoding(): Unresolvable symbol");
        }

        result_stack.push_back(symbol);
        High = probability_distribution_table_[symbol].second;
        Low = probability_distribution_table_[symbol].first;
    }

    debug::log(debug::to_stderr, debug::debug_log,
        "progressive_decoding(): High = ", std::fixed, std::setprecision(8), High,
        ", Low = ", std::fixed, std::setprecision(8), Low,
        ", represent_float_arithmetic_data = ", represent_float_arithmetic_data_, "\n");

    // std::ranges::sort(result_stack);
    // std::ranges::reverse(result_stack);
    output_.reserve(data_size_ + output_.size());
    output_.insert(output_.end(), result_stack.begin(), result_stack.end());
}
