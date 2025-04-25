#include "arithmetic.h"
#include <algorithm>
#include <ranges>
#include <stdexcept>
#include "log.hpp"

void arithmetic::form_symbol_pool_from_input_data()
{
    for (const auto & src : input_) {
        symbol_pool_[src]++;
    }

    data_size_ = input_.size();
}

std::string div(const uint64_t left, const uint64_t right)
{
    if (left > right) {
        throw std::invalid_argument("Numerical division prohibit result > 1");
    }

    auto balance = [](std::string & left, std::string & right)
    {
        const auto max = std::max(left.size(), right.size());
        left.insert(left.begin(), max - left.size(), '0');
        right.insert(right.begin(), max - right.size(), '0');
    };

    auto ulltostr = [](uint64_t data)->std::string
    {
        std::string ret;
        while (data != 0) {
            ret += std::to_string(data % 10);
            data /= 10;
        }

        return ret;
    };

    // if left > right?
    auto larger_than = [](const std::string& left, const std::string& right)->bool
    {
        const auto left_literal = strtoll(left.c_str(), nullptr, 10);
        const auto right_literal = strtoll(right.c_str(), nullptr, 10);
        return left_literal > right_literal;
    };

    // wrap to CPU binary calculation
    auto mod = [&ulltostr](
        const std::string& target,
        const std::string& divisor,
        std::string & result,
        std::string & remainder)->bool
    {
        const auto target_literal = std::strtoull(target.c_str(), nullptr, 10);
        const auto divisor_literal = std::strtoull(divisor.c_str(), nullptr, 10);
        result = ulltostr(target_literal / divisor_literal);
        remainder = ulltostr(target_literal % divisor_literal);
        return target_literal % divisor_literal == 0;
    };

    auto const_this_ = ulltostr(left);
    auto that_ = ulltostr(right);

    std::string result;
    uint64_t index = 0;
    const auto max_result_size = std::max(const_this_.size() * 4, 16ull);
    std::string current_session_target;

    while (true)
    {
        if (result.size() > max_result_size) {
            break;
        }

        if (index >= const_this_.size()) {
            const_this_ += std::string(const_this_.size() - index + 1, '0'); // padding
        }

        current_session_target += const_this_[index];
        index++;
        if (!larger_than(current_session_target, that_)) {
            result += '0';
        } else {
            std::string remainder, quo;
            balance(current_session_target, that_);
            const auto reminder_is_zero = mod(current_session_target, that_, quo, remainder);
            result += quo;
            if (!reminder_is_zero) {
                current_session_target = remainder;
            } else if (index == const_this_.size()) {
                break;
            } else {
                current_session_target.clear();
            }
        }
    }

    return "0." + result;
}

void arithmetic::form_probability_table_from_existing_symbol_pool()
{
    for (const auto &[symbol, occurrence] : symbol_pool_) {
        probability_table_[symbol] = float_t::make_absolute_precision_type(div(occurrence, data_size_));
    }
}

void arithmetic::form_probability_distribution_from_existing_probability_table(float_t stage_min, const float_t& stage_max)
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
    float_t Low = float_t::make_absolute_precision_type("0");
    float_t High = float_t::make_absolute_precision_type("1");
    output_bits_.clear();

    for (const auto & src : input_) {
        form_probability_distribution_from_existing_probability_table(Low, High);
        Low = probability_distribution_table_[src].first;
        High = probability_distribution_table_[src].second;

        // debug::log(debug::to_stderr, debug::debug_log,
        //     "progressive_symbolization(): Redistributed interval: [", Low.to_string(), ", ", High.to_string(), ")\n");
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
    const auto halfOfFinalLowerBound = FinalLowerBound / 2;
    const auto halfOfFinalHigherBound = FinalHigherBound / 2;
    represent_float_arithmetic_data_ = halfOfFinalLowerBound + halfOfFinalHigherBound;

    // debug::log(debug::to_stderr,
    //     debug::debug_log, "progressive_symbolization(): Redistributed interval: [", Low.to_string(), ", ", High.to_string(), ")\n",
    //     debug::debug_log, "represent_float_arithmetic_data = ", represent_float_arithmetic_data_.to_string(), "\n");
}

void arithmetic::progressive_decoding()
{
    // find symbol within given range
    auto find_symbol_within_given_range = [&](const float_t& data, uint8_t & result)->bool
    {
        for (const auto & [symbol, range]
             : probability_distribution_table_)
        {
            float_t upper_bond = range.second;
            float_t lower_bond = range.first;
            const bool lower_bounded = data >= lower_bond;
            const bool upper_bounded = data <= upper_bond;

            if (lower_bounded && upper_bounded) {
                result = symbol;
                return true;
            }
        }

        result = 0;
        return false;
    };

    std::vector<uint8_t> result_stack;
    result_stack.reserve(data_size_);
    float_t Low = float_t::make_absolute_precision_type("0");
    float_t High = float_t::make_absolute_precision_type("1");
    const float_t code = represent_float_arithmetic_data_;
    // debug::log(debug::to_stderr, debug::debug_log, "progressive_decoding(): code: ", code.to_string(), "\n");

    for (uint64_t i = 0; i < data_size_; i++)
    {
        form_probability_distribution_from_existing_probability_table(Low, High);
        // debug::log(debug::to_stderr, debug::debug_log,
        //     "progressive_decoding(): Redistributed interval: [", Low.to_string(), ", ", High.to_string(), ")\n");
        uint8_t symbol = 0;
        if (!find_symbol_within_given_range(represent_float_arithmetic_data_, symbol)) {
            throw std::invalid_argument("arithmetic::progressive_decoding(): Unresolvable symbol");
        }

        result_stack.push_back(symbol);
        High = probability_distribution_table_[symbol].second;
        Low = probability_distribution_table_[symbol].first;
    }

    // debug::log(debug::to_stderr, debug::debug_log,
    //     "progressive_decoding(): High = ", High.to_string(),
    //     ", Low = ", Low.to_string(),
    //     ", represent_float_arithmetic_data = ", represent_float_arithmetic_data_.to_string(), "\n");

    output_.reserve(data_size_ + output_.size());
    output_.insert(output_.end(), result_stack.begin(), result_stack.end());
}
