#pragma once

#define REBASE_ADDRESS(base) ((uintptr_t)(base) + GetBaseAddress())

typedef int (*sceKernelDebugOutText_t)(int channel, const char* text);
inline sceKernelDebugOutText_t sceKernelDebugOutText = nullptr;


inline uint64_t g_execStart = 0;
inline uint64_t g_execEnd = 0;
inline uint64_t g_moduleEnd = 0;



uint64_t GetBaseAddress();
size_t CodeScanSize();
size_t DataScanSize();



uintptr_t ReadableEnd(uintptr_t addr, size_t need);
bool RangeReadable(uintptr_t addr, size_t span);
bool InExec(uintptr_t addr, size_t span);
bool InModule(uintptr_t addr, size_t span);
bool SafePtr(uintptr_t p, size_t len);
bool hasValidOffset(uint64_t address);
bool QuickPtr(uintptr_t p);
bool SafeStrStr(uintptr_t addr, const char* target, size_t maxScan = 64);



int HexNibble(char c);
uintptr_t AobScan(const char* pattern, bool lazy = false, uintptr_t baseOverride = 0, size_t scanSize = 0x10000000);
uintptr_t FindString(const char* str, bool lazy = false, uintptr_t baseOverride = 0, size_t scanSize = 0x10000000);
uintptr_t FindStringXref(const char* str, bool lazy = false, uintptr_t baseOverride = 0, size_t scanSize = 0x10000000);
uintptr_t ResolveRip(uintptr_t insn, int dispOffset, int insnLen);
uintptr_t FindFuncStart(uintptr_t addr, size_t maxBack = 0x1000);
uintptr_t ResolveCall(uintptr_t callInsn);
uintptr_t FindCalleeAfterString(const char* str, size_t window = 0x40);
uintptr_t ResolveTailCallee(uintptr_t func, size_t maxLen = 0x400, size_t nearWindow = 0x20000);
uintptr_t ResolveFirstCallee(uintptr_t func, size_t maxLen = 0x40);



uintptr_t FindSetText();
void ResolveTextPatch(uintptr_t setText);



void DbgPrint(const char* fmt, ...);

void DetectAndNotifyGame();

template <typename T>
void PatchValue(uintptr_t addr, T value)
{
    sceKernelMprotect((const void *)addr, sizeof(T), 7);
    *(volatile T *)addr = value;
}
