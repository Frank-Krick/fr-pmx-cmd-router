project('fr-pmx-ctl-router', 'cpp', default_options : ['cpp_std=c++23'])
pipewire_dep = dependency('libpipewire-0.3')

include_directory = include_directories('include')

executable(
  'fr-pmx-ctl-router',
  'src/main.cpp',
  dependencies: [pipewire_dep],
  include_directories: [include_directory])

midi_rounting_table_unit_tests = executable(
  'midi routing table', 
  'tests/utils/test_midi_routing_table.cpp',
  include_directories: [include_directory])

test(
  'test midi routing table',
  midi_rounting_table_unit_tests)

build_set_params_message_tests = executable(
  'build set params message',
  'tests/utils/test_build_set_params_message.cpp',
  dependencies: [pipewire_dep],
  include_directories: [include_directory])

test(
  'test build set params message',
  build_set_params_message_tests)

