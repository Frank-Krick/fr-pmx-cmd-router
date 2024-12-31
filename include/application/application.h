#pragma once

#include "pipewire/filter.h"
#include "processing/group_channels_midi_processor.h"
#include "processing/midi_cc_message_processor_implementations.h"
#include "processing/midi_message_processor.h"
#include "utils/midi_routing_table.h"
#include "utils/node_registry.h"

#include <pipewire/context.h>
#include <pipewire/main-loop.h>

namespace application {

class Application {
public:
  Application(int argc, char *argv[]);

  static void registry_event_global(void *data, uint32_t id,
                                    uint32_t permissions, const char *c_type,
                                    uint32_t version,
                                    const struct spa_dict *props);

  static void registry_event_global_remove(void *data, uint32_t id);

  static void on_process(void *user_data, struct spa_io_position *position);

  static void handle_filter_param_update(void *data, void *port_data,
                                         uint32_t id,
                                         const struct spa_pod *pod);

  static void do_quit(void *user_data, int signal_number);

  void shutdown();

  int run();

private:
  struct pw_main_loop *loop;
  struct pw_context *context;
  struct pw_core *core;
  struct pw_registry *registry;
  struct pw_filter *filter;
  struct spa_hook registry_listener;
  utils::NodeRegistry node_registry;
  utils::InputChannelsMidiRoutingTable input_channels_midi_routing_table;
  processing::GroupChannelsMidiRoutingTable group_channels_midi_routing_table;
  std::array<unsigned int, 4> group_to_node_id_mapping;
  struct processing::port *input_channels_port;
  struct processing::port *group_channels_port;
  processing::MidiMessageProcessor midi_message_processor;
  std::optional<processing::InputChannelPortProcessor>
      input_channel_port_processor;
  std::optional<processing::GroupChannelsPortProcessor>
      group_channel_port_processor;
};

} // namespace application
