#pragma once

#include <memory>
#include <string>
namespace processing {

struct parameter {
  unsigned int id;
  std::string name;
  std::string device_name;
  double min;
  double max;
  std::string full_name;

  parameter(unsigned int id, std::string name, std::string device_name,
            double min, double max)
      : id(id), name(name), device_name(device_name), min(min), max(max),
        full_name(device_name + ":" + name) {}

  bool operator==(const processing::parameter &other) const {
    return this->id == other.id;
  }
};

extern std::shared_ptr<parameter> none;
extern std::shared_ptr<parameter> saturator_level_in;
extern std::shared_ptr<parameter> saturator_drive;
extern std::shared_ptr<parameter> saturator_blend;
extern std::shared_ptr<parameter> saturator_level_out;
extern std::shared_ptr<parameter> compressor_threshold;
extern std::shared_ptr<parameter> compressor_ratio;
extern std::shared_ptr<parameter> compressor_attack;
extern std::shared_ptr<parameter> compressor_release;
extern std::shared_ptr<parameter> compressor_makeup;
extern std::shared_ptr<parameter> compressor_knee;
extern std::shared_ptr<parameter> compressor_mix;
extern std::shared_ptr<parameter> equalizer_low;
extern std::shared_ptr<parameter> equalizer_mid;
extern std::shared_ptr<parameter> equalizer_high;
extern std::shared_ptr<parameter> equalizer_master;
extern std::shared_ptr<parameter> equalizer_low_mid;
extern std::shared_ptr<parameter> equalizer_mid_high;

} // namespace processing
