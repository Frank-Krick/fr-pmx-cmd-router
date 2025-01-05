#include "osc_service/osc_service.h"
#include "processing/midi_message_processor.h"
#include "processing/parameters.h"

#include <iostream>
#include <oscpp/client.hpp>
#include <sstream>

constexpr void add_channel_number(
    std::ostringstream &os,
    processing::MidiMessageProcessor::parameter_change_event &update) {
  if (update.layer == processing::MidiMessageProcessor::A) {
    os << "A/";
  } else if (update.layer == processing::MidiMessageProcessor::B) {
    os << "B/";
  }
}

constexpr void add_channel_type(
    std::ostringstream &os,
    processing::MidiMessageProcessor::parameter_change_event &update) {
  if (update.channel_type == processing::MidiMessageProcessor::INPUT) {
    os << "/I/";
  } else if (update.channel_type == processing::MidiMessageProcessor::GROUP) {
    os << "/G/";
  } else if (update.channel_type == processing::MidiMessageProcessor::LAYER) {
    os << "/L/";
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

constexpr void add_parameter(std::ostringstream &os,
                             processing::parameter &parameter) {
  os << "/" << parameter.name;
}

std::string osc_service::OscService::create_message_path(
    processing::MidiMessageProcessor::parameter_change_event &update) {
  std::ostringstream os;

  add_channel_type(os, update);
  add_channel_number(os, update);
  add_device(os, update);
  add_parameter(os, *update.parameter);

  return os.str();
}

void osc_service::OscService::add_message(
    OSCPP::Client::Packet &packet,
    processing::MidiMessageProcessor::parameter_change_event &update,
    const char *message_path) {
  packet.openMessage(message_path, 1).float32(update.value).closeMessage();
}
