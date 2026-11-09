#pragma once

// Longest prologue we will ever overwrite. The patch is instruction aligned,
// so a 14 byte absolute jump can grow past 14 bytes when the last instruction
// it covers straddles the end.
#define DETOUR_MAX_PATCH 32

typedef struct _GHSDK_Detour
{
    void* FunctionPtr;
    void* HookPtr;
    void* StubPtr;                      // call this to reach the original function
    void* MapPtr;                       // one page: stub, jump back, bridge
    size_t MapSize;
    size_t PatchSize;                   // bytes overwritten at FunctionPtr
    uint8_t Original[DETOUR_MAX_PATCH]; // exact bytes to put back on detach
    bool Installed;
} Detour;

// OutStub receives the stub pointer before the target is patched, so a hook
// that fires immediately always has a live original to call through.
void* Detour_Attach(Detour* This, uint64_t FunctionPtr, void* HookPtr, void** OutStub);
void Detour_Detach(Detour* This);

#define DECLARE_DETOUR(name) Detour name##_d{};

#define CreateDetour(name, offset)                                          \
    do {                                                                    \
        if (hasValidOffset(offset))                                         \
        {                                                                   \
            Detour_Attach(&name##_d, (uint64_t)(offset),                    \
                (void*)name##_h, (void**)&name##_s);                        \
                                                                            \
            if (name##_s == 0)                                              \
            {                                                               \
                DbgPrint("[Detour] FAILED %s at 0x%llX",                    \
                    #name, (unsigned long long)(offset));                   \
            }                                                               \
            else                                                            \
            {                                                               \
                DbgPrint("[Detour] hooked %s at 0x%llX -> 0x%llX "          \
                    "(stub 0x%llX, %zu bytes patched)",                     \
                    #name,                                                  \
                    (unsigned long long)(offset),                           \
                    (unsigned long long)(uintptr_t)name##_h,                \
                    (unsigned long long)(uintptr_t)name##_s,                \
                    name##_d.PatchSize);                                    \
            }                                                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
            DbgPrint("[Detour] skipped %s - invalid offset 0x%llX",         \
                #name, (unsigned long long)(offset));                       \
        }                                                                   \
    } while (0)

// The jump width is chosen per target now, so these only exist so older call
// sites keep compiling.
#define CreateDetour32(name, offset) CreateDetour(name, offset)
#define CreateDetour64(name, offset) CreateDetour(name, offset)

// Takes an optional second argument so both RemoveDetour(name) and the older
// RemoveDetour(name, offset) call sites compile. The offset is ignored - the
// struct already knows the address and Detour_Detach logs it.
#define RemoveDetour(name, ...)                                             \
    do {                                                                    \
        if (name##_d.Installed)                                             \
        {                                                                   \
            Detour_Detach(&name##_d);                                       \
            name##_s = 0;                                                   \
        }                                                                   \
    } while (0)