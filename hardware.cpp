//
// Created by RGAA on 2024-01-18.
//

#include "hardware.h"

#include <iostream>
#include <Windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include "tc_common_new/log.h"
#include "tc_common_new/string_ext.h"

#pragma comment(lib, "wbemuuid.lib")

namespace tc
{

//    [2024-03-25 11:09:06.238][thread 47940][hardware.cpp:248,tc::Hardware::DetectCpuDisk][info] : Driver name: ViGEmBus
//    [2024-03-25 11:09:06.238][thread 47940][hardware.cpp:256,tc::Hardware::DetectCpuDisk][info] : Driver display name: Nefarius Virtual Gamepad Emulation Service
//    [2024-03-25 11:09:06.238][thread 47940][hardware.cpp:248,tc::Hardware::DetectCpuDisk][info] : Driver name: vjoy
//    [2024-03-25 11:09:06.238][thread 47940][hardware.cpp:256,tc::Hardware::DetectCpuDisk][info] : Driver display name: vJoy Device

    void Hardware::Detect() {
        this->DetectCpuDisk();
        //this->DetectMac();
    }

    int Hardware::DetectCpuDisk() {
        HRESULT hres;

        // 初始化COM库
        CoUninitialize();
        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres)) {
            std::cout << "COM library initialization failed." << std::endl;
            return 1;
        }

        // 初始化安全
        hres = CoInitializeSecurity(
                NULL,
                -1,
                NULL,
                NULL,
                RPC_C_AUTHN_LEVEL_DEFAULT,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_NONE,
                NULL
        );
        if (FAILED(hres)) {
            std::cout << "Failed to initialize security." << std::endl;
            CoUninitialize();
            return 1;
        }

        // 创建WMI接口
        IWbemLocator *pLoc = NULL;
        hres = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator,
                (LPVOID *) &pLoc
        );
        if (FAILED(hres)) {
            std::cout << "Failed to create IWbemLocator object." << std::endl;
            CoUninitialize();
            return 1;
        }

        IWbemServices *pSvc = NULL;
        // 使用WMI连接到CIMV2命名空间
        hres = pLoc->ConnectServer(
                _bstr_t(L"ROOT\\CIMV2"),
                NULL,
                NULL,
                0,
                NULL,
                0,
                0,
                &pSvc
        );
        if (FAILED(hres)) {
            std::cout << "Could not connect to WMI server." << std::endl;
            pLoc->Release();
            CoUninitialize();
            return 1;
        }

        std::cout << "Connected to WMI server." << std::endl;

        // 设置安全
        hres = CoSetProxyBlanket(
                pSvc,
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHZ_NONE,
                NULL,
                RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_NONE
        );
        if (FAILED(hres)) {
            std::cout << "Could not set proxy blanket." << std::endl;
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return 1;
        }

        // 获取CPU序列号
        IEnumWbemClassObject *pEnumerator = NULL;
        hres = pSvc->ExecQuery(
                bstr_t("WQL"),
                bstr_t("SELECT * FROM Win32_Processor"),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pEnumerator
        );

        if (FAILED(hres)) {
            std::cout << "Query for processor information failed." << std::endl;
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return 1;
        }

        // 获取数据
        IWbemClassObject *pclsObj = NULL;
        ULONG uReturn = 0;
        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn) {
                break;
            }

            VARIANT vtProp;
            hr = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);  // "ProcessorId" 是属性名，不同的信息需要查询不同的属性名
            if (SUCCEEDED(hr)) {
                std::wcout << "CPU ID : " << vtProp.bstrVal << std::endl;
                hw_cpu_.id_ = StringExt::ToUTF8(vtProp.bstrVal);
            }
            VariantClear(&vtProp);

            pclsObj->Release();
        }

        //////
        hres = pSvc->ExecQuery(
                bstr_t("WQL"),
                bstr_t("SELECT * FROM Win32_DiskDrive"),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pEnumerator);

        if (FAILED(hres)) {
            std::cout << "Query for operating system name failed."
                      << " Error code = 0x"
                      << std::hex << hres << std::endl;
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return 1; // Program has failed.
        }


        uReturn = 0;
        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
                                           &pclsObj, &uReturn);
            if (0 == uReturn || FAILED(hr)) {
                break;
            }

            HwDisk disk;

            VARIANT vtProp;

