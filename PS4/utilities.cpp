#include "headers.hpp"

game g_id = NONE;

uint64_t GetBaseAddress()
{
    static uint64_t cached = 0;
    if (cached)
        return cached;

    SceKernelVirtualQueryInfo info;
    void* address = nullptr;

    while (sceKernelVirtualQuery(address, SCE_KERNEL_VQ_FIND_NEXT, &info, sizeof(info)) >= 0)
    {
        uintptr_t s = (uintptr_t)info.start;
        uintptr_t e = (uintptr_t)info.end;

        if (e <= s)
            break;

        address = (void*)e;

        if (info.protection == 5 && !strcmp(info.name, "executable"))
        {
            cached = s;
            g_execStart = s;
            g_execEnd = e;
            g_moduleEnd = e;

            SceKernelVirtualQueryInfo next;
            uintptr_t probe = e;

            for (int hop = 0; hop < 16; ++hop)
            {
                if (sceKernelVirtualQuery((void*)probe, 0, &next, sizeof(next)) < 0)
                    break;

                uintptr_t ns = (uintptr_t)next.start;
                uintptr_t ne = (uintptr_t)next.end;

                if (ne <= ns)
                    break;
                if (ns > g_moduleEnd)
                    break;
                if ((next.protection & 1) == 0)
                    break;

                g_moduleEnd = ne;
                probe = ne;
            }

            return cached;
        }
    }

    return 0;
}

size_t CodeScanSize()
{
    uintptr_t base = GetBaseAddress();
    if (!base || !g_execEnd || g_execEnd <= base) return 0;
    return (size_t)(g_execEnd - base);
}

size_t DataScanSize()
{
    uintptr_t base = GetBaseAddress();
    if (!base || !g_moduleEnd || g_moduleEnd <= base) return 0;
    return (size_t)(g_moduleEnd - base);
}

uintptr_t ReadableEnd(uintptr_t addr, size_t need)
{
    if (addr < 0x10000 || addr >= 0x0000800000000000ULL)
        return 0;
    if (addr + need < addr)
        return 0;

    SceKernelVirtualQueryInfo info;
    uintptr_t end = 0;
    uintptr_t probe = addr;

    for (int hop = 0; hop < 8; ++hop)
    {
        if (sceKernelVirtualQuery((void*)probe, 0, &info, sizeof(info)) < 0)
            break;

        if ((info.protection & 1) == 0)
            break;

        uintptr_t s = (uintptr_t)info.start;
        uintptr_t e = (uintptr_t)info.end;

        if (e <= s)
            break;
        if (probe < s || probe >= e)
            break;
        if (end && s > end)
            break;

        end = e;
        probe = e;

        if (end >= addr + need)
            break;
    }

    return end;
}

bool RangeReadable(uintptr_t addr, size_t span)
{
    if (!addr || addr + span < addr)
        return false;

    uintptr_t end = ReadableEnd(addr, span);
    return end != 0 && (addr + span) <= end;
}

bool InExec(uintptr_t addr, size_t span)
{
    if (!GetBaseAddress() || !g_execStart || !g_execEnd)
        return false;
    if (addr < g_execStart)
        return false;
    if (addr + span < addr)
        return false;
    return (addr + span) <= g_execEnd;
}

bool InModule(uintptr_t addr, size_t span)
{
    if (!GetBaseAddress() || !g_execStart || !g_moduleEnd)
        return false;
    if (addr < g_execStart)
        return false;
    if (addr + span < addr)
        return false;
    return (addr + span) <= g_moduleEnd;
}

bool SafePtr(uintptr_t p, size_t len)
{
    if (p < 0x10000) return false;
    if (p + len < p) return false;

    static uintptr_t cacheStart = 0;
    static uintptr_t cacheEnd = 0;

    if (cacheEnd && p >= cacheStart && p + len <= cacheEnd)
        return true;

    uintptr_t end = ReadableEnd(p, len);
    if (!end || p + len > end)
        return false;

    cacheStart = p;
    cacheEnd = end;
    return true;
}

