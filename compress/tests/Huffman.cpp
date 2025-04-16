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

    Huffman huffman(input, output);
    huffman.count_data_frequencies();
    huffman.build_binary_tree_based_on_the_frequency_map();
    huffman.walk_through_tree();
    huffman.encode_using_constructed_pairs();
    std::cout << huffman.export_dumped_data() << std::endl;
}
