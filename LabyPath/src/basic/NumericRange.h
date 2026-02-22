/*
 * NumericRange.h
 *
 *  Created on: Feb 19, 2018
 *      Author: florian
 */

#ifndef BASIC_NUMERICRANGE_H_
#define BASIC_NUMERICRANGE_H_

#include <cstdint>
#include <optional>

namespace laby {

template<typename T>
class NumericRange;

template<typename T>
class NumericRangeIterator {

public:

    NumericRangeIterator(int32_t i, const NumericRange<T>& range) :
            i { i }, //
            _range { &range } {

    }
    NumericRangeIterator(const NumericRangeIterator&) = default;

    NumericRangeIterator& operator=(const NumericRangeIterator& it) {
        i = it.i;
        _range = it._range;
        return *this;
    }

    bool operator !=(const NumericRangeIterator& it) {
        return i != it.i || _range != it._range;
    }

    bool operator ==(const NumericRangeIterator& it) {
        return i == it.i && _range == it._range;
    }

    T operator *() {
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
        T diff = (end - begin) / step;
        _end = diff + static_cast<T>(1);
        _step = step;
    }

    NumericRange(const T& begin, const T& end, const T& step) {
        _begin = begin;
        T diff = (end - begin) / step;
        _end = diff + static_cast<T>(1);
        _step = step;
    }

    NumericRangeIterator<T> begin() {
        return NumericRangeIterator<T> { 0, *this };
    }

    NumericRangeIterator<T> end() {
        return NumericRangeIterator<T> { _end, *this };
    }

    T getValue(int32_t i) const {
        return _begin + i * _step;
    }

private:
    T _begin;
    int32_t _end;
    T _step;
};

struct NumericHelper {
    static std::optional<int32_t> reduce(const int32_t& x, const int32_t& detailSize, const int32_t& globalSize);
};

} /* namespace laby */
#endif /* BASIC_NUMERICRANGE_H_ */
