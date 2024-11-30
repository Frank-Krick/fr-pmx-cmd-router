#pragma once

#include <array>
#include <optional>
#include <string>
#include <sys/types.h>

namespace utils {

struct RoutingTableTargetParameter {
  std::string parameter_name;
};

struct RoutingTableTargetNode {
  unsigned int id;
};

class MidiRoutingTable {
public:
  std::optional<RoutingTableTargetParameter>
  find_target_parameter(std::array<u_int8_t, 3> midi_message) {
    if ((midi_message[0] & 0b11110000) == 0b10110000) {
      if (midi_message[1] < MidiRoutingTable::control_number_mapping.size()) {
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
          RoutingTableTargetParameter{"Saturator:level_in"},
          RoutingTableTargetParameter{"Saturator:drive"},
          RoutingTableTargetParameter{"Saturator:blend"},
          RoutingTableTargetParameter{"Saturator:level_out"},
          RoutingTableTargetParameter{"Compressor:threshold"},
          RoutingTableTargetParameter{"Compressor:ratio"},
          RoutingTableTargetParameter{"Compressor:attack"},
          RoutingTableTargetParameter{"Compressor:release"},
          RoutingTableTargetParameter{"Compressor:makeup"},
          RoutingTableTargetParameter{"Compressor:knee"},
          RoutingTableTargetParameter{"Compressor:mix"},
          std::nullopt,
          RoutingTableTargetParameter{"Equalizer:low"},
          RoutingTableTargetParameter{"Equalizer:mid"},
          RoutingTableTargetParameter{"Equalizer:high"},
          RoutingTableTargetParameter{"Equalizer:master"},
          RoutingTableTargetParameter{"Equalizer:low_mid"},
          RoutingTableTargetParameter{"Equalizer:mid_high"},
      };
};

} // namespace utils
