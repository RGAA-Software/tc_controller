//
// Created by hy on 2024/3/23.
//

#ifndef TC_APPLICATION_DRIVER_MANAGER_H
#define TC_APPLICATION_DRIVER_MANAGER_H

#include <memory>

namespace tc
{

    class Hardware;

    class VigemDriverManager {
    public:

        static std::shared_ptr<VigemDriverManager> Make();

        VigemDriverManager();
        void Init();

    private:
        std::shared_ptr<Hardware> hardware_ = nullptr;

    };

}

#endif //TC_APPLICATION_DRIVER_MANAGER_H
