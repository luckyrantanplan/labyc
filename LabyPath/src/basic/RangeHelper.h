/*
 * RangeHelper.h
 *
 *  Created on: Jun 20, 2018
 *      Author: florian
 */

#ifndef BASIC_RANGEHELPER_H_
#define BASIC_RANGEHELPER_H_

namespace laby {

template<typename T>
class HalfedgeRangeIterator {

public:

    HalfedgeRangeIterator(bool start, T& circulator) :
            start { start }, //
            circulator { circulator } {

    }
    HalfedgeRangeIterator(const HalfedgeRangeIterator&) = default;

    HalfedgeRangeIterator& operator=(const HalfedgeRangeIterator& it) {
        start = it.start;
        circulator = it.circulator;
        return *this;
    }

    bool operator !=(const HalfedgeRangeIterator& it) const {
        return start != it.start || circulator != it.circulator;
    }

    bool operator ==(const HalfedgeRangeIterator& it) const {
        return start == it.start && circulator == it.circulator;
    }

    const typename T::reference operator *() {
        return *circulator;

    }

    HalfedgeRangeIterator& operator ++() { //prefix increment
        start = false;
        ++circulator;

        return *this;
    }

private:
    bool start;
    T& circulator;
};

template<typename T>
class HalfedgeRange {

public:

    explicit HalfedgeRange(T&& circulator) :
            circulator { circulator }, ending { circulator } {
    }

    explicit HalfedgeRange(const T& circulator) :
            circulator { circulator }, ending { circulator } {
    }

    HalfedgeRangeIterator<T> begin() {
        return HalfedgeRangeIterator<T> { true, circulator };
    }

    HalfedgeRangeIterator<T> end() {
        return HalfedgeRangeIterator<T> { false, ending };
    }

    T circulator;
    T ending;
};

template<typename T>
class RangeIterator {
public:
    RangeIterator(T begin, T end) :
            beginIt { begin }, endIt { end } {
    }

    T& begin() {
        return beginIt;
    }

    T& end() {
        return endIt;
    }

private:
    T beginIt;
    T endIt;

};

class RangeHelper {
public:
    template<typename T>
    static auto make(T& begin, T& end) {
        return RangeIterator<T>(begin, end);
    }
    template<typename T>
    static auto make(T&& begin, T&& end) {
        return RangeIterator<T>(begin, end);
    }

    template<typename T>
    static HalfedgeRange<T> make(T& circulator) {
        return HalfedgeRange<T>(circulator);
    }
    template<typename T>
    static HalfedgeRange<T> make(T&& circulator) {
        return HalfedgeRange<T>(circulator);
    }
};

} /* namespace laby */

#endif /* BASIC_RANGEHELPER_H_ */
