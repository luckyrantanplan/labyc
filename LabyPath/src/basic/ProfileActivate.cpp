/*
 * ProfileActivate.cpp
 *
 *  Created on: Mar 7, 2018
 *      Author: florian
 */

#include "ProfileActivate.h"

#include "EasyProfilerCompat.h"
//#include <iostream>

namespace laby {
void ProfileActivate::start() {
    EASY_PROFILER_ENABLE;
    profiler::startListen();
}

void ProfileActivate::end() {
    profiler::stopListen();
    profiler::dumpBlocksToFile("test_profile.prof");
}
} /* namespace laby */
