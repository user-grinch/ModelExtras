/*
 *  Credits:
 *           LINK2012 MemoryMgr
 *           LINK2012 Injector
 *           Plugin-SDK
 */

#pragma once
#include "../injector/injector.hpp"

#include <concepts>
#include <cstdint>
#include <string>
#include <unordered_map>

#define WRAPPER __declspec(naked)
#define DEPRECATED __declspec(deprecated)
#define EAXJMP(a) {_asm mov eax, a _asm jmp eax}
#define VARJMP(a) {_asm jmp a}
#define WRAPARG(a) UNREFERENCED_PARAMETER(a)

#define NOVMT __declspec(novtable)
#define SETVMT(a) *((DWORD_PTR *)this) = (DWORD_PTR)a

enum class ePatchType
{
    Call,
    Jump,
    Nothing,
};

namespace Memory
{
constexpr size_t BUFFER_SIZE = 32;

std::string Addr2Bytes(uintptr_t address)
{
    unsigned char buffer[sizeof(address)];
    std::memcpy(buffer, &address, sizeof(address)); // Copy address bytes into buffer

    std::ostringstream oss;
    for (size_t i = 0; i < sizeof(buffer); ++i)
    {
        oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }

    return oss.str();
}

struct SnapShot
{
  private:
    bool saved, vp;
    uintptr_t address;
    size_t size;
    std::string original = "";
    std::string patch = "";

  public:
    SnapShot(uintptr_t address, size_t size, const std::string &patch = "", bool vp = true)
    {
        this->saved = false;
        this->address = address;
        this->size = size;
        this->vp = vp;
        this->original.resize(BUFFER_SIZE);
        this->patch.resize(BUFFER_SIZE);
        injector::ReadMemoryRaw(address, original.data(), size, vp);

        if (patch == "")
        {
            this->patch = std::string(size, '\x90');
        }
        else
        {
            this->patch = patch;
        }
    }

    SnapShot(uintptr_t address, size_t size, uintptr_t patchAddr, bool vp = true)
    {
        this->saved = false;
        this->address = address;
        this->size = size;
        this->vp = vp;
        this->original.resize(BUFFER_SIZE);
        this->patch.resize(BUFFER_SIZE);
        injector::ReadMemoryRaw(address, original.data(), size, vp);
        injector::ReadMemoryRaw(patchAddr, patch.data(), size, vp);
    }

    void Clear()
    {
        original.clear();
        patch.clear();
    }

    void Update(bool patch)
    {
        if (patch)
        {
            Patch();
        }
        else
        {
            Restore();
        }
    }

    void Patch()
    {
        injector::WriteMemoryRaw(address, patch.data(), size, vp);
    }

    void Restore()
    {
        injector::WriteMemoryRaw(address, original.data(), size, vp);
    }

