/*
 * ConfigAll.cpp
 *
 *  Created on: Jul 26, 2018
 *      Author: florian
 */

#include "ConfigAll.h"
#include "protoc/AllConfig.pb.h"
#include <fstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <iostream>
#include <sstream>

namespace laby {

void ConfigAll::decodeFenetre(const proto::Fenetre& message) {
    set(render_config.fenetre.height_point, message.height_point());
    set(render_config.fenetre.interactive_display, message.interactive_display());
    set(render_config.fenetre.screen_heigth_pixel, message.screen_heigth_pixel());
    set(render_config.fenetre.screen_width_pixel, message.screen_width_pixel());
    switch (message.surface()) {
    case proto::Fenetre_Surface_NONE: {
        render_config.fenetre.surface = render_config.fenetre.Surface::NONE;
        break;
    }
    case proto::Fenetre_Surface_PDF: {
        render_config.fenetre.surface = render_config.fenetre.Surface::PDF;
        break;
    }
    case proto::Fenetre_Surface_SVG: {
        render_config.fenetre.surface = render_config.fenetre.Surface::SVG;
        break;
    }
    default: {
        // do nothing
        break;
    }
    }
    set(render_config.fenetre.width_point, message.width_point());
}

void ConfigAll::decodePenStroke(const proto::PenStroke& message) {
    set(render_config.penConfig.antisymmetric_amplitude, message.antisymmetric_amplitude());
    set(render_config.penConfig.antisymmetric_freq, message.antisymmetric_freq());
    set(render_config.penConfig.antisymmetric_seed, message.antisymmetric_seed());
    set(render_config.penConfig.canvas_size, message.canvas_size());
    set(render_config.penConfig.symmetric_amplitude, message.symmetric_amplitude());
    set(render_config.penConfig.symmetric_freq, message.symmetric_freq());
    set(render_config.penConfig.symmetric_seed, message.symmetric_seed());
    set(render_config.penConfig.thickness, message.thickness());
}

void ConfigAll::decodeGraphicRendering(const proto::GraphicRendering& rendering) {
    set(render_config.color_b, rendering.color_b());
    set(render_config.color_g, rendering.color_g());
    set(render_config.color_r, rendering.color_r());
    set(render_config.emulate_pencil, rendering.emulate_pencil());

    if (rendering.has_fenetre()) {
        const proto::Fenetre& fenetre = rendering.fenetre();
        decodeFenetre(fenetre);
    }
    set(render_config.little_square_rectangle_width, rendering.little_square_rectangle_width());
    set(render_config.little_square_width, rendering.little_square_width());
    if (rendering.has_penconfig()) {
        const proto::PenStroke& penConfig = rendering.penconfig();
        decodePenStroke(penConfig);
    }

    set(render_config.smoothing_iterations, rendering.smoothing_iterations());
    set(render_config.smoothing_tension, rendering.smoothing_tension());
    set(render_config.translate, rendering.translate());
    set(render_config.zoom, rendering.zoom());
}

void ConfigAll::printTest() {
    proto::AllConfig message;

    message.set_filename("leaves2.svg");
    message.set_simplificationoforiginalsvg(0.1);

    auto& render_config = *message.mutable_ggraphicrendering();

    render_config.set_zoom(4.);

    render_config.mutable_fenetre()->set_screen_width_pixel(1000);
    render_config.mutable_fenetre()->set_screen_heigth_pixel(1000);
    render_config.mutable_fenetre()->set_surface(proto::Fenetre_Surface_SVG);
    render_config.mutable_fenetre()->set_interactive_display(true);
    render_config.set_emulate_pencil(true);
    render_config.mutable_penconfig()->set_thickness(1. / 4.);
    render_config.mutable_penconfig()->set_antisymmetric_amplitude(0.3);
    render_config.mutable_penconfig()->set_antisymmetric_freq(10);
    render_config.mutable_penconfig()->set_antisymmetric_seed(5.);
    render_config.mutable_penconfig()->set_symmetric_amplitude(0.1);
    render_config.mutable_penconfig()->set_symmetric_freq(3);
    render_config.mutable_penconfig()->set_symmetric_seed(8.);

    std::string s;
    const auto printStatus = google::protobuf::util::MessageToJsonString(message, &s);
    if (!printStatus.ok()) {
        std::cerr << "failed to encode config JSON: " << printStatus.ToString() << std::endl;
        return;
    }
    std::cout << s << std::endl;
}

void ConfigAll::decodeProtobuf(const std::string& configFilename) {
    std::ifstream fs(configFilename);
    if (!fs.is_open()) {
        std::cerr << "failed to open config file: " << configFilename << std::endl;
        return;
    }

    std::stringstream strStream;
    strStream << fs.rdbuf(); // read the file

    proto::AllConfig message;
    const auto parseStatus = google::protobuf::util::JsonStringToMessage(strStream.str(), &message);
    if (!parseStatus.ok()) {
        std::cerr << "failed to parse config JSON: " << parseStatus.ToString() << std::endl;
        return;
    }

    if (message.has_ggraphicrendering()) {
        const proto::GraphicRendering& rendering = message.ggraphicrendering();
        decodeGraphicRendering(rendering);
    }

    set(simplificationOfOriginalSVG, message.simplificationoforiginalsvg());
    set(filename, message.filename());
}

} /* namespace laby */
