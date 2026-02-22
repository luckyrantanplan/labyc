/*
 * Main.cpp
 *
 *  Created on: Nov 10, 2017
 *      Author: florian
 */

#include <iostream>

#include "basic/ProfileActivate.h"
#include "MessageIO.h"
//#include "test/VoronoiSeg.h"

//#include "test/PolygonSetTest.h"

//#include "test/PathRenderingTest.h"

//#include "test/GrayPenTest.h"
//#include "test/StreamLineTest.h"

int main(int argc, char *argv[]) {
    using namespace laby;
    std::ios::sync_with_stdio(false);
    ProfileActivate::start();

    return MessageIO::parseMessage(argc, argv);

}

