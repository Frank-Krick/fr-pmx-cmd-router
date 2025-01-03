#pragma once

#include "processing/parameters.h"
#include "processing/port.h"
#include "spa/debug/pod.h"
#include "utils/node_registry.h"
#include "utils/pod_message_builder.h"

#include <pipewire/client.h>
#include <pipewire/filter.h>
#include <pipewire/main-loop.h>
#include <pipewire/node.h>

namespace utils {

class PipewireService {
public:
  void set_parameter(Node &node, processing::parameter &parameter,
                     double value);

  static std::optional<struct pw_buffer *>
  dequeue_buffer(processing::port *port);

private:
  struct pw_invoke_set_param_data {
    pw_client *target_node;
    double value;
    char parameter_name[2048];
  };

  struct pw_main_loop *loop;
};

} // namespace utils
