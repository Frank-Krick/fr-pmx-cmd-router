#include "processing/midi_message_processor.h"
#include "processing/parameters.h"
#include <limits>
#include <microtest/microtest.h>
#include <sys/types.h>
#include <utils/midi_routing_table.h>

using namespace utils;

TEST(FindTargetParameterShouldMapMidiControlValues) {
  ASSERT_STREQ(processing::saturator_level_in->full_name, "Saturator:level_in");

  auto table = InputChannelsMidiRoutingTable();
  auto target = table.find_target_parameter(
      processing::MidiMessageProcessor::midi_cc_message{1, 1, 1});

  ASSERT_STREQ("Saturator:level_in", target->full_name);

  target = table.find_target_parameter({1, 2, 32});
  ASSERT_STREQ(target->full_name, "Saturator:drive");

  target = table.find_target_parameter({1, 3, 32});
  ASSERT_STREQ(target->full_name, "Saturator:blend");

  target = table.find_target_parameter({1, 4, 32});
  ASSERT_STREQ(target->full_name, "Saturator:level_out");

  target = table.find_target_parameter({1, 5, 32});
  ASSERT_STREQ(target->full_name, "Compressor:threshold");

  target = table.find_target_parameter({1, 6, 32});
  ASSERT_STREQ(target->full_name, "Compressor:ratio");

  target = table.find_target_parameter({1, 7, 32});
  ASSERT_STREQ(target->full_name, "Compressor:attack");

  target = table.find_target_parameter({1, 8, 32});
  ASSERT_STREQ(target->full_name, "Compressor:release");

  target = table.find_target_parameter({1, 9, 32});
  ASSERT_STREQ(target->full_name, "Compressor:makeup");

  target = table.find_target_parameter({1, 10, 32});
  ASSERT_STREQ(target->full_name, "Compressor:knee");

  target = table.find_target_parameter({1, 11, 32});
  ASSERT_STREQ(target->full_name, "Compressor:mix");

  target = table.find_target_parameter({1, 13, 32});
  ASSERT_STREQ(target->full_name, "Equalizer:low");

  target = table.find_target_parameter({1, 14, 32});
  ASSERT_STREQ(target->full_name, "Equalizer:mid");

  target = table.find_target_parameter({1, 15, 32});
  ASSERT_STREQ(target->full_name, "Equalizer:high");

  target = table.find_target_parameter({1, 16, 32});
  ASSERT_STREQ(target->full_name, "Equalizer:master");

  target = table.find_target_parameter({1, 17, 32});
  ASSERT_STREQ(target->full_name, "Equalizer:low_mid");

  target = table.find_target_parameter({1, 18, 32});
  ASSERT_STREQ(target->full_name, "Equalizer:mid_high");
}

TEST(FindTargetParameterShouldRejectUnmappedControlNumbers) {
  auto table = InputChannelsMidiRoutingTable();

  ASSERT_EQ(table.find_target_parameter({1, 12, 0}), processing::none);

  for (u_int8_t i = 19; i < std::numeric_limits<u_int8_t>::max(); i++) {
    ASSERT_EQ(table.find_target_parameter({1, i, 0}), processing::none);
  }
}

TEST(FindTargetNodeShouldNotHaveAnyNodesAfterConstruction) {
  auto table = InputChannelsMidiRoutingTable();
  for (u_int8_t i = 1; i < 17; i++) {
    u_int8_t control_byte = i | 0b10110000;
    ASSERT_FALSE(table.find_target_node({control_byte, 0, 0}));
  }
}

TEST(FindTargetNodeShouldReturnTheNodeForTheChannelIfRegistered) {
  auto table = InputChannelsMidiRoutingTable();

  table.register_target_node(4, 466);

  auto target_node = table.find_target_node({0b11010100, 0, 0});
  ASSERT_TRUE(target_node);
  ASSERT_EQ(target_node.value().id, 466);
}

TEST(
    FindTargetNodeShouldReturnAnEmptyOptionIfTheNodeWasRegisteredAndThenCleared) {
  auto table = InputChannelsMidiRoutingTable();

  table.register_target_node(4, 466);
  table.register_target_node(7, 466);
  table.register_target_node(9, 466);

  ASSERT_TRUE(table.find_target_node({0b11010100, 0, 0}));
  ASSERT_TRUE(table.find_target_node({0b11010111, 0, 0}));
  ASSERT_TRUE(table.find_target_node({0b11011001, 0, 0}));

  table.clear_target_node_by_id(466);

  ASSERT_FALSE(table.find_target_node({0b11010100, 0, 0}));
  ASSERT_FALSE(table.find_target_node({0b11010111, 0, 0}));
  ASSERT_FALSE(table.find_target_node({0b11011001, 0, 0}));
}

TEST_MAIN();
