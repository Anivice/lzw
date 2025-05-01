#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include <ranges>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <numeric>
#include <string>

namespace transformer
{
    inline std::pair<std::string, std::size_t> forward(std::string_view text)
    {
        const std::size_t n = text.size();
        if (n == 0) return {"", 0};

        // Build a suffix array of rotations: SA[i] = starting index of i-th lexicographic rotation
        std::vector<std::size_t> SA(n);
        std::iota(SA.begin(), SA.end(), 0);

        auto cmp = [n, text](const std::size_t a, const std::size_t b)
        {
            // Compare rotations (a .. a+n-1) and (b .. b+n-1) mod n
            for (std::size_t k = 0; k < n; ++k)
            {
                const char ca = text[(a + k) % n];
                const char cb = text[(b + k) % n];
                if (ca != cb) {
                    return ca < cb;
                }
            }
            return false;
        };
        std::ranges::sort(SA, cmp);

        // Build the BWT by taking the char preceding each rotation
        std::string out;
        out.resize(n);
        std::size_t primary = 0;            // row where original string appears

        for (std::size_t idx = 0; idx < n; ++idx)
        {
            const auto j = SA[idx];
            out[idx] = text[(j + n - 1) % n];
            if (j == 0) {
                primary = idx;
            }
        }
        return {std::move(out), primary};
    }

    inline std::string inverse(std::string_view bwt, const std::size_t primary)
    {
        const std::size_t n = bwt.size();
        if (n == 0) return {};

        // 1. Generate 'first' column by stable-sorting BWT characters
        std::vector<std::size_t> idx(n);       // maps row → position in BWT
        std::iota(idx.begin(), idx.end(), 0);
        std::ranges::stable_sort(idx,
                                 [&bwt](const std::size_t a, const std::size_t b){ return bwt[a] < bwt[b]; });

        // 2. Build LF-mapping: next position = idx[row]
        std::vector<std::size_t> LF(n);
        for (std::size_t row = 0; row < n; ++row) {
            LF[idx[row]] = row;
        }

        // 3. Walk LF starting from primary row to recover text backwards
        std::string text(n, '\0');
        std::size_t row = primary;
        for (std::size_t i = n; i-- > 0; )
        {
            text[i] = bwt[row];
            row = LF[row];
        }
        return text;
    }
}

#endif //TRANSFORMER_H
