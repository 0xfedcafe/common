#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <ranges>
#include <string>

#include "es3n1n/common/memory/size.hpp"

namespace math {
    enum class BigIntSerializationMode : std::uint8_t {
        HEX_LOWERCASE = 0,
        HEX_UPPERCASE,
    };

    enum class BigIntDeserializationMode : std::uint8_t {
        HEX = 0,
        DEC,
    };

    template <std::size_t Bits>
        requires(Bits > *memory::kWordInBits && Bits % CHAR_BIT == 0)
    class FixedInt {
    public:
        constexpr FixedInt() = default;
        ~FixedInt() = default;

        template <std::unsigned_integral Ty>
        constexpr explicit FixedInt(Ty value) {
            auto size = memory::size_of<Ty>();
            if (size > memory::size_of<memory::HalfWord>()) {
                for (size_t i = 0; value; i++) {
                    // cast instead of ull because we're using memory::Word
                    data_[i] = value & ((static_cast<memory::Word>(1) << *memory::kHalfWordInBits) - 1);
                    value >>= *memory::kHalfWordInBits;
                }
            } else {
                data_[0] = value;
            }
        }

        constexpr explicit FixedInt(const std::string_view value) {
            memory::Word accumulator = 0;
            memory::Word multiplier = 1;
            std::size_t i = 0;

            for (const auto c : std::ranges::reverse_view(value)) {
                const memory::Word digit = c - '0';
                if (accumulator >= std::numeric_limits<memory::HalfWord>::max()) {
                    data_[i] = accumulator & ((static_cast<memory::Word>(1) << *memory::kHalfWordInBits) - 1);
                    accumulator >>= *memory::kHalfWordInBits;
                    multiplier = 1;
                    i++;
                }
                accumulator += digit * multiplier;
                multiplier *= 10;
            }
            if (accumulator > 0) {
                data_[i] = accumulator;
            }
        }


        constexpr explicit FixedInt(const std::span<uint8_t> value) {
            for(size_t i = 0; i < value.size(); i++) {
                data_[(value.size() - i - 1) / (*memory::kHalfWordInBits / 8)] = value[(i * 8) % *memory::kHalfWordInBits];
            }
        }


        [[nodiscard]] constexpr auto operator<=>(const FixedInt&) const = default;

        [[nodiscard]] constexpr FixedInt& operator-() {
            data_[kNumberOfHalfWords - 1] ^= 1 << (*memory::kWordInBits - 1);
            return *this;
        }

        constexpr FixedInt& operator+=(const FixedInt& rhs) {
            memory::Word carry = 0;

            for (std::size_t i = 0; i < kNumberOfHalfWords; i++) {
                memory::Word res = data_[i] + rhs.data_[i] + carry;
                carry = (res >> *memory::kHalfWordInBits);
                data_[i] = res & ((1 << *memory::kHalfWordInBits) - 1);
            }

            return *this;
        }

        const FixedInt& operator-=(const FixedInt& rhs) {
            rhs = -rhs;
            *this += rhs;
            return *this;
        }

        constexpr FixedInt& operator*=(const FixedInt& rhs) {
            FixedInt result;

            memory::Word carry = 0;
            for (size_t fPos = 0; fPos < kNumberOfHalfWords; fPos++) {
                for (size_t sPos = 0; sPos < kNumberOfHalfWords; sPos++) {
                    memory::Word res = data_[fPos] * rhs.data_[sPos] + carry;
                    result.data_[fPos + sPos] += res & ((1 << *memory::kHalfWordInBits) - 1);
                    carry = (res >> *memory::kHalfWordInBits);
                }
                carry = 0;
            }

            *this = result;
            return *this;
        }

        [[nodiscard]] constexpr std::string to_string(const BigIntSerializationMode mode = BigIntSerializationMode::HEX_LOWERCASE) const {
            std::string result;

            for (auto& word : data_) {
                switch (mode) {
                case BigIntSerializationMode::HEX_LOWERCASE:
                    result += std::format("{:0{}x}", word, kNumberOfHexDigitsPerWord);
                    break;
                case BigIntSerializationMode::HEX_UPPERCASE:
                    result += std::format("{:0{}X}", word, kNumberOfHexDigitsPerWord);
                    break;
                }
            }

            return result;
        }

    private:
        constexpr static auto kNumberOfHalfWords = (Bits + (*memory::kHalfWordInBits - 1)) / *memory::kHalfWordInBits;
        constexpr static auto kNumberOfHexDigitsPerWord = *(memory::kHalfWordInBits / 4_bits);

        std::array<memory::Word, kNumberOfHalfWords> data_ = {};
    };

    using Int128 = FixedInt<128>;
    using Int256 = FixedInt<256>;
} // namespace math
