#include "arithmetic.h"
#include "log.hpp"
#include <cstring>

int main()
{
    // std::vector < uint8_t > input = {
    //     'A', 'B', 'C', 'D', 'E', 'F', 'G',
    //                         'E', 'F', 'G',                'H',
    //     'A', 'B', 'C', 'D', 'E', 'F', 'G',                'H',
    //          'B', 'C',
    //                                                             'S', 'Q',  'P', 'R' };
    std::vector < uint8_t > input = { 'A', 'B', 'A', 'C' };
    debug::set_log_level(debug::L_DEBUG_FG);
    debug::log(debug::to_stderr, debug::debug_log, "Data before encoding: ", input, "\n");

    std::vector < uint8_t > output, output2;
    arithmetic arithmetic_compress(input, output);
    arithmetic_compress.form_symbol_pool_from_input_data();
    arithmetic_compress.form_probability_table_from_existing_symbol_pool();
    arithmetic_compress.form_probability_distribution_from_existing_probability_table();
    arithmetic_compress.progressive_symbolization();

    const auto table = arithmetic_compress.simple_dump();
    const auto result = arithmetic_compress.simple_get_result();

    arithmetic arithmetic_decompress(output, output2);
    arithmetic_decompress.simple_import(table);
    arithmetic_decompress.simple_import_result(result);

    arithmetic_decompress.form_probability_table_from_existing_symbol_pool();
    arithmetic_decompress.form_probability_distribution_from_existing_probability_table();
    arithmetic_decompress.progressive_decoding();

    debug::log(debug::to_stderr, debug::debug_log, "Data after decoding: ", output2, "\n");

}
