#include "application/application.h"
#include "osc_service/osc_service.h"
#include "processing/midi_cc_message_processor_implementations.h"
#include "processing/midi_message_processor.h"
#include "processing/parameters.h"
#include "utils/pipewire_service.h"

#include <array>
#include <cstring>
#include <iostream>
#include <ostream>
#include <ranges>
#include <sstream>
#include <string>

#include <pipewire/loop.h>
#include <pipewire/pipewire.h>

#include <spa/control/control.h>
#include <spa/debug/pod.h>
#include <spa/pod/builder.h>
#include <spa/pod/iter.h>
#include <spa/pod/parser.h>
#include <spa/pod/pod.h>
#include <spa/utils/dict.h>

void application::Application::on_process(void *user_data,
                                          struct spa_io_position *position) {
  auto this_pointer = static_cast<Application *>(user_data);

  std::array<processing::MidiMessageProcessor::parameter_change_event, 10>
      updates{};

  this_pointer->midi_message_processor.process_port_messages(
      this_pointer->input_channels_port, updates,
      this_pointer->input_channel_port_processor.value());

  this_pointer->midi_message_processor.process_port_messages(
      this_pointer->group_channels_port, updates,
      this_pointer->group_channel_port_processor.value());

  for (auto &&update : updates | std::ranges::views::filter([](auto &e) {
                         return e.parameter != processing::none;
                       })) {
    std::cout << "Change event: " << update.parameter->full_name << ": "
              << update.value << std::endl;
  }

  std::cout << "Sending osc update" << std::endl;
  auto pw_buffer =
      utils::PipewireService::dequeue_buffer(this_pointer->updates_port);

  if (!pw_buffer) {
    return;
  }

  pw_filter_queue_buffer(this_pointer->updates_port, pw_buffer.value());
  /*
  char osc_buffer[1000];
  auto actual_size =
      osc_service::OscService::build_message(updates, &osc_buffer, 1000);

  if (actual_size > 0) {
    std::cout << "Sending osc update" << std::endl;
    auto pw_buffer =
        utils::PipewireService::dequeue_buffer(this_pointer->updates_port);

    if (!pw_buffer) {
      return;
    }

    struct spa_buffer *spa_buffer;
    spa_buffer = pw_buffer.value()->buffer;
    if (spa_buffer->datas[0].data == NULL) {
      std::cout << "Buffer has no data" << std::endl;
      return;
    }

    auto data_ptr = spa_buffer->datas[0].data;
    auto max_size = spa_buffer->datas[0].maxsize;
    if (actual_size > max_size) {
      std::cout << "Buffer size too small" << std::endl;
      return;
    }

    memcpy(data_ptr, osc_buffer, actual_size);
    spa_buffer->datas[0].chunk->size = actual_size;
    pw_filter_queue_buffer(this_pointer->updates_port, pw_buffer.value());
  }
  */
}

void application::Application::handle_filter_param_update(
    void *user_data, void *port_data, uint32_t id, const struct spa_pod *pod) {
  auto this_pointer = static_cast<Application *>(user_data);

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
          this_pointer->input_channels_midi_routing_table
              .clear_target_node_by_channel(i);
        } else {
          this_pointer->input_channels_midi_routing_table.register_target_node(
              i, assignment->second);
        }
      }

      struct spa_dict_item items[1];
      items[0] = SPA_DICT_ITEM_INIT("pmx.channel_mapping", value.c_str());
      struct spa_dict update_dict = SPA_DICT_INIT(items, 1);
      pw_filter_update_properties(this_pointer->filter, nullptr, &update_dict);
    } else if (key == "pmx.group_channel_node_ids") {
      std::string assignment;
      std::istringstream string_stream(value);
      std::vector<unsigned int> node_ids;
      while (std::getline(string_stream, assignment, ',')) {
        node_ids.push_back(std::stoi(assignment));
      }

      if (node_ids.size() == 4) {

        this_pointer->group_to_node_id_mapping = {node_ids[0], node_ids[1],
                                                  node_ids[2], node_ids[3]};

        struct spa_dict_item items[1];
        items[0] =
            SPA_DICT_ITEM_INIT("pmx.group_channel_node_ids", value.c_str());
        struct spa_dict update_dict = SPA_DICT_INIT(items, 1);
        pw_filter_update_properties(this_pointer->filter, nullptr,
                                    &update_dict);
      }
    }
  }
}

