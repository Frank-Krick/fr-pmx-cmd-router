#pragma once

#include "processing/parameters.h"

#include <optional>
#include <unordered_map>

#include <sys/types.h>

namespace processing {

class GroupChannelsMidiRoutingTable {
public:
  struct RoutingTableTargetParameter {
    u_int8_t group_id;
    struct parameter &parameter = none;
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
          {10, {1, equalizer_high}},   {11, {2, equalizer_high}},
          {12, {3, equalizer_high}},   {13, {4, equalizer_high}},
          {14, {1, equalizer_mid}},    {15, {2, equalizer_mid}},
          {16, {3, equalizer_mid}},    {17, {4, equalizer_mid}},
          {18, {1, equalizer_low}},    {19, {2, equalizer_low}},
          {20, {3, equalizer_low}},    {21, {4, equalizer_low}},
          {48, {1, equalizer_master}}, {49, {2, equalizer_master}},
          {50, {3, equalizer_master}}, {51, {4, equalizer_master}},
      };
};

} // namespace processing
