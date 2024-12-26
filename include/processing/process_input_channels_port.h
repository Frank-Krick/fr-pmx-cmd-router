#pragma once

#include "pipewire/node.h"
#include "processing/port.h"
#include "utils/midi_routing_table.h"
#include "utils/node_registry.h"
#include "utils/pod_message_builder.h"

#include <iostream>
#include <ostream>
#include <pipewire/filter.h>
#include <pipewire/main-loop.h>
#include <pipewire/node.h>

#include <spa/control/control.h>
#include <spa/debug/pod.h>
#include <spa/pod/iter.h>
#include <spa/pod/pod.h>

namespace processing {

struct pw_invoke_set_param_data {
  unsigned int target_node_id;
  pw_client *target_node;
  double value;
  char parameter_name[2048];
};

class ProcessInputChannelsPort {
public:
  static void on_process(processing::port *port,
                         utils::MidiRoutingTable &routing_table,
                         utils::NodeRegistry &node_registry,
                         struct pw_main_loop *loop) {

    struct pw_buffer *pw_buffer;
    if ((pw_buffer = pw_filter_dequeue_buffer(port)) == nullptr) {
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
                auto target_node_id =
                    routing_table.find_target_node({data[0], data[1], data[2]});
                auto target_parameter = routing_table.find_target_parameter(
                    {data[0], data[1], data[2]});
                if (target_node_id && target_parameter) {
                  std::cout << std::endl
                            << "target_node: " << target_node_id.value().id
                            << std::endl
                            << "target_parameter: "
                            << target_parameter.value().parameter_name
                            << std::endl;

                  auto target_node =
                      node_registry.get_node_by_id(target_node_id->id);

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
                        pw_main_loop_get_loop(loop),
                        [](struct spa_loop *loop, bool async, u_int32_t seq,
                           const void *data, size_t size, void *user_data) {
                          const pw_invoke_set_param_data *param_data =
                              static_cast<const pw_invoke_set_param_data *>(
                                  data);

                          u_int8_t buffer[1024];
                          auto pod = utils::PodMessageBuilder::
                              build_set_params_message(
                                  buffer, sizeof(buffer),
                                  param_data->parameter_name,
                                  param_data->value);

                          spa_debug_pod(0, nullptr, pod);

                          pw_node_set_param(reinterpret_cast<struct pw_node *>(
                                                param_data->target_node),
                                            SPA_PARAM_Props, 0, pod);
                          std::cout << std::endl
                                    << "set target node parameter."
                                    << std::endl;

                          return 0;
                        },
                        0, &invoke_data, sizeof(pw_invoke_set_param_data),
                        false, nullptr);
                  } else {
                    std::cout
                        << std::endl
                        << "couldn't find target pipewire node in registry."
                        << std::endl;
                  }
                } else {
                  std::cout
                      << "found target node: " << target_node_id.has_value()
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

    pw_filter_queue_buffer(port, pw_buffer);
  }
};

} // namespace processing
