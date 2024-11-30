#include "pipewire/keys.h"
#include "pipewire/properties.h"
#include "pipewire/stream.h"
#include "spa/buffer/buffer.h"
#include "spa/pod/iter.h"
#include "spa/pod/pod.h"
#include "spa/utils/dict.h"
#include <algorithm>
#include <cstdint>
#include <getopt.h>
#include <iostream>
#include <math.h>
#include <ostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
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

#define PERIOD_NSEC (SPA_NSEC_PER_SEC / 8)

struct port {};

struct data {
  struct pw_main_loop *loop;
  struct pw_filter *filter;
  struct port *port;
  uint32_t clock_id;
  int64_t offset;
  uint64_t position;
  utils::MidiRoutingTable routing_table;
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
      std::cout << std::endl << "received pod, not sequence" << std::endl;
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
              auto target_node = my_data->routing_table.find_target_node(
                  {data[0], data[1], data[2]});
              auto target_parameter =
                  my_data->routing_table.find_target_parameter(
                      {data[0], data[1], data[2]});
              if (target_node && target_parameter) {
                std::cout << std::endl
                          << "target_node: " << target_node.value().id
                          << std::endl
                          << "target_parameter: "
                          << target_parameter.value().parameter_name
                          << std::endl;
              }
              break;
            }
          }
        } else {
          std::cout << "invalid message length" << std::endl;
        }
      } else {
        std::cout << std::endl
                  << "received message" << std::endl
                  << "type: " << pod_control->type << std::endl;
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
          my_data->routing_table.register_target_node(i, assignment->first);
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

static void do_quit(void *userdata, int signal_number) {
  data *data = static_cast<struct data *>(userdata);
  pw_main_loop_quit(data->loop);
}

static void handle_registry_event(void *data, uint32_t id, uint32_t permissions,
                                  const char *type, uint32_t version,
                                  const struct spa_dict *props) {
  std::cout << std::endl
            << "object id: " << id << std::endl
            << "type: " << type << std::endl
            << "version: " << version << std::endl;
}

static const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = handle_registry_event,
};

int main(int argc, char *argv[]) {
  struct data data = {};
  uint8_t buffer[1024];
  struct spa_pod_builder builder;

  pw_init(&argc, &argv);

  data.loop = pw_main_loop_new(NULL);

  pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
  pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

  auto context = pw_context_new(pw_main_loop_get_loop(data.loop),
                                NULL /* properties */, 0 /* user_data size */);
  auto core = pw_context_connect(context, NULL /* properties */,
                                 0 /* user_data size */);

  auto registry =
      pw_core_get_registry(core, PW_VERSION_REGISTRY, 0 /* user_data size */);

  struct spa_hook registry_listener;
  spa_zero(registry_listener);
  //  pw_registry_add_listener(registry, &registry_listener, &registry_events,
  //                           NULL);

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

  spa_pod_builder_init(&builder, buffer, sizeof(buffer));

  if (pw_filter_connect(data.filter, PW_FILTER_FLAG_RT_PROCESS, NULL, 0) < 0) {
    fprintf(stderr, "can't connect\n");
    return -1;
  }

  pw_main_loop_run(data.loop);
  pw_proxy_destroy((struct pw_proxy *)registry);
  pw_core_disconnect(core);
  pw_context_destroy(context);
  pw_filter_destroy(data.filter);
  pw_main_loop_destroy(data.loop);
  pw_deinit();

  return 0;
}
