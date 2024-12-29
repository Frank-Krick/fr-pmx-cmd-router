#pragma once

#include "processing/parameters.h"

#include <array>
#include <functional>

#include <sys/types.h>

namespace processing {

typedef std::function<double(struct parameter &, u_int8_t control_value)>
    interpolator;

struct parameter_change_event {
  parameter &parameter = none;
  u_int8_t control_value;
  double value;
};

// define data storage for events with ranges with ranges
// https://hannes.hauswedell.net/post/2019/11/30/range_intro/
//
class MidiMessageProcessor {};

} // namespace processing
