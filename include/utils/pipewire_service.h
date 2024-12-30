#pragma once

#include "processing/parameters.h"
#include "spa/debug/pod.h"
#include "utils/node_registry.h"
#include "utils/pod_message_builder.h"

#include <pipewire/client.h>
#include <pipewire/main-loop.h>
#include <pipewire/node.h>

namespace utils {

class PipewireService {
public:
  void set_parameter(Node &node, processing::parameter &parameter,
                     double value) {

    pw_invoke_set_param_data invoke_data{node.client, value};
    strncpy(invoke_data.parameter_name, parameter.full_name.c_str(),
            sizeof(invoke_data.parameter_name));
    pw_loop_invoke(
        pw_main_loop_get_loop(loop),
        [](struct spa_loop *loop, bool async, u_int32_t seq, const void *data,
           size_t size, void *user_data) {
          const pw_invoke_set_param_data *param_data =
              static_cast<const pw_invoke_set_param_data *>(data);

          u_int8_t buffer[1024];
          auto pod = utils::PodMessageBuilder::build_set_params_message(
              buffer, sizeof(buffer), param_data->parameter_name,
              param_data->value);

          spa_debug_pod(0, nullptr, pod);

          pw_node_set_param(
              reinterpret_cast<struct pw_node *>(param_data->target_node),
              SPA_PARAM_Props, 0, pod);

          return 0;
        },
        0, &invoke_data, sizeof(pw_invoke_set_param_data), false, nullptr);
  }

private:
  struct pw_invoke_set_param_data {
    pw_client *target_node;
    double value;
    char parameter_name[2048];
  };

  struct pw_main_loop *loop;
};

} // namespace utils