bool hasValidOffset(uint64_t address)
{
    return address && address != 0xDEADBEEF && QuickPtr((uintptr_t)address);
}

bool QuickPtr(uintptr_t p)
{
    return p >= 0x10000 && p < 0x0000800000000000ULL;
}

bool SafeStrStr(uintptr_t addr, const char* target, size_t maxScan)
{
    if (!QuickPtr(addr) || !target)
        return false;

    const size_t tlen = strlen(target);
    if (tlen == 0 || tlen > maxScan)
        return false;

    uintptr_t end = ReadableEnd(addr, maxScan);
    if (!end || end <= addr)
        return false;

    size_t avail = (size_t)(end - addr);
    if (avail > maxScan) avail = maxScan;
    if (avail < tlen) return false;

    const char* p = (const char*)addr;

    for (size_t i = 0; i + tlen <= avail; i++)
    {
        if (p[i] == '\0')
            return false;

        size_t j = 0;
        while (j < tlen && p[i + j] == target[j])
            j++;

        if (j == tlen)
            return true;
    }

    return false;
}

int HexNibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

uintptr_t AobScan(const char* pattern, bool lazy, uintptr_t baseOverride, size_t scanSize)
{
    if (!pattern)
        return 0;

    uint8_t bytes[256];
    uint8_t hiMask[256];
    uint8_t loMask[256];
    size_t len = 0;

    const char* p = pattern;
    while (*p && len < 256)
    {
        if (*p == ' ') { ++p; continue; }

        if ((p[0] == '?' || p[0] == '*') && (p[1] == '?' || p[1] == '*' || p[1] == ' ' || p[1] == 0))
        {
            bytes[len] = 0;
            hiMask[len] = 0;
            loMask[len] = 0;
            ++len;
            ++p;
            if (*p == '?' || *p == '*') ++p;
            continue;
        }

        if (p[1] == 0)
            return 0;

        int hi = (p[0] == '?' || p[0] == '*') ? -2 : HexNibble(p[0]);
        int lo = (p[1] == '?' || p[1] == '*') ? -2 : HexNibble(p[1]);

        if (hi == -1 || lo == -1)
            return 0;

        uint8_t val = 0, hm = 0, lm = 0;

        if (hi >= 0) { val |= (uint8_t)(hi << 4); hm = 0xF0; }
        if (lo >= 0) { val |= (uint8_t)lo;        lm = 0x0F; }

        bytes[len] = val;
        hiMask[len] = hm;
        loMask[len] = lm;
        ++len;
        p += 2;
    }

    if (len == 0)
        return 0;

    const uintptr_t base = baseOverride ? baseOverride : GetBaseAddress();
    if (!base)
        return 0;

    if (!baseOverride)
    {
        size_t cs = CodeScanSize();
        if (!cs) return 0;
        if (cs < scanSize) scanSize = cs;
    }
    else if (base >= g_execStart && base < g_execEnd)
    {
        size_t cs = (size_t)(g_execEnd - base);
        if (cs < scanSize) scanSize = cs;
    }

    if (scanSize < len || base + scanSize < base)
        return 0;

    size_t fixedCount = 0;
    for (size_t j = 0; j < len; ++j)
        if (hiMask[j] || loMask[j]) ++fixedCount;

    const size_t tolerance = lazy ? (fixedCount / 8 + 1) : 0;
    const uintptr_t last = base + scanSize - len;

    uintptr_t bestAddr = 0;
    size_t bestMiss = tolerance + 1;
    uintptr_t validEnd = 0;

    for (uintptr_t cur = base; cur <= last; ++cur)
    {
        if (cur + len > validEnd)
        {
            validEnd = ReadableEnd(cur, len);

            if (!validEnd || cur + len > validEnd)
            {
                uintptr_t next = validEnd ? validEnd : ((cur & ~(uintptr_t)0xFFF) + 0x1000);
                if (next <= cur) break;
                cur = next - 1;
                validEnd = 0;
                continue;
            }
        }

        const uint8_t* b = (const uint8_t*)cur;
        size_t miss = 0;

        for (size_t j = 0; j < len; ++j)
        {
            const uint8_t m = hiMask[j] | loMask[j];
            if (m && (b[j] & m) != (bytes[j] & m))
            {
                if (++miss > tolerance)
                    break;
            }
        }

        if (miss == 0)
            return cur;

        if (lazy && miss <= tolerance && miss < bestMiss)
        {
            bestMiss = miss;
            bestAddr = cur;
        }
    }

    return lazy ? bestAddr : 0;
}

