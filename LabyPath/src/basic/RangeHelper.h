/*
 * RangeHelper.h
 *
 *  Created on: Jun 20, 2018
 *      Author: florian
 */

#ifndef BASIC_RANGEHELPER_H_
#define BASIC_RANGEHELPER_H_

#include <utility>

namespace laby {

template <typename T> class HalfedgeRangeIterator {

public:
    HalfedgeRangeIterator(bool isStart, T circ) : _start{isStart}, _circulator{std::move(circ)} {}

    HalfedgeRangeIterator(const HalfedgeRangeIterator&) = default;

    HalfedgeRangeIterator(HalfedgeRangeIterator&&) noexcept = default;

    HalfedgeRangeIterator& operator=(const HalfedgeRangeIterator&) = default;

    HalfedgeRangeIterator& operator=(HalfedgeRangeIterator&&) noexcept = default;

    ~HalfedgeRangeIterator() = default;

    bool operator!=(const HalfedgeRangeIterator& it) const { return _start != it._start || _circulator != it._circulator; }

    bool operator==(const HalfedgeRangeIterator& it) const { return _start == it._start && _circulator == it._circulator; }

    typename T::reference operator*() { return *_circulator; }

    HalfedgeRangeIterator& operator++() { // prefix increment
        _start = false;
        ++_circulator;

        return *this;
    }

private:
    bool _start;
    T _circulator;
};

template <typename T> class HalfedgeRange {

public:
    explicit HalfedgeRange(T circ) : _circulator{circ}, _ending{std::move(circ)} {}

    HalfedgeRangeIterator<T> begin() { return HalfedgeRangeIterator<T>{true, _circulator}; }

    HalfedgeRangeIterator<T> end() { return HalfedgeRangeIterator<T>{false, _ending}; }

private:
    T _circulator;
    T _ending;
};

template <typename T> class RangeIterator {
public:
    RangeIterator(T begin, T end) : _beginIt{std::move(begin)}, _endIt{std::move(end)} {}

    T& begin() { return _beginIt; }

    T& end() { return _endIt; }

private:
    T _beginIt;
    T _endIt;
};

class RangeHelper {
public:
    template <typename T> static auto make(T begin, T end) { return RangeIterator<T>(std::move(begin), std::move(end)); }

    template <typename T> static HalfedgeRange<T> make(T circulator) { return HalfedgeRange<T>(std::move(circulator)); }
};

} /* namespace laby */

#endif /* BASIC_RANGEHELPER_H_ */
