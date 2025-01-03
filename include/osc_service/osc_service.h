#pragma once

#include "oscpp/client.hpp"
#include "processing/midi_message_processor.h"

#include <array>
#include <cstddef>

namespace osc_service {

class OscService {
public:
  template <size_t N>
  static size_t build_message(
      std::array<processing::MidiMessageProcessor::parameter_change_event, N>
          &updates,
      void *buffer, size_t max_size) {
    OSCPP::Client::Packet packet(buffer, max_size);
    packet.openBundle(0L);

    for (auto &&update : updates) {
      add_message(packet, update);
    }

    packet.closeBundle();
    return packet.size();
  }

private:
  static void
  add_message(OSCPP::Client::Packet &packet,
              processing::MidiMessageProcessor::parameter_change_event &update);
};

} // namespace osc_service
