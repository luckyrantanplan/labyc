/*
 * Net.hpp
 *
 *  Created on: Feb 8, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_NET_H_
#define ANISOTROP_NET_H_


#include <bits/stdint-intn.h>
#include <complex>
#include <cstddef>
#include <vector>

#include "../GeomData.h"


namespace laby {
namespace basic {
class LinearGradient;
} /* namespace basic */
} /* namespace laby */

namespace laby {
namespace aniso {

class Pin {
public:
    Pin(Vertex& vertex, const double& thickness) :
            _vertex(&vertex), _thickness(thickness) {
    }

    double thickness() const {
        return _thickness;
    }
    Vertex& vertex() {
        return *_vertex;
    }

    const Vertex& vertex() const {
        return *_vertex;
    }

    void setPolyConvexIndex(const std::size_t& polyConvexIndex) {
        _polyConvexIndex = polyConvexIndex;
    }

    std::size_t polyConvexIndex() const {
        return _polyConvexIndex;
    }
private:
    Vertex* _vertex = nullptr;
    double _thickness = 0;
    std::size_t _polyConvexIndex = 0;

};

class Net {
public:

    Net(const Pin& source, const Pin & target, const int32_t& id = 0) : //
            _source(source), //
            _target(target), //
            _id { id } {
    }

    //  Net(const Net &) = default;

    void setTarget(Pin& p) {
        _target = p;
    }

    void setSource(Pin& p) {
        _source = p;
    }

    const Pin& source() const {
        return _source;
    }

    const Pin& target() const {
        return _target;
    }

    Pin& source() {
        return _source;
    }

    Pin& target() {
        return _target;
    }

    basic::LinearGradient gradient() const;

    static std::vector<std::complex<double> > extractPins(const std::vector<Net>& nets);

    const std::vector<size_t>& path() const {
        return _path;
    }

    std::vector<size_t>& path() {
        return _path;
    }

    int32_t id() const {
        return _id;
    }

    void markPlaced() {
        _isPlaced = true;

    }

    bool isPlaced() const {
        return _isPlaced;
    }

private:

    Pin _source;
    Pin _target;
    std::vector<std::size_t> _path;
    int32_t _id = 0;
    bool _isPlaced = false;
}
;

} /* namespace aniso */
} /* namespace laby */

#endif /* NET_H_ */
