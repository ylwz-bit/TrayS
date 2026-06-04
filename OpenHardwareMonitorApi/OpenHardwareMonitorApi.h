#pragma once
#include <memory>
#include "OpenHardwareMonitorGlobal.h"
#include <map>
#include <string>

namespace OpenHardwareMonitorApi
{
    class IOpenHardwareMonitor
    {
    public:
        virtual void GetHardwareInfo() = 0;
        virtual float CpuTemperature() = 0;
        virtual float GpuTemperature() = 0;
        virtual float HDDTemperature() = 0;
        virtual float MainboardTemperature() = 0;
        virtual float GpuUsage() = 0;
        virtual const std::map<std::wstring, float>& AllHDDTemperature() = 0;
        virtual const std::map<std::wstring, float>& AllCpuTemperature() = 0;
        virtual const std::map<std::wstring, float>& AllHDDUsage() = 0;

        virtual void SetCpuEnable(bool enable) = 0;
        virtual void SetGpuEnable(bool enable) = 0;
        virtual void SetHddEnable(bool enable) = 0;
        virtual void SetMainboardEnable(bool enable) = 0;
    };

    std::shared_ptr<IOpenHardwareMonitor> CreateInstance();
}
std::shared_ptr<OpenHardwareMonitorApi::IOpenHardwareMonitor> m_pMonitor{};
extern "C" OPENHARDWAREMONITOR_API void GetTemperature(float* fCpu,float * fGpu,float* fMain,float *fHdd,int iHDD,float * fCpuPackge)
{
    if (m_pMonitor == 0)
    {
        m_pMonitor = OpenHardwareMonitorApi::CreateInstance();
        if(fCpu)
            m_pMonitor->SetCpuEnable(true);
        if (fGpu)
            m_pMonitor->SetGpuEnable(true);
        if (fHdd)
            m_pMonitor->SetHddEnable(true);
        if (fMain)
            m_pMonitor->SetMainboardEnable(true);
    }
    m_pMonitor->GetHardwareInfo();
    if (fCpu)
    {
        // 兼容混合架构CPU(P-Core/E-Core): Core Average > CPU Package > 首个传感器
        auto& cpuMap = m_pMonitor->AllCpuTemperature();
        auto iter = cpuMap.end();
        auto it1 = cpuMap.find(L"Core Average");
        if (it1 != cpuMap.end()) iter = it1;
        if (iter == cpuMap.end())
        {
            auto it2 = cpuMap.find(L"CPU Package");
            if (it2 != cpuMap.end()) iter = it2;
        }
        if (iter == cpuMap.end() && !cpuMap.empty())
            iter = cpuMap.begin();
        if (iter != cpuMap.end())
            *fCpu = iter->second;
    }
    if (fCpuPackge)
    {
        auto& cpuMap = m_pMonitor->AllCpuTemperature();
        auto iter = cpuMap.find(L"CPU Package");
        if (iter == cpuMap.end())
        {
            if (!cpuMap.empty())
            {
                iter = cpuMap.begin();
                auto it2 = cpuMap.find(L"Core Average");
                if (it2 != cpuMap.end()) iter = it2;
            }
        }
        if (iter != cpuMap.end())
            *fCpuPackge = iter->second;
    }
    if (fGpu)
        *fGpu = m_pMonitor->GpuTemperature();
    if (fMain)
        *fMain = m_pMonitor->MainboardTemperature();
    if (fHdd)
    {
        auto& hddMap = m_pMonitor->AllHDDTemperature();
        auto iter = hddMap.begin();
        if (iHDD == -1)
        {
            float f = 0;
            for (auto it = hddMap.begin(); it != hddMap.end(); ++it)
            {
                if (it->second > f)
                    f = it->second;
            }
            *fHdd = f;
        }
        else
        {
            for (int i = 0; i < iHDD && iter != hddMap.end(); i++)
            {
                ++iter;
            }
            if (iter != hddMap.end())
                *fHdd = iter->second;
        }
    }
}