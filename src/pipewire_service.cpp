#include "utils/pipewire_service.h"

void utils::PipewireService::set_parameter(Node &node,
                                           processing::parameter &parameter,
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

std::optional<struct pw_buffer *>
utils::PipewireService::dequeue_buffer(processing::port *port) {
  struct pw_buffer *pw_buffer;
  if ((pw_buffer = pw_filter_dequeue_buffer(port)) == nullptr) {
    return {};
  }

  return pw_buffer;
}