uintptr_t FindString(const char* str, bool lazy, uintptr_t baseOverride, size_t scanSize)
{
    if (!str)
        return 0;

    const size_t slen = strlen(str);
    if (slen == 0)
        return 0;

    const uintptr_t base = baseOverride ? baseOverride : GetBaseAddress();
    if (!base)
        return 0;

    if (!baseOverride)
    {
        size_t ds = DataScanSize();
        if (!ds) return 0;
        if (ds < scanSize) scanSize = ds;
    }
    else if (base >= g_execStart && base < g_moduleEnd)
    {
        size_t ds = (size_t)(g_moduleEnd - base);
        if (ds < scanSize) scanSize = ds;
    }

    if (scanSize < slen || base + scanSize < base)
        return 0;

    const uintptr_t last = base + scanSize - slen;
    uintptr_t validEnd = 0;

    for (uintptr_t cur = base; cur <= last; ++cur)
    {
        if (cur + slen > validEnd)
        {
            validEnd = ReadableEnd(cur, slen);

            if (!validEnd || cur + slen > validEnd)
            {
                uintptr_t next = validEnd ? validEnd : ((cur & ~(uintptr_t)0xFFF) + 0x1000);
                if (next <= cur) break;
                cur = next - 1;
                validEnd = 0;
                continue;
            }
        }

        const char* b = (const char*)cur;
        bool hit = true;

        for (size_t j = 0; j < slen; ++j)
        {
            char a = b[j];
            char c = str[j];

            if (lazy)
            {
                if (a >= 'A' && a <= 'Z') a += 32;
                if (c >= 'A' && c <= 'Z') c += 32;
            }

            if (a != c) { hit = false; break; }
        }

        if (hit)
            return cur;
    }

    return 0;
}

uintptr_t FindStringXref(const char* str, bool lazy, uintptr_t baseOverride, size_t scanSize)
{
    const uintptr_t base = baseOverride ? baseOverride : GetBaseAddress();
    if (!base)
        return 0;

    uintptr_t strAddr = FindString(str, lazy, base);
    if (!strAddr)
        return 0;

    if (!baseOverride)
    {
        size_t cs = CodeScanSize();
        if (!cs) return 0;
        if (cs < scanSize) scanSize = cs;
    }
    else if (base >= g_execStart && base < g_execEnd)
    {
        size_t cs = (size_t)(g_execEnd - base);
        if (cs < scanSize) scanSize = cs;
    }

    if (scanSize < 7 || base + scanSize < base)
        return 0;

    const uintptr_t last = base + scanSize - 7;
    uintptr_t validEnd = 0;

    for (uintptr_t cur = base; cur <= last; ++cur)
    {
        if (cur + 7 > validEnd)
        {
            validEnd = ReadableEnd(cur, 7);

            if (!validEnd || cur + 7 > validEnd)
            {
                uintptr_t next = validEnd ? validEnd : ((cur & ~(uintptr_t)0xFFF) + 0x1000);
                if (next <= cur) break;
                cur = next - 1;
                validEnd = 0;
                continue;
            }
        }

        const uint8_t* b = (const uint8_t*)cur;

        if ((b[0] == 0x48 || b[0] == 0x4C) && (b[1] == 0x8D || b[1] == 0x8B) && (b[2] & 0xC7) == 0x05)
        {
            int32_t disp = 0;
            memcpy(&disp, b + 3, 4);

            if ((uintptr_t)(cur + 7 + disp) == strAddr)
                return cur;
        }
    }

    return 0;
}

