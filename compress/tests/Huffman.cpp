#include "Huffman.h"

int main()
{
    std::vector < uint8_t > input = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G',
                            'E', 'F', 'G',                'H',
        'A', 'B', 'C', 'D', 'E', 'F', 'G',                'H',
             'B', 'C',
                                                                'S', 'Q',  'P', 'R' };
    std::vector < uint8_t > output;
    std::vector < uint8_t > backup_of_input = input;

    Huffman huffman(input, output);
    huffman.count_data_frequencies();
    huffman.build_binary_tree_based_on_the_frequency_map();
    huffman.walk_through_tree();
    huffman.encode_using_constructed_pairs();
    const auto table = huffman.export_table();

    Huffman huffman2(backup_of_input, output);
    huffman2.import_table(table);
}
