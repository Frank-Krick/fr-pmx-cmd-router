#include <limits>
#include <microtest/microtest.h>
#include <sys/types.h>
#include <utils/midi_routing_table.h>

using namespace utils;

TEST(FindTargetParameterShouldMapMidiControlValues) {
  auto table = MidiRoutingTable();

  auto target = table.find_target_parameter({0b10110001, 1, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Saturator:level_in");

  target = table.find_target_parameter({0b10110101, 2, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Saturator:drive");

  target = table.find_target_parameter({0b10110011, 3, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Saturator:blend");

  target = table.find_target_parameter({0b10110011, 4, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Saturator:level_out");

  target = table.find_target_parameter({0b10110011, 5, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Compressor:threshold");

  target = table.find_target_parameter({0b10110011, 6, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Compressor:ratio");

  target = table.find_target_parameter({0b10110011, 7, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Compressor:attack");

  target = table.find_target_parameter({0b10110011, 8, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Compressor:release");

  target = table.find_target_parameter({0b10110011, 9, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Compressor:makeup");

  target = table.find_target_parameter({0b10110011, 10, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Compressor:knee");

  target = table.find_target_parameter({0b10110011, 11, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Compressor:mix");

  target = table.find_target_parameter({0b10110011, 13, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Equalizer:low");

  target = table.find_target_parameter({0b10110011, 14, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Equalizer:mid");

  target = table.find_target_parameter({0b10110011, 15, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Equalizer:high");

  target = table.find_target_parameter({0b10110011, 16, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Equalizer:master");

  target = table.find_target_parameter({0b10110011, 17, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Equalizer:low_mid");

  target = table.find_target_parameter({0b10110011, 18, 32});
  ASSERT_TRUE(target);
  ASSERT_STREQ(target.value().parameter_name, "Equalizer:mid_high");
}

TEST(FindTargetParameterShouldRejectUnmappedControlNumbers) {
  auto table = MidiRoutingTable();

  ASSERT_FALSE(table.find_target_parameter({0b10111000, 12, 0}));

  for (u_int8_t i = 19; i < std::numeric_limits<u_int8_t>::max(); i++) {
    ASSERT_FALSE(table.find_target_parameter({0b10111000, i, 0}));
  }
}

TEST(FindTargetParameterShouldRejectUnsupportedMessageTypes) {
  auto table = MidiRoutingTable();

  // First byte not a control byte, invalid midi message
  ASSERT_FALSE(table.find_target_parameter({0b00000000, 0, 0}));
  ASSERT_FALSE(table.find_target_parameter({0b00100000, 0, 0}));
  // Polyphonic aftertouch
  ASSERT_FALSE(table.find_target_parameter({0b10100100, 0, 0}));
  // Program change
  ASSERT_FALSE(table.find_target_parameter({0b11000101, 0, 0}));
  // Channel Pressure
  ASSERT_FALSE(table.find_target_parameter({0b11010100, 0, 0}));
  // Pitch Bend
  ASSERT_FALSE(table.find_target_parameter({0b11100100, 0, 0}));
  // Sysex
  ASSERT_FALSE(table.find_target_parameter({0b11110100, 0, 0}));
}

TEST(FindTargetNodeShouldNotHaveAnyNodesAfterConstruction) {
  auto table = MidiRoutingTable();
  for (u_int8_t i = 1; i < 17; i++) {
    u_int8_t control_byte = i | 0b10110000;
    ASSERT_FALSE(table.find_target_node({control_byte, 0, 0}));
  }
}

TEST(FindTargetNodeShouldReturnTheNodeForTheChannelIfRegistered) {
  auto table = MidiRoutingTable();

  table.register_target_node(4, 466);

  auto target_node = table.find_target_node({0b11010100, 0, 0});
  ASSERT_TRUE(target_node);
  ASSERT_EQ(target_node.value().id, 466);
}

TEST(
    FindTargetNodeShouldReturnAnEmptyOptionIfTheNodeWasRegisteredAndThenCleared) {
  auto table = MidiRoutingTable();

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
