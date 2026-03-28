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

    auto operator=(const HalfedgeRangeIterator&) -> HalfedgeRangeIterator& = default;

    auto operator=(HalfedgeRangeIterator&&) noexcept -> HalfedgeRangeIterator& = default;

    ~HalfedgeRangeIterator() = default;

    auto operator!=(const HalfedgeRangeIterator& other) const -> bool {
        return _start != other._start || _circulator != other._circulator;
    }

    auto operator==(const HalfedgeRangeIterator& other) const -> bool {
        return _start == other._start && _circulator == other._circulator;
    }

    auto operator*() -> typename T::reference { return *_circulator; }

    auto operator++() -> HalfedgeRangeIterator& { // prefix increment
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

    auto begin() -> HalfedgeRangeIterator<T> { return HalfedgeRangeIterator<T>{true, _circulator}; }

    auto end() -> HalfedgeRangeIterator<T> { return HalfedgeRangeIterator<T>{false, _ending}; }

private:
    T _circulator;
    T _ending;
};

template <typename T> class RangeIterator {
public:
    RangeIterator(T begin, T end) : _beginIt{std::move(begin)}, _endIt{std::move(end)} {}

    auto begin() -> T& { return _beginIt; }

    auto end() -> T& { return _endIt; }

private:
    T _beginIt;
    T _endIt;
};

class RangeHelper {
public:
    template <typename T> static auto make(T begin, T end) { return RangeIterator<T>(std::move(begin), std::move(end)); }

    template <typename T> static auto make(T circulator) -> HalfedgeRange<T> { return HalfedgeRange<T>(std::move(circulator)); }
};

} /* namespace laby */

#endif /* BASIC_RANGEHELPER_H_ */
