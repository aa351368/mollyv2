#include "device.hh"
#include "config.hh"

#include <iostream>
#include <unistd.h>
#include <syslog.h>
#include <csignal>
#include <sys/stat.h>
#include <sstream>
#include <string.h>
#include <vector>
#include <wordexp.h>
#include <getopt.h>

using namespace molly;
using namespace std;

void runCommand(const std::string& command)
{
  if (command.empty()) {
    return;
  }

  syslog(LOG_INFO, "Invoking command: %s", command.c_str());

  pid_t pid = fork();

  if (pid < 0)
  {
    syslog(LOG_ERR, "Error forking to invoke command: %s", command.c_str());
    return;
  }
  else if (pid == 0)
  {
    // Child process
    wordexp_t p;
    char** w;

    // Expand command into arguments, handles spaces and quotes
    if (wordexp(command.c_str(), &p, 0) != 0) {
        syslog(LOG_ERR, "Failed to parse command: %s", command.c_str());
        exit(EXIT_FAILURE);
    }

    w = p.we_wordv;
    // execvp replaces the child process with the command
    if (execvp(w[0], w) == -1) {
        syslog(LOG_ERR, "Failed to exec command '%s': %s", w[0], strerror(errno));
    }

    wordfree(&p);
    exit(0);
  }
}

bool shutdown = false;

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -h, --help            Show this help message\n"
              << "  -c, --config FILE     Path to configuration file\n"
              << "  -d, --device DEVICE   Path to the device (e.g., /dev/big_red_button)\n";
}

int main(int argc, char* argv[])
{
  Config config;
  std::string config_file_path;

  // --- Argument Parsing ---
  const struct option long_options[] = {
      {"help",    no_argument,       0, 'h'},
      {"config",  required_argument, 0, 'c'},
      {"device",  required_argument, 0, 'd'},
      {0, 0, 0, 0}
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "hc:d:", long_options, NULL)) != -1) {
      switch (opt) {
          case 'h':
              print_usage(argv[0]);
              return EXIT_SUCCESS;
          case 'c':
              config_file_path = optarg;
              break;
          case 'd':
              config.device_path = optarg;
              break;
          default:
              print_usage(argv[0]);
              return EXIT_FAILURE;
      }
  }

  if (setlogmask(LOG_UPTO(LOG_INFO)) < 0)
  {
    cerr << "Error setting log mask" << endl;
    exit(1);
  }

  auto signal_handler = [](int signo)
  {
      syslog(LOG_INFO, "Received %s -- shutting down", strsignal(signo));
      shutdown = true;
  };

  // Setup signal handlers for graceful shutdown
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  // SIGKILL cannot be caught or handled.
  
  // Ignore SIGCHLD and SIGHUP signals
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN); // TODO: Could be used to reload configuration

  // --- Configuration Loading ---
  // 1. Load from specified config file
  if (!config_file_path.empty()) {
      if (!parse_config_file(config_file_path, config)) {
          syslog(LOG_ERR, "Could not open specified config file: %s", config_file_path.c_str());
          // Continue with defaults/overrides
      }
  } else {
      // 2. Or, try loading from default location
      parse_config_file("/etc/mollyd.conf", config);
  }

  // --- Logging Setup ---
  openlog("mollyd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
  syslog(LOG_INFO, "Starting mollyd");

  // Command-line arguments for device path have already overridden config file settings.

  DeviceState lastState = DeviceState::Unknown;
  Device device;

  while (!shutdown)
  {
    if (!device.isOpen())
    {
      try
      {
        syslog(LOG_INFO, "Opening device");
        device.open(config.device_path);
        syslog(LOG_INFO, "Device opened");
      }
      catch (MollyError err)
      {
        syslog(LOG_ERR, "Error trying to open device: %s", err.what());
        usleep(config.error_poll_interval_ms * 1000);
        continue;
      }
    }

    DeviceState state;
    try
    {
      state = device.sample();
    }
    catch (MollyError err)
    {
      syslog(LOG_ERR, "Error reading from device: %s", err.what());

      try
      {
        device.close();
        syslog(LOG_INFO, "Device closed");
      }
      catch (MollyError err)
      {
        syslog(LOG_ERR, "Error closing device: %s", err.what());
      }

      continue;
    }

    switch (state)
    {
      case DeviceState::ButtonPressed:
        if (lastState != DeviceState::ButtonPressed)
        {
          syslog(LOG_INFO, "STATE: Pressed");
          runCommand(config.on_press_cmd);
        }
        break;
      case DeviceState::LidOpen:
        if (lastState != DeviceState::LidOpen && lastState != DeviceState::ButtonPressed)
        {
          syslog(LOG_INFO, "STATE: Open");
          runCommand(config.on_open_cmd);
        }
        break;
      case DeviceState::LidClosed:
        if (lastState != DeviceState::LidClosed)
        {
          syslog(LOG_INFO, "STATE: Closed");
          runCommand(config.on_close_cmd);
        }
        break;
      default:
        // For Unavailable state, do nothing and let the loop continue
        continue;
    }

    lastState = state;

    usleep(config.poll_interval_ms * 1000);
  }

  syslog(LOG_INFO, "mollyd shutting down.");
  closelog();
}
