#pragma once

#include <string>

namespace processing {
enum channel_type { INPUT = 1, GROUP };
enum device_type { SATURATOR = 1, COMPRESSOR, EQUALIZER };

struct parameter_update_event {
  channel_type channel_type;
  device_type device_type;
  std::string parameter_name;
  double value;
};

} // namespace processing