static const pw_filter_events filter_events = {
    .version = PW_VERSION_FILTER_EVENTS,
    .param_changed = application::Application::handle_filter_param_update,
    .process = application::Application::on_process,
};

static const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = application::Application::registry_event_global,
    .global_remove = application::Application::registry_event_global_remove,
};

application::Application::Application(int argc, char *argv[]) {
  pw_init(&argc, &argv);

  loop = pw_main_loop_new(NULL);

  pw_loop_add_signal(pw_main_loop_get_loop(loop), SIGINT, do_quit, this);
  pw_loop_add_signal(pw_main_loop_get_loop(loop), SIGTERM, do_quit, this);

  context = pw_context_new(pw_main_loop_get_loop(loop), nullptr, 0);
  core = pw_context_connect(context, nullptr, 0);
  registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);

  spa_zero(registry_listener);
  pw_registry_add_listener(registry, &registry_listener, &registry_events,
                           this);

  filter = pw_filter_new_simple(
      pw_main_loop_get_loop(loop), "fr-pmx-cmd-router",
      pw_properties_new(PW_KEY_MEDIA_TYPE, "Midi", PW_KEY_MEDIA_CATEGORY,
                        "Filter", PW_KEY_MEDIA_CLASS, "Midi/Sink",
                        PW_KEY_MEDIA_ROLE, "DSP", "pmx.channel_mapping", "",
                        NULL),
      &filter_events, this);

  input_channels_port =
      static_cast<struct processing::port *>(pw_filter_add_port(
          filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS,
          sizeof(struct processing::port),
          pw_properties_new(PW_KEY_FORMAT_DSP, "8 bit raw midi",
                            PW_KEY_PORT_NAME, "input_channels_port", NULL),
          NULL, 0));

  group_channels_port =
      static_cast<struct processing::port *>(pw_filter_add_port(
          filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS,
          sizeof(struct processing::port),
          pw_properties_new(PW_KEY_FORMAT_DSP, "8 bit raw midi",
                            PW_KEY_PORT_NAME, "group_channels_port", NULL),
          NULL, 0));

  updates_port = static_cast<struct processing::port *>(pw_filter_add_port(
      filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS,
      sizeof(struct processing::port),
      pw_properties_new(PW_KEY_FORMAT_DSP, "Osc", PW_KEY_PORT_NAME, "updates",
                        NULL),
      NULL, 0));

  if (!updates_port) {
    std::cout << "Couldn't create updates port: " << updates_port << std::endl;
  }

  input_channel_port_processor = processing::InputChannelPortProcessor(
      input_channels_midi_routing_table, node_registry);

  group_channel_port_processor = processing::GroupChannelsPortProcessor(
      group_channels_midi_routing_table, node_registry,
      group_to_node_id_mapping);
}

int application::Application::run() {
  if (pw_filter_connect(filter, PW_FILTER_FLAG_RT_PROCESS, NULL, 0) < 0) {
    fprintf(stderr, "can't connect\n");
    return -1;
  }

  pw_main_loop_run(loop);
  pw_proxy_destroy((struct pw_proxy *)registry);
  pw_core_disconnect(core);
  pw_filter_destroy(filter);
  pw_filter_destroy(filter);
  pw_main_loop_destroy(loop);
  pw_deinit();

  return 0;
}

void application::Application::shutdown() { pw_main_loop_quit(loop); }

void application::Application::do_quit(void *user_data, int signal_number) {
  auto this_pointer = static_cast<Application *>(user_data);
  this_pointer->shutdown();
}

void application::Application::registry_event_global(
    void *data, uint32_t id, uint32_t permissions, const char *c_type,
    uint32_t version, const struct spa_dict *props) {
  auto this_pointer = static_cast<Application *>(data);

  std::string type(c_type);
  if (type == "PipeWire:Interface:Node") {
    auto client = static_cast<pw_client *>(pw_registry_bind(
        this_pointer->registry, id, c_type, PW_VERSION_CLIENT, 0));
    this_pointer->node_registry.add_node(id, client);
  }
}

void application::Application::registry_event_global_remove(void *data,
                                                            uint32_t id) {
  auto this_pointer = static_cast<Application *>(data);

  this_pointer->node_registry.delete_node_by_id(id);
}
