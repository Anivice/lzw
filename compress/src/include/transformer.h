#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include <ranges>
#include <vector>
#include <utility>
#include <cstdint>
#include <cassert>
#include <libcubwt.cuh>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <numeric>

namespace transformer
{
    inline
    std::pair<std::vector<uint8_t>, std::size_t>
    forward(const std::vector<uint8_t>& text_with_sentinel)
    {
        if (text_with_sentinel.empty()) {
            throw std::invalid_argument("input is empty");
        }

        void* storage = nullptr;
        if (libcubwt_allocate_device_storage(&storage,
            static_cast<int64_t>(text_with_sentinel.size() + 2)) != LIBCUBWT_NO_ERROR) {
            throw std::runtime_error("GPU memory allocation failed");
        }

        std::vector<uint8_t> bwt(text_with_sentinel.size());

        // ---- call ----------------------------------------------------------
        const int64_t primary = libcubwt_bwt(storage,
                                             text_with_sentinel.data(),
                                             bwt.data(),
                                             static_cast<int64_t>(text_with_sentinel.size()));
        // --------------------------------------------------------------------

        libcubwt_free_device_storage(storage);

        if (primary < 0) {
            throw std::runtime_error("libcubwt_bwt: " + std::to_string(primary));
        }

        return { std::move(bwt), static_cast<std::size_t>(primary) };
    }

    inline std::vector<uint8_t> inverse(const std::vector<uint8_t> &bwt, const std::size_t primary)
    {
        void* storage = nullptr;
        if (libcubwt_allocate_device_storage(&storage, static_cast<long>(bwt.size())) != LIBCUBWT_NO_ERROR) {
            throw std::runtime_error("GPU memory allocation failed");
        }

        std::vector<uint8_t> result(bwt.size());

        if (libcubwt_unbwt(storage, bwt.data(), result.data(), static_cast<long>(bwt.size()), nullptr,
                       static_cast<int32_t>(primary)) != LIBCUBWT_NO_ERROR)
        {
            throw std::runtime_error("Inverse failed");
        }

        libcubwt_free_device_storage(storage);
        return result;
    }

    inline std::pair<std::vector<uint8_t>, std::size_t> forward_cpu(const std::vector<uint8_t>& text_with_sentinel)
    {
        const std::size_t n = text_with_sentinel.size();
        if (n == 0) return {{}, 0};

        // Build a suffix array of rotations: SA[i] = starting index of i-th lexicographic rotation
        std::vector<std::size_t> SA(n);
        std::iota(SA.begin(), SA.end(), 0);

        auto cmp = [n, text_with_sentinel](const std::size_t a, const std::size_t b)
        {
            // Compare rotations (a .. a+n-1) and (b .. b+n-1) mod n
            for (std::size_t k = 0; k < n; ++k)
            {
                const uint8_t ca = text_with_sentinel[(a + k) % n];
                const uint8_t cb = text_with_sentinel[(b + k) % n];
                if (ca != cb) {
                    return ca < cb;
                }
            }
            return false;
        };
        std::ranges::sort(SA, cmp);

        // Build the BWT by taking the char preceding each rotation
        std::vector<uint8_t> out;
        out.resize(n);
        std::size_t primary = 0;            // row where the original string appears

        for (std::size_t idx = 0; idx < n; ++idx)
        {
            const auto j = SA[idx];
            out[idx] = text_with_sentinel[(j + n - 1) % n];
            if (j == 0) {
                primary = idx;
            }
        }
        return {std::move(out), primary};
    }

    inline std::vector<uint8_t> inverse_cpu(const std::vector<uint8_t> & bwt, const std::size_t primary)
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
        std::vector<uint8_t> text(n, '\0');
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
