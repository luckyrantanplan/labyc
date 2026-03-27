/*
 * Main.cpp
 *
 *  Created on: Nov 10, 2017
 *      Author: florian
 */

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>

#include "MessageIO.h"
#include "basic/ProfileActivate.h"
// #include "test/VoronoiSeg.h"

// #include "test/PolygonSetTest.h"

// #include "test/PathRenderingTest.h"

// #include "test/GrayPenTest.h"
// #include "test/StreamLineTest.h"

auto main(int argc, char* argv[]) -> int {
    using namespace laby;
    std::ios::sync_with_stdio(false);
    ProfileActivate::start();

    std::vector<const char*> rawArguments;
    rawArguments.reserve(static_cast<std::size_t>(argc));
    std::copy_n(argv, argc, std::back_inserter(rawArguments));

    std::vector<std::string_view> arguments;
    arguments.reserve(static_cast<std::size_t>(argc));
    for (const char* argument : rawArguments) {
        arguments.emplace_back(argument);
    }

    return MessageIO::parseMessage(arguments);
}
