#include "pipewire/context.h"
#include "pipewire/core.h"
#include "pipewire/keys.h"
#include "pipewire/loop.h"
#include "pipewire/main-loop.h"
#include "pipewire/node.h"
#include "pipewire/properties.h"
#include "pipewire/stream.h"
#include "spa/buffer/buffer.h"
#include "spa/param/param.h"
#include "spa/pod/iter.h"
#include "spa/pod/pod.h"
#include "spa/support/loop.h"
#include "spa/utils/dict.h"
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

#include <pipewire/filter.h>
#include <pipewire/pipewire.h>

#include <spa/control/control.h>
#include <spa/debug/pod.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <sys/types.h>

#include <spa/debug/pod.h>

#include "utils/midi_routing_table.h"
#include "utils/node_registry.h"
#include "utils/pod_message_builder.h"

#define PERIOD_NSEC (SPA_NSEC_PER_SEC / 8)

struct port {};

struct data {
  struct pw_main_loop *loop;
  struct pw_filter *filter;
  struct pw_registry *registry;
  struct port *port;
  uint32_t clock_id;
  int64_t offset;
  uint64_t position;
  utils::MidiRoutingTable routing_table;
  utils::NodeRegistry node_registry;
};

struct pw_invoke_set_param_data {
  unsigned int target_node_id;
  pw_client *target_node;
  double value;
  char parameter_name[2048];
};

static void on_process(void *userdata, struct spa_io_position *position) {
  struct data *my_data = static_cast<struct data *>(userdata);
  struct pw_buffer *pw_buffer;

  if ((pw_buffer = pw_filter_dequeue_buffer(my_data->port)) == nullptr) {
    return;
  }

  for (unsigned int i = 0; i < pw_buffer->buffer->n_datas; i++) {
    auto pod = static_cast<struct spa_pod *>(spa_pod_from_data(
        pw_buffer->buffer->datas[i].data, pw_buffer->buffer->datas[i].maxsize,
        pw_buffer->buffer->datas[i].chunk->offset,
        pw_buffer->buffer->datas[i].chunk->size));

    if (!spa_pod_is_sequence(pod)) {
      continue;
    }

    struct spa_pod_control *pod_control;
    SPA_POD_SEQUENCE_FOREACH(reinterpret_cast<struct spa_pod_sequence *>(pod),
                             pod_control) {
      if (pod_control->type == SPA_CONTROL_Midi) {
        uint8_t *data = nullptr;
        uint32_t length;
        spa_pod_get_bytes(&pod_control->value, (const void **)&data, &length);

        if (length == 3) {
          if (data[0] < 0b10000000) {
            std::cout << "fist byte is not a status byte" << std::endl;
          } else {
            uint8_t channel = data[0] & 0b00001111;
            uint8_t message_type = data[0] & 0b11110000;
            std::cout << std::endl
                      << "received message" << std::endl
                      << "channel: " << std::hex << (unsigned)channel
                      << std::endl
                      << "message type value: " << std::dec
                      << (unsigned)message_type << std::endl;
            switch (message_type) {
            case 0b10110000:
              std::cout << "message type: Control Change" << std::endl
                        << "control number: " << std::dec << (unsigned)data[1]
                        << std::endl
                        << "value: " << std::dec << (unsigned)data[2]
                        << std::endl;
              auto target_node_id = my_data->routing_table.find_target_node(
                  {data[0], data[1], data[2]});
              auto target_parameter =
                  my_data->routing_table.find_target_parameter(
                      {data[0], data[1], data[2]});
              if (target_node_id && target_parameter) {
                std::cout << std::endl
                          << "target_node: " << target_node_id.value().id
                          << std::endl
                          << "target_parameter: "
                          << target_parameter.value().parameter_name
                          << std::endl;

                auto target_node =
                    my_data->node_registry.get_node_by_id(target_node_id->id);

                if (target_node) {
                  double normalized_value = (double)data[2] / 127.0;
                  double result =
                      target_parameter->min +
                      (target_parameter->max - target_parameter->min) *
                          normalized_value;
                  pw_invoke_set_param_data invoke_data{
                      target_node_id.value().id, target_node->client, result};
                  strncpy(invoke_data.parameter_name,
                          target_parameter->parameter_name.c_str(),
                          sizeof(invoke_data.parameter_name));
                  pw_loop_invoke(
                      pw_main_loop_get_loop(my_data->loop),
                      [](struct spa_loop *loop, bool async, u_int32_t seq,
                         const void *data, size_t size, void *user_data) {
                        const pw_invoke_set_param_data *param_data =
                            static_cast<const pw_invoke_set_param_data *>(data);

                        u_int8_t buffer[1024];
                        auto pod =
                            utils::PodMessageBuilder::build_set_params_message(
                                buffer, sizeof(buffer),
                                param_data->parameter_name, param_data->value);

                        spa_debug_pod(0, nullptr, pod);

                        pw_node_set_param(reinterpret_cast<struct pw_node *>(
                                              param_data->target_node),
                                          SPA_PARAM_Props, 0, pod);
                        std::cout << std::endl
                                  << "set target node parameter." << std::endl;

                        return 0;
                      },
                      0, &invoke_data, sizeof(pw_invoke_set_param_data), false,
                      my_data);
                } else {
                  std::cout << std::endl
                            << "couldn't find target pipewire node in registry."
                            << std::endl;
                }
              } else {
                std::cout << "found target node: " << target_node_id.has_value()
                          << std::endl
                          << "found target parameter: "
                          << target_parameter.has_value() << std::endl;
              }
              break;
            }
          }
        } else {
          std::cout << "invalid message length" << std::endl;
        }
      }
    }
  }

  pw_filter_queue_buffer(my_data->port, pw_buffer);
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

  data.port = static_cast<struct port *>(
      pw_filter_add_port(data.filter, PW_DIRECTION_INPUT,
                         PW_FILTER_PORT_FLAG_MAP_BUFFERS, sizeof(struct port),
                         pw_properties_new(PW_KEY_FORMAT_DSP, "8 bit raw midi",
                                           PW_KEY_PORT_NAME, "input", NULL),
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
