/*
 * Copyright (C) 2018 Ola Benderius
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

#include <iostream>
#include <fstream>
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
      0 == commandlineArguments.count("channel") ||
      0 == commandlineArguments.count("conversion")
      ) {
    std::cerr << argv[0] << " interfaces to the analog-to-digital converters on the BeagleBone Blue." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --freq=<frequency> --cid=<OpenDaVINCI session> --channel=<the ADC channel to read> --conversion=<constant to convert bits values to volt values> [--id=<Identifier in case of multiple sensors] [--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --freq=10 --cid=111 --channel=0 --voltageConversion=1.8" << std::endl;
    retCode = 1;
  } else {
    uint32_t const ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
    bool const VERBOSE{commandlineArguments.count("verbose") != 0};
    uint16_t const CID = std::stoi(commandlineArguments["cid"]);
    std::string const CHANNEL = commandlineArguments["channel"];
    float const FREQ = std::stof(commandlineArguments["freq"]);
    float const CONVERSION2VOLT = std::stof(commandlineArguments["conversion"]);

    cluon::OD4Session od4{CID};

    auto atFrequency{[&CONVERSION2VOLT, &CHANNEL, &ID, &VERBOSE, &od4]() -> bool
      {
        int32_t output{0};
        std::ifstream adcNode("/sys/bus/iio/devices/iio:device0/in_voltage" + CHANNEL + "_raw");
        if (adcNode.is_open()) {
          std::string str;
          std::getline(adcNode, str);
          output = std::stoi(str);
        } else {
          std::cerr << "Failed to read from /sys/bus/iio/devices/iio:device0/in_voltage" + CHANNEL + "_raw." << std::endl;
        }
        adcNode.close();
        float const voltage{output * CONVERSION2VOLT / 4095.0f};

        opendlv::proxy::VoltageReading voltageReading;
        voltageReading.voltage(voltage);
      
        opendlv::proxy::DistanceReading distanceReading;
        float distance{(1.0f / (voltage / 10.13f)) - 3.8f };
        if(voltage > 3.0f && distance < 40.0f) {
          distanceReading.distance(distance/100.0f);
        } else {
          distanceReading.distance(-1);
        }


        cluon::data::TimeStamp sampleTime = cluon::time::now();
        od4.send(voltageReading, sampleTime, ID);
      
        od4.send(distanceReading, sampleTime, ID);
        if (VERBOSE) {
          std::cout << "Voltage reading: " << voltageReading.voltage() << " V." << std::endl;
          std::cout << "Distance reading: " << distanceReading.distance() << " m." << std::endl;
        }
        return true;
      }};
    if (std::stoi(CHANNEL) >= 0 && std::stoi(CHANNEL) < 5) {
      od4.timeTrigger(FREQ, atFrequency);
    } else {
      std::cerr << "Not supported channel number, must be between 0 and 4." << std::endl;
    }
  }
  return retCode;
}
