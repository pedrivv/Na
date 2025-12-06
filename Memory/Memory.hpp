#pragma once

#include <Windows.h>
#include <unordered_map>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

class MemoryUtils {
public:
    inline static void* vmPtr = nullptr;
    inline static void* pVMAddr = nullptr;
    inline static void* cpuAddr = nullptr;

    inline static std::unordered_map<uintptr_t, uintptr_t> Cache;

    using PGMPhysReadFunc = int(__cdecl*)(void*, uintptr_t, void*, size_t);
    using VMMGetCpuByIdFunc = void* (__cdecl*)(void*, int);
    using PGMPhysGCPtr2GCPhysFunc = int(__cdecl*)(void*, uintptr_t, uintptr_t*);
    using PGMPhysSimpleWriteGCPhysFunc = int(__cdecl*)(void*, uintptr_t, void*, size_t);

    inline static PGMPhysReadFunc ogPhysRead = nullptr;
    inline static VMMGetCpuByIdFunc ogCPU = nullptr;
    inline static PGMPhysGCPtr2GCPhysFunc ogCast = nullptr;
    inline static PGMPhysSimpleWriteGCPhysFunc ogWrite = nullptr;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
        buffer->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static int __cdecl HookedPGMPhysRead(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
        if (!vmPtr) {
            vmPtr = pVM;
            std::cout << "Memory initialized: " << vmPtr << std::endl;
        }
        return ogPhysRead(pVM, GCPhys, pvBuf, cbRead);
    }

    static int HookWrite(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
        return ogWrite(pVM, GCPhys, pvBuf, cbRead);
    }

    static int HookRead(void* pVM, uintptr_t GCPhys, void* pvBuf, size_t cbRead) {
        return ogPhysRead(pVM, GCPhys, pvBuf, cbRead);
    }

    static void* CPU(void* pVM, int cpuId) {
        return ogCPU(pVM, cpuId);
    }

    static int Cast(void* pVCpu, uintptr_t address, uintptr_t* physAddress) {
        return ogCast(pVCpu, address, physAddress);
    }

    static void Initialize(void* pVM) {
        pVMAddr = pVM;
        cpuAddr = CPU(pVM, 0);
        Cache.clear();
    }

    static constexpr uint32_t MAX_CPU = 4U;
    static constexpr uintptr_t MIN_VALID_ADDRESS = 0x1000;

    static bool Convert(uintptr_t address, uintptr_t& phys) {
        phys = 0;
        if (address <= MIN_VALID_ADDRESS) return false;

        auto it = Cache.find(address);
        if (it != Cache.end() && it->second != 0) {
            phys = it->second;
            return true;
        }

        for (uint32_t i = 0; i < MAX_CPU; ++i) {
            void* cpu = CPU(pVMAddr, i);
            if (!cpu) continue;

            uintptr_t tempPhys = 0;
            if (Cast(cpu, address, &tempPhys) == 0) {
                phys = tempPhys;
                Cache[address] = tempPhys;
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool Read(uintptr_t address, T& out) {
        uintptr_t physAddress;
        if (!Convert(address, physAddress)) return false;
        return HookRead(pVMAddr, physAddress, &out, sizeof(T)) == 0;
    }

    template<typename T>
    T Read(uintptr_t address) {
        T result{};
        Read(address, result);
        return result;
    }

    template<typename T>
    static void Write(uintptr_t address, const T& value) {
        uintptr_t physAddress;
        if (Convert(address, physAddress)) {
            HookWrite(pVMAddr, physAddress, (void*)&value, sizeof(T));
        }
    }

    template<typename T>
    static bool ReadFast2(uintptr_t address, T* data) {
        uintptr_t physAddress;
        if (!Convert(address, physAddress)) return false;
        return HookRead(pVMAddr, physAddress, data, sizeof(T)) == 0;
    }

    template<typename T>
    bool ReadArray(uintptr_t address, std::vector<T>& array) {
        uintptr_t convertedAddress;
        bool result = Convert(address, convertedAddress);
        if (!result) {
            return false;
        }

        size_t size = sizeof(T) * array.size();
        DWORD status = HookRead(pVMAddr, convertedAddress, array.data(), size);

        return status == 0;
    }

    std::string utf16_to_utf8(const std::wstring& wstr) {
        std::string str;
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            str.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, nullptr, nullptr);
        }
        return str;
    }

    std::string String(uintptr_t address, int size, bool unicode = true) {
        std::vector<byte> stringBytes(size);
        if (!ReadArray(address, stringBytes)) return "";

        std::string result;
        if (unicode) {
            std::wstring wstr(reinterpret_cast<wchar_t*>(stringBytes.data()), size / 2);
            result = utf16_to_utf8(wstr);
        }
        else {
            result = std::string(reinterpret_cast<char*>(stringBytes.data()), size);
        }

        size_t nullPos = result.find('\0');
        if (nullPos != std::string::npos) {
            result = result.substr(0, nullPos);
        }

        return result;
    }
};
inline MemoryUtils Mem;