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
#include "protoc/AllConfig.pb.h"
#include <fstream>
#include <google/protobuf/util/json_util.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace laby {

auto MessageIO::parseMessage(const std::vector<std::string_view>& arguments) -> int {

    std::cout << "Start " << arguments.size() << '\n';

    if (arguments.size() < 2U) {
        std::cout << "conf missing" << '\n';
        return 1;
    }
    std::cout << arguments.at(1) << '\n';
    const std::string filename(arguments.at(1));
    std::cout << filename << '\n';

    std::fstream inputFileStream;
    inputFileStream.open(filename, std::fstream::in);

    std::stringstream strStream;
    strStream << inputFileStream.rdbuf(); // read the file

    proto::AllConfig message;
    google::protobuf::util::JsonStringToMessage(strStream.str(), &message);

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

    if (message.has_ggraphicrendering()) {
        const proto::GraphicRendering& renderconf = message.ggraphicrendering();
        const GraphicRendering graphicRendering(renderconf);
    }
    return 0;
}

} // namespace laby
