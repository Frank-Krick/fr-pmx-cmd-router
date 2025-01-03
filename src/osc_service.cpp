#include "osc_service/osc_service.h"
#include "processing/midi_message_processor.h"

#include <oscpp/client.hpp>
#include <sstream>

constexpr void add_channel_number(
    std::ostringstream &os,
    processing::MidiMessageProcessor::parameter_change_event &update) {
  if (update.layer == processing::MidiMessageProcessor::A) {
    os << 'A';
  } else if (update.layer == processing::MidiMessageProcessor::B) {
    os << 'B';
  }

  if (update.channel_type == processing::MidiMessageProcessor::INPUT) {
    os << update.channel_number << '/';
  } else if (update.channel_type == processing::MidiMessageProcessor::GROUP) {
    os << update.channel_number << '/';
  } else if (update.channel_type == processing::MidiMessageProcessor::LAYER) {
    os << '/';
  }
}

constexpr void add_channel_type(
    std::ostringstream &os,
    processing::MidiMessageProcessor::parameter_change_event &update) {
  os << '/';
  if (update.channel_type == processing::MidiMessageProcessor::INPUT) {
    os << "I/";
  } else if (update.channel_type == processing::MidiMessageProcessor::GROUP) {
    os << "G/";
  } else if (update.channel_type == processing::MidiMessageProcessor::LAYER) {
    os << "L/";
  }
}

constexpr void
add_device(std::ostringstream &os,
           processing::MidiMessageProcessor::parameter_change_event &update) {
  if (update.parameter->device_name == "Saturator") {
    os << "S";
  } else if (update.parameter->device_name == "Compressor") {
    os << "C";
  } else if (update.parameter->device_name == "Equalizer") {
    os << "E";
  }
}

constexpr std::string create_message_path(
    processing::MidiMessageProcessor::parameter_change_event &update) {
  std::ostringstream os;

  add_channel_type(os, update);
  add_channel_number(os, update);
  add_device(os, update);

  return os.str();
}

void osc_service::OscService::add_message(
    OSCPP::Client::Packet &packet,
    processing::MidiMessageProcessor::parameter_change_event &update) {
  packet.openMessage(create_message_path(update).c_str(), 1)
      .float32(update.value)
      .closeMessage();
}