    void Save()
    {
        injector::ReadMemoryRaw(address, original.data(), size, vp);
    }
};

template <typename AT> inline AT DynBaseAddress(AT address)
{
    return (AT)GetModuleHandle(nullptr) - 0x400000 + address;
}

template <typename T, typename AT> inline void Patch(AT address, T value)
{
    DWORD dwProtect[2];
    VirtualProtect((void *)address, sizeof(T), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
    *(T *)address = value;
    VirtualProtect((void *)address, sizeof(T), dwProtect[0], &dwProtect[1]);
}

template <typename AT> inline void Nop(AT address, unsigned int nCount)
{
    DWORD dwProtect[2];
    VirtualProtect((void *)address, nCount, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
    memset((void *)address, 0x90, nCount);
    VirtualProtect((void *)address, nCount, dwProtect[0], &dwProtect[1]);
}

template <typename AT, typename HT> inline void Hook(AT address, HT hook, ePatchType nType = ePatchType::Nothing)
{
    DWORD dwProtect[2];
    switch (nType)
    {
    case ePatchType::Jump:
        VirtualProtect((void *)address, 5, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
        *(BYTE *)address = 0xE9;
        break;
    case ePatchType::Call:
        VirtualProtect((void *)address, 5, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
        *(BYTE *)address = 0xE8;
        break;
    default:
        VirtualProtect((void *)((DWORD)address + 1), 4, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
        break;
    }
    DWORD dwHook;
    _asm
        {
            mov		eax, hook
            mov		dwHook, eax
        }

    *(ptrdiff_t *)((DWORD)address + 1) = (DWORD)dwHook - (DWORD)address - 5;
    if (nType == ePatchType::Nothing)
        VirtualProtect((void *)((DWORD)address + 1), 4, dwProtect[0], &dwProtect[1]);
    else
        VirtualProtect((void *)address, 5, dwProtect[0], &dwProtect[1]);
}
inline void ExtractCall(void *dst, uintptr_t a)
{
    *(uintptr_t *)dst = (uintptr_t)(*(uintptr_t *)(a + 1) + a + 5);
}
template <typename T> inline void InterceptCall(void *dst, T func, uintptr_t a)
{
    ExtractCall(dst, a);
    Hook(a, func);
}
template <typename T> inline void InterceptVmethod(void *dst, T func, uintptr_t a)
{
    *(uintptr_t *)dst = *(uintptr_t *)a;
    Patch(a, func);
}

inline void MakeCALL(uintptr_t address, void *func, bool vp = true)
{
    injector::MakeCALL(DynBaseAddress(address), func, vp);
}

inline void MakeJMP(uintptr_t address, void *func, bool vp = true)
{
    injector::MakeJMP(DynBaseAddress(address), func, vp);
}

inline void GetRaw(uintptr_t address, const char *value, size_t size, bool vp = true)
{
    injector::ReadMemoryRaw(address, (void *)value, size, vp);
}

inline void SetRaw(uintptr_t address, const char *value, size_t size, bool vp = true)
{
    injector::WriteMemoryRaw(address, (void *)value, size, vp);
}

inline void MoveUp(uintptr_t address, size_t size, size_t offset, bool vp = true)
{
    char buf[BUFFER_SIZE];
    GetRaw(address, buf, size, vp);
    SetRaw(address - offset, buf, size, vp);
}

inline void MoveDown(uintptr_t address, size_t size, size_t offset, bool vp = true)
{
    char buf[BUFFER_SIZE];
    GetRaw(address, buf, size, vp);
    SetRaw(address + offset, buf, size, vp);
}

template <uintptr_t address, typename... Args> inline void Call(Args... args)
{
    reinterpret_cast<void(__cdecl *)(Args...)>(address)(args...);
}

template <uintptr_t address, typename... Args> inline void CallStd(Args... args)
{
    reinterpret_cast<void(__stdcall *)(Args...)>(address)(args...);
}

template <typename Ret, uintptr_t address, typename... Args> inline Ret CallStdAndReturn(Args... args)
{
    return reinterpret_cast<Ret(__stdcall *)(Args...)>(address)(args...);
}

template <typename Ret, uintptr_t address, typename... Args> inline Ret CallAndReturn(Args... args)
{
    return reinterpret_cast<Ret(__cdecl *)(Args...)>(address)(args...);
}

template <uintptr_t address, typename C, typename... Args> inline void CallMethod(C _this, Args... args)
{
    reinterpret_cast<void(__thiscall *)(C, Args...)>(address)(_this, args...);
}

template <typename Ret, uintptr_t address, typename C, typename... Args>
inline Ret CallMethodAndReturn(C _this, Args... args)
{
    return reinterpret_cast<Ret(__thiscall *)(C, Args...)>(address)(_this, args...);
}

template <uintptr_t tableIndex, typename C, typename... Args> inline void CallVirtualMethod(C _this, Args... args)
{
    reinterpret_cast<void(__thiscall *)(C, Args...)>((*reinterpret_cast<void ***>(_this))[tableIndex])(_this, args...);
}

template <typename Ret, uintptr_t tableIndex, typename C, typename... Args>
inline Ret CallVirtualMethodAndReturn(C _this, Args... args)
{
    return reinterpret_cast<Ret(__thiscall *)(C, Args...)>((*reinterpret_cast<void ***>(_this))[tableIndex])(_this,
                                                                                                             args...);
}

template <uintptr_t at, uintptr_t end, class FuncT> inline void Inline(FuncT func)
{
    static std::unique_ptr<FuncT> static_func;
    static_func.reset(new FuncT(std::move(func)));

    // Encapsulates the call to static_func
    struct Caps
    {
        void operator()(injector::reg_pack &regs)
        {
            (*static_func)(regs);
        }
    };

    // Does the actual MakeInline
    return MakeInline<Caps>(injector::lazy_pointer<at>::get(), injector::lazy_pointer<end>::get());
}
}; // namespace Memory
