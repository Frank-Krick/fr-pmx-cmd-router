#pragma once

#include <string>
namespace processing {

struct parameter {
  std::string name;
  std::string device_name;
  std::string full_name;
  double min;
  double max;
};

constexpr struct parameter make_parameter(std::string name,
                                          std::string device_name, double min,
                                          double max) {
  return {name, device_name, device_name + ":" + name, min, max};
}

inline auto none = make_parameter("none", "none", 0.0, 0.0);
inline auto saturator_level_in =
    make_parameter("level_in", "Saturator", 0.015625, 64.0);
inline auto saturator_drive = make_parameter("drive", "Saturator", 0.1, 10.0);
inline auto saturator_blend = make_parameter("blend", "Saturator", -10, 10);
inline auto saturator_level_out =
    make_parameter("level_out", "Saturator", 0.015625, 64.0);
inline auto compressor_threshold =
    make_parameter("threshold", "Compressor", 0.000977, 1.0);
inline auto compressor_ratio = make_parameter("ratio", "Compressor", 1.0, 20.0);
inline auto compressor_attack =
    make_parameter("attack", "Compressor", 0.01, 2000.0);
inline auto compressor_release =
    make_parameter("release", "Compressor", 0.01, 2000.0);
inline auto compressor_makeup =
    make_parameter("makeup", "Compressor", 1.0, 64.0);
inline auto compressor_knee = make_parameter("knee", "Compressor", 1.0, 8.0);
inline auto compressor_mix = make_parameter("mix", "Compressor", 0.0, 1.0);
inline auto equalizer_low = make_parameter("low", "Equalizer", -24, 24);
inline auto equalizer_mid = make_parameter("mid", "Equalizer", -24, 24);
inline auto equalizer_high = make_parameter("high", "Equalizer", -24, 24);
inline auto equalizer_master = make_parameter("master", "Equalizer", -24, 24);
inline auto equalizer_low_mid =
    make_parameter("low_mid", "Equalizer", 0.0, 1000.0);
inline auto equalizer_mid_high =
    make_parameter("mid_high", "Equalizer", 1000.0, 20000.0);

} // namespace processing
