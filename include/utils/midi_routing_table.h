#pragma once

#include <array>
#include <optional>
#include <string>
#include <sys/types.h>

namespace utils {

struct RoutingTableTargetParameter {
  std::string parameter_name;
  double min;
  double max;
};

struct RoutingTableTargetNode {
  unsigned int id;
};

class InputChannelsMidiRoutingTable {
public:
  std::optional<RoutingTableTargetParameter>
  find_target_parameter(std::array<u_int8_t, 3> midi_message) {
    if ((midi_message[0] & 0b11110000) == 0b10110000) {
      if (midi_message[1] <
          InputChannelsMidiRoutingTable::control_number_mapping.size()) {
        return control_number_mapping[midi_message[1]];
      }
    }
    return {};
  }

  std::optional<RoutingTableTargetNode>
  find_target_node(std::array<u_int8_t, 3> midi_message) {
    return channel_node_mapping[midi_message[0] & 0b00001111];
  }

  void register_target_node(u_int8_t channel_number, unsigned int id) {
    channel_node_mapping[channel_number] = RoutingTableTargetNode{id};
  }

  void clear_target_node_by_id(unsigned int id) {
    for (u_int8_t i = 0; i < channel_node_mapping.size(); i++) {
      if (channel_node_mapping[i].has_value() &&
          channel_node_mapping[i].value().id == id) {
        channel_node_mapping[i] = std::nullopt;
      }
    }
  }

  void clear_target_node_by_channel(u_int8_t channel) {
    if (channel < 17) {
      channel_node_mapping[channel] = std::nullopt;
    }
  }

private:
  inline static std::array<std::optional<RoutingTableTargetNode>, 17>
      channel_node_mapping{};

  inline static std::array<std::optional<RoutingTableTargetParameter>, 19>
      control_number_mapping{
          std::nullopt,
          RoutingTableTargetParameter{"Saturator:level_in", 0.015625, 64.0},
          RoutingTableTargetParameter{"Saturator:drive", 0.1, 10.0},
          RoutingTableTargetParameter{"Saturator:blend", -10, 10},
          RoutingTableTargetParameter{"Saturator:level_out", 0.015625, 64.0},
          RoutingTableTargetParameter{"Compressor:threshold", 0.000977, 1.0},
          RoutingTableTargetParameter{"Compressor:ratio", 1.0, 20.0},
          RoutingTableTargetParameter{"Compressor:attack", 0.01, 2000.0},
          RoutingTableTargetParameter{"Compressor:release", 0.01, 2000.0},
          RoutingTableTargetParameter{"Compressor:makeup", 1.0, 64.0},
          RoutingTableTargetParameter{"Compressor:knee", 1.0, 8.0},
          RoutingTableTargetParameter{"Compressor:mix", 0.0, 1.0},
          std::nullopt,
          RoutingTableTargetParameter{"Equalizer:low", -24, 24},
          RoutingTableTargetParameter{"Equalizer:mid", -24, 24},
          RoutingTableTargetParameter{"Equalizer:high", -24, 24},
          RoutingTableTargetParameter{"Equalizer:master", -24, 24},
          RoutingTableTargetParameter{"Equalizer:low_mid", 0.0, 1000.0},
          RoutingTableTargetParameter{"Equalizer:mid_high", 1000.0, 20000.0},
      };
};

} // namespace utils
