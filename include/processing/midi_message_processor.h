#pragma once

#include "processing/parameters.h"
#include "processing/port.h"
#include "spa/pod/pod.h"

#include <functional>

#include <optional>
#include <span>
#include <sys/types.h>

#include <pipewire/filter.h>
#include <spa/control/control.h>
#include <spa/pod/iter.h>

namespace processing {

typedef std::function<double(struct parameter &, u_int8_t control_value)>
    parameter_interpolator;

struct parameter_change_event {
  parameter &parameter = none;
  u_int8_t control_value;
  double value;
};

class MidiMessageProcessor {
public:
  void process_port(processing::port *port,
                    std::span<parameter_change_event> updates) {

    struct pw_buffer *pw_buffer;
    if ((pw_buffer = pw_filter_dequeue_buffer(port)) == nullptr) {
      return;
    }

    for (unsigned int i = 0; i < pw_buffer->buffer->n_datas; i++) {
      auto pod = get_pod_sequence(pw_buffer, i);

      if (pod) {
        struct spa_pod_control *pod_control;
        SPA_POD_SEQUENCE_FOREACH(pod.value(), pod_control) {
          if (pod_control->type == SPA_CONTROL_Midi) {
            uint8_t *data = nullptr;
            uint32_t length;
            spa_pod_get_bytes(&pod_control->value, (const void **)&data,
                              &length);
          }
        }
      }
    }
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

  std::optional<std::array<uint8_t, 3>>
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

    return {};
  }
};

} // namespace processing
