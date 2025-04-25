#include "arithmetic.h"
#include "log.hpp"

int main()
{
    std::vector < uint8_t > input = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G',
                            'E', 'F', 'G',                'H',
        'A', 'B', 'C', 'D', 'E', 'F', 'G',                'H',
        'B', 'C',
        'S', 'Q',
        // 'P', 'R'
    };
    // std::vector < uint8_t > input = { 'A', 'B', 'A', 'C' };
    debug::set_log_level(debug::L_DEBUG_FG);
    debug::log(debug::to_stderr, debug::debug_log, "Data before encoding: ", input, "\n");

    std::vector < uint8_t > output, output2;
    arithmetic arithmetic_compress(input, output);
    arithmetic_compress.form_symbol_pool_from_input_data();
    arithmetic_compress.form_probability_table_from_existing_symbol_pool();
    arithmetic_compress.progressive_symbolization();

    const auto table = arithmetic_compress.simple_dump();
    const auto result = arithmetic_compress.simple_get_result();
    const auto len = arithmetic_compress.simple_get_data_size();
    const auto output_bits = arithmetic_compress.simple_get_output_bits();

    arithmetic arithmetic_decompress(output, output2);
    arithmetic_decompress.simple_import(table);
    arithmetic_decompress.simple_import_result(len, result);
    arithmetic_decompress.simple_import_output_bits(output_bits);

    arithmetic_decompress.form_probability_table_from_existing_symbol_pool();
    arithmetic_decompress.progressive_decoding();

    debug::log(debug::to_stderr, debug::debug_log, "Data after decoding: ", output2, "\n");

}
