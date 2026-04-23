/*
 * Net.hpp
 *
 *  Created on: Feb 8, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_NET_H_
#define ANISOTROP_NET_H_

#include <complex>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "../GeomData.h"

namespace laby::basic {
class LinearGradient;
} /* namespace laby::basic */

namespace laby::aniso {

class Pin {
  public:
    Pin(Vertex& vertex, double thickness) : _vertex(&vertex), _thickness(thickness) {}

    [[nodiscard]] auto thickness() const -> double {
        return _thickness;
    }

    auto vertex() -> Vertex& {
        return *_vertex;
    }

    [[nodiscard]] auto vertex() const -> const Vertex& {
        return *_vertex;
    }

    void setPolyConvexIndex(std::size_t polyConvexIndex) {
        _polyConvexIndex = polyConvexIndex;
    }

    [[nodiscard]] auto polyConvexIndex() const -> std::size_t {
        return _polyConvexIndex;
    }

  private:
    Vertex* _vertex = nullptr;
    double _thickness = 0.0;
    std::size_t _polyConvexIndex = 0;
};

class Net {
  public:
    struct SourcePin {
        Pin value;
    };

    struct TargetPin {
        Pin value;
    };

    Net(SourcePin source, TargetPin target, int32_t netId = 0)
        :                        //
          _source(source.value), //
          _target(target.value), //
          _id{netId} {}

    //  Net(const Net &) = default;

    void setTarget(Pin& pin) {
        _target = pin;
    }

    void setSource(Pin& pin) {
        _source = pin;
    }

    [[nodiscard]] auto source() const -> const Pin& {
        return _source;
    }

    [[nodiscard]] auto target() const -> const Pin& {
        return _target;
    }

    auto source() -> Pin& {
        return _source;
    }

    auto target() -> Pin& {
        return _target;
    }

    [[nodiscard]] auto gradient() const -> basic::LinearGradient;

    [[nodiscard]] static auto
    extractPins(const std::vector<Net>& nets) -> std::vector<std::complex<double>>;

    [[nodiscard]] auto path() const -> const std::vector<std::size_t>& {
        return _path;
    }

    auto path() -> std::vector<std::size_t>& {
        return _path;
    }

    [[nodiscard]] auto id() const -> int32_t {
        return _id;
    }

    void markPlaced() {
        _isPlaced = true;
    }

    [[nodiscard]] auto isPlaced() const -> bool {
        return _isPlaced;
    }

  private:
    Pin _source;
    Pin _target;
    std::vector<std::size_t> _path;
    int32_t _id = 0;
    bool _isPlaced = false;
};

} /* namespace laby::aniso */

#endif /* NET_H_ */
