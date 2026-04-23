#pragma once

#include <string>

#include "../protoc/AllConfig.pb.h"
#include "ComplexField2D.h"
#include "HqNoise.h"

namespace laby::generator {

class HqNoise2DSampler {
  public:
    static auto sample(const proto::HqNoise& config) -> ComplexField2D;
    static auto writePreviewSvg(const std::string& path, const ComplexField2D& field,
                                const proto::HqNoise& config) -> void;
};

} // namespace laby::generator