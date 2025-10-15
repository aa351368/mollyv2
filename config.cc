#include "config.hh"
#include <fstream>
#include <algorithm>
#include <syslog.h>

namespace molly
{

  // Helper to trim whitespace from start and end of a string
  static std::string trim(const std::string& s)
  {
      size_t first = s.find_first_not_of(" \t\n\r");
      if (std::string::npos == first)
      {
          return s;
      }
      size_t last = s.find_last_not_of(" \t\n\r");
      return s.substr(first, (last - first + 1));
  }

  bool parse_config_file(const std::string& path, Config& config)
  {
    std::ifstream configFile(path);
    if (!configFile.is_open())
    {
      // It's not an error if the default file doesn't exist
      return false;
    }

    syslog(LOG_INFO, "Loading configuration from %s", path.c_str());

    std::string line;
    while (std::getline(configFile, line))
    {
      line = trim(line);
      if (line.empty() || line[0] == '#' || line[0] == ';')
      {
        continue;
      }

      std::string::size_type delimiterPos = line.find('=');
      if (delimiterPos != std::string::npos)
      {
        std::string key = trim(line.substr(0, delimiterPos));
        std::string value = trim(line.substr(delimiterPos + 1));

        if (key == "device")
        {
          config.device_path = value;
        }
        else if (key == "on_press")
        {
          config.on_press_cmd = value;
        }
        else if (key == "on_open")
        {
          config.on_open_cmd = value;
        }
        else if (key == "on_close")
        {
          config.on_close_cmd = value;
        }
        else if (key == "poll_interval_ms")
        {
          config.poll_interval_ms = std::stoi(value);
        }
        else if (key == "error_poll_interval_ms")
        {
          config.error_poll_interval_ms = std::stoi(value);
        }
      }
    }
    return true;
  }

} // namespace molly