/*
 * MessageIO.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: florian
 */

#include "MessageIO.h"

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "SkeletonGrid.h"
#include "Anisotrop/Placement.h"
#include "protoc/AllConfig.pb.h"
#include "Rendering/GraphicRendering.h"
#include "AlternaRoute/AlternateRoute.h"

namespace laby {

int MessageIO::parseMessage(int argc, char *argv[]) {

    std::cout << "Start " << argc << std::endl;

    if (argc < 2) {
        std::cout << "conf missing" << std::endl;
    }
    std::cout << argv[1] << std::endl;
    std::string filename(argv[1]);
    std::cout << filename << std::endl;

    std::fstream fs;
    fs.open(filename, std::fstream::in);

    std::stringstream strStream;
    strStream << fs.rdbuf(); //read the file

    proto::AllConfig message;
    google::protobuf::util::JsonStringToMessage(strStream.str(), &message);

    if (message.has_skeletongrid()) {
        std::cout << "create Skeleton grid" << std::endl;
        SkeletonGrid(message.skeletongrid());
    }
    if (message.has_routing()) {
        const proto::Routing& routingconf = message.routing();

        if (routingconf.has_placement()) {
            const proto::Placement& placement_conf = routingconf.placement();
            aniso::Placement placement(placement_conf, routingconf.filepaths());
        }
        if (routingconf.has_alternaterouting()) {
            const proto::AlternateRouting& alternate_conf = routingconf.alternaterouting();
            AlternateRoute route(alternate_conf, routingconf.filepaths());
        }

    }

    if (message.has_ggraphicrendering()) {
        const proto::GraphicRendering& renderconf = message.ggraphicrendering();
        GraphicRendering g(renderconf);
    }
    return 0;
}

} /* namespace laby */
