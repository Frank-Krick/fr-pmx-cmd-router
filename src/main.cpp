#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <math.h>
#include <ostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

#include <pipewire/context.h>
#include <pipewire/core.h>
#include <pipewire/filter.h>
#include <pipewire/keys.h>
#include <pipewire/loop.h>
#include <pipewire/main-loop.h>
#include <pipewire/pipewire.h>
#include <pipewire/properties.h>

#include <spa/control/control.h>
#include <spa/debug/pod.h>
#include <spa/pod/builder.h>
#include <spa/pod/iter.h>
#include <spa/pod/parser.h>
#include <spa/pod/pod.h>
#include <spa/utils/dict.h>

#include <sys/types.h>

#include "processing/group_channels_midi_processor.h"
#include "utils/midi_routing_table.h"
#include "utils/node_registry.h"

#include "processing/port.h"
#include "processing/process_group_channels_port.h"
#include "processing/process_input_channels_port.h"

#define PERIOD_NSEC (SPA_NSEC_PER_SEC / 8)

struct data {
  struct pw_main_loop *loop;
  struct pw_filter *filter;
  struct pw_registry *registry;
  struct processing::port *input_channels_port;
  struct processing::port *group_channels_port;
  uint32_t clock_id;
  int64_t offset;
  uint64_t position;
  utils::InputChannelsMidiRoutingTable routing_table;
  processing::GroupChannelsMidiRoutingTable group_channels_routing_table;
  utils::NodeRegistry node_registry;
  std::array<unsigned int, 4> group_to_node_id_mapping;
};

static void on_process(void *user_data, struct spa_io_position *position) {
  struct data *my_data = static_cast<struct data *>(user_data);

  processing::ProcessInputChannelsPort::on_process(
      my_data->input_channels_port, my_data->routing_table,
      my_data->node_registry, my_data->loop);

  processing::ProcessGroupChannelsPort::on_process(
      my_data->group_channels_port, my_data->group_channels_routing_table,
      my_data->node_registry, my_data->loop, my_data->group_to_node_id_mapping);
}

static void handle_filter_param_update(void *data, void *port_data, uint32_t id,
                                       const struct spa_pod *pod) {
  struct spa_pod_prop *prop;
  struct spa_pod_object *obj = (struct spa_pod_object *)pod;
  SPA_POD_OBJECT_FOREACH(obj, prop) {
    struct spa_pod_parser parser;
    spa_pod_parser_pod(&parser, &prop->value);

    char *c_key = nullptr;
    char *c_value = nullptr;
    spa_pod_parser_get_struct(&parser, SPA_POD_String(&c_key),
                              SPA_POD_String(&c_value));

    if (c_key == nullptr && c_value == nullptr) {
      return;
    }
    struct data *my_data = reinterpret_cast<struct data *>(data);

    std::string key(c_key);
    std::string value(c_value);
    if (key == "pmx.channel_mapping") {
      std::string assignment;
      std::istringstream string_stream(value);
      int channel;
      std::vector<std::pair<int, int>> assignments;
      while (std::getline(string_stream, assignment, ',')) {
        if (assignment.starts_with("[")) {
          assignment[0] = ' ';
          channel = std::stoi(assignment);
        } else {
          assignments.push_back(std::make_pair(channel, stoi(assignment)));
        }
      }

      for (u_int8_t i = 0; i < 17; i++) {
        auto assignment = std::find_if(
            assignments.begin(), assignments.end(),
            [i](const auto &assignment) { return assignment.first == i; });

        if (assignment == assignments.end()) {
          my_data->routing_table.clear_target_node_by_channel(i);
        } else {
          my_data->routing_table.register_target_node(i, assignment->second);
        }
      }

      struct spa_dict_item items[1];
      items[0] = SPA_DICT_ITEM_INIT("pmx.channel_mapping", value.c_str());
      struct spa_dict update_dict = SPA_DICT_INIT(items, 1);
      pw_filter_update_properties(my_data->filter, nullptr, &update_dict);
    } else if (key == "pmx.group_channel_node_ids") {
      std::string assignment;
      std::istringstream string_stream(value);
      std::vector<unsigned int> node_ids;
      while (std::getline(string_stream, assignment, ',')) {
        node_ids.push_back(std::stoi(assignment));
      }

      if (node_ids.size() == 4) {

        my_data->group_to_node_id_mapping = {node_ids[0], node_ids[1],
                                             node_ids[2], node_ids[3]};

        struct spa_dict_item items[1];
        items[0] =
            SPA_DICT_ITEM_INIT("pmx.group_channel_node_ids", value.c_str());
        struct spa_dict update_dict = SPA_DICT_INIT(items, 1);
        pw_filter_update_properties(my_data->filter, nullptr, &update_dict);
      }
    }
  }
}

