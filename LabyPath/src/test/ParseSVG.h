/*
 * ParseSVG.h
 *
 *  Created on: Jun 14, 2018
 *      Author: florian
 */

#ifndef TEST_PARSESVG_H_
#define TEST_PARSESVG_H_

namespace laby {

class GeomFeatures;
class ConfigAll;

namespace aniso {
class Cell;
}

class ParseSVG {
public:

    static int test(int argc, char *argv[]);
private:

    static aniso::Cell createCell(const ConfigAll& config);
    static void route(const ConfigAll& config, aniso::Cell& cell, GeomFeatures& geoFeature);

};

} /* namespace laby */

#endif /* TEST_PARSESVG_H_ */