uintptr_t ResolveRip(uintptr_t insn, int dispOffset, int insnLen)
{
    if (!insn || dispOffset < 0 || insnLen <= 0)
        return 0;

    if (!InExec(insn, (size_t)dispOffset + 4))
        return 0;

    if (!RangeReadable(insn, (size_t)dispOffset + 4))
        return 0;

    int32_t disp = 0;
    memcpy(&disp, (const void*)(insn + dispOffset), 4);

    uintptr_t target = insn + insnLen + disp;

    if (!RangeReadable(target, 1))
        return 0;

    return target;
}

uintptr_t FindFuncStart(uintptr_t addr, size_t maxBack)
{
    if (!addr || !InExec(addr, 1))
        return 0;

    uintptr_t floor = (addr > maxBack) ? (addr - maxBack) : 0;
    if (floor < g_execStart + 1)
        floor = g_execStart + 1;
    if (addr < floor)
        return 0;

    uintptr_t subRspFallback = 0;

    for (uintptr_t a = addr; a >= floor; --a)
    {
        if (!InExec(a - 1, 5) || !RangeReadable(a - 1, 5))
            return subRspFallback;

        const uint8_t* b = (const uint8_t*)a;
        uint8_t prev = *(const uint8_t*)(a - 1);

        if (b[0] == 0x55 && b[1] == 0x48 && b[2] == 0x89 && b[3] == 0xE5)
            return a;

        if (b[0] == 0x55 && b[1] == 0x48 && b[2] == 0x8B && b[3] == 0xEC)
            return a;

        if (!subRspFallback && (prev == 0xCC || prev == 0x90))
        {
            if (b[0] == 0x48 && b[1] == 0x83 && b[2] == 0xEC)
                subRspFallback = a;
            else if (b[0] == 0x48 && b[1] == 0x81 && b[2] == 0xEC)
                subRspFallback = a;
        }
    }

    return subRspFallback;
}

uintptr_t ResolveCall(uintptr_t insn)
{
    if (!insn || !InExec(insn, 5) || !RangeReadable(insn, 5))
        return 0;

    uint8_t op = *(const uint8_t*)insn;
    if (op != 0xE8 && op != 0xE9)
        return 0;

    int32_t rel = 0;
    memcpy(&rel, (const void*)(insn + 1), 4);

    uintptr_t target = insn + 5 + rel;

    if (!InExec(target, 1) || !RangeReadable(target, 1))
        return 0;

    return target;
}

uintptr_t FindCalleeAfterString(const char* str, size_t window)
{
    uintptr_t xref = FindStringXref(str, true);
    if (!xref) return 0;

    for (size_t i = 0; i < window; ++i)
    {
        uintptr_t a = xref + i;

        if (!InExec(a, 5) || !RangeReadable(a, 5))
            return 0;

        if (*(const uint8_t*)a == 0xE8)
            return ResolveCall(a);
    }

    return 0;
}

