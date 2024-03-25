//
// Created by hy on 2024/3/23.
//

#include "vigem_driver_manager.h"

#include "hardware.h"

namespace tc
{

    std::shared_ptr<VigemDriverManager> VigemDriverManager::Make() {
        return std::make_shared<VigemDriverManager>();
    }

    VigemDriverManager::VigemDriverManager() {
    }

    void VigemDriverManager::Init() {
        hardware_ = std::make_shared<Hardware>();
        hardware_->Detect();
    }

}