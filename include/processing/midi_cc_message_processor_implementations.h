#pragma once

#include "processing/midi_message_processor.h"
#include "utils/interpolator.h"
#include "utils/midi_routing_table.h"
#include "utils/node_registry.h"

#include <optional>

namespace processing {

namespace implementation {

class InputChannelMidiMessageProcessor {
public:
  InputChannelMidiMessageProcessor(
      utils::InputChannelsMidiRoutingTable &routing_table,
      utils::NodeRegistry &node_registry)
      : routing_table(routing_table), node_registry(node_registry) {}

  std::optional<MidiMessageProcessor::parameter_change_event>
  operator()(MidiMessageProcessor::midi_cc_message message) {
    auto target_parameter = routing_table.find_target_parameter(message);
    auto target_node = routing_table.find_target_node(message);

    if (target_parameter && target_node) {
      auto node = node_registry.get_node_by_id(target_node->id);

      if (node) {
        auto target_value = utils::Interpolator::interpolate(
            target_parameter.value(), message.value);
      }
    }

    return {};
  }

private:
  utils::InputChannelsMidiRoutingTable &routing_table;
  utils::NodeRegistry &node_registry;
};

} // namespace implementation

} // namespace processing