uintptr_t ResolveTailCallee(uintptr_t func, size_t maxLen, size_t nearWindow)
{
    if (!func || !InExec(func, 1))
        return 0;

    uintptr_t calls[128];
    int counts[128];
    bool tail[128];
    int n = 0;

    for (size_t i = 0; i < maxLen; )
    {
        uintptr_t a = func + i;

        if (!InExec(a, 5) || !RangeReadable(a, 5))
            break;

        uint8_t op = *(const uint8_t*)a;

        if (op == 0xE8 || op == 0xE9)
        {
            uintptr_t t = ResolveCall(a);
            bool internal = (t && FindFuncStart(t) == func);

            if (t && !internal)
            {
                uint8_t next = 0;
                if (InExec(a + 5, 1) && RangeReadable(a + 5, 1))
                    next = *(const uint8_t*)(a + 5);

                bool isTail = (op == 0xE9) || (next == 0xC3);

                int f = -1;
                for (int k = 0; k < n; ++k)
                    if (calls[k] == t) { f = k; break; }

                if (f < 0 && n < 128) { calls[n] = t; counts[n] = 1; tail[n] = isTail; n++; }
                else if (f >= 0) { counts[f]++; if (isTail) tail[f] = true; }
            }

            i += 5;
            continue;
        }

        ++i;
    }

    if (n == 0) return 0;

    uintptr_t near = 0;
    uint64_t bestDist = nearWindow;
    for (int k = 0; k < n; ++k)
    {
        uint64_t d = (calls[k] > func) ? (calls[k] - func) : (func - calls[k]);
        if (d < bestDist) { bestDist = d; near = calls[k]; }
    }
    if (near) return near;

    for (int k = 0; k < n; ++k)
        if (counts[k] > 1 && tail[k]) return calls[k];

    for (int k = 0; k < n; ++k)
        if (tail[k]) return calls[k];

    uintptr_t best = 0; int bc = 0;
    for (int k = 0; k < n; ++k)
        if (counts[k] > bc) { bc = counts[k]; best = calls[k]; }
    return best;
}

uintptr_t ResolveFirstCallee(uintptr_t func, size_t maxLen)
{
    if (!func || !InExec(func, 1))
        return 0;

    for (size_t i = 0; i < maxLen; i++)
    {
        uintptr_t a = func + i;

        if (!InExec(a, 5) || !RangeReadable(a, 5))
            break;

        uint8_t op = *(const uint8_t*)a;
        if (op != 0xE8 && op != 0xE9) continue;

        uintptr_t t = ResolveCall(a);
        if (!t || t == func) continue;

        return t;
    }

    return 0;
}

uintptr_t FindSetText()
{
    uintptr_t base = GetBaseAddress();
    if (!base) return 0;

    const uint8_t* b = (const uint8_t*)base;
    const size_t scanSize = CodeScanSize();

    uintptr_t fallback = 0;
    int seen = 0;

    for (size_t i = 0; i + 7 <= scanSize; ++i)
    {
        if ((i & 0xFFF) == 0 ||
            ((i + 7) & ~(size_t)0xFFF) != (i & ~(size_t)0xFFF))
        {
            if (!RangeReadable(base + i, 7))
            {
                size_t next = (i & ~(size_t)0xFFF) + 0x1000;
                if (next <= i) break;
                i = next - 1;
                continue;
            }
        }

        if (b[i] != 0x48 && b[i] != 0x4C) continue;
        if (b[i + 1] != 0x8D && b[i + 1] != 0x8B) continue;
        if ((b[i + 2] & 0xC7) != 0x05) continue;

        uintptr_t t = base + i + 7 + *(const int32_t*)(base + i + 3);

        if (!RangeReadable(t, 16)) continue;
        if (*(const uint64_t*)t != *(const uint64_t*)"Hud Elem") continue;
        if (memcmp((const void*)t, "Hud Elem String", 15) != 0) continue;

        uintptr_t fn = FindFuncStart(base + i);

        int stride = 0;
        int mult = 1;
        bool gotMult = false;

        if (fn)
        {
            unsigned char* p = (unsigned char*)fn;

            for (int k = 0; k < 0x200 && !stride; )
            {
                if (!RangeReadable(fn + k, 8)) break;

                unsigned char* q = p + k;
                bool rexw = q[0] >= 0x48 && q[0] <= 0x4F;

                if (rexw && q[1] == 0x69 && (q[2] & 0xC0) == 0xC0) { stride = *(int*)(q + 3); break; }
                if (rexw && q[1] == 0x6B && (q[2] & 0xC0) == 0xC0) { stride = (signed char)q[3]; break; }

                if (rexw && q[1] == 0x8D && (q[2] & 0xC7) == 0x04 &&
                    (q[3] & 7) != 4 && ((q[3] >> 3) & 7) != 4 &&
                    (q[3] & 7) == ((q[3] >> 3) & 7)) {
                    mult *= 1 + (1 << (q[3] >> 6));
                    gotMult = true;
                    k += 4;
                    continue;
                }

                if (rexw && q[1] == 0xC1 && ((q[2] >> 3) & 7) == 4) {
                    mult *= 1 << q[3];
                    gotMult = true;
                    k += 4;
                    continue;
                }

                if (rexw && (q[1] == 0x01 || q[1] == 0x03) && gotMult) { stride = mult; break; }

                k++;
            }
        }

        if (fn && !fallback) fallback = fn;

        if (fn && stride >= 128 && stride <= 512)
            return fn;

        if (++seen >= 8) break;
    }

    return fallback;
}

