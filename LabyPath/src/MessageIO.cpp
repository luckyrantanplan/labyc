/*
 * MessageIO.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: florian
 */

#include "MessageIO.h"

#include "AlternaRoute/AlternateRoute.h"
#include "Anisotrop/Placement.h"
#include "Rendering/GraphicRendering.h"
#include "SkeletonGrid.h"
#include "generator/ComplexField2D.h"
#include "generator/HqNoise2DSampler.h"
#include "generator/StreamLine.h"
#include "protoc/AllConfig.pb.h"
#include <cmath>
#include <fstream>
#include <google/protobuf/util/json_util.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace laby {

namespace {

auto runHqNoiseStage(const proto::HqNoise& config) -> void {
    const generator::ComplexField2D field = generator::HqNoise2DSampler::sample(config);
    writeComplexField(config.filepaths().outputfile(), field);
    if (!config.previewfile().empty()) {
        generator::HqNoise2DSampler::writePreviewSvg(config.previewfile(), field, config);
    }
}

auto buildStreamLineConfig(const proto::StreamLineCfg& config,
                           const generator::ComplexField2DMeta& meta)
    -> generator::StreamLine::Config {
    generator::StreamLine::Config streamConfig{};
    streamConfig.resolution = config.resolution() > 0
                                  ? config.resolution()
                                  : static_cast<int>(std::max(1L, std::lround(1.0 / meta.scale)));
    streamConfig.simplify_distance = config.simplifydistance();
    streamConfig.dRat = config.drat();
    streamConfig.epsilon = config.epsilon();
    streamConfig.size = config.size() > 0.0
                            ? config.size()
                            : static_cast<double>(std::max(meta.width, meta.height)) * meta.scale;
    streamConfig.divisor = config.divisor();
    streamConfig.sample_scale = meta.scale;
    streamConfig.old_RegularGrid = true;
    return streamConfig;
}

auto runStreamLineStage(const proto::StreamLineCfg& config) -> void {
    generator::ComplexField2D field = generator::readComplexField(config.filepaths().inputfile());
    generator::StreamLine streamLine(buildStreamLineConfig(config, field.meta),
                                     std::move(field.values));
    streamLine.render();

    const CGAL::Bbox_2 bbox(
        field.meta.originX, field.meta.originY,
        field.meta.originX + static_cast<double>(field.meta.width) * field.meta.scale,
        field.meta.originY + static_cast<double>(field.meta.height) * field.meta.scale);
    GraphicRendering::printRibbonSvg(bbox, config.filepaths().outputfile(),
                                     config.strokethickness(), streamLine.ribbons());
}

} // namespace

auto MessageIO::parseMessage(const std::vector<std::string_view>& arguments) -> int {

    std::cout << "Start " << arguments.size() << '\n';

    if (arguments.size() < 2U) {
        std::cout << "conf missing" << '\n';
        return 1;
    }
    std::cout << arguments.at(1) << '\n';
    const std::string filename(arguments.at(1));
    std::cout << filename << '\n';

    std::ifstream inputFileStream(filename);
    if (!inputFileStream.is_open()) {
        std::cerr << "failed to open config file: " << filename << '\n';
        return 1;
    }

    std::stringstream strStream;
    strStream << inputFileStream.rdbuf(); // read the file

    try {
        proto::AllConfig message;
        const auto parseStatus =
            google::protobuf::util::JsonStringToMessage(strStream.str(), &message);
        if (!parseStatus.ok()) {
            std::cerr << "failed to parse config JSON: " << parseStatus.ToString() << '\n';
            return 1;
        }

        if (message.has_hqnoise()) {
            runHqNoiseStage(message.hqnoise());
        }
        if (message.has_skeletongrid()) {
            std::cout << "create Skeleton grid" << '\n';
            SkeletonGrid(message.skeletongrid());
        }
        if (message.has_routing()) {
            const proto::Routing& routingconf = message.routing();

            if (routingconf.has_placement()) {
                const proto::Placement& placementConfig = routingconf.placement();
                const aniso::Placement placement(placementConfig, routingconf.filepaths());
            }
            if (routingconf.has_alternaterouting()) {
                const proto::AlternateRouting& alternateConfig = routingconf.alternaterouting();
                const AlternateRoute route(alternateConfig, routingconf.filepaths());
            }
        }
        if (message.has_streamline()) {
            runStreamLineStage(message.streamline());
        }

        if (message.has_ggraphicrendering()) {
            const proto::GraphicRendering& renderconf = message.ggraphicrendering();
            const GraphicRendering graphicRendering(renderconf);
        }
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}

} // namespace laby
