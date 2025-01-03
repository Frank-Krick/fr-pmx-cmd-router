#pragma once

#include "processing/parameters.h"
#include "processing/port.h"
#include "utils/node_registry.h"
#include "utils/pipewire_service.h"

#include <functional>
#include <optional>

#include <sys/types.h>

#include <pipewire/filter.h>
#include <spa/control/control.h>
#include <spa/pod/iter.h>
#include <spa/pod/pod.h>

namespace processing {

class MidiMessageProcessor {
public:
  enum layer { A, B };

  enum channel_type { INPUT, GROUP, LAYER };

  class parameter_change_event {
  public:
    std::shared_ptr<processing::parameter> parameter;
    utils::Node node;
    u_int8_t control_value;
    double value;
    u_int8_t channel_number;
    enum layer layer;
    enum channel_type channel_type;

    parameter_change_event()
        : parameter(none), control_value(0), value(0), channel_number(0),
          layer(A), channel_type(INPUT) {}

    parameter_change_event(std::shared_ptr<struct parameter> parameter,
                           utils::Node node, u_int8_t control_value,
                           double value, enum layer layer,
                           u_int8_t channel_number,
                           enum channel_type channel_type)
        : parameter(parameter), node(node), control_value(control_value),
          value(value), channel_number(channel_number), layer(layer),
          channel_type(channel_type) {}

    parameter_change_event(const parameter_change_event &) = default;
    parameter_change_event(parameter_change_event &&) = default;

    parameter_change_event &operator=(const parameter_change_event &other) {
      this->parameter = other.parameter;
      this->control_value = other.control_value;
      this->value = other.value;
      this->node = other.node;
      this->channel_number = other.channel_number;
      this->layer = other.layer;
      this->channel_type = other.channel_type;
      return *this;
    }

    parameter_change_event &operator=(parameter_change_event &&other) {
      this->parameter = other.parameter;
      this->control_value = other.control_value;
      this->value = other.value;
      this->node = other.node;
      this->channel_number = other.channel_number;
      this->layer = other.layer;
      this->channel_type = other.channel_type;
      return *this;
    }
  };

  struct midi_cc_message {
    u_int8_t channel;
    u_int8_t cc_number;
    u_int8_t value;
  };

  typedef std::function<std::optional<parameter_change_event>(midi_cc_message)>
      midi_cc_message_processor;

  template <class I>
  void process_port_messages(processing::port *port, I &updates,
                             midi_cc_message_processor message_processor) {

    auto pw_buffer = utils::PipewireService::dequeue_buffer(port);

    if (!pw_buffer) {
      return;
    }

    for (unsigned int i = 0; i < pw_buffer.value()->buffer->n_datas; i++) {
      auto pod = get_pod_sequence(pw_buffer.value(), i);

      if (!pod)
        continue;

      struct spa_pod_control *pod_control;
      auto event_list_iterator = updates.begin();
      SPA_POD_SEQUENCE_FOREACH(pod.value(), pod_control) {
        auto midi_cc_message = get_midi_cc_message(pod_control);
        if (midi_cc_message) {
          auto event = message_processor(midi_cc_message.value());
          if (event) {
            event_list_iterator->parameter = event.value().parameter;
            event_list_iterator->node = event.value().node;
            event_list_iterator->control_value = event.value().control_value;
            event_list_iterator->value = event.value().value;
            event_list_iterator++;
          }
        }
      }
    }

    pw_filter_queue_buffer(port, pw_buffer.value());
  }

private:
  std::optional<struct spa_pod_sequence *>
  get_pod_sequence(struct pw_buffer *pw_buffer, size_t index) {
    auto pod = static_cast<struct spa_pod *>(
        spa_pod_from_data(pw_buffer->buffer->datas[index].data,
                          pw_buffer->buffer->datas[index].maxsize,
                          pw_buffer->buffer->datas[index].chunk->offset,
                          pw_buffer->buffer->datas[index].chunk->size));

    if (!spa_pod_is_sequence(pod)) {
      return {};
    }

    return reinterpret_cast<struct spa_pod_sequence *>(pod);
  }

  std::optional<midi_cc_message>
  get_midi_cc_message(struct spa_pod_control *pod_control) {
    if (pod_control->type != SPA_CONTROL_Midi) {
      return {};
    }

    uint8_t *data = nullptr;
    uint32_t length;
    spa_pod_get_bytes(&pod_control->value, (const void **)&data, &length);

    if (length != 3) {
      return {};
    }

    if (data[0] < 0b10000000) {
      return {};
    }

    uint8_t channel = data[0] & 0b00001111;
    uint8_t message_type = data[0] & 0b11110000;

    if (message_type != 0b10110000) {
      return {};
    }

    return midi_cc_message{channel, data[1], data[2]};
  }
};

} // namespace processing
