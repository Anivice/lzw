# LZW Compression
def lzw_compress(input_string_compression):
    # Initialize dictionary with single character entries
    dictionary = {chr(i): i for i in range(256)}
    dict_size = 256
    current_string = ""
    compressed_data_compression = []

    # Iterate over each character in the input string
    for symbol in input_string_compression:
        print(f"=> {symbol}")
        print(f"    -- Current string: `{current_string}`")
        current_string_plus_symbol = current_string + symbol
        print(f"    -- Check dictionary")
        if current_string_plus_symbol in dictionary:
            print(f"        -- Combined symbol `{current_string_plus_symbol}` is in dictionary")
            current_string = current_string_plus_symbol
        else:
            print(f"        -- Combined symbol `{current_string_plus_symbol}` not in dictionary")
            compressed_data_compression.append(dictionary[current_string])
            print(f"            -- Output previous symbol combination `{current_string}` with entry `{dictionary[current_string]}`")
            print(f"            -- Compressed data is now `{compressed_data_compression}`")
            dictionary[current_string_plus_symbol] = dict_size
            print(f"            -- Created symbol `{current_string_plus_symbol}` assigned with value {dict_size}")
            dict_size += 1
            print(f"            -- Updated current string as `{symbol}`")
            current_string = symbol

    # Add the last string to the output
    if current_string:
        print(f"-- Added tailing data from key `{current_string}`, which is `{dictionary[current_string]}`")
        compressed_data_compression.append(dictionary[current_string])

    return compressed_data_compression


# LZW Decompression
def lzw_decompress(compressed_data_decompression):
    # Initialize dictionary and variables
    dictionary = {i: chr(i) for i in range(256)}
    dict_size = 256
    current_string = chr(compressed_data_decompression.pop(0))  # The first code corresponds to a single character
    decompressed_data = [current_string]
    entry = ""

    # Iterate over the compressed data to reconstruct the string
    for code in compressed_data_decompression:
        if code in dictionary:
            entry = dictionary[code]
        elif code == dict_size:
            entry = current_string + current_string[0]
        decompressed_data.append(entry)

        # Add new entry to the dictionary
        dictionary[dict_size] = current_string + entry[0]
        dict_size += 1

        current_string = entry

    return ''.join(decompressed_data)


input_string = "ABABABABA"
print(f"Original String: {input_string}")

compressed_data = lzw_compress(input_string)
print(f"Compressed Data: {compressed_data}")

decompressed_string = lzw_decompress(compressed_data)
print(f"Decompressed String: {decompressed_string}")
