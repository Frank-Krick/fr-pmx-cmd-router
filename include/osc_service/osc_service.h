#pragma once

#include "oscpp/client.hpp"
#include "processing/midi_message_processor.h"

#include <array>
#include <cstddef>
#include <ranges>

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

    std::array<std::string, N> message_paths;
    auto message_paths_it = message_paths.begin();
    for (auto &&update : updates | std::ranges::views::filter([](auto &e) {
                           return e.parameter != processing::none;
                         })) {
      *message_paths_it = create_message_path(update);
      add_message(packet, update, message_paths_it->c_str());
      message_paths_it++;
    }

    packet.closeBundle();
    return packet.size();
  }

private:
  static std::string create_message_path(
      processing::MidiMessageProcessor::parameter_change_event &update);

  static void
  add_message(OSCPP::Client::Packet &packet,
              processing::MidiMessageProcessor::parameter_change_event &update,
              const char *message_path);
};

} // namespace osc_service
