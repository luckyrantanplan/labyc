/*
 * NumericRange.h
 *
 *  Created on: Feb 19, 2018
 *      Author: florian
 */

#ifndef BASIC_NUMERICRANGE_H_
#define BASIC_NUMERICRANGE_H_

#include <cmath>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace laby {

template <typename T> class NumericRange;

template <typename T> class NumericRangeIterator {

public:
    NumericRangeIterator(int32_t idx, const NumericRange<T>& range) : _index{idx}, _range{&range} {}

    NumericRangeIterator(const NumericRangeIterator&) = default;

    NumericRangeIterator(NumericRangeIterator&&) noexcept = default;

    auto operator=(const NumericRangeIterator&) -> NumericRangeIterator& = default;

    auto operator=(NumericRangeIterator&&) noexcept -> NumericRangeIterator& = default;

    ~NumericRangeIterator() = default;

    auto operator!=(const NumericRangeIterator& it) const -> bool { return _index != it._index || _range != it._range; }

    auto operator==(const NumericRangeIterator& it) const -> bool { return _index == it._index && _range == it._range; }

    auto operator*() const -> T { return _range->getValue(_index); }

    auto operator++() -> NumericRangeIterator& { // prefix increment
        ++_index;
        return *this;
    }

private:
    int32_t _index;
    const NumericRange<T>* _range;
};

template <typename T> class NumericRange {

public:
    NumericRange(T begin, T end, T step) : _begin{begin}, _end{computeEnd(begin, end, step)}, _step{step} {}

    [[nodiscard]] auto begin() const -> NumericRangeIterator<T> { return NumericRangeIterator<T>{0, *this}; }

    [[nodiscard]] auto end() const -> NumericRangeIterator<T> { return NumericRangeIterator<T>{_end, *this}; }

    [[nodiscard]] auto getValue(int32_t i) const -> T { return _begin + i * _step; }

private:
    /// Compute the number of steps, using std::lround for floating-point types
    /// to avoid truncation issues (e.g., 4.0/0.5 = 7.9999... → 7 instead of 8).
    static auto computeEnd(const T& begin, const T& end, const T& step) -> int32_t {
        if constexpr (std::is_floating_point_v<T>) {
            return static_cast<int32_t>(std::lround((end - begin) / step)) + 1;
        }
        else {
            return static_cast<int32_t>((end - begin) / step) + 1;
        }
    }

    T _begin;
    int32_t _end;
    T _step;
};

struct NumericHelper {
    static auto reduce(const int32_t& x, const int32_t& detailSize, const int32_t& globalSize) -> std::optional<int32_t>;
};

} /* namespace laby */
#endif /* BASIC_NUMERICRANGE_H_ */