static const pw_filter_events filter_events = {
    .version = PW_VERSION_FILTER_EVENTS,
    .param_changed = handle_filter_param_update,
    .process = on_process,
};

static void registry_event_global(void *data, uint32_t id, uint32_t permissions,
                                  const char *c_type, uint32_t version,
                                  const struct spa_dict *props) {
  struct data *my_data = static_cast<struct data *>(data);

  std::string type(c_type);
  if (type == "PipeWire:Interface:Node") {
    auto client = static_cast<pw_client *>(
        pw_registry_bind(my_data->registry, id, c_type, PW_VERSION_CLIENT, 0));
    my_data->node_registry.add_node(id, client);
    std::cout << "Registry event global" << std::endl
              << "type: " << type << std::endl;
  }
}

static void registry_event_global_remove(void *data, uint32_t id) {
  struct data *my_data = static_cast<struct data *>(data);

  my_data->node_registry.delete_node_by_id(id);
}

static const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global,
    .global_remove = registry_event_global_remove,
};

static void do_quit(void *userdata, int signal_number) {
  data *data = static_cast<struct data *>(userdata);
  pw_main_loop_quit(data->loop);
}

int main(int argc, char *argv[]) {
  struct data data = {};

  pw_init(&argc, &argv);

  data.loop = pw_main_loop_new(NULL);

  pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
  pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

  auto context = pw_context_new(pw_main_loop_get_loop(data.loop), nullptr, 0);
  auto core = pw_context_connect(context, nullptr, 0);
  data.registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
  struct spa_hook registry_listener;
  spa_zero(registry_listener);
  pw_registry_add_listener(data.registry, &registry_listener, &registry_events,
                           &data);

  data.filter = pw_filter_new_simple(
      pw_main_loop_get_loop(data.loop), "fr-pmx-cmd-router",
      pw_properties_new(PW_KEY_MEDIA_TYPE, "Midi", PW_KEY_MEDIA_CATEGORY,
                        "Filter", PW_KEY_MEDIA_CLASS, "Midi/Sink",
                        PW_KEY_MEDIA_ROLE, "DSP", "pmx.channel_mapping", "",
                        NULL),
      &filter_events, &data);

  data.input_channels_port =
      static_cast<struct processing::port *>(pw_filter_add_port(
          data.filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS,
          sizeof(struct processing::port),
          pw_properties_new(PW_KEY_FORMAT_DSP, "8 bit raw midi",
                            PW_KEY_PORT_NAME, "input_channels_port", NULL),
          NULL, 0));

  data.group_channels_port =
      static_cast<struct processing::port *>(pw_filter_add_port(
          data.filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS,
          sizeof(struct processing::port),
          pw_properties_new(PW_KEY_FORMAT_DSP, "8 bit raw midi",
                            PW_KEY_PORT_NAME, "group_channels_port", NULL),
          NULL, 0));

  if (pw_filter_connect(data.filter, PW_FILTER_FLAG_RT_PROCESS, NULL, 0) < 0) {
    fprintf(stderr, "can't connect\n");
    return -1;
  }

  pw_main_loop_run(data.loop);
  pw_proxy_destroy((struct pw_proxy *)data.registry);
  pw_core_disconnect(core);
  pw_filter_destroy(data.filter);
  pw_filter_destroy(data.filter);
  pw_main_loop_destroy(data.loop);
  pw_deinit();

  return 0;
}
