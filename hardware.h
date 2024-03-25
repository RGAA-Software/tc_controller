//
// Created by RGAA on 2024-01-18.
//

#ifndef PLC_CONTROLLER_HARDWARE_H
#define PLC_CONTROLLER_HARDWARE_H

#include <string>
#include <vector>

namespace tc
{

    class HwCPU {
    public:
        std::string id_;
    };

    class HwDisk {
    public:
        std::string name_;
        std::string model_;
        std::string status_;
        std::string serial_number_;
        std::string interface_type_;
    };

    class Hardware {
    public:

        static Hardware* Instance() {
            static Hardware hw;
            return &hw;
        }

        void Detect();
        void Dump();

    private:
        int DetectCpuDisk();
        void DetectMac();

    public:

        HwCPU hw_cpu_;
        std::vector<HwDisk> hw_disks_;

    };

}

#endif //PLC_CONTROLLER_HARDWARE_H
