/*
 * StreamLineTest.h
 *
 *  Created on: Mar 26, 2018
 *      Author: florian
 */

#ifndef TEST_STREAMLINETEST_H_
#define TEST_STREAMLINETEST_H_

namespace laby {
namespace aniso {

class Cell;

class StreamLineTest {
public:
    static int test(int argc, char *argv[]);

private:

    static aniso::Cell createCell();
}
;

} /* namespace aniso */
} /* namespace laby */

#endif /* TEST_STREAMLINETEST_H_ */
