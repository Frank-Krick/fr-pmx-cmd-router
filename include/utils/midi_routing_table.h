#pragma once

#include "processing/midi_message_processor.h"
#include "processing/parameters.h"

#include <array>
#include <optional>
#include <sys/types.h>

namespace utils {

struct RoutingTableTargetNode {
  unsigned int id;
};

class InputChannelsMidiRoutingTable {
public:
  std::optional<struct processing::parameter> find_target_parameter(
      processing::MidiMessageProcessor::midi_cc_message &message) {
    if (message.cc_number <
        InputChannelsMidiRoutingTable::control_number_mapping.size()) {
      return control_number_mapping[message.cc_number];
    }

    return {};
  }

  std::optional<RoutingTableTargetNode>
  find_target_node(processing::MidiMessageProcessor::midi_cc_message &message) {
    if (message.channel < channel_node_mapping.size()) {
      return channel_node_mapping[message.channel];
    }

    return {};
  }

  std::optional<struct processing::parameter>
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

  inline static std::array<std::optional<processing::parameter>, 19>
      control_number_mapping{
          std::nullopt,
          processing::saturator_level_in,
          processing::saturator_drive,
          processing::saturator_blend,
          processing::saturator_level_out,
          processing::compressor_threshold,
          processing::compressor_ratio,
          processing::compressor_attack,
          processing::compressor_release,
          processing::compressor_makeup,
          processing::compressor_knee,
          processing::compressor_mix,
          std::nullopt,
          processing::equalizer_low,
          processing::equalizer_mid,
          processing::equalizer_high,
          processing::equalizer_master,
          processing::equalizer_low_mid,
          processing::equalizer_mid_high,
      };
};

} // namespace utils
