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
        if (m_pMonitor)
        {
            if(fCpu)
                m_pMonitor->SetCpuEnable(true);
            if (fGpu)
                m_pMonitor->SetGpuEnable(true);
            if (fHdd)
                m_pMonitor->SetHddEnable(true);
            if (fMain)
                m_pMonitor->SetMainboardEnable(true);
        }
    }
    if (!m_pMonitor)
        return;
    m_pMonitor->GetHardwareInfo();
    auto& cpuTemps = m_pMonitor->AllCpuTemperature();
    if(fCpu)
    {
        *fCpu = 0;
        if(!cpuTemps.empty())
        {
            // 兼容混合架构(P-Core/E-Core): CPU Core #1 > Core Average > CPU Package > 首个
            auto iter = cpuTemps.find(L"CPU Core #1");
            if(iter == cpuTemps.end())
                iter = cpuTemps.find(L"Core Average");
            if(iter == cpuTemps.end())
                iter = cpuTemps.find(L"CPU Package");
            if(iter == cpuTemps.end())
                iter = cpuTemps.begin();
            if(iter != cpuTemps.end())
                *fCpu = iter->second;
        }
    }
    if(fCpuPackge)
    {
        *fCpuPackge = 0;
        if(!cpuTemps.empty())
        {
            auto iter = cpuTemps.find(L"CPU Package");
            if(iter != cpuTemps.end())
                *fCpuPackge = iter->second;
            else
            {
                auto iter2 = cpuTemps.find(L"Core Average");
                if(iter2 != cpuTemps.end())
                    *fCpuPackge = iter2->second;
                else if(!cpuTemps.empty())
                {
                    iter = cpuTemps.begin();
                    *fCpuPackge = iter->second;
                }
            }
        }
    }
    if(fGpu)
        *fGpu = m_pMonitor->GpuTemperature();
    if(fMain)
        *fMain = m_pMonitor->MainboardTemperature();
    if(fHdd)
    {
        *fHdd = 0;
        auto& hddTemps = m_pMonitor->AllHDDTemperature();
        if(!hddTemps.empty())
        {
            if(iHDD == -1)
            {
                float f = 0;
                for(auto& item : hddTemps)
                {
                    if(item.second > f)
                        f = item.second;
                }
                *fHdd = f;
            }
            else
            {
                auto iter = hddTemps.begin();
                for(int i = 0; i < iHDD && iter != hddTemps.end(); i++)
                    ++iter;
                if(iter != hddTemps.end())
                    *fHdd = iter->second;
            }
        }
    }
}