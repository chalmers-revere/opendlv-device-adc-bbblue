/*
 * Copyright (C) 2020 Björnborg Nguyen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fstream>
#include <iostream>
#include <string>

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

// Lipo jack channel 6, conversion: 1.8*11 = 19.8
// DC jack channel 5, conversion: 1.8*11 = 19.8

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid") ||
      0 == commandlineArguments.count("freq") ||
      0 == commandlineArguments.count("channel")) {
    std::cerr << argv[0]
              << " interfaces to the analog-to-digital converters on the "
                 "BeagleBone Blue."
              << std::endl;
    std::cerr << "Usage:   " << argv[0]
              << " --freq=<frequency> --cid=<OpenDaVINCI session> "
                 "--channel=<the ADC channel to read> [--id=<Identifier in "
                 "case of multiple sensors] [--verbose]"
              << std::endl;
    std::cerr << "Example: " << argv[0] << " --freq=10 --cid=111 --channel=0 "
              << std::endl;
    retCode = 1;
  } else {
    uint32_t const ID{
        (commandlineArguments["id"].size() != 0)
            ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"]))
            : 0};
    bool const VERBOSE{commandlineArguments.count("verbose") != 0};
    uint16_t const CID = std::stoi(commandlineArguments["cid"]);
    std::string const CHANNELSTR = commandlineArguments["channel"];
    float const FREQ = std::stof(commandlineArguments["freq"]);
    // float const CONVERSION2VOLT =
    // std::stof(commandlineArguments["conversion"]);
    if (std::stoi(CHANNELSTR) < 0 && std::stoi(CHANNELSTR) > 6) {
      std::cerr << "Not supported channel number, must be between 0 and 6."
                << std::endl;
    }
    uint8_t const CHANNELINT = std::stoi(CHANNELSTR);
    float conversion2Volt;
    if (CHANNELINT < 5) {
      conversion2Volt = 1.8f;
    } else {
      conversion2Volt = 19.8f;
    }

    cluon::OD4Session od4{CID};

    auto atFrequency{[&conversion2Volt, &CHANNELSTR, &CHANNELINT, &ID, &VERBOSE,
                      &od4]() -> bool {
      int32_t output{0};
      std::ifstream adcNode("/sys/bus/iio/devices/iio:device0/in_voltage" +
                            CHANNELSTR + "_raw");
      if (adcNode.is_open()) {
        std::string str;
        std::getline(adcNode, str);
        output = std::stoi(str);
      } else {
        std::cerr << "Failed to read from "
                     "/sys/bus/iio/devices/iio:device0/in_voltage" +
                         CHANNELSTR + "_raw."
                  << std::endl;
      }
      adcNode.close();
      float voltage{output * conversion2Volt / 4095.0f};

      if (CHANNELINT == 5) {
        voltage += -0.15f;
      } else if (CHANNELINT == 6) {
        voltage += -0.1f;
      }

      opendlv::proxy::VoltageReading voltageReading;
      voltageReading.voltage(voltage);
      cluon::data::TimeStamp sampleTime = cluon::time::now();
      od4.send(voltageReading, sampleTime, ID);

      if (VERBOSE) {
        std::cout << "Voltage reading: " << voltageReading.voltage() << " V."
                  << std::endl;
      }
      return od4.isRunning();
    }};
    od4.timeTrigger(FREQ, atFrequency);
  }
  return retCode;
}