//            std::cout << "----------------------------------" << std::endl;
//            std::cout << "Retrieve DISK Info" << std::endl;
//            std::cout << "----------------------------------" << std::endl;
            // Get the value of the Name property
            hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
//            std::wcout << " Disk Name : " << vtProp.bstrVal << std::endl;
            disk.name_ = StringExt::ToUTF8(vtProp.bstrVal);
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
//            std::wcout << " Disk Model : " << vtProp.bstrVal << std::endl;
            disk.model_ = StringExt::ToUTF8(vtProp.bstrVal);
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"Status", 0, &vtProp, 0, 0);
//            std::wcout << " Status : " << vtProp.bstrVal << std::endl;
            disk.status_ = StringExt::ToUTF8(vtProp.bstrVal);
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
//            std::wcout << " Device ID : " << vtProp.bstrVal << std::endl;
            VariantClear(&vtProp);


            hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
//            std::wcout << " SerialNumber : " << vtProp.bstrVal << std::endl;
            disk.serial_number_ = StringExt::ToUTF8(vtProp.bstrVal);
            VariantClear(&vtProp);

            //InterfaceType
            hr = pclsObj->Get(L"InterfaceType", 0, &vtProp, 0, 0);
            disk.interface_type_ = StringExt::ToUTF8(vtProp.bstrVal);
            VariantClear(&vtProp);

            pclsObj->Release();

            if (disk.interface_type_ == "IDE") {
                hw_disks_.push_back(disk);
            }
        }

        // system driver
        hres = pSvc->ExecQuery(
                bstr_t("WQL"),
                bstr_t("SELECT * FROM Win32_SystemDriver"),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pEnumerator);

        if (FAILED(hres)) {
            std::cout << "Query for operating system name failed."
                      << " Error code = 0x"
                      << std::hex << hres << std::endl;
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return 1; // Program has failed.
        }

        uReturn = 0;
        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,&pclsObj, &uReturn);
            if (0 == uReturn || FAILED(hr)) {
                break;
            }
            VARIANT vtProp;
            hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
            if (FAILED(hr)) {
                continue;
            }
            auto name = StringExt::ToUTF8(vtProp.bstrVal);
            LOGI("Driver name: {}", name);
            VariantClear(&vtProp);

            hr = pclsObj->Get(L"DisplayName", 0, &vtProp, 0, 0);
            if (FAILED(hr)) {
                continue;
            }
            auto display_name = StringExt::ToUTF8(vtProp.bstrVal);
            LOGI("Driver display name: {}", display_name);
            VariantClear(&vtProp);

            pclsObj->Release();
        }

        // 清理资源
        pSvc->Release();
        pLoc->Release();
        pEnumerator->Release();
        CoUninitialize();

        return 0;
    }

    void Hardware::Dump() {
        LOGI("==> CPU id: {}", hw_cpu_.id_);
        LOGI("==> Total disks: {}", hw_disks_.size());
        for (auto &disk: hw_disks_) {
            LOGI("  name: {}", disk.name_);
            LOGI("  model: {}", disk.model_);
            LOGI("  status: {}", disk.status_);
            LOGI("  serial number: {}", disk.serial_number_);
            LOGI("  interface type: {}", disk.interface_type_);
            LOGI("-------------------------------------");
        }
    }

    void Hardware::DetectMac() {
//        QList<QNetworkInterface> nets = QNetworkInterface::allInterfaces();// 获取所有网络接口列表
//        int nCnt = nets.count();
//        QString strMacAddr = "";
//        for(int i = 0; i < nCnt; i ++)
//        {
//            // 如果此网络接口被激活并且正在运行并且不是回环地址，则就是我们需要找的Mac地址
//            if(nets[i].flags().testFlag(QNetworkInterface::IsUp)
//                && nets[i].flags().testFlag(QNetworkInterface::IsRunning)
//                && !nets[i].flags().testFlag(QNetworkInterface::IsLoopBack))
//            {
//                strMacAddr = nets[i].hardwareAddress();
//                LOGI("MAC: {}", strMacAddr.toStdString());
//                //break;
//            }
//        }
        //return strMacAddr;
    }

}