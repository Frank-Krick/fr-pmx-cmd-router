#include "processing/parameters.h"
#include <memory>

std::shared_ptr<processing::parameter> processing::none =
    std::make_shared<processing::parameter>(0, "none", "none", 0.0, 0.0);
std::shared_ptr<processing::parameter> processing::saturator_level_in =
    std::make_shared<processing::parameter>(1, "level_in", "Saturator",
                                            0.015625, 64.0);
std::shared_ptr<processing::parameter> processing::saturator_drive =
    std::make_shared<processing::parameter>(2, "drive", "Saturator", 0.1, 10.0);
std::shared_ptr<processing::parameter> processing::saturator_blend =
    std::make_shared<processing::parameter>(3, "blend", "Saturator", -10, 10);
std::shared_ptr<processing::parameter> processing::saturator_level_out =
    std::make_shared<processing::parameter>(4, "level_out", "Saturator",
                                            0.015625, 64.0);
std::shared_ptr<processing::parameter> processing::compressor_threshold =
    std::make_shared<processing::parameter>(5, "threshold", "Compressor",
                                            0.000977, 1.0);
std::shared_ptr<processing::parameter> processing::compressor_ratio =
    std::make_shared<processing::parameter>(6, "ratio", "Compressor", 1.0,
                                            20.0);
std::shared_ptr<processing::parameter> processing::compressor_attack =
    std::make_shared<processing::parameter>(7, "attack", "Compressor", 0.01,
                                            2000.0);
std::shared_ptr<processing::parameter> processing::compressor_release =
    std::make_shared<processing::parameter>(8, "release", "Compressor", 0.01,
                                            2000.0);
std::shared_ptr<processing::parameter> processing::compressor_makeup =
    std::make_shared<processing::parameter>(9, "makeup", "Compressor", 1.0,
                                            64.0);
std::shared_ptr<processing::parameter> processing::compressor_knee =
    std::make_shared<processing::parameter>(10, "knee", "Compressor", 1.0, 8.0);
std::shared_ptr<processing::parameter> processing::compressor_mix =
    std::make_shared<processing::parameter>(11, "mix", "Compressor", 0.0, 1.0);
std::shared_ptr<processing::parameter> processing::equalizer_low =
    std::make_shared<processing::parameter>(12, "low", "Equalizer", -24, 24);
std::shared_ptr<processing::parameter> processing::equalizer_mid =
    std::make_shared<processing::parameter>(13, "mid", "Equalizer", -24, 24);
std::shared_ptr<processing::parameter> processing::equalizer_high =
    std::make_shared<processing::parameter>(14, "high", "Equalizer", -24, 24);
std::shared_ptr<processing::parameter> processing::equalizer_master =
    std::make_shared<processing::parameter>(15, "master", "Equalizer", -24, 24);
std::shared_ptr<processing::parameter> processing::equalizer_low_mid =
    std::make_shared<processing::parameter>(16, "low_mid", "Equalizer", 0.0,
                                            1000.0);
std::shared_ptr<processing::parameter> processing::equalizer_mid_high =
    std::make_shared<processing::parameter>(17, "mid_high", "Equalizer", 1000.0,
                                            20000.0);
