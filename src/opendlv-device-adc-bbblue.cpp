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

/*
 * Code is following the example of the MIT-licenced Robotics Cape Library: https://github.com/StrawsonDesign/Robotics_Cape_Installer
 */ 

#include <fcntl.h>
#include <sys/mman.h>

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid") || 0 == commandlineArguments.count("freq") || 0 == commandlineArguments.count("channel")) {
    std::cerr << argv[0] << " interfaces to the analog-to-digital converters on the BeagleBone Blue." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --freq=<frequency> --cid=<OpenDaVINCI session> --channel=<the ADC channel to read> [--id=<Identifier in case of multiple sensors] [--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --freq=10 --cid=111 --channel=0" << std::endl;
    retCode = 1;
  } else {
    uint32_t const ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
    bool const VERBOSE{commandlineArguments.count("verbose") != 0};
    uint16_t const CID = std::stoi(commandlineArguments["cid"]);
    uint32_t const CHANNEL = std::stoi(commandlineArguments["channel"]);
    float const FREQ = std::stof(commandlineArguments["freq"]);

    int32_t fd = open("/dev/mem", O_RDWR);
    if (fd == -1) {
      std::cerr << "Unable to open /dev/mem" << std::endl;
      return -1;
    }

    uint32_t const MMAP_OFFSET{0x44C00000};
    uint32_t const MMAP_SIZE{0x481AEFFF - MMAP_OFFSET};

    uint32_t *map = static_cast<uint32_t *>(mmap(nullptr, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MMAP_OFFSET));
    if(map == MAP_FAILED) {
      close(fd);
      std::cerr << "Unable to map /dev/mem" << std::endl;
      return -1;
    }

    uint32_t const CM_WKUP{0x44E00400};
    uint32_t const CM_WKUP_ADC_TSC_CLKCTRL{0xBC};
    uint32_t const MODULEMODE_ENABLE{0x2};
    
    map[(CM_WKUP + CM_WKUP_ADC_TSC_CLKCTRL - MMAP_OFFSET) / 4] |= MODULEMODE_ENABLE;

    while (!(map[(CM_WKUP + CM_WKUP_ADC_TSC_CLKCTRL - MMAP_OFFSET) / 4] & MODULEMODE_ENABLE)) {
      std::this_thread::sleep_for(std::chrono::duration<double>(1e-5));
      if (VERBOSE) {
        std::cout << "Waiting for CM_WKUP_ADC_TSC_CLKCTRL to enable with MODULEMODE_ENABLE" << std::endl;
      }
    }

    uint32_t const ADC_TSC{0x44E0D000};
    uint32_t const ADC_CTRL{ADC_TSC + 0x40};
    uint32_t const ADC_STEPCONFIG_WRITE_PROTECT_OFF{0x01 << 2};
    uint32_t const ADC_STEPENABLE{ADC_TSC + 0x54};

    map[(ADC_CTRL - MMAP_OFFSET) / 4] &= !0x01;
    map[(ADC_CTRL - MMAP_OFFSET) / 4] |= ADC_STEPCONFIG_WRITE_PROTECT_OFF;

    uint32_t const ADCSTEPCONFIG1{ADC_TSC + 0x64};
    uint32_t const ADCSTEPDELAY1{ADC_TSC + 0x68};
    uint32_t const ADCSTEPCONFIG2{ADC_TSC + 0x6C};
    uint32_t const ADCSTEPDELAY2{ADC_TSC + 0x70};
    uint32_t const ADCSTEPCONFIG3{ADC_TSC + 0x74};
    uint32_t const ADCSTEPDELAY3{ADC_TSC + 0x78};
    uint32_t const ADCSTEPCONFIG4{ADC_TSC + 0x7C};
    uint32_t const ADCSTEPDELAY4{ADC_TSC + 0x80};
    uint32_t const ADCSTEPCONFIG5{ADC_TSC + 0x84};
    uint32_t const ADCSTEPDELAY5{ADC_TSC + 0x88};
    uint32_t const ADCSTEPCONFIG6{ADC_TSC + 0x8C};
    uint32_t const ADCSTEPDELAY6{ADC_TSC + 0x90};
    uint32_t const ADCSTEPCONFIG7{ADC_TSC + 0x94};
    uint32_t const ADCSTEPDELAY7{ADC_TSC + 0x98};
    uint32_t const ADCSTEPCONFIG8{ADC_TSC + 0x9C};
    uint32_t const ADCSTEPDELAY8{ADC_TSC + 0xA0};

    uint32_t const ADC_AVG8{0b011 << 2};

    uint32_t const ADC_SW_ONESHOT{0b00};

    map[(ADCSTEPCONFIG1 - MMAP_OFFSET) / 4] = 0x00 << 19 | ADC_AVG8 | ADC_SW_ONESHOT;
    map[(ADCSTEPDELAY1 - MMAP_OFFSET) / 4]  = 0 << 24;
    map[(ADCSTEPCONFIG2 - MMAP_OFFSET) / 4] = 0x01 << 19 | ADC_AVG8 | ADC_SW_ONESHOT;
    map[(ADCSTEPDELAY2 - MMAP_OFFSET) / 4]  = 0 << 24;
    map[(ADCSTEPCONFIG3 - MMAP_OFFSET) / 4] = 0x02 << 19 | ADC_AVG8 | ADC_SW_ONESHOT;
    map[(ADCSTEPDELAY3 - MMAP_OFFSET) / 4]  = 0 << 24;
    map[(ADCSTEPCONFIG4 - MMAP_OFFSET) / 4] = 0x03 << 19 | ADC_AVG8 | ADC_SW_ONESHOT;
    map[(ADCSTEPDELAY4 - MMAP_OFFSET) / 4]  = 0 << 24;
    map[(ADCSTEPCONFIG5 - MMAP_OFFSET) / 4] = 0x04 << 19 | ADC_AVG8 | ADC_SW_ONESHOT;
    map[(ADCSTEPDELAY5 - MMAP_OFFSET) / 4]  = 0 << 24;
    map[(ADCSTEPCONFIG6 - MMAP_OFFSET) / 4] = 0x05 << 19 | ADC_AVG8 | ADC_SW_ONESHOT;
    map[(ADCSTEPDELAY6 - MMAP_OFFSET) / 4]  = 0 << 24;
    map[(ADCSTEPCONFIG7 - MMAP_OFFSET) / 4] = 0x06 << 19 | ADC_AVG8 | ADC_SW_ONESHOT;
    map[(ADCSTEPDELAY7 - MMAP_OFFSET) / 4]  = 0 << 24;
    map[(ADCSTEPCONFIG8 - MMAP_OFFSET) / 4] = 0x07 << 19 | ADC_AVG8 | ADC_SW_ONESHOT;
    map[(ADCSTEPDELAY8 - MMAP_OFFSET) / 4]  = 0 << 24;

    map[(ADC_CTRL - MMAP_OFFSET) / 4] |= 0x01;

    cluon::OD4Session od4{CID};


    uint32_t const FIFO0COUNT{ADC_TSC + 0xE4};
    uint32_t const FIFO_COUNT_MASK{0b01111111};

    uint32_t const ADC_FIFO0DATA{ADC_TSC + 0x100};
    uint32_t const ADC_FIFO_MASK{0xFFF};

    auto atFrequency{[&map, &FIFO0COUNT, &MMAP_OFFSET, &FIFO_COUNT_MASK, &ADC_FIFO0DATA, &ADC_FIFO_MASK, &ADC_STEPENABLE, &CHANNEL, &ID, &VERBOSE, &od4]() -> bool
      {
        int32_t output;
        while (map[(FIFO0COUNT - MMAP_OFFSET) / 4] & FIFO_COUNT_MASK) {
          output =  map[(ADC_FIFO0DATA - MMAP_OFFSET) / 4] & ADC_FIFO_MASK;
        }

        map[(ADC_STEPENABLE - MMAP_OFFSET) / 4] |= (0x01 << (CHANNEL + 1));

        while(!(map[(FIFO0COUNT - MMAP_OFFSET) / 4] & FIFO_COUNT_MASK)) {
        }

        output =  map[(ADC_FIFO0DATA - MMAP_OFFSET) / 4] & ADC_FIFO_MASK;
        float voltage = output * 1.8f / 4095.0f;

        opendlv::proxy::VoltageReading voltageReading;
        voltageReading.voltage(voltage);

        cluon::data::TimeStamp sampleTime;
        od4.send(voltageReading, sampleTime, ID);
        if (VERBOSE) {
          std::cout << "Voltage reading is " << voltageReading.voltage() << " V." << std::endl;
        }
        return true;
      }};

    od4.timeTrigger(FREQ, atFrequency);
  }
  return retCode;
}
