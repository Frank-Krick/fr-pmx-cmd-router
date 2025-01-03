#pragma once

#include "processing/group_channels_midi_processor.h"
#include "processing/midi_message_processor.h"
#include "processing/parameters.h"
#include "utils/interpolator.h"
#include "utils/midi_routing_table.h"
#include "utils/node_registry.h"

#include <iostream>
#include <optional>

namespace processing {

class InputChannelPortProcessor {
public:
  InputChannelPortProcessor(const InputChannelPortProcessor &) = default;
  InputChannelPortProcessor(InputChannelPortProcessor &&) = default;
  InputChannelPortProcessor &

  operator=(const InputChannelPortProcessor &other) {
    this->node_registry = other.node_registry;
    this->routing_table = other.routing_table;
    return *this;
  };

  InputChannelPortProcessor &operator=(InputChannelPortProcessor &&other) {
    this->node_registry = other.node_registry;
    this->routing_table = other.routing_table;
    return *this;
  };

  InputChannelPortProcessor(utils::InputChannelsMidiRoutingTable &routing_table,
                            utils::NodeRegistry &node_registry)
      : routing_table(routing_table), node_registry(node_registry) {}

  std::optional<MidiMessageProcessor::parameter_change_event>
  operator()(MidiMessageProcessor::midi_cc_message message) {
    auto target_parameter = routing_table.find_target_parameter(message);

    if (target_parameter == processing::none) {
      std::cout << "couldn't find target parameter" << std::endl;
      return {};
    }

    auto target_node = routing_table.find_target_node(message);
    if (!target_node) {
      std::cout << "couldn't find target node" << std::endl;
      return {};
    }

    auto node = node_registry.get_node_by_id(target_node->id);

    if (!node) {
      std::cout << "couldn't find node" << std::endl;
      return {};
    }

    auto target_value =
        utils::Interpolator::interpolate(*target_parameter, message.value);

    return MidiMessageProcessor::parameter_change_event(
        target_parameter, node.value(), message.value, target_value,
        MidiMessageProcessor::A, message.channel, MidiMessageProcessor::INPUT);
  }

private:
  utils::InputChannelsMidiRoutingTable &routing_table;
  utils::NodeRegistry &node_registry;
};

class GroupChannelsPortProcessor {
public:
  GroupChannelsPortProcessor(const GroupChannelsPortProcessor &) = default;
  GroupChannelsPortProcessor(GroupChannelsPortProcessor &&) = default;
  GroupChannelsPortProcessor &

  operator=(const GroupChannelsPortProcessor &other) {
    this->group_to_node_id_mapping = other.group_to_node_id_mapping;
    this->node_registry = other.node_registry;
    this->routing_table = other.routing_table;
    return *this;
  };

  GroupChannelsPortProcessor &operator=(GroupChannelsPortProcessor &&other) {
    this->group_to_node_id_mapping = other.group_to_node_id_mapping;
    this->node_registry = other.node_registry;
    this->routing_table = other.routing_table;
    return *this;
  };

  GroupChannelsPortProcessor(
      processing::GroupChannelsMidiRoutingTable &routing_table,
      utils::NodeRegistry &node_registry,
      std::array<unsigned int, 4> &group_to_node_id_mapping)
      : routing_table(routing_table), node_registry(node_registry),
        group_to_node_id_mapping(group_to_node_id_mapping) {}

  std::optional<MidiMessageProcessor::parameter_change_event>
  operator()(MidiMessageProcessor::midi_cc_message message) {
    auto target_parameter =
        routing_table.get_target_by_cc_number(message.cc_number);

    if (!target_parameter) {
      std::cout << "no target param found" << std::endl;
      return {};
    }

    auto target_id =
        group_to_node_id_mapping[target_parameter.value().group_id - 1];
    auto node = node_registry.get_node_by_id(target_id);

    if (!node) {
      std::cout << "no node found" << std::endl;
      return {};
    }

    auto target_value = utils::Interpolator::interpolate(
        *target_parameter.value().parameter, message.value);

    return MidiMessageProcessor::parameter_change_event(
        target_parameter.value().parameter, node.value(), message.value,
        target_value, MidiMessageProcessor::A,
        target_parameter.value().group_id, MidiMessageProcessor::GROUP);
  }

private:
  processing::GroupChannelsMidiRoutingTable &routing_table;
  utils::NodeRegistry &node_registry;
  std::array<unsigned int, 4> &group_to_node_id_mapping;
};

} // namespace processing
