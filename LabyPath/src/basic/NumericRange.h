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

namespace laby {

template<typename T>
class NumericRange;

template<typename T>
class NumericRangeIterator {

public:

    NumericRangeIterator(int32_t idx, const NumericRange<T>& range) :
            i { idx }, //
            _range { &range } {

    }
    NumericRangeIterator(const NumericRangeIterator&) = default;

    NumericRangeIterator& operator=(const NumericRangeIterator& it) {
        i = it.i;
        _range = it._range;
        return *this;
    }

    bool operator !=(const NumericRangeIterator& it) const {
        return i != it.i || _range != it._range;
    }

    bool operator ==(const NumericRangeIterator& it) const {
        return i == it.i && _range == it._range;
    }

    T operator *() const {
        return _range->getValue(i);

    }

    NumericRangeIterator& operator ++() { //prefix increment
        ++i;
        return *this;
    }

private:
    int32_t i;
    const NumericRange<T>* _range;

};

template<typename T>
class NumericRange {

public:

    NumericRange(T&& begin, T&& end, T&& step) {
        _begin = begin;
        _end = computeEnd(begin, end, step);
        _step = step;
    }

    NumericRange(const T& begin, const T& end, const T& step) {
        _begin = begin;
        _end = computeEnd(begin, end, step);
        _step = step;
    }

    NumericRangeIterator<T> begin() const {
        return NumericRangeIterator<T> { 0, *this };
    }

    NumericRangeIterator<T> end() const {
        return NumericRangeIterator<T> { _end, *this };
    }

    T getValue(int32_t i) const {
        return _begin + i * _step;
    }

private:
    /// Compute the number of steps, using std::lround for floating-point types
    /// to avoid truncation issues (e.g., 4.0/0.5 = 7.9999... → 7 instead of 8).
    static int32_t computeEnd(const T& begin, const T& end, const T& step) {
        if constexpr (std::is_floating_point_v<T>) {
            return static_cast<int32_t>(std::lround((end - begin) / step)) + 1;
        } else {
            return static_cast<int32_t>((end - begin) / step) + 1;
        }
    }

    T _begin;
    int32_t _end;
    T _step;
};

struct NumericHelper {
    static std::optional<int32_t> reduce(const int32_t& x, const int32_t& detailSize, const int32_t& globalSize);
};

} /* namespace laby */
#endif /* BASIC_NUMERICRANGE_H_ */
