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
    std::shared_ptr<struct parameter> parameter = none;
  };

  std::optional<RoutingTableTargetParameter>
  get_target_by_cc_number(u_int8_t cc_number) {
    if (cc_number >= 10 && cc_number < 14) {
      return RoutingTableTargetParameter{static_cast<u_int8_t>(cc_number - 9),
                                         equalizer_high};
    } else if (cc_number >= 14 && cc_number < 18) {
      return RoutingTableTargetParameter{static_cast<u_int8_t>(cc_number - 13),
                                         equalizer_mid};
    } else if (cc_number >= 18 && cc_number < 22) {
      return RoutingTableTargetParameter{static_cast<u_int8_t>(cc_number - 17),
                                         equalizer_low};
    } else if (cc_number >= 48 && cc_number < 52) {
      return RoutingTableTargetParameter{static_cast<u_int8_t>(cc_number - 47),
                                         equalizer_master};
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
