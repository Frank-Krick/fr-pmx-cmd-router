#pragma once

#include "processing/parameters.h"

#include <cstdint>

namespace utils {

class Interpolator {
public:
  static double interpolate(processing::parameter &parameter,
                            uint8_t control_value) {
    double normalized_value = (double)control_value / 127.0;
    return parameter.min + (parameter.max - parameter.min) * normalized_value;
  }
};

} // namespace utils