void ResolveTextPatch(uintptr_t setText)
{
    if (!hasValidOffset(setText) || !QuickPtr(setText))
        return;

    size_t limit = 0x308;
    while (limit > 16 && !RangeReadable(setText, limit))
        limit -= 8;

    if (limit <= 16)
        return;

    const int end = (int)limit - 8;

    unsigned char* p = (unsigned char*)setText;

    int stride = 0;
    int mult = 1;
    bool gotMult = false;
    int lastLea = -1;

    const int strideEnd = end < 0x100 ? end : 0x100;

    for (int i = 0; i < strideEnd && !stride; ) {
        unsigned char* q = p + i;
        bool rexw = q[0] >= 0x48 && q[0] <= 0x4F;

        if (rexw && q[1] == 0x8D && (q[2] & 0xC7) == 0x05) {
            lastLea = i;
            i += 7;
            continue;
        }

        if (rexw && q[1] == 0x69 && (q[2] & 0xC0) == 0xC0) { stride = *(int*)(q + 3); break; }
        if (rexw && q[1] == 0x6B && (q[2] & 0xC0) == 0xC0) { stride = (signed char)q[3]; break; }

        if (rexw && q[1] == 0x8D && (q[2] & 0xC7) == 0x04 &&
            (q[3] & 7) != 4 && ((q[3] >> 3) & 7) != 4 &&
            (q[3] & 7) == ((q[3] >> 3) & 7)) {
            mult *= 1 + (1 << (q[3] >> 6));
            gotMult = true;
            i += 4;
            continue;
        }

        if (rexw && q[1] == 0xC1 && ((q[2] >> 3) & 7) == 4) {
            mult *= 1 << q[3];
            gotMult = true;
            i += 4;
            continue;
        }

        if (rexw && (q[1] == 0x01 || q[1] == 0x03) && gotMult) { stride = mult; break; }

        i++;
    }

    uintptr_t hudBase = 0;
    if (lastLea >= 0 && lastLea + 7 <= end)
        hudBase = setText + lastLea + 7 + *(int*)(p + lastLea + 3);

    if (hudBase && !SafePtr(hudBase, 8))
        hudBase = 0;

    int lea = -1;

    for (int i = 0; i < end; i++) {
        unsigned char* q = p + i;
        if (q[0] < 0x40 || q[0] > 0x4F) continue;
        if (q[1] != 0x8D) continue;
        if ((q[2] & 0xC7) != 0x05) continue;

        uintptr_t t = setText + i + 7 + *(int*)(q + 3);
        if (!SafePtr(t, 16)) continue;
        if (strncmp((const char*)t, "Hud Elem String", 15) != 0) continue;

        lea = i + 7;
        break;
    }

    int slot = -1;

    if (lea >= 0) {
        int slotEnd = lea + 0x40;
        if (slotEnd > end) slotEnd = end;

        for (int i = lea; i < slotEnd; i++) {
            unsigned char* q = p + i;
            if (q[0] >= 0x40 && q[0] <= 0x4F) q++;
            if (q[0] != 0xFF) continue;

            unsigned char m = q[1];
            if (((m >> 3) & 7) != 2) continue;
            if ((m & 7) == 4) continue;

            if ((m & 0xC0) == 0x00) { slot = 0; break; }
            if ((m & 0xC0) == 0x40) { slot = (signed char)q[2]; break; }
            if ((m & 0xC0) == 0x80) { slot = *(int*)(q + 2); break; }
        }
    }

    uintptr_t ss = 0;

    if (lea >= 0) {
        int leaOff = lea - 7;
        int ssEnd = leaOff + 0x40;
        if (ssEnd > end) ssEnd = end;

        int bestDist = 0x7FFFFFFF;

        for (int i = 0; i < ssEnd; i++) {
            if (i >= leaOff && i < leaOff + 7) continue;
            if (lastLea >= 0 && i == lastLea) continue;

            unsigned char* q = p + i;
            if (q[0] != 0x48 && q[0] != 0x4C) continue;
            if (q[1] != 0x8B && q[1] != 0x8D) continue;
            if ((q[2] & 0xC7) != 0x05) continue;

            uintptr_t t = setText + i + 7 + *(int*)(q + 3);
            if (t & 7) continue;
            if (!SafePtr(t, 8)) continue;
            if (hudBase) {
                uintptr_t d = t > hudBase ? t - hudBase : hudBase - t;
                if (d > 0x2000000) continue;
            }

            int dist = i > leaOff ? i - leaOff : leaOff - i;
            if (dist < bestDist) { bestDist = dist; ss = t; }
        }
    }

    if (stride >= 128 && stride <= 512)
        g_textPatch_vals.hudElemStride = stride;

    if (slot >= 8 && slot <= 256 && !(slot & 7))
        g_textPatch_vals.constructVtSlot = slot;

    if (hudBase && !hasValidOffset(g_textPatch_vals.hudElemBase))
        g_textPatch_vals.hudElemBase = hudBase;

    if (ss && !hasValidOffset(g_textPatch_vals.scriptSystem))
        g_textPatch_vals.scriptSystem = ss;
}

