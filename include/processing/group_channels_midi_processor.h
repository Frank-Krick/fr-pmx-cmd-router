#pragma once

#include <optional>
#include <string>
#include <sys/types.h>
#include <unordered_map>

namespace processing {

class GroupChannelsMidiRoutingTable {
public:
  struct RoutingTableTargetParameter {
    u_int8_t group_id;
    std::string parameter_name;
    double min;
    double max;
  };

  std::optional<RoutingTableTargetParameter>
  get_target_by_cc_number(u_int8_t cc_number) {
    if (control_number_mapping.contains(cc_number)) {
      return control_number_mapping[cc_number];
    }

    return {};
  }

private:
  inline static std::unordered_map<u_int32_t, RoutingTableTargetParameter>
      control_number_mapping{
          {10, {1, "Equalizer:high", -24.0, 24.0}},
          {11, {2, "Equalizer:high", -24.0, 24.0}},
          {12, {3, "Equalizer:high", -24.0, 24.0}},
          {13, {4, "Equalizer:high", -24.0, 24.0}},
          {14, {1, "Equalizer:mid", -24.0, 24.0}},
          {15, {2, "Equalizer:mid", -24.0, 24.0}},
          {16, {3, "Equalizer:mid", -24.0, 24.0}},
          {17, {4, "Equalizer:mid", -24.0, 24.0}},
          {18, {1, "Equalizer:low", -24.0, 24.0}},
          {19, {2, "Equalizer:low", -24.0, 24.0}},
          {20, {3, "Equalizer:low", -24.0, 24.0}},
          {21, {4, "Equalizer:low", -24.0, 24.0}},
          {48, {1, "Equalizer:master", -24.0, 24.0}},
          {49, {2, "Equalizer:master", -24.0, 24.0}},
          {50, {3, "Equalizer:master", -24.0, 24.0}},
          {51, {4, "Equalizer:master", -24.0, 24.0}},
      };
};

} // namespace processing
