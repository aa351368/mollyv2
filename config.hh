#pragma once

#include <string>

namespace molly
{
  struct Config
  {
    std::string device_path = "/dev/big_red_button";
    std::string on_press_cmd = "";
    std::string on_open_cmd = "";
    std::string on_close_cmd = "";
    int poll_interval_ms = 20;
    int error_poll_interval_ms = 1000;
  };

  // Parses an INI-style config file. Returns false on error.
  bool parse_config_file(const std::string& path, Config& config);

} // namespace molly