#pragma once

#include <cstdint>
#include <stdexcept>

namespace mc {

// Implements java.util.Random's 48-bit LCG for later legacy worldgen work.
class JavaRandom {
public:
    explicit JavaRandom(std::int64_t seed) noexcept
        : seed_((static_cast<std::uint64_t>(seed) ^ multiplier) & mask) {}

    [[nodiscard]] std::uint32_t next(unsigned bits) noexcept {
        seed_ = (seed_ * multiplier + addend) & mask;
        return static_cast<std::uint32_t>(seed_ >> (48U - bits));
    }

    [[nodiscard]] std::int32_t nextInt() noexcept {
        return static_cast<std::int32_t>(next(32));
    }

    [[nodiscard]] std::int32_t nextInt(std::int32_t bound) {
        if (bound <= 0) {
            throw std::invalid_argument("bound must be positive");
        }
        if ((bound & -bound) == bound) {
            return static_cast<std::int32_t>((static_cast<std::int64_t>(bound) * next(31)) >> 31);
        }
        std::int32_t bits;
        std::int32_t value;
        do {
            bits = static_cast<std::int32_t>(next(31));
            value = bits % bound;
        } while (bits - value + (bound - 1) < 0);
        return value;
    }

private:
    static constexpr std::uint64_t multiplier = 0x5DEECE66DULL;
    static constexpr std::uint64_t addend = 0xBULL;
    static constexpr std::uint64_t mask = (1ULL << 48U) - 1ULL;
    std::uint64_t seed_{};
};

} // namespace mc
