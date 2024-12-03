#include "pipewire/core.h"
#include "pipewire/keys.h"
#include "pipewire/main-loop.h"
#include "pipewire/properties.h"
#include "pipewire/stream.h"
#include <getopt.h>
#include <iostream>
#include <math.h>
#include <ostream>
#include <signal.h>
#include <stdio.h>

#include <pipewire/filter.h>
#include <pipewire/pipewire.h>

#include <spa/control/control.h>
#include <spa/debug/pod.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <sys/types.h>

#include <spa/debug/pod.h>

#define PERIOD_NSEC (SPA_NSEC_PER_SEC / 8)

struct port {};

struct data {
  struct pw_main_loop *loop;
  struct pw_filter *filter;
  struct pw_registry *registry;
  struct port *port;
};

static void on_process(void *userdata, struct spa_io_position *position) {
  struct data *my_data = static_cast<struct data *>(userdata);
  struct pw_buffer *pw_buffer;

  if ((pw_buffer = pw_filter_dequeue_buffer(my_data->port)) == nullptr) {
    return;
  }

  pw_filter_queue_buffer(my_data->port, pw_buffer);

  std::cout << "processed " << std::endl;
}

static const pw_filter_events filter_events = {
    .version = PW_VERSION_FILTER_EVENTS,
    .process = on_process,
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
  pw_filter_destroy(data.filter);
  pw_main_loop_destroy(data.loop);
  pw_deinit();

  return 0;
}
