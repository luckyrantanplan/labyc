/*
 * PathRenderingTest.h
 *
 *  Created on: Mar 16, 2018
 *      Author: florian
 */

#ifndef TEST_PATHRENDERINGTEST_H_
#define TEST_PATHRENDERINGTEST_H_
namespace laby {
namespace generator {
class StreamLine;
} /* namespace generator */

namespace aniso {
class Cell;

class PathRenderingTest {
public:
    static int test(int argc, char *argv[]);

private:
    static aniso::Cell createCell();
    static generator::StreamLine createStreamLine();
};
} /* namespace aniso */
} /* namespace laby */

#endif /* TEST_PATHRENDERINGTEST_H_ */
