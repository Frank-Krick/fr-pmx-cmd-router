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
  std::shared_ptr<struct processing::parameter> find_target_parameter(
      processing::MidiMessageProcessor::midi_cc_message message) {

    if (message.cc_number == 0) {
      return processing::none;
    } else if (message.cc_number == 1) {
      return processing::saturator_level_in;
    } else if (message.cc_number == 2) {
      return processing::saturator_drive;
    } else if (message.cc_number == 3) {
      return processing::saturator_blend;
    } else if (message.cc_number == 4) {
      return processing::saturator_level_out;
    } else if (message.cc_number == 5) {
      return processing::compressor_threshold;
    } else if (message.cc_number == 6) {
      return processing::compressor_ratio;
    } else if (message.cc_number == 7) {
      return processing::compressor_attack;
    } else if (message.cc_number == 8) {
      return processing::compressor_release;
    } else if (message.cc_number == 9) {
      return processing::compressor_makeup;
    } else if (message.cc_number == 10) {
      return processing::compressor_knee;
    } else if (message.cc_number == 11) {
      return processing::compressor_mix;
    } else if (message.cc_number == 12) {
      return processing::none;
    } else if (message.cc_number == 13) {
      return processing::equalizer_low;
    } else if (message.cc_number == 14) {
      return processing::equalizer_mid;
    } else if (message.cc_number == 15) {
      return processing::equalizer_high;
    } else if (message.cc_number == 16) {
      return processing::equalizer_master;
    } else if (message.cc_number == 17) {
      return processing::equalizer_low_mid;
    } else if (message.cc_number == 18) {
      return processing::equalizer_mid_high;
    } else {
      return processing::none;
    }
  }

  std::optional<RoutingTableTargetNode>
  find_target_node(processing::MidiMessageProcessor::midi_cc_message &message) {
    if (message.channel < channel_node_mapping.size()) {
      return channel_node_mapping[message.channel];
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

  inline static std::array<std::shared_ptr<processing::parameter>, 19>
      control_number_mapping{
          processing::none,
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
          processing::none,
          processing::equalizer_low,
          processing::equalizer_mid,
          processing::equalizer_high,
          processing::equalizer_master,
          processing::equalizer_low_mid,
          processing::equalizer_mid_high,
      };
};

} // namespace utils
