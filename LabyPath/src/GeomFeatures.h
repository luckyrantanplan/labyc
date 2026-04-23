/*
 * GeomFeatures.h
 *
 *  Recreated: GeomFeatures.h was missing from the repository.
 *  Reconstructed from usage in StreamLineTest.cpp, ParseSVG.cpp,
 *  PolygonSetTest.cpp, and PoissonGeneratorTest.cpp.
 */

#ifndef GEOMFEATURES_H_
#define GEOMFEATURES_H_

#include <complex>
#include <utility>
#include <vector>

#include "OrientedRibbon.h"

namespace laby {

class GeomFeatures {
  public:
    GeomFeatures() = default;

    auto arr() -> OrientedRibbon& { return _arr; }
    [[nodiscard]] auto arr() const -> const OrientedRibbon& { return _arr; }

    auto nets() -> std::vector<std::complex<double>>& { return _nets; }
    [[nodiscard]] auto nets() const -> const std::vector<std::complex<double>>& { return _nets; }

    void setNets(std::vector<std::complex<double>> nets) { _nets = std::move(nets); }

  private:
    OrientedRibbon _arr;
    std::vector<std::complex<double>> _nets;
};

} /* namespace laby */

#endif /* GEOMFEATURES_H_ */