void DbgPrint(const char* fmt, ...)
{
#ifdef _DEBUG
    char body[1024];
    va_list va;
    va_start(va, fmt);
    vsnprintf(body, sizeof(body), fmt, va);
    va_end(va);

    char msg[1088];
    int len = snprintf(msg, sizeof(msg), "[GSCLabs] %s\n", body);
    if (len < 0)
        return;
    if (len >= (int)sizeof(msg))
        len = (int)sizeof(msg) - 1;

    int fd = sceKernelOpen("/data/GSCLabs/log.txt", SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_APPEND, 0777);
    if (fd < 0)
        return;

    int written = 0;
    while (written < len)
    {
        int r = sceKernelWrite(fd, msg + written, len - written);
        if (r <= 0)
            break;
        written += r;
    }

    sceKernelClose(fd);
#endif
}

void DetectAndNotifyGame()
{
    for (size_t i = 0; i < sizeof(gameDetectionTable) / sizeof(GameDetectionEntry); i++)
    {
        const auto& entry = gameDetectionTable[i];

        if (!hasValidOffset(entry.address))
            continue;

        if (SafeStrStr(entry.address, "Multiplayer") || SafeStrStr(entry.address, "multiProgress") || SafeStrStr(entry.address, "multiplayer"))
        {
            g_id = entry.id;
            break;
        }
    }

    struct GameStringMap
    {
        game id;
        const char* path;
    };

    const GameStringMap g_game_mappings[] =
    {
        { IW8, "C:\\trees\\iw8" },
        { S4, "C:\\trees\\s4" },

        { IW9_SP, "C:\\workspace\\iw9_sp" },

        { IW9_MP, "C:\\workspace\\iw9" },
        { IW9_MP, "C:\\iw9_release" },
        { IW9_MP, "C:\\iw9_submission" },
    };

    if (g_id == NONE)
    {
        for (size_t i = 0; i < sizeof(g_game_mappings) / sizeof(GameStringMap); i++)
        {
            const GameStringMap& map = g_game_mappings[i];

            if (!map.path || map.path[0] == '\0')
                continue;

            uintptr_t string_address = FindString(map.path, true);

            DbgPrint("DETECT path[%u] %s -> 0x%llx",
                (unsigned)i, map.path, (unsigned long long)string_address);

            if (string_address)
            {
                g_id = map.id;
                break;
            }
        }
    }

    switch ((int)g_id)
    {
    case T5_SP:
        DbgPrint("BO1 (T5) singleplayer detected!");
        g_legacy = true;
        break;

    case T5_MP:
        DbgPrint("BO1 (T5) multiplayer detected!");
        g_legacy = true;
        break;

    case T6_SP:
        DbgPrint("BO2 (T6) singleplayer detected!");
        g_legacy = true;
        break;

    case T6_MP:
        DbgPrint("BO2 (T6) multiplayer detected!");
        g_legacy = true;
        break;

    case T6_ZM:
        DbgPrint("BO2 (T6) zombies detected!");
        g_legacy = true;
        break;

    case IW6_SP:
        DbgPrint("GHO (IW6) singleplayer detected!");
        g_legacy = true;
        break;

    case IW6_MP:
        DbgPrint("GHO (IW6) multiplayer detected!");
        g_legacy = true;
        break;

    case S1_SP:
        DbgPrint("AW (S1) singleplayer detected!");
        g_legacy = true;
        break;

    case S1_MP:
        DbgPrint("AW (S1) multiplayer detected!");
        g_legacy = true;
        break;

    case T7:
        DbgPrint("BO3 (T7) detected!");
        break;

    case IW7:
        DbgPrint("IW (IW7) detected!");
        break;

    case H1_SP:
        DbgPrint("MWR (H1) singleplayer detected!");
        g_legacy = true;
        break;

    case H1_MP:
        DbgPrint("MWR (H1) multiplayer detected!");
        g_legacy = true;
        break;

    case S2_SP:
        DbgPrint("WWII (S2) singleplayer detected!");
        break;

    case S2_MP:
        DbgPrint("WWII (S2) multiplayer detected!");
        break;

    case T8:
        DbgPrint("BO4 (T8) detected!");
        break;

    case IW8:
        DbgPrint("MW (IW8) detected!");
        break;

    case H2:
        DbgPrint("MW2CR (H2) detected!");
        g_legacy = true;
        break;

    case T9:
        DbgPrint("CW (T9) detected!");
        break;

    case S4:
        DbgPrint("VG (S4) detected!");
        break;

    case IW9_SP:
        DbgPrint("MWII (IW9) singleplayer detected!");
        break;

    case IW9_MP:
        DbgPrint("MWII (IW9) multiplayer detected!");
        break;

    case JUP_SP:
        DbgPrint("MWIII (JUP) singleplayer detected!");
        break;

    case JUP_MP:
        DbgPrint("MWIII (JUP) multiplayer detected!");
        break;

    case T10_SP:
        DbgPrint("BO6 (T10) singleplayer detected!");
        break;

    case T10_MP:
        DbgPrint("BO6 (T10) multiplayer detected!");
        break;

    default:
        DbgPrint("Unsupported title detected!");
        break;
    }

    g_offsets.init((int)g_id);
    g_findXAsset_vals.init((int)g_id);
    g_overflow_vals.init((int)g_id);
    g_textPatch_vals.init((int)g_id);

    DbgPrint("LEGACY GAME - %s", g_legacy ? "true" : "false");
}
