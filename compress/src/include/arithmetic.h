#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include <map>
#include <utility>
#include <vector>
#include "absolute_precision_type.h"

class arithmetic
{
private:
    using float_t = absolute_precision_type;
    using probability_table_type = std::map < uint8_t, float_t >;
    using symbol_pool_type = std::map < uint8_t, uint64_t >;
    using probability_distribution_table_type = std::map < uint8_t, std::pair < float_t, float_t >, std::less<> >;
    using output_bits_type = std::vector < bool >;

    std::vector < uint8_t > input_;
    std::vector < uint8_t > & output_;
    output_bits_type output_bits_; // Track bits generated during renormalization
    probability_table_type probability_table_;
    symbol_pool_type symbol_pool_;
    probability_distribution_table_type probability_distribution_table_;
    uint64_t data_size_{};
    float_t represent_float_arithmetic_data_{};

public:
    arithmetic(std::vector < uint8_t > input, std::vector < uint8_t > & output)
        : input_(std::move(input)), output_(output) { }

    void form_symbol_pool_from_input_data();
    void form_probability_table_from_existing_symbol_pool();
    void form_probability_distribution_from_existing_probability_table(float_t, const float_t&);
    void progressive_symbolization();
    void progressive_decoding();

    symbol_pool_type simple_dump() { return symbol_pool_; }
    void simple_import(const symbol_pool_type & symbol_pool) { symbol_pool_ = symbol_pool; }
    uint64_t simple_get_data_size() { return data_size_; }
    float_t simple_get_result() { return represent_float_arithmetic_data_; }
    void simple_import_result(const uint64_t data_size, const float_t represent_float_arithmetic_data ) {
        data_size_ = data_size;
        represent_float_arithmetic_data_ = represent_float_arithmetic_data;
    }
    output_bits_type simple_get_output_bits() { return output_bits_; }
    void simple_import_output_bits(const output_bits_type & output_bits) {
        output_bits_ = output_bits;
    }
};

#endif //ARITHMETIC_H
