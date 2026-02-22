/*
 * ConfigAll.h
 *
 *  Created on: Jul 26, 2018
 *      Author: florian
 */

#ifndef TEST_CONFIGALL_H_
#define TEST_CONFIGALL_H_

#include <string>

#include "Anisotrop/Cell.h"
#include "Anisotrop/Placement.h"
#include "Rendering/GraphicRendering.h"
#include "SkeletonGrid.h"

namespace proto {
class Fenetre;
class PenStroke;
class GraphicRendering;
class Placement;
class Routing;
} /* namespace proto */

namespace laby {

class ConfigAll {

public:
    template<typename T>
    void set(T& value, const T& protoValue) {
        T ref { };
        if (protoValue != ref) {
            value = protoValue;
        }
    }

    void decodeProtobuf(const std::string& configFilename);

    GraphicRendering::Config render_config;

    std::string filename;
    double simplificationOfOriginalSVG = 0.1;
    static void printTest();
private:

    void decodeGraphicRendering(const proto::GraphicRendering& rendering);
    void decodeFenetre(const proto::Fenetre& message);

    void decodePenStroke(const proto::PenStroke& message);

};

} /* namespace laby */

#endif /* TEST_CONFIGALL_H_ */
