#include "repeator.h"

int main()
{
    std::vector<uint8_t> data(1024, 0);
    std::vector<uint8_t> data2 = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', };
    std::vector<uint8_t> out, out2;
    repeator::repeator compressor(data2, out);
    compressor.encode();
    repeator::repeator decompressor(out, out2);
    decompressor.decode();
}
