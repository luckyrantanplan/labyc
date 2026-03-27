/*
 * MessageIO.h
 *
 *  Created on: Aug 1, 2018
 *      Author: florian
 */

#ifndef MESSAGEIO_H_
#define MESSAGEIO_H_

#include <string_view>
#include <vector>

namespace laby {

class MessageIO {
  public:
    static auto parseMessage(const std::vector<std::string_view>& arguments) -> int;
};

} // namespace laby

#endif /* MESSAGEIO_H_ */
