# Compression Algorithms and Their Implementations

## Introduction

This is an implementation of the two well-known and widely deployed compression algorithms,
namely Lempel-Ziv-Welch (LZW) and Huffman coding.

Lempel-Ziv-Welchm, or LZW in short for the sake of convenience naming,
typically employs a fixed bit size for the result in its coding style,
though the coding algorithm never specifically detailed any bit size and varying bit length 
in encoded data is fairly common.
Huffman coding, on the other hand, is known for its varying bit length in encoded data.

This implementation mixes these two algorithms in the hope that it can reach
the maximum compression result possible.

## Terminology

### *Unit Bit Size*, *Unit Size* and *Variable Bit Size*

**Unit Size** is the size of the smallest unit in a storage system.
**Unit Bit Size** is the bit size of the smallest unit in a storage system.
For example, for hard disks, their unit size, the smallest size of operation is typically 512 bytes.
This means you can and only can read and write $N \times 512$ bytes at a time from its controller.
A typical computer system is 8-bit aligned in bits, with each 8 bits forming a byte,
and 4096-byte, i.e., 4 KB, aligned on disk filesystem.
This means that for a typical computer, the file size on disk is always $8 \times 4096 \times N$ bits.

Though computer storage usually employs unit size to boost performance and avoid severe fragmentation,
which can create small gaps that, when combined, 
are large enough to fit a new program but cannot form a continuous usable space,
a variable size in a storage unit usually offers lower storage requirement and in turn,
occupy less storage space.
**Variable Bit Size** means that for each symbol represented in a continuous data stream,
its occupied bit size is not fixed and can change dynamically,
and for all symbols, their unit size does not necessarily agree with each other.

## Lempel-Ziv-Welchm Algorithm

Lempel-Ziv-Welchm algorithm can be encoded and decoded without the knowledge on the full scale of the whole data.
LZW can encode data stream and build a dictionary along with the encoding process,
and decoder does not rely on a dictionary to decode data.
It can build the dictionary in the decoding process.

The following is an extremely simplified demonstration of the LZW implementation used in the main example.

```python
def lzw_compress(input_string_compression):
    # Initialize dictionary with single character entries
    dictionary = {chr(i): i for i in range(256)}
    dict_size = 256
    current_string = ""
    compressed_data_compression = []

    # Iterate over each character in the input string
    for symbol in input_string_compression:
        current_string_plus_symbol = current_string + symbol
        if current_string_plus_symbol in dictionary:
            current_string = current_string_plus_symbol
        else:
            compressed_data_compression.append(dictionary[current_string])
            dictionary[current_string_plus_symbol] = dict_size
            dict_size += 1
            current_string = symbol

    # Add the last string to the output
    if current_string:
        compressed_data_compression.append(dictionary[current_string])

    return compressed_data_compression
```

As is shown above, the compressor first initialized a dictionary of 256 entries,
since we are encoding on an 8-bit system, with possible dangling data that have $2^{8} = 256$ possibilities.
Each entry is assigned with the corresponding word, which is exactly the key.

Say, we compress the string `"ABABABABA"`.

```markdown
Original String: ABABABABA
=> A
    -- Current string: ``
    -- Check dictionary
        -- Combined symbol `A` is in dictionary
=> B
    -- Current string: `A`
    -- Check dictionary
        -- Combined symbol `AB` not in dictionary
            -- Output previous symbol combination `A` with entry `65`
            -- Compressed data is now `[65]`
            -- Created symbol `AB` assigned with value 256
            -- Updated current string as `B`
=> A
    -- Current string: `B`
    -- Check dictionary
        -- Combined symbol `BA` not in dictionary
            -- Output previous symbol combination `B` with entry `66`
            -- Compressed data is now `[65, 66]`
            -- Created symbol `BA` assigned with value 257
            -- Updated current string as `A`
=> B
    -- Current string: `A`
    -- Check dictionary
        -- Combined symbol `AB` is in dictionary
=> A
    -- Current string: `AB`
    -- Check dictionary
        -- Combined symbol `ABA` not in dictionary
            -- Output previous symbol combination `AB` with entry `256`
            -- Compressed data is now `[65, 66, 256]`
            -- Created symbol `ABA` assigned with value 258
            -- Updated current string as `A`
=> B
    -- Current string: `A`
    -- Check dictionary
        -- Combined symbol `AB` is in dictionary
=> A
    -- Current string: `AB`
    -- Check dictionary
        -- Combined symbol `ABA` is in dictionary
=> B
    -- Current string: `ABA`
    -- Check dictionary
        -- Combined symbol `ABAB` not in dictionary
            -- Output previous symbol combination `ABA` with entry `258`
            -- Compressed data is now `[65, 66, 256, 258]`
            -- Created symbol `ABAB` assigned with value 259
            -- Updated current string as `B`
=> A
    -- Current string: `B`
    -- Check dictionary
        -- Combined symbol `BA` is in dictionary
-- Added tailing data from key `BA`, which is `257`
Compressed Data: [65, 66, 256, 258, 257]
```


