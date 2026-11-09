#include "headers.hpp"

DECLARE_DETOUR(DB_FindXAssetHeader)
DECLARE_DETOUR(HECmd_SetText)
DECLARE_DETOUR(Scr_ReadFile_FastFile)
DECLARE_DETOUR(Scr_GetCheckSum)

void* g_hudelems = nullptr;
unsigned long long allocAddr;

ScePthread g_ncsInjectThread = nullptr;

uintptr_t g_ncsArr = 0;
int g_ncsBase = 0;
uintptr_t* g_myArr = nullptr;
char g_emptyStr[1] = { 0 };
int g_nextSlot = 0;

struct custom_text_slot {
    unsigned int id;
    char value[2048];
    bool active;
};

std::array<custom_text_slot, 40> custom_text_slots = [] {
    std::array<custom_text_slot, 40> a{};
    for (int i = 0; i < 40; i++) {
        a[i].id = 999 + i;
        a[i].value[0] = 0;
        a[i].active = false;
    }

    return a;
    }();

std::array<int, 256> g_idxToSlot = [] {
    std::array<int, 256> a{};
    for (int i = 0; i < 256; i++) a[i] = -1;
    return a;
    }();

struct NcsLoc {
    int entry;
    int arrOff;
    int countOff;
    uintptr_t table;
    int stride;
    uintptr_t head;
    uintptr_t inner;
};

NcsLoc g_ncsLoc = { -1, 0, 0, 0, 0, 0, 0 };

static int g_ncsNewCount = 0;

static int NcsScore(uintptr_t arr, int count) {
    int hits = 0;
    int n = count < 16 ? count : 16;

    for (int i = 0; i < n; i++) {
        const char* s = (const char*)*(uintptr_t*)(arr + 8 * i);
        if (!SafePtr((uintptr_t)s, 1)) continue;

        bool slash = false;
        bool lower = false;
        bool upper = false;

        for (int j = 0; j < 64 && s[j]; j++)
            if (s[j] == '/') slash = true;
            else if (s[j] >= 'a' && s[j] <= 'z') lower = true;
            else if (s[j] >= 'A' && s[j] <= 'Z') upper = true;

        if (slash && upper && !lower) hits++;
    }

    return hits;
}

static int SlotForIdx(uint32_t idx) {
    if (idx >= 256)
        return 0;

    if (g_idxToSlot[idx] >= 0)
        return g_idxToSlot[idx];

    int slot = g_nextSlot % 40;

    g_nextSlot++;

    g_idxToSlot[idx] = slot;

    return slot;
}

bool NcsProbe(uintptr_t table, int tableStride, int maxEntries, NcsLoc* out) 
{
    NcsLoc found[16];
    int foundCount[16];
    int n = 0;

    int rejEntry = 0, rejHead = 0, rejInner = 0, rejArr = 0, rejStr = 0, rejCount = 0;

    for (int e = 0; e < maxEntries && n < 16; e++) 
    {
        uintptr_t entry = table + tableStride * e;
        if (!SafePtr(entry, 24))
        {
            rejEntry++;
            continue;
        }

        uintptr_t head = *(uintptr_t*)entry;
        if (!SafePtr(head, 24))
        {
            rejHead++;
            continue;
        }

        uintptr_t inner = *(uintptr_t*)head;
        if (!SafePtr(inner, 64))
        {
            rejInner++;
            continue;
        }

        for (int arrOff = 8; arrOff <= 56 && n < 16; arrOff += 8) {
            uintptr_t arr = *(uintptr_t*)(inner + arrOff);
            if (!SafePtr(arr, 64))
            {
                rejArr++;
                continue;
            }

            int ok = 0;
            for (int i = 0; i < 8; i++) 
            {
                uintptr_t s = *(uintptr_t*)(arr + 8 * i);
                if (!SafePtr(s, 1))
                    continue;

                if (*(char*)s)
                    ok++;
            }

            if (ok < 8)
            {
                rejStr++;
                continue;
            }

            bool tookOne = false;

            for (int cOff = 8; cOff <= 40; cOff += 4) 
            {
                if (cOff >= arrOff && cOff < arrOff + 8)
                    continue;

                int count = *(int*)(inner + cOff);
                if (count <= 8 || count > 100000)
                    continue;

                if (!SafePtr(arr, 8 * (size_t)count))
                    continue;

                uintptr_t last = *(uintptr_t*)(arr + 8 * (count - 1));
                if (!SafePtr(last, 1))
                    continue;

                found[n].entry = e;
                found[n].arrOff = arrOff;
                found[n].countOff = cOff;
                found[n].table = table;
                found[n].stride = tableStride;
                found[n].head = head;
                found[n].inner = inner;
                foundCount[n] = count;

                n++;
                tookOne = true;

                break;
            }

            if (!tookOne)
                rejCount++;
        }
    }


    if (!n)
        return false;

    int pick = 0;
    int bestScore = -1;

    for (int i = 0; i < n; i++) {
        uintptr_t arr = *(uintptr_t*)(found[i].inner + found[i].arrOff);
        int score = NcsScore(arr, foundCount[i]);

        if (score > bestScore || (score == bestScore && foundCount[i] > foundCount[pick])) {
            bestScore = score;
            pick = i;
        }
    }

    *out = found[pick];

    return true;
}

int FindNcsCandidates(uintptr_t* out, int max)
{
    uintptr_t base = GetBaseAddress();
    if (!base) return 0;

    const uint8_t* b = (const uint8_t*)base;
    const size_t scanSize = CodeScanSize();
    int n = 0;

    for (size_t i = 0; i + 40 <= scanSize && n < max; ++i)
    {
        if ((i & 0xFFF) == 0 ||
            ((i + 40) & ~(size_t)0xFFF) != (i & ~(size_t)0xFFF))
            if (!RangeReadable(base + i, 40))
            {
                size_t next = (i & ~(size_t)0xFFF) + 0x1000;
                if (next <= i) break;
                i = next - 1;
                continue;
            }

        if (b[i] != 0x48 && b[i] != 0x4C) continue;
        if (b[i + 1] != 0x8D) continue;
        if ((b[i + 2] & 0xC7) != 0x05) continue;

        bool mul3 = false;

        for (size_t j = i + 7; j + 4 <= i + 7 + 32; ++j)
        {
            if ((b[j] == 0x48 || b[j] == 0x4C || b[j] == 0x4A || b[j] == 0x4E) &&
                b[j + 1] == 0x8D &&
                (b[j + 2] & 0xC0) == 0x00 && (b[j + 2] & 0x07) == 0x04)
            {
                uint8_t sib = b[j + 3];
                if (((sib >> 6) & 3) == 1 && ((sib >> 3) & 7) == (sib & 7))
                {
                    mul3 = true;
                    continue;
                }
            }

            if (mul3 &&
                (b[j] == 0x48 || b[j] == 0x4C || b[j] == 0x4A || b[j] == 0x4E) &&
                b[j + 1] == 0x8B &&
                (b[j + 2] & 0xC0) == 0x00 && (b[j + 2] & 0x07) == 0x04 &&
                ((b[j + 3] >> 6) & 3) == 3)
            {
                uintptr_t t = ResolveRip(base + i, 3, 7) & ~(uintptr_t)7;

                if (t && SafePtr(t, 24))
                {
                    bool dup = false;
                    for (int k = 0; k < n; ++k)
                        if (out[k] == t) { dup = true; break; }

                    if (!dup) out[n++] = t;
                }
                break;
            }
        }
    }

    for (int i = 0; i < n - 1; ++i)
        for (int j = i + 1; j < n; ++j)
            if (out[j] < out[i]) { uintptr_t t = out[i]; out[i] = out[j]; out[j] = t; }

    return n;
}

bool NcsInject_Install() {
    uintptr_t inner = g_ncsLoc.inner;
    uintptr_t head = g_ncsLoc.head;
    uintptr_t entry = g_ncsLoc.table + g_ncsLoc.stride * g_ncsLoc.entry;

    if (!SafePtr(inner, 64) || !SafePtr(head, 24))
        return false;

    if (g_textPatch_vals.iw7Abi && !SafePtr(entry, 24))
        return false;

    uintptr_t oldArr = *(uintptr_t*)(inner + g_ncsLoc.arrOff);
    int count = *(int*)(inner + g_ncsLoc.countOff);

    if (count <= 0 || count > 100000)
        return false;

    if (!SafePtr(oldArr, 8 * (size_t)count))
        return false;

    g_ncsBase = g_textPatch_vals.iw7Abi ? 0 : *(uint16_t*)(head + 16);

    for (auto& s : custom_text_slots) {
        s.value[0] = 0;
        s.active = false;
    }

    for (int i = 0; i < 256; i++)
        g_idxToSlot[i] = -1;

    g_nextSlot = 0;

    if (g_textPatch_vals.iw7Abi) {
        unsigned int slot = 0;
        for (auto& s : custom_text_slots)
            s.id = (unsigned int)count + slot++;
    }

    unsigned int maxId = 0;
    for (auto& s : custom_text_slots)
        if (s.id > maxId) maxId = s.id;

    int newCount = (int)(maxId - g_ncsBase) + 1;
    if (newCount < count) newCount = count;

    uintptr_t* arr = (uintptr_t*)malloc(sizeof(uintptr_t) * newCount);
    if (!arr)
        return false;

    for (int i = 0; i < newCount; i++)
        arr[i] = (uintptr_t)g_emptyStr;

    memcpy(arr, (void*)oldArr, sizeof(uintptr_t) * count);

    int mapped = 0;
    for (auto& s : custom_text_slots) {
        int idx = (int)s.id - g_ncsBase;
        if (idx >= 0 && idx < newCount) {
            arr[idx] = (uintptr_t)s.value;
            mapped++;
        }
    }

    g_myArr = arr;

    *(uintptr_t*)(inner + g_ncsLoc.arrOff) = (uintptr_t)arr;
    *(int*)(inner + g_ncsLoc.countOff) = newCount;

    if (g_textPatch_vals.iw7Abi)
        *(uint32_t*)(entry + 8) = newCount;

    g_ncsArr = (uintptr_t)arr;
    g_ncsNewCount = newCount;

    DbgPrint("[NCS] swap arr=0x%llX src=0x%llX inner=0x%llX count %d -> %d base=%d mapped=%d",
        (unsigned long long)g_ncsArr, (unsigned long long)oldArr,
        (unsigned long long)inner, count, newCount, g_ncsBase, mapped);

    return true;
}

void HECmd_SetText_h(int64_t scrContext, int64_t entref) {
    char string[1024];
    bool ours;
    uint32_t idx;

    string[0] = 0;

    if (!hasValidOffset(g_textPatch_vals.scriptSystem) ||
        !SafePtr(g_textPatch_vals.scriptSystem, 8)) {
        HECmd_SetText_s(scrContext, entref);
        return;
    }

    int64_t scriptObj = *(int64_t*)g_textPatch_vals.scriptSystem;
    if (!SafePtr((uintptr_t)scriptObj, 8)) {
        HECmd_SetText_s(scrContext, entref);
        return;
    }

    int64_t vt = *(int64_t*)scriptObj;
    if (!SafePtr((uintptr_t)vt, (size_t)g_textPatch_vals.constructVtSlot + 8)) {
        HECmd_SetText_s(scrContext, entref);
        return;
    }

    uintptr_t fn = *(uintptr_t*)(vt + g_textPatch_vals.constructVtSlot);
    if (!SafePtr(fn, 1)) {
        HECmd_SetText_s(scrContext, entref);
        return;
    }

    if (g_textPatch_vals.iw7Abi) {
        int a1 = (int)scrContext;
        ((ConstructMsg_IW7_t)fn)(scriptObj, 0, 0, "Hud Elem String", string, 1024);
        ours = (a1 & 0xFFFF0000) == 0x10000;
        idx = (uint16_t)a1;
    }
    else {
        ((ConstructMsg_t)fn)(scriptObj, scrContext, 0, 0, "Hud Elem String", string, 1024);
        ours = (entref & 0xFF00000000LL) == 0x100000000LL;
        idx = (uint32_t)entref;
    }

    if (g_ncsArr && string[0] && ours && idx < 256) {
        if (!hasValidOffset(g_textPatch_vals.hudElemBase) ||
            g_textPatch_vals.hudElemStride < 128 ||
            g_textPatch_vals.hudElemStride > 512) {
            HECmd_SetText_s(scrContext, entref);
            return;
        }

        uintptr_t hud = g_textPatch_vals.hudElemBase +
            (uintptr_t)g_textPatch_vals.hudElemStride * idx;

        if (!SafePtr(hud, 152)) {
            HECmd_SetText_s(scrContext, entref);
            return;
        }

        auto& s = custom_text_slots[SlotForIdx(idx)];

        strncpy(s.value, string, sizeof(s.value) - 1);
        s.value[sizeof(s.value) - 1] = 0;
        s.active = true;

        HECmd_SetText_s(scrContext, entref);

        *(uint32_t*)(hud + 148) = s.id + 1;
        return;
    }

    HECmd_SetText_s(scrContext, entref);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* HECmd_GetHudElem(scr_entref_t entref)
{
    if (entref.classnum != 1 || !g_hudelems)
        return nullptr;

    size_t stride = 0;
    switch (g_id) 
    {
    case IW6_MP:
        stride = sizeof(IW6_MP_hudelem_s);
        break;

    case S1_MP:
        stride = sizeof(S1_MP_hudelem_s);
        break;

    case H1_MP:
        stride = sizeof(H1_MP_hudelem_s);
        break;

    case S2_MP:
        stride = sizeof(S2_MP_hudelem_s);
        break;
    }

    if (!stride)
        return nullptr;

    return (uint8_t*)g_hudelems + stride * entref.entnum;
}

void HECmd_ClearAllTextAfterHudElem(scr_entref_t entref) 
{
    void* hudPtr = HECmd_GetHudElem(entref);
    if (!hudPtr)
        return;

    he_type_t type = (he_type_t)-1;
    int text = -1;

    switch (g_id) 
    {
    case IW6_MP: {
        auto h = (IW6_MP_hudelem_s*)hudPtr;
        type = h->type;
        text = h->text;
        break;
    }

    case S1_MP: {
        auto h = (S1_MP_hudelem_s*)hudPtr;
        type = h->type;
        text = h->text;
        break;
    }

    case H1_MP: {
        auto h = (H1_MP_hudelem_s*)hudPtr;
        type = h->type;
        text = h->text;
        break;
    }

    case S2_MP: {
        auto h = (S2_MP_hudelem_s*)hudPtr;
        type = h->type;
        text = h->text;
        break;
    }

    default:
        return;
    }

    if (type == HE_TYPE_TEXT && text + 1 < g_overflow_vals.count) {
        int first = text + g_overflow_vals.start;

        for (int i = first; i < g_overflow_vals.end; ++i)
            SV_SetConfigString_s(i, "");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Com_GetCurrentCoDPlayMode() 
{
    return *(int32_t*)(g_offsets.Com_GetCurrentCoDPlayMode);
}

#define ZA_MAX_DELTA 0x7FFF0000ULL

char* DB_ZoneAlloc(size_t n, uint64_t base)
{
    DbgPrint("[ZA] ENTER n=%llu (0x%llX) base=0x%llX",
        (unsigned long long)n, (unsigned long long)n, base);

    if (!base || n == 0) {
        DbgPrint("[ZA] BAIL: base=0 or n=0");
        return nullptr;
    }

    const uint64_t need = ((uint64_t)n + 0xF) & ~(uint64_t)0xF;
    const uint64_t runNeed = need + 0x40;
    const uint64_t aligned = ((uint64_t)n + 0xFFFF) & ~(uint64_t)0xFFFF;
    const uint64_t mapSize = aligned;

    for (int pass = 0; pass < 2; ++pass)
    {
        uint64_t lo;
        uint64_t hi;

        if (pass == 0) {
            lo = base & ~(uint64_t)0xFFFFFF;
            hi = lo + 0x1000000;
        }
        else {
            lo = (base > 0x8000000) ? ((base - 0x8000000) & ~(uint64_t)0xFFFF) : 0x10000;
            hi = (base + 0x8000000) & ~(uint64_t)0xFFFF;
        }

        DbgPrint("[ZA] pass %d window 0x%llX..0x%llX need=0x%llX mapSize=0x%llX",
            pass, lo, hi, need, mapSize);

        uint64_t squatHit = 0;
        uint64_t squatDelta = ~(uint64_t)0;

        uint64_t gapHit = 0;
        uint64_t gapDelta = ~(uint64_t)0;

        uint64_t rsvHit = 0;
        uint64_t rsvDelta = ~(uint64_t)0;

        uint64_t probe = lo;
        int iter = 0;
        int regions = 0;

        while (probe < hi && iter < 8192)
        {
            SceKernelVirtualQueryInfo info;
            memset(&info, 0, sizeof(info));

            ++iter;

            if (sceKernelVirtualQuery((void*)probe, 0, &info, sizeof(info)) < 0 ||
                (uint64_t)info.end <= (uint64_t)info.start)
            {
                uint64_t gs = probe;
                uint64_t ge = probe;

                while (ge < hi)
                {
                    SceKernelVirtualQueryInfo gi;
                    memset(&gi, 0, sizeof(gi));

                    if (sceKernelVirtualQuery((void*)ge, 0, &gi, sizeof(gi)) >= 0 &&
                        (uint64_t)gi.end > (uint64_t)gi.start)
                        break;

                    ge += 0x10000;
                    ++iter;
                }

                const uint64_t gstart = (gs + 0xFFFF) & ~(uint64_t)0xFFFF;

                if (ge > gstart && ge - gstart >= mapSize)
                {
                    uint64_t cand = gstart;
                    uint64_t best = gstart;
                    uint64_t bestD = ~(uint64_t)0;

                    while (cand + mapSize <= ge)
                    {
                        const uint64_t d = (cand > base) ? (cand - base) : (base - cand);
                        if (d < bestD) { bestD = d; best = cand; }
                        cand += 0x10000;
                    }

                    if (bestD < gapDelta) {
                        gapDelta = bestD;
                        gapHit = best;
                    }

                    DbgPrint("[ZA] gap 0x%llX..0x%llX best=0x%llX delta=0x%llX",
                        gstart, ge, best, bestD);
                }

                probe = ge;
                continue;
            }

            ++regions;

            const uint64_t regionEnd = (uint64_t)info.end;

            const bool rw =
                (info.protection & (PROT_READ | PROT_WRITE)) == (PROT_READ | PROT_WRITE);

            if (pass == 0)
                DbgPrint("[ZA]   region 0x%llX..0x%llX size=0x%llX prot=0x%X%s",
                    (uint64_t)info.start, regionEnd,
                    regionEnd - (uint64_t)info.start, info.protection,
                    rw ? " RW"
                    : (info.protection == 0 ? " (reserved - claimable)"
                        : " (not RW - skipped)"));

            uint64_t rs = (uint64_t)info.start;
            uint64_t re = regionEnd;

            if (rs < lo) rs = lo;
            if (re > hi) re = hi;

            if (rw && re > rs + runNeed)
            {
                const uint64_t ss = (rs + 0xF) & ~(uint64_t)0xF;
                const uint64_t se = re & ~(uint64_t)0xF;

                uint64_t run = 0;
                uint64_t p = se;

                while (p > ss)
                {
                    p -= 8;

                    if (*(const uint64_t*)p) { run = 0; continue; }

                    run += 8;

                    if (run >= runNeed)
                    {
                        const uint64_t hit = (p + 0x20) & ~(uint64_t)0xF;
                        const uint64_t d = (hit > base) ? (hit - base) : (base - hit);

                        if (d < squatDelta) { squatDelta = d; squatHit = hit; }

                        run = 0;
                    }
                }
            }
            else if (!rw && info.protection == 0 && re > rs && (re - rs) >= mapSize)
            {
                uint64_t c = (rs + 0xFFFF) & ~(uint64_t)0xFFFF;
                uint64_t slot = 0;

                while (c + mapSize <= re) { slot = c; c += 0x10000; }

                if (slot)
                {
                    const uint64_t d = (slot > base) ? (slot - base) : (base - slot);
                    if (d < rsvDelta) { rsvDelta = d; rsvHit = slot; }
                }
            }

            probe = (regionEnd > probe) ? regionEnd : probe + 0x10000;
        }

        DbgPrint("[ZA] pass %d scanned %d regions: squat=0x%llX (delta=0x%llX) gap=0x%llX (delta=0x%llX) reserved=0x%llX (delta=0x%llX)",
            pass, regions, squatHit, squatDelta, gapHit, gapDelta, rsvHit, rsvDelta);

#define ZA_PREFER_SQUAT 0

#if ZA_PREFER_SQUAT
        if (squatHit && squatDelta <= gapDelta && squatDelta <= rsvDelta)
        {
            DbgPrint("[ZA] WIN=SQUAT 0x%llX (+0x%llX from base, block %s)",
                squatHit, squatDelta,
                (squatHit & ~(uint64_t)0xFFFFFF) == (base & ~(uint64_t)0xFFFFFF) ? "SAME" : "OTHER");

            return (char*)squatHit;
        }
#endif

        if (gapHit)
        {
            void* out = nullptr;

            int r = sceKernelMmap((void*)gapHit, mapSize,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0, &out);

            if (r < 0)
                DbgPrint("[ZA] gap mmap FAILED at 0x%llX r=%d (0x%X)", gapHit, r, r);
            else if ((uint64_t)out != gapHit)
                DbgPrint("[ZA] gap mmap MOVED 0x%llX -> %p", gapHit, out);
            else {
                DbgPrint("[ZA] WIN=GAP 0x%llX (+0x%llX from base)", gapHit, gapDelta);
                return (char*)out;
            }
        }

        if (rsvHit)
        {
            void* out = nullptr;

            int r = sceKernelMmap((void*)rsvHit, mapSize,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0, &out);

            if (r == 0 && (uint64_t)out == rsvHit) {
                DbgPrint("[ZA] WIN=RESERVED(map) 0x%llX (+0x%llX from base)", rsvHit, rsvDelta);
                return (char*)out;
            }

            if (r < 0)
                DbgPrint("[ZA] reserved mmap FAILED at 0x%llX r=%d (0x%X) - trying mprotect", rsvHit, r, r);
            else
                DbgPrint("[ZA] reserved mmap MOVED 0x%llX -> %p - trying mprotect", rsvHit, out);

            int mp = sceKernelMprotect((void*)rsvHit, mapSize, PROT_READ | PROT_WRITE);
            if (mp == 0) {
                DbgPrint("[ZA] WIN=RESERVED(mprotect) 0x%llX (+0x%llX from base)", rsvHit, rsvDelta);
                return (char*)rsvHit;
            }

            DbgPrint("[ZA] reserved mprotect FAILED at 0x%llX mp=%d (0x%X)", rsvHit, mp, mp);
        }
    }

    {
        const size_t page = 0x4000;
        size_t want = (n + page - 1) & ~(page - 1);

        void* out = nullptr;

        int r = sceKernelMmap(nullptr, want,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0, &out);

        if (r == 0 && out)
        {
            const uint64_t o = (uint64_t)out;
            const uint64_t d = (o > base) ? (o - base) : (base - o);

            if (d < ZA_MAX_DELTA) {
                DbgPrint("[ZA] WIN=MAP-ANY %p (+0x%llX from base, %llu bytes)",
                    out, (unsigned long long)d, (unsigned long long)want);
                return (char*)out;
            }

            DbgPrint("[ZA] REJECT MAP-ANY %p (+0x%llX from base exceeds 0x%llX) - unmapping",
                out, (unsigned long long)d, (unsigned long long)ZA_MAX_DELTA);

            sceKernelMunmap(out, want);
        }
        else
        {
            DbgPrint("[ZA] anonymous mmap of %llu bytes failed r=%d (0x%X)",
                (unsigned long long)want, r, r);
        }
    }

    DbgPrint("[ZA] FAIL: no slot for %llu bytes near 0x%llX",
        (unsigned long long)n, base);

    return nullptr;
}


void* DB_FindXAssetHeader_h(int type, void* asset) 
{
    const char* name = (const char*)asset;
    uint64_t hash = (uint64_t)asset;

    void* original = nullptr;

    if (g_id == T8 || g_id == T9 || g_id == IW9_MP || g_id == JUP_MP || g_id == T10_MP)
        original = DB_FindXAssetHeader_s(type, (void*)hash);
    else
        original = DB_FindXAssetHeader_s(type, (void*)name);

    if (g_id == T7)
    {
        if ((type & 0xFF) != 0x36 || !name ||
            strcmp(name, "scripts/shared/duplicaterender_mgr.gsc") != 0)
            return original;

        ScriptParseTree* asset = (ScriptParseTree*)original;
        if (!asset || !SafePtr((uintptr_t)asset, sizeof(ScriptParseTree)) || !asset->buffer)
            return original;

        static ScriptParseTree* target = nullptr;
        static void* orig_buffer = nullptr;
        static uint32_t orig_len = 0;
        static void* installed_ptr = nullptr;
        static uint64_t installed_key = 0;
        static char* slot = nullptr;
        static size_t slot_size = 0;
        static uint64_t og_magic = 0;
        static uint32_t og_crc = 0;

        if (target && target != asset)
        {
            DbgPrint("[T7] BAIL: second entry 0x%llX (locked to 0x%llX)",
                (unsigned long long)asset, (unsigned long long)target);
            return original;
        }

        if (asset->buffer != orig_buffer && asset->buffer != installed_ptr)
        {
            target = asset;
            orig_buffer = asset->buffer;
            orig_len = asset->len;

            slot = nullptr;
            slot_size = 0;

            const char* ob = (const char*)orig_buffer;
            const uint32_t noff = *(const uint32_t*)(ob + 0x34);

            og_magic = *(const uint64_t*)ob;
            og_crc = *(const uint32_t*)(ob + 0x08);

            DbgPrint("[T7] og magic=0x%llX crc=0x%X len=%u entry='%s'", (unsigned long long)og_magic, og_crc, orig_len, (const char*)asset->name);

            if (noff < orig_len)
                DbgPrint("[T7] og embedded name='%s'", ob + noff);

            installed_key = 0;
        }

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));
        if (size < 0x48)
            return original;

        const char* src = (const char*)(allocAddr + sizeof(int));

        if (!SafePtr((uintptr_t)src, (size_t)size))
            return original;

        if (*(const uint64_t*)src != og_magic)
        {
            DbgPrint("[T7] BAIL: magic staged=0x%llX og=0x%llX",
                (unsigned long long) * (const uint64_t*)src, (unsigned long long)og_magic);
            return original;
        }

        uint64_t key = 0xCBF29CE484222325ull;
        for (int i = 0; i < size; ++i)
        {
            key ^= (unsigned char)src[i];
            key *= 0x100000001B3ull;
        }

        if (key == installed_key && asset->buffer == installed_ptr)
            return original;

        char* dst;

        if (size <= (int)orig_len)
        {
            dst = (char*)orig_buffer;
            DbgPrint("[T7] IN-PLACE (slack=%d)", (int)orig_len - size);
        }
        else if (slot && slot_size >= (size_t)size)
            dst = slot;
        else
        {
            dst = DB_ZoneAlloc((size_t)size, (uint64_t)orig_buffer);
            if (!dst)
            {
                DbgPrint("[T7] BAIL: zone alloc %d failed", size);
                return original;
            }

            slot = dst;
            slot_size = (size_t)size;
            DbgPrint("[T7] carved slot 0x%llX", (unsigned long long)dst);
        }

        memcpy(dst, src, (size_t)size);

        *(uint32_t*)(dst + 0x08) = og_crc;

        DbgPrint("[T7] staged embedded name='%s'", dst + *(const uint32_t*)(dst + 0x34));

        asset->buffer = dst;
        installed_ptr = dst;
        installed_key = key;

        DbgPrint("[T7] INSTALLED buffer=0x%llX len=%d", (unsigned long long)dst, size);

        return original;
    }

    else if (g_id == IW7)
    {
        std::string replace = "";

        if (Com_GetCurrentCoDPlayMode() == 1) replace = "2730";
        if (Com_GetCurrentCoDPlayMode() == 2) replace = "2781";
        if (Com_GetCurrentCoDPlayMode() == 3) replace = "2643";

        if (strcmp(name, replace.c_str()) != 0) return original;

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));

        if (size <= 0) return original;

        char* allocMem = (char*)malloc(size + 1);
        if (!allocMem) return original;

        const char* src = (const char*)(allocAddr + sizeof(int));
        memcpy(allocMem, src, size);
        allocMem[size] = '\0';

        int scanBytes = 0;
        while (true) {
            if (scanBytes >= size) {
                free(allocMem);
                return original;
            }

            if (src[scanBytes] == '\0') { scanBytes++; break; }
            scanBytes++;
        }

        int vars[3] = { 0 };
        if (scanBytes + (int)sizeof(vars) > size) {
            free(allocMem);
            return original;
        }
        memcpy(vars, src + scanBytes, sizeof(vars));

        const int headerPos = scanBytes + (int)sizeof(vars);
        const int compLen = vars[0];
        const int len = vars[1];
        const int bcLen = vars[2];
        const int bufBytes = compLen ? compLen : len;

        if (bcLen <= 0 || bufBytes <= 0) {
            free(allocMem);
            return original;
        }

        ScriptFile* asset = (ScriptFile*)DB_FindXAssetHeader_s(type, (void*)name);

        if (!asset) { free(allocMem); return original; }

        const int origBcLen = asset->bytecodeLen;

        asset->compressedLen = compLen;
        asset->len = len;
        asset->bytecodeLen = bcLen;
        asset->buffer = allocMem + headerPos;

        const char* srcBc = allocMem + headerPos + bufBytes;

        size_t paddedOrigLen = (origBcLen + 15) & ~15;

        if (bcLen <= (int)paddedOrigLen)
            memcpy(asset->bytecode, srcBc, (size_t)bcLen);
        else {
            char* slot =
                DB_ZoneAlloc(bcLen, (uint64_t)asset->bytecode);
            if (!slot) {
                asset->bytecodeLen = origBcLen;
                free(allocMem);
                return original;
            }

            memcpy(slot, srcBc, (size_t)bcLen);
            asset->bytecode = reinterpret_cast<unsigned char*>(slot);
        }

        original = asset;
    }

    else if (g_id == S2_MP || g_id == S2_SP)
    {
        if (strcmp(name, g_findXAsset_vals.name) != 0) return original;

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));

        if (size <= 0) return original;

        char* allocMem = (char*)malloc(size + 1);
        if (!allocMem) return original;

        const char* src = (const char*)(allocAddr + sizeof(int));
        memcpy(allocMem, src, size);
        allocMem[size] = '\0';

        int scanBytes = 0;
        while (true) {
            if (scanBytes >= size) {
                free(allocMem);
                return original;
            }

            if (src[scanBytes] == '\0') { scanBytes++; break; }
            scanBytes++;
        }

        int vars[3] = { 0 };
        if (scanBytes + (int)sizeof(vars) > size) {
            free(allocMem);
            return original;
        }
        memcpy(vars, src + scanBytes, sizeof(vars));

        const int headerPos = scanBytes + (int)sizeof(vars);
        const int compLen = vars[0];
        const int len = vars[1];
        const int bcLen = vars[2];
        const int bufBytes = compLen ? compLen : len;

        if (bcLen <= 0 || bufBytes <= 0) {
            free(allocMem);
            return original;
        }

        ScriptFile* asset = (ScriptFile*)DB_FindXAssetHeader_s(type, (void*)name);

        if (!asset) { free(allocMem); return original; }

        const int origBcLen = asset->bytecodeLen;

        asset->compressedLen = compLen;
        asset->len = len;
        asset->bytecodeLen = bcLen;
        asset->buffer = allocMem + headerPos;

        const char* srcBc = allocMem + headerPos + bufBytes;

        size_t paddedOrigLen = (origBcLen + 15) & ~15;

        if (bcLen <= (int)paddedOrigLen)
            memcpy(asset->bytecode, srcBc, (size_t)bcLen);
        else {
            char* slot =
                DB_ZoneAlloc(bcLen, (uint64_t)asset->bytecode);
            if (!slot) {
                asset->bytecodeLen = origBcLen;
                free(allocMem);
                return original;
            }

            memcpy(slot, srcBc, (size_t)bcLen);
            asset->bytecode = reinterpret_cast<unsigned char*>(slot);
        }

        original = asset;
    }

    else if (g_id == T8)
    {
        const uint64_t masked = hash & 0x7FFFFFFFFFFFFFFFull;
        if ((type & 0xFF) != 0x30 || masked != 0x014F5577678DAB03ull)
            return original;

        if (!original || !SafePtr((uintptr_t)original, 0x20))
            return original;

        char* base = (char*)original;

        static void* orig_buffer = nullptr;
        static uint32_t orig_len = 0;
        static void* installed_ptr = nullptr;
        static uint64_t installed_key = 0;
        static char* slot = nullptr;
        static size_t slot_size = 0;
        static uint32_t buf_off = 0;

        static uint64_t og_magic = 0;
        static uint64_t og_name = 0;
        static uint32_t og_crc = 0;

        if (!buf_off)
        {
            if ((*(const uint64_t*)base & 0x7FFFFFFFFFFFFFFFull) != masked)
            {
                DbgPrint("[T8] BAIL: name mismatch [0]=%016llX",
                    (unsigned long long) * (const uint64_t*)base);
                return original;
            }

            char* b8 = *(char**)(base + 0x08);
            uint32_t l8 = *(const uint32_t*)(base + 0x10);
            char* b10 = *(char**)(base + 0x10);
            uint32_t l10 = *(const uint32_t*)(base + 0x18);

            if (b8 && l8 >= 0x60 && l8 < 0x2000000 && SafePtr((uintptr_t)b8, l8))
                buf_off = 0x08;
            else if (b10 && l10 >= 0x60 && l10 < 0x2000000 && SafePtr((uintptr_t)b10, l10))
                buf_off = 0x10;
            else
            {
                DbgPrint("[T8] BAIL: no valid layout b8=0x%llX l8=%u b10=0x%llX l10=%u",
                    (unsigned long long)b8, l8, (unsigned long long)b10, l10);
                return original;
            }

            DbgPrint("[T8] layout buf_off=0x%X (%s)", buf_off, buf_off == 8 ? "0x18" : "0x20");
        }

        char** pbuf = (char**)(base + buf_off);
        uint32_t* plen = (uint32_t*)(base + buf_off + 8);

        if (!*pbuf)
            return original;

        if (*pbuf != orig_buffer && *pbuf != installed_ptr)
        {
            orig_buffer = *pbuf;
            orig_len = *plen;

            slot = nullptr;
            slot_size = 0;

            const char* ob = (const char*)orig_buffer;

            og_magic = *(const uint64_t*)ob;
            og_crc = *(const uint32_t*)(ob + 0x08);
            og_name = *(const uint64_t*)(ob + 0x10);

            DbgPrint("[T8] og magic=0x%llX crc=0x%X name=0x%llX len=%u exp_off=0x%X exp_cnt=%u",
                (unsigned long long)og_magic, og_crc, (unsigned long long)og_name, orig_len,
                *(const uint32_t*)(ob + 0x30), *(const uint16_t*)(ob + 0x1E));

            installed_key = 0;
        }

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));
        if (size < 0x60)
            return original;

        const char* src = (const char*)(allocAddr + sizeof(int));

        if (!SafePtr((uintptr_t)src, (size_t)size))
            return original;

        if (*(const uint64_t*)src != og_magic)
        {
            DbgPrint("[T8] BAIL: magic staged=0x%llX og=0x%llX",
                (unsigned long long) * (const uint64_t*)src, (unsigned long long)og_magic);
            return original;
        }

        uint64_t key = 0xCBF29CE484222325ull;
        for (int i = 0; i < size; ++i)
        {
            key ^= (unsigned char)src[i];
            key *= 0x100000001B3ull;
        }

        if (key == installed_key && *pbuf == installed_ptr)
            return original;

        const bool in_place = (size <= (int)orig_len);

        char* dst;

        if (in_place)
            dst = (char*)orig_buffer;
        else if (slot && slot_size >= (size_t)size)
            dst = slot;
        else
        {
            dst = DB_ZoneAlloc((size_t)size, (uint64_t)orig_buffer);
            if (!dst)
            {
                DbgPrint("[T8] BAIL: zone alloc %d failed", size);
                return original;
            }

            slot = dst;
            slot_size = (size_t)size;
        }

        memcpy(dst, src, (size_t)size);

        if (og_name)
            *(uint64_t*)(dst + 0x10) = og_name;

        *(uint32_t*)(dst + 0x08) = og_crc;
        *(uint32_t*)(dst + 0x48) = (uint32_t)size;

        DbgPrint("[T8] staged exp_off=0x%X exp_cnt=%u imp_off=0x%X imp_cnt=%u inc=%u %s",
            *(const uint32_t*)(dst + 0x30), *(const uint16_t*)(dst + 0x1E),
            *(const uint32_t*)(dst + 0x38), *(const uint16_t*)(dst + 0x28),
            *(const uint16_t*)(dst + 0x58), in_place ? "IN-PLACE" : "ZONE");

        *pbuf = dst;
        *plen = (uint32_t)size;
        installed_ptr = dst;
        installed_key = key;

        DbgPrint("[T8] INSTALLED buffer=0x%llX len=%d key=0x%llX",
            (unsigned long long)dst, size, (unsigned long long)key);

        g_hudResetPending = 1;

        return original;
    }

    else if (g_id == IW8)
    {
        if (strcmp(name, g_findXAsset_vals.name) != 0) return original;

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));

        if (size <= 0) return original;

        char* allocMem = (char*)malloc(size + 1);
        if (!allocMem) return original;

        const char* src = (const char*)(allocAddr + sizeof(int));
        memcpy(allocMem, src, size);
        allocMem[size] = '\0';

        int scanBytes = 0;
        while (true) {
            if (scanBytes >= size) {
                free(allocMem);
                return original;
            }

            if (src[scanBytes] == '\0') { scanBytes++; break; }
            scanBytes++;
        }

        int vars[3] = { 0 };
        if (scanBytes + (int)sizeof(vars) > size) {
            free(allocMem);
            return original;
        }
        memcpy(vars, src + scanBytes, sizeof(vars));

        const int headerPos = scanBytes + (int)sizeof(vars);
        const int compLen = vars[0];
        const int len = vars[1];
        const int bcLen = vars[2];
        const int bufBytes = compLen ? compLen : len;

        if (bcLen <= 0 || bufBytes <= 0) {
            free(allocMem);
            return original;
        }

        ScriptFile* asset = (ScriptFile*)DB_FindXAssetHeader_s(type, (void*)name);

        if (!asset) { free(allocMem); return original; }

        const int origBcLen = asset->bytecodeLen;

        asset->compressedLen = compLen;
        asset->len = len;
        asset->bytecodeLen = bcLen;
        asset->buffer = allocMem + headerPos;

        const char* srcBc = allocMem + headerPos + bufBytes;

        size_t paddedOrigLen = (origBcLen + 15) & ~15;

        if (bcLen <= (int)paddedOrigLen)
            memcpy(asset->bytecode, srcBc, (size_t)bcLen);
        else {
            char* slot =
                DB_ZoneAlloc(bcLen, (uint64_t)asset->bytecode);
            if (!slot) {
                asset->bytecodeLen = origBcLen;
                free(allocMem);
                return original;
            }

            memcpy(slot, srcBc, (size_t)bcLen);
            asset->bytecode = reinterpret_cast<unsigned char*>(slot);
        }

        original = asset;
    }

    else if (g_id == T9)
    {
        const uint64_t masked = hash & 0x7FFFFFFFFFFFFFFF;
        if ((type & 0xFF) != 0x44 || masked != 0x25B0DFB952C4A18Dull)
            return original;

        ScriptParseTree_T9* asset = (ScriptParseTree_T9*)original;
        if (!asset || !asset->buffer)
            return original;

        static void* orig_buffer = nullptr;
        static uint32_t orig_len = 0;
        static void* installed_ptr = nullptr;
        static uint64_t installed_key = 0;
        static char* slot = nullptr;
        static size_t slot_size = 0;

        static uint64_t og_magic = 0;
        static uint64_t og_name = 0;
        static uint32_t og_crc = 0;

        if (asset->buffer != orig_buffer && asset->buffer != installed_ptr)
        {
            orig_buffer = asset->buffer;
            orig_len = (uint32_t)asset->len;

            const char* ob = (const char*)orig_buffer;

            og_magic = *(const uint64_t*)ob;
            og_crc = *(const uint32_t*)(ob + 0x08);
            og_name = *(const uint64_t*)(ob + 0x10);

            const uint32_t oe = *(const uint32_t*)(ob + 0x38);
            const uint16_t oc = *(const uint16_t*)(ob + 0x1A);

            DbgPrint("[T9] og magic=0x%llX crc=0x%X name=0x%llX len=%u exp_off=0x%X exp_cnt=%u",
                (unsigned long long)og_magic, og_crc, (unsigned long long)og_name,
                orig_len, oe, oc);

            if (oe + 48 <= orig_len)
                DbgPrint("[T9] og exp[0] %016llX %016llX %016llX %08X %08X %08X",
                    (unsigned long long) * (const uint64_t*)(ob + oe),
                    (unsigned long long) * (const uint64_t*)(ob + oe + 8),
                    (unsigned long long) * (const uint64_t*)(ob + oe + 0x10),
                    *(const uint32_t*)(ob + oe + 0x18),
                    *(const uint32_t*)(ob + oe + 0x20),
                    *(const uint32_t*)(ob + oe + 0x28));

            installed_key = 0;
        }

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));
        if (size < 0x58)
            return original;

        const char* src = (const char*)(allocAddr + sizeof(int));

        if (!SafePtr((uintptr_t)src, (size_t)size))
            return original;

        if (*(const uint64_t*)src != og_magic)
        {
            DbgPrint("[T9] BAIL: magic staged=0x%llX og=0x%llX",
                (unsigned long long) * (const uint64_t*)src, (unsigned long long)og_magic);
            return original;
        }

        uint64_t key = 0xCBF29CE484222325ull;
        for (int i = 0; i < size; ++i)
        {
            key ^= (unsigned char)src[i];
            key *= 0x100000001B3ull;
        }

        if (key == installed_key && asset->buffer == installed_ptr)
            return original;

        const bool in_place = (size <= (int)orig_len);

        char* dst;

        if (in_place)
            dst = (char*)orig_buffer;
        else if (slot && slot_size >= (size_t)size)
            dst = slot;
        else
        {
            dst = DB_ZoneAlloc((size_t)size, (uint64_t)orig_buffer);
            if (!dst)
            {
                DbgPrint("[T9] BAIL: zone alloc %d failed", size);
                return original;
            }

            slot = dst;
            slot_size = (size_t)size;
        }

        memcpy(dst, src, (size_t)size);

        if (og_name)
            *(uint64_t*)(dst + 0x10) = og_name;

        *(uint32_t*)(dst + 0x08) = og_crc;
        *(uint32_t*)(dst + 0x48) = (uint32_t)size;

     
        {
            const uint16_t strCount = *(const uint16_t*)(dst + 0x18);
            const uint32_t strOff = *(const uint32_t*)(dst + 0x30);

            bool ok = (strCount > 0) && (strOff + 8u <= (uint32_t)size);
            uint32_t p = strOff;

            for (uint16_t i = 0; ok && i < strCount; ++i)
            {
                if (p + 8 > (uint32_t)size)
                {
                    ok = false;
                    break;
                }

                const uint32_t so = *(const uint32_t*)(dst + p);
                const uint8_t  na = *(const uint8_t*)(dst + p + 4);

                if (so + 4 >= (uint32_t)size || (uint8_t)dst[so] != 0x8B ||
                    (uint8_t)dst[so + 2] != 0x00)
                {
                    ok = false;
                    break;
                }

                const uint32_t end = so + 3u + (uint8_t)dst[so + 1] - 1u;

                if (end >= (uint32_t)size || dst[end] != 0)
                {
                    ok = false;
                    break;
                }

                p += 8u + 4u * na;
            }

            if (ok && p == *(const uint32_t*)(dst + 0x44))
            {
                p = strOff;

                for (uint16_t i = 0; i < strCount; ++i)
                {
                    *(uint32_t*)(dst + p) += 3u;
                    p += 8u + 4u * (uint8_t)dst[p + 4];
                }

                DbgPrint("[T9] string table: repointed %u entries past the "
                    "0x8B prefix", strCount);
            }
            else
            {
                DbgPrint("[T9] string table: layout not recognised "
                    "(count=%u off=0x%X) - left unchanged", strCount, strOff);
            }
        }

        DbgPrint("[T9] staged exp_off=0x%X exp_cnt=%u imp_off=0x%X imp_cnt=%u inc=%u %s",
            *(const uint32_t*)(dst + 0x38), *(const uint16_t*)(dst + 0x1A),
            *(const uint32_t*)(dst + 0x3C), *(const uint16_t*)(dst + 0x1C),
            *(const uint16_t*)(dst + 0x24), in_place ? "IN-PLACE" : "ZONE");

        asset->buffer = dst;
        asset->len = (int32_t)size;
        installed_ptr = dst;
        installed_key = key;

      
        using ScrLink_t = int64_t(*)(uint32_t inst, void* script);
        ScrLink_t ScrLink = (ScrLink_t)REBASE_ADDRESS(0x15BEFF0);

        const int64_t linkRes = ScrLink(0, (void*)dst);

        DbgPrint("[T9] relink inst=0 buffer=0x%llX -> %lld",
            (unsigned long long)dst, (long long)linkRes);

        DbgPrint("[T9] INSTALLED buffer=0x%llX len=%d key=0x%llX",
            (unsigned long long)dst, size, (unsigned long long)key);

        return original;
    }

    else if (g_id == S4)
    {
        std::string replace = "";

        if (Com_GetCurrentCoDPlayMode() == 1) replace = "";
        if (Com_GetCurrentCoDPlayMode() == 2) replace = "2486";
        if (Com_GetCurrentCoDPlayMode() == 3) replace = "2392";

        if (strcmp(name, replace.c_str()) != 0) return original;

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));

        if (size <= 0) return original;

        char* allocMem = (char*)malloc(size + 1);
        if (!allocMem) return original;

        const char* src = (const char*)(allocAddr + sizeof(int));
        memcpy(allocMem, src, size);
        allocMem[size] = '\0';

        int scanBytes = 0;
        while (true) {
            if (scanBytes >= size) {
                free(allocMem);
                return original;
            }

            if (src[scanBytes] == '\0') { scanBytes++; break; }
            scanBytes++;
        }

        int vars[3] = { 0 };
        if (scanBytes + (int)sizeof(vars) > size) {
            free(allocMem);
            return original;
        }
        memcpy(vars, src + scanBytes, sizeof(vars));

        const int headerPos = scanBytes + (int)sizeof(vars);
        const int compLen = vars[0];
        const int len = vars[1];
        const int bcLen = vars[2];
        const int bufBytes = compLen ? compLen : len;

        if (bcLen <= 0 || bufBytes <= 0) {
            free(allocMem);
            return original;
        }

        ScriptFile_S4* asset = (ScriptFile_S4*)DB_FindXAssetHeader_s(type, (void*)name);
        if (!asset) { free(allocMem); return original; }

        const int origBcLen = asset->bytecodeLen;

        asset->compressedLen = compLen;
        asset->len = len;
        asset->bytecodeLen = bcLen;
        asset->buffer = allocMem + headerPos;

        const char* srcBc = allocMem + headerPos + bufBytes;

        size_t paddedOrigLen = (origBcLen + 15) & ~15;

        if (bcLen <= (int)paddedOrigLen)
            memcpy((void*)asset->bytecode, srcBc, (size_t)bcLen);
        else {
            char* slot =
                DB_ZoneAlloc(bcLen, (uint64_t)asset->bytecode);
            if (!slot) {
                asset->bytecodeLen = origBcLen;
                free(allocMem);
                return original;
            }

            memcpy(slot, srcBc, (size_t)bcLen);
            asset->bytecode = reinterpret_cast<unsigned char*>(slot);
        }

        original = asset;
    }

    else if (g_id == IW9_MP)
    {
        if (type != 70 || hash != 0x6F294F32B60C71F4)
            return original;

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));

        if (size <= 0) return original;

        char* allocMem = (char*)malloc(size + 1);
        if (!allocMem) return original;

        const char* src = (const char*)(allocAddr + sizeof(int));
        memcpy(allocMem, src, size);
        allocMem[size] = '\0';

        int scanBytes = 0;
        while (true) {
            if (scanBytes >= size) {
                free(allocMem);
                return original;
            }

            if (src[scanBytes] == '\0') { scanBytes++; break; }
            scanBytes++;
        }

        int vars[3] = { 0 };
        if (scanBytes + (int)sizeof(vars) > size) {
            free(allocMem);
            return original;
        }
        memcpy(vars, src + scanBytes, sizeof(vars));

        const int headerPos = scanBytes + (int)sizeof(vars);
        const int compLen = vars[0];
        const int len = vars[1];
        const int bcLen = vars[2];
        const int bufBytes = compLen ? compLen : len;

        if (bcLen <= 0 || bufBytes <= 0) {
            free(allocMem);
            return original;
        }

        ScriptFile_IW9* asset = (ScriptFile_IW9*)
            DB_FindXAssetHeader_s(type, (void*)hash);
        if (!asset) { free(allocMem); return original; }

        const int origBcLen = asset->bytecodeLen;
        uint64_t zoneBase = (uint64_t)asset->bytecode & ~0xFFFFFF;

        asset->compressedLen = compLen;
        asset->len = len;
        asset->bytecodeLen = bcLen;
        asset->buffer = allocMem + headerPos;

        const char* srcBc = allocMem + headerPos + bufBytes;
        size_t paddedOrigLen = (origBcLen + 15) & ~15;

        if (bcLen <= (int)paddedOrigLen)
            memcpy((void*)asset->bytecode, srcBc, (size_t)bcLen);
        else {
            char* slot = DB_ZoneAlloc((size_t)bcLen, zoneBase);
            if (!slot) {
                asset->bytecodeLen = origBcLen;
                free(allocMem);
                return original;
            }

            memcpy(slot, srcBc, (size_t)bcLen);
            asset->bytecode = reinterpret_cast<unsigned char*>(slot);
        }

        original = asset;
    }

    else if (g_id == JUP_MP)
    {
        const uint64_t masked = hash & 0x7FFFFFFFFFFFFFFF;

        if (masked != 0x732DFDE13310CF8Bull)
            return original;

        ScriptParseTree* asset = (ScriptParseTree*)DB_FindXAssetHeader_s(type, (void*)hash);
        if (!asset || !asset->buffer)
            return original;

        static const bool relocate_test = false;

        static void* orig_buffer = nullptr;
        static uint32_t orig_len = 0;
        static void* installed_ptr = nullptr;
        static uint64_t installed_key = 0;
        static char* slot = nullptr;
        static size_t slot_size = 0;
        static bool relocated = false;

        static uint64_t og_magic = 0;
        static uint64_t og_name = 0;
        static uint64_t og_ns = 0;
        static uint64_t og_ns2 = 0;
        static uint64_t og_autoexec = 0;
        static uint32_t og_crc = 0;

        if (asset->buffer != orig_buffer && asset->buffer != installed_ptr)
        {
            orig_buffer = asset->buffer;
            orig_len = asset->len;

            slot = nullptr;
            slot_size = 0;
            relocated = false;

            const char* ob = (const char*)orig_buffer;

            og_magic = *(const uint64_t*)ob;
            og_name = *(const uint64_t*)(ob + 8);
            og_crc = *(const uint32_t*)(ob + 0x28);
            og_ns = 0;
            og_ns2 = 0;
            og_autoexec = 0;

            const uint32_t oe = *(const uint32_t*)(ob + 0x40);
            const uint16_t oc = *(const uint16_t*)(ob + 0x18);

            for (uint16_t i = 0; i < oc; ++i)
            {
                const uint32_t e = oe + i * 40;
                if (e + 40 > orig_len)
                    break;

                if (i == 0)
                {
                    og_ns = *(const uint64_t*)(ob + e + 8);
                    og_ns2 = *(const uint64_t*)(ob + e + 0x10);
                }

                if (((unsigned char)ob[e + 0x25] & 0x01) && !og_autoexec)
                    og_autoexec = *(const uint64_t*)(ob + e);
            }

            installed_key = 0;
        }

        if (relocate_test)
        {
            if (relocated)
                return asset;

            if (orig_len < 8)
                return original;

            char* copy = DB_ZoneAlloc((size_t)orig_len, (uint64_t)orig_buffer);
            if (!copy)
                return original;
  
            memcpy(copy, (const void*)orig_buffer, (size_t)orig_len);

            asset->buffer = copy;
            asset->len = orig_len;
            installed_ptr = copy;
            relocated = true;

            return asset;
        }

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));

        if (size < 0x60)
            return original;

        const char* src = (const char*)(allocAddr + sizeof(int));

        if (*(const uint64_t*)src != og_magic)
            return original;

        const uint32_t crc = *(const uint32_t*)(src + 0x28);
        const uint64_t key = ((uint64_t)(uint32_t)size << 32) | crc;

        if (key == installed_key && asset->buffer == installed_ptr)
            return asset;

        const bool in_place = (size <= (int)orig_len);

        char* dst;

        if (in_place)
            dst = (char*)orig_buffer;
        else if (slot && slot_size >= (size_t)size)
            dst = slot;
        else
        {
            dst = DB_ZoneAlloc((size_t)size, (uint64_t)orig_buffer);
            if (!dst)
                return original;

            slot = dst;
            slot_size = (size_t)size;
        }

        memcpy(dst, src, (size_t)size);

        if (og_name)
            *(uint64_t*)(dst + 8) = og_name;

        *(uint32_t*)(dst + 0x28) = og_crc;

        const uint32_t exp_off = *(const uint32_t*)(dst + 0x40);
        const uint16_t exp_cnt = *(const uint16_t*)(dst + 0x18);
        const uint32_t imp_off = *(const uint32_t*)(dst + 0x4c);
        const uint16_t imp_cnt = *(const uint16_t*)(dst + 0x1e);

        uint64_t src_ns = 0;
        uint32_t first_export = 0;
        uint16_t exp_valid = 0;
        bool has_autoexec = false;

        for (uint16_t i = 0; i < exp_cnt; ++i)
        {
            const uint32_t e = exp_off + i * 40;
            if (e + 40 > (uint32_t)size)
                break;

            if (i == 0)
            {
                src_ns = *(const uint64_t*)(dst + e + 8);
                first_export = e;
            }

            exp_valid = i + 1;

            if (((unsigned char)dst[e + 0x25] & 0x01) && !has_autoexec)
                has_autoexec = true;

            if (og_ns)
                *(uint64_t*)(dst + e + 8) = og_ns;

            *(uint64_t*)(dst + e + 0x10) = og_ns2;
        }

            uint32_t p = imp_off;
            uint16_t remapped = 0;

            for (uint16_t i = 0; i < imp_cnt; ++i)
            {
                if (p + 24 > (uint32_t)size)
                    break;

                const uint64_t iname = *(const uint64_t*)(dst + p);
                const uint64_t ins = *(const uint64_t*)(dst + p + 8);
                const uint16_t naddr = *(const uint16_t*)(dst + p + 16);

                bool self = false;

                if (ins == src_ns)
                {
                    for (uint16_t j = 0; j < exp_valid; ++j)
                    {
                        if (*(const uint64_t*)(dst + exp_off + j * 40) == iname)
                        {
                            self = true;
                            break;
                        }
                    }
                }

                if (self)
                {
                    *(uint64_t*)(dst + p + 8) = og_ns;
                    ++remapped;
                }

                p += 24 + (uint32_t)naddr * 4;
            }

        if (!has_autoexec && exp_valid && og_autoexec)
        {
            *(uint64_t*)(dst + first_export) = og_autoexec;
            dst[first_export + 0x25] = 0x15;
        }

        asset->buffer = dst;
        asset->len = (uint32_t)size;
        installed_ptr = dst;
        installed_key = key;

        return asset;
    }

    else if (g_id == T10_MP)
    {
        const uint64_t masked = hash & 0x7FFFFFFFFFFFFFFF;
        if (masked != 0x31FCFD26E002B5ADull)
            return original;

        ScriptParseTree* asset = (ScriptParseTree*)DB_FindXAssetHeader_s(type, (void*)hash);
        if (!asset || !asset->buffer)
            return original;

        static void* orig_buffer = nullptr;
        static uint32_t orig_len = 0;
        static void* installed_ptr = nullptr;
        static uint64_t installed_key = 0;
        static char* slot = nullptr;
        static size_t slot_size = 0;

        static uint64_t og_magic = 0;
        static uint64_t og_name = 0;
        static uint64_t og_ns = 0;
        static uint64_t og_autoexec = 0;
        static uint32_t og_crc = 0;
        static uint32_t og_unk28 = 0;

        if (asset->buffer != orig_buffer && asset->buffer != installed_ptr)
        {
            orig_buffer = asset->buffer;
            orig_len = asset->len;

            const char* ob = (const char*)orig_buffer;

            og_magic = *(const uint64_t*)ob;
            og_name = *(const uint64_t*)(ob + 8);
            og_crc = *(const uint32_t*)(ob + 0x24);
            og_unk28 = *(const uint32_t*)(ob + 0x28);
            og_ns = 0;
            og_autoexec = 0;

            const uint32_t oe = *(const uint32_t*)(ob + 0x40);
            const uint16_t oc = *(const uint16_t*)(ob + 0x18);

            for (uint16_t i = 0; i < oc; ++i)
            {
                const uint32_t e = oe + i * 32;
                if (e + 32 > orig_len)
                    break;

                if (!og_ns)
                    og_ns = *(const uint64_t*)(ob + e + 8);

                if (((unsigned char)ob[e + 0x1D] & 0x01) && !og_autoexec)
                    og_autoexec = *(const uint64_t*)(ob + e);
            }

            installed_key = 0;
        }

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));
        if (size < 0x58)
            return original;

        const char* src = (const char*)(allocAddr + sizeof(int));

        if (!SafePtr((uintptr_t)src, (size_t)size))
            return original;

        if (*(const uint64_t*)src != og_magic)
            return original;

        uint64_t key = 0xCBF29CE484222325ull;
        for (int i = 0; i < size; ++i) {
            key ^= (unsigned char)src[i];
            key *= 0x100000001B3ull;
        }

        if (key == installed_key && asset->buffer == installed_ptr)
            return asset;

        const bool in_place = (size <= (int)orig_len);

        char* dst;

        if (in_place)
            dst = (char*)orig_buffer;
        else if (slot && slot_size >= (size_t)size)
            dst = slot;
        else
        {
            dst = DB_ZoneAlloc((size_t)size, (uint64_t)orig_buffer);
            if (!dst)
                return original;

            slot = dst;
            slot_size = (size_t)size;
        }

        memcpy(dst, src, (size_t)size);

        if (og_name)
            *(uint64_t*)(dst + 8) = og_name;

        *(uint32_t*)(dst + 0x24) = og_crc;
        *(uint32_t*)(dst + 0x28) = og_unk28;

        const uint32_t exp_off = *(const uint32_t*)(dst + 0x40);
        const uint16_t exp_cnt = *(const uint16_t*)(dst + 0x18);

        if (exp_off + (uint32_t)exp_cnt * 32 > (uint32_t)size)
            return original;

        uint64_t src_ns = 0;
        uint32_t first_export = 0;
        bool has_autoexec = false;

        for (uint16_t i = 0; i < exp_cnt; ++i)
        {
            const uint32_t e = exp_off + i * 32;
            if (e + 32 > (uint32_t)size)
                break;

            if (!src_ns)
            {
                src_ns = *(const uint64_t*)(dst + e + 8);
                first_export = e;
            }

            if (((unsigned char)dst[e + 0x1D] & 0x01) && !has_autoexec)
            {
                if (og_autoexec)
                    *(uint64_t*)(dst + e) = og_autoexec;

                has_autoexec = true;
            }
        }

        if (!has_autoexec && first_export && og_autoexec)
        {
            *(uint64_t*)(dst + first_export) = og_autoexec;
            dst[first_export + 0x1D] |= 0x15;
        }

        if (src_ns && og_ns && src_ns != og_ns)
            for (int i = 0; i + 8 <= size; )
            {
                if (*(const uint64_t*)(dst + i) == src_ns)
                {
                    *(uint64_t*)(dst + i) = og_ns;
                    i += 8;
                }
                else
                    ++i;
            }

        asset->buffer = dst;
        asset->len = (uint32_t)size;
        installed_ptr = dst;
        installed_key = key;

        return asset;
    }

    else
    {
        if (strcmp(name, g_findXAsset_vals.name) != 0) return original;

        int size = 0;
        memcpy(&size, (const void*)allocAddr, sizeof(int));

        if (size > 0) {
            const char* src = (const char*)(allocAddr + sizeof(int));
            int scanBytes = 0;
            while (true) {
                if (scanBytes >= size)
                    return original;

                if (src[scanBytes] == '\0') { scanBytes++; break; }
                scanBytes++;
            }

            int vars[3] = { 0 };
            if (scanBytes + (int)sizeof(vars) <= size) {
                memcpy(vars, src + scanBytes, sizeof(vars));

                const int headerPos = scanBytes + (int)sizeof(vars);
                const int clen = vars[0];
                const int ulen = vars[1];
                const int blen = vars[2];
                const int bufBytes = clen ? clen : ulen;

                if (blen > 0 && bufBytes > 0) {
                    size_t totalData = (size_t)bufBytes + (size_t)blen;

                    char* dataBuf =
                        (char*)PMem_AllocFromSource_NoDebug_s(totalData + 1, 4, 2, 5);
                    if (!dataBuf)
                        return original;

                    memcpy(dataBuf, src + headerPos, totalData);
                    dataBuf[totalData] = '\0';

                    ScriptFile* custom = (ScriptFile*)PMem_AllocFromSource_NoDebug_s(
                        sizeof(ScriptFile), 4, 2, 5);
                    if (custom) {
                        custom->name = ((ScriptFile*)original)->name;
                        original = custom;

                        custom->compressedLen = clen;
                        custom->len = ulen;
                        custom->bytecodeLen = blen;
                        custom->buffer = dataBuf;
                        custom->bytecode = (unsigned char*)(dataBuf + bufBytes);
                    }
                }
            }
        }
    }



    return original;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define T5_PARSER_PUB_BASE   (g_id == T5_MP ? 0x2FBB7A0 : 0x2C475A0)
#define T5_PARSER_GLOB_BASE  (g_id == T5_MP ? 0x2FBB7E0 : 0x2C475E0)  
#define T5_SCR_VAR_PUB_BASE  (g_id == T5_MP ? 0x301BA80 : 0xDEADBEEF)

#define SCR_PARSER_PUB_SOURCEBUF(inst) (*(char**)(REBASE_ADDRESS(T5_PARSER_PUB_BASE) + (uint64_t)(inst) * 32 + 24))
#define SCR_PARSER_GLOB_SAVELOOKUP(inst) (*(char**)(REBASE_ADDRESS(T5_PARSER_GLOB_BASE) + (uint64_t)(inst) * 80 + 56))
#define SCR_PARSER_GLOB_SAVELOOKUPLEN(inst) (*(int*)  (REBASE_ADDRESS(T5_PARSER_GLOB_BASE) + (uint64_t)(inst) * 80 + 64))
#define SCR_VAR_PUB(inst) (REBASE_ADDRESS(T5_SCR_VAR_PUB_BASE) + (uint64_t)(inst) * 120)

ChecksumEntry checksumTable[] = {
    { "mp_array", "tdm", 0x71D411B2, 0x75AB6 },
    { "mp_array", "dm", 0xB7EFD6DE, 0x759B4 },
    { "mp_array", "sd", 0x4CCE7C7F, 0x7725E },
    { "mp_array", "dom", 0x7FD6ECBE, 0x77809 },
    { "mp_array", "koth", 0x200DD8A6, 0x7720E },
    { "mp_array", "dem", 0xDC1983BE, 0x77726 },
    { "mp_array", "ctf", 0x7A1C7F90, 0x77C82 },
    { "mp_array", "sab", 0xBF975DC4, 0x7700F },
    { "mp_array", "hlnd", 0x1517CB1F, 0x75C58 },
    { "mp_array", "oic", 0x64426ED2, 0x75E2C },
    { "mp_array", "gun", 0x4AEFB2B3, 0x75E7C },
    { "mp_array", "shrp", 0xD60143A8, 0x763EB },
    { "mp_cracked", "tdm", 0x910F9AA2, 0x756F3 },
    { "mp_cracked", "dm", 0xB83067CE, 0x755F1 },
    { "mp_cracked", "sd", 0xBA9D536F, 0x76E9B },
    { "mp_cracked", "dom", 0xB20ABDAE, 0x77446 },
    { "mp_cracked", "koth", 0x211D1996, 0x76E4B },
    { "mp_cracked", "dem", 0x2D5754AE, 0x77363 },
    { "mp_cracked", "ctf", 0xE4A23C80, 0x778BF },
    { "mp_cracked", "sab", 0xB91C52B4, 0x76C4C },
    { "mp_cracked", "hlnd", 0x438E620F, 0x75895 },
    { "mp_cracked", "oic", 0x5FBCB7C2, 0x75A69 },
    { "mp_cracked", "gun", 0xF9CDC1A3, 0x75AB9 },
    { "mp_cracked", "shrp", 0x2FCB9098, 0x76028 },
    { "mp_crisis", "tdm", 0x2F9CE158, 0x731B1 },
    { "mp_crisis", "dm", 0x24E9D584, 0x730AF },
    { "mp_crisis", "sd", 0x8AD50E65, 0x74959 },
    { "mp_crisis", "dom", 0xE05F0364, 0x74F04 },
    { "mp_crisis", "koth", 0xA7B5E14C, 0x74909 },
    { "mp_crisis", "dem", 0x4A845A64, 0x74E21 },
    { "mp_crisis", "ctf", 0x5116D0B6, 0x7537D },
    { "mp_crisis", "sab", 0x2CC677EA, 0x7470A },
    { "mp_crisis", "hlnd", 0x4B9AE505, 0x73353 },
    { "mp_crisis", "oic", 0xB38AE678, 0x73527 },
    { "mp_crisis", "gun", 0xFC890D99, 0x73577 },
    { "mp_crisis", "shrp", 0x9957A2CE, 0x73AE6 },
    { "mp_firingrange", "tdm", 0x7ADEFC40, 0x71D8D },
    { "mp_firingrange", "dm", 0xE85AE36C, 0x71C8B },
    { "mp_firingrange", "sd", 0x1DBA428D, 0x73535 },
    { "mp_firingrange", "dom", 0x08A9C94C, 0x73AE0 },
    { "mp_firingrange", "koth", 0x17D3D134, 0x734E5 },
    { "mp_firingrange", "dem", 0xB846E04C, 0x739FD },
    { "mp_firingrange", "ctf", 0xC42CA71E, 0x73F59 },
    { "mp_firingrange", "sab", 0x224F7352, 0x732E6 },
    { "mp_firingrange", "hlnd", 0x67F6812D, 0x71F2F },
    { "mp_firingrange", "oic", 0x266A0960, 0x72103 },
    { "mp_firingrange", "gun", 0x7EC766C1, 0x72153 },
    { "mp_firingrange", "shrp", 0xB4264F36, 0x726C2 },
    { "mp_duga", "tdm", 0xB57C3231, 0x764AB },
    { "mp_duga", "dm", 0x8F360C85, 0x763A9 },
    { "mp_duga", "sd", 0x4755F684, 0x77C53 },
    { "mp_duga", "dom", 0xB9C942A5, 0x781FE },
    { "mp_duga", "koth", 0xB40DBFBD, 0x77C03 },
    { "mp_duga", "dem", 0x49820BA5, 0x7811B },
    { "mp_duga", "ctf", 0x4682B913, 0x78677 },
    { "mp_duga", "sab", 0xB1A9775F, 0x77A04 },
    { "mp_duga", "hlnd", 0x3232ABE4, 0x7664D },
    { "mp_duga", "oic", 0x7F4C6911, 0x76821 },
    { "mp_duga", "gun", 0x0A955CD0, 0x76871 },
    { "mp_duga", "shrp", 0xF8008BFB, 0x76DE0 },
    { "mp_hanoi", "tdm", 0x1FFB4738, 0x7607C },
    { "mp_hanoi", "dm", 0x4B930A8C, 0x75F7A },
    { "mp_hanoi", "sd", 0xA1D1334B, 0x77824 },
    { "mp_hanoi", "dom", 0x4871E8AC, 0x77DCF },
    { "mp_hanoi", "koth", 0x6C28C3C4, 0x777D4 },
    { "mp_hanoi", "dem", 0xA765F1AC, 0x77CEC },
    { "mp_hanoi", "ctf", 0x7CAEE89A, 0x78248 },
    { "mp_hanoi", "sab", 0x909E85E6, 0x775D5 },
    { "mp_hanoi", "hlnd", 0xE628A0AB, 0x7621E },
    { "mp_hanoi", "oic", 0x3D081618, 0x763F2 },
    { "mp_hanoi", "gun", 0x46AB7897, 0x76442 },
    { "mp_hanoi", "shrp", 0x2B9E5D82, 0x769B1 },
    { "mp_cairo", "tdm", 0xB5135839, 0x747E1 },
    { "mp_cairo", "dm", 0xB5EB0B8D, 0x746DF },
    { "mp_cairo", "sd", 0xAFA4284C, 0x75F89 },
    { "mp_cairo", "dom", 0xA6F369AD, 0x76534 },
    { "mp_cairo", "koth", 0xCD3F64C5, 0x75F39 },
    { "mp_cairo", "dem", 0xD45372AD, 0x76451 },
    { "mp_cairo", "ctf", 0xD673919B, 0x769AD },
    { "mp_cairo", "sab", 0x426ABEE7, 0x75D3A },
    { "mp_cairo", "hlnd", 0xBC5C15AC, 0x74983 },
    { "mp_cairo", "oic", 0xC012A719, 0x74B57 },
    { "mp_cairo", "gun", 0xDC65FD98, 0x74BA7 },
    { "mp_cairo", "shrp", 0xAE8DE683, 0x75116 },
    { "mp_havoc", "tdm", 0xC61F0CBC, 0x72E88 },
    { "mp_havoc", "dm", 0xD12F58E8, 0x72D86 },
    { "mp_havoc", "sd", 0xDBE093C9, 0x74630 },
    { "mp_havoc", "dom", 0xD6F846C8, 0x74BDB },
    { "mp_havoc", "koth", 0x14E4F4B0, 0x745E0 },
    { "mp_havoc", "dem", 0xE58B9DC8, 0x74AF8 },
    { "mp_havoc", "ctf", 0x6697381A, 0x75054 },
    { "mp_havoc", "sab", 0x9B12474E, 0x743E1 },
    { "mp_havoc", "hlnd", 0xC77BAA69, 0x7302A },
    { "mp_havoc", "oic", 0xA6DF51DC, 0x731FE },
    { "mp_havoc", "gun", 0x650FFAFD, 0x7324E },
    { "mp_havoc", "shrp", 0x93A23A32, 0x737BD },
    { "mp_cosmodrome", "tdm", 0xEC9493EB, 0x76928 },
    { "mp_cosmodrome", "dm", 0xD3B78B17, 0x76826 },
    { "mp_cosmodrome", "sd", 0xD9F93638, 0x780D0 },
    { "mp_cosmodrome", "dom", 0x78B4F0F7, 0x7867B },
    { "mp_cosmodrome", "koth", 0xE84BD8DF, 0x78080 },
    { "mp_cosmodrome", "dem", 0xA2A607F7, 0x78598 },
    { "mp_cosmodrome", "ctf", 0x7E6326C9, 0x78AF4 },
    { "mp_cosmodrome", "sab", 0x4D4F62FD, 0x77E81 },
    { "mp_cosmodrome", "hlnd", 0x9A9CF4D8, 0x76ACA },
    { "mp_cosmodrome", "oic", 0x3D15210B, 0x76C9E },
    { "mp_cosmodrome", "gun", 0x49EFCA6C, 0x76CEE },
    { "mp_cosmodrome", "shrp", 0x3E2FEEE1, 0x7725D },
    { "mp_nuked", "tdm", 0x393F514E, 0x715C0 },
    { "mp_nuked", "dm", 0x7D2DB7A2, 0x714BE },
    { "mp_nuked", "sd", 0xDD34CAA1, 0x72D68 },
    { "mp_nuked", "dom", 0xD1F5CDC2, 0x73313 },
    { "mp_nuked", "koth", 0x1E1972DA, 0x72D18 },
    { "mp_nuked", "dem", 0xA07D96C2, 0x73230 },
    { "mp_nuked", "ctf", 0x8709A630, 0x7378C },
    { "mp_nuked", "sab", 0xFAEA387C, 0x72B19 },
    { "mp_nuked", "hlnd", 0x75A52001, 0x71762 },
    { "mp_nuked", "oic", 0x9F65A82E, 0x71936 },
    { "mp_nuked", "gun", 0x0E4604ED, 0x71986 },
    { "mp_nuked", "shrp", 0x77D85118, 0x71EF5 },
    { "mp_radiation", "tdm", 0x2F2BAB69, 0x771CE },
    { "mp_radiation", "dm", 0x0C62A1BD, 0x770CC },
    { "mp_radiation", "sd", 0x61FD20BC, 0x78976 },
    { "mp_radiation", "dom", 0x2AF537DD, 0x78F21 },
    { "mp_radiation", "koth", 0x7C4ABCF5, 0x78926 },
    { "mp_radiation", "dem", 0x21B100DD, 0x78E3E },
    { "mp_radiation", "ctf", 0x83D8A84B, 0x7939A },
    { "mp_radiation", "sab", 0xA72D2A97, 0x78727 },
    { "mp_radiation", "hlnd", 0xFAC8F61C, 0x77370 },
    { "mp_radiation", "oic", 0xEA8B8249, 0x77544 },
    { "mp_radiation", "gun", 0xA3A24B08, 0x77594 },
    { "mp_radiation", "shrp", 0x05B57333, 0x77B03 },
    { "mp_mountain", "tdm", 0x76837FD7, 0x73A12 },
    { "mp_mountain", "dm", 0x3503F62B, 0x73910 },
    { "mp_mountain", "sd", 0x9D0D952A, 0x751BA },
    { "mp_mountain", "dom", 0x4CF28C4B, 0x75765 },
    { "mp_mountain", "koth", 0x133D1163, 0x7516A },
    { "mp_mountain", "dem", 0xC38E554B, 0x75682 },
    { "mp_mountain", "ctf", 0x5BD63CB9, 0x75BDE },
    { "mp_mountain", "sab", 0x156D3F05, 0x74F6B },
    { "mp_mountain", "hlnd", 0xC28D6A8A, 0x73BB4 },
    { "mp_mountain", "oic", 0x47E756B7, 0x73D88 },
    { "mp_mountain", "gun", 0x87F53F76, 0x73DD8 },
    { "mp_mountain", "shrp", 0x6EBE07A1, 0x74347 },
    { "mp_villa", "tdm", 0x2457A9D2, 0x72DE6 },
    { "mp_villa", "dm", 0xBF7C4626, 0x72CE4 },
    { "mp_villa", "sd", 0xB956E1A5, 0x7458E },
    { "mp_villa", "dom", 0xC2E84C46, 0x74B39 },
    { "mp_villa", "koth", 0x8018A55E, 0x7453E },
    { "mp_villa", "dem", 0xB2439546, 0x74A56 },
    { "mp_villa", "ctf", 0x76067DB4, 0x74FB2 },
    { "mp_villa", "sab", 0xC24F8A00, 0x7433F },
    { "mp_villa", "hlnd", 0x62118705, 0x72F88 },
    { "mp_villa", "oic", 0xA03B90B2, 0x7315C },
    { "mp_villa", "gun", 0xDD3C95F1, 0x731AC },
    { "mp_villa", "shrp", 0x5A20749C, 0x7371B },
    { "mp_russianbase", "tdm", 0xC5C08C4F, 0x74CB3 },
    { "mp_russianbase", "dm", 0x684E417B, 0x74BB1 },
    { "mp_russianbase", "sd", 0x23AD5B1C, 0x7645B },
    { "mp_russianbase", "dom", 0x3AAED75B, 0x76A06 },
    { "mp_russianbase", "koth", 0x3D76E343, 0x7640B },
    { "mp_russianbase", "dem", 0x13DD6E5B, 0x76923 },
    { "mp_russianbase", "ctf", 0xD8DA922D, 0x76E7F },
    { "mp_russianbase", "sab", 0x67BB0061, 0x7620C },
    { "mp_russianbase", "hlnd", 0x0EC729BC, 0x74E55 },
    { "mp_russianbase", "oic", 0xB851696F, 0x75029 },
    { "mp_russianbase", "gun", 0xFF142150, 0x75079 },
    { "mp_russianbase", "shrp", 0x45EE3645, 0x755E8 },
    { "mp_berlinwall2", "tdm", 0x9F15F003, 0x7618A },
    { "mp_berlinwall2", "dm", 0xCE2EC02F, 0x76088 },
    { "mp_berlinwall2", "sd", 0x5A2B9E10, 0x77932 },
    { "mp_berlinwall2", "dom", 0x09614E0F, 0x77EDD },
    { "mp_berlinwall2", "koth", 0xC7CFB3F7, 0x778E2 },
    { "mp_berlinwall2", "dem", 0x1EF9A50F, 0x77DFA },
    { "mp_berlinwall2", "ctf", 0x273A3561, 0x78356 },
    { "mp_berlinwall2", "sab", 0x90DEE095, 0x776E3 },
    { "mp_berlinwall2", "hlnd", 0x25EA94B0, 0x7632C },
    { "mp_berlinwall2", "oic", 0xE7059523, 0x76500 },
    { "mp_berlinwall2", "gun", 0x7D8CA144, 0x76550 },
    { "mp_berlinwall2", "shrp", 0x4AD37F79, 0x76ABF },
    { "mp_discovery", "tdm", 0x2402C353, 0x7695C },
    { "mp_discovery", "dm", 0xE6604F7F, 0x7685A },
    { "mp_discovery", "sd", 0x5F6F7A60, 0x78104 },
    { "mp_discovery", "dom", 0x93CB3D5F, 0x786AF },
    { "mp_discovery", "koth", 0xB9B96B47, 0x780B4 },
    { "mp_discovery", "dem", 0x00EE945F, 0x785CC },
    { "mp_discovery", "ctf", 0x3B2D0EB1, 0x78B28 },
    { "mp_discovery", "sab", 0x9B24DDE5, 0x77EB5 },
    { "mp_discovery", "hlnd", 0x59209100, 0x76AFE },
    { "mp_discovery", "oic", 0xADF10873, 0x76CD2 },
    { "mp_discovery", "gun", 0xEE9BA194, 0x76D22 },
    { "mp_discovery", "shrp", 0x453690C9, 0x77291 },
    { "mp_kowloon", "tdm", 0xBAF17F66, 0x7A827 },
    { "mp_kowloon", "dm", 0x871489BA, 0x7A725 },
    { "mp_kowloon", "sd", 0xF63257B9, 0x7BFCF },
    { "mp_kowloon", "dom", 0x9AD33FDA, 0x7C57A },
    { "mp_kowloon", "koth", 0x43365CF2, 0x7BF7F },
    { "mp_kowloon", "dem", 0x4A8808DA, 0x7C497 },
    { "mp_kowloon", "ctf", 0xFAACBE48, 0x7C9F3 },
    { "mp_kowloon", "sab", 0x145BCC94, 0x7BD80 },
    { "mp_kowloon", "hlnd", 0xE0A58D19, 0x7A9C9 },
    { "mp_kowloon", "oic", 0x0D823646, 0x7AB9D },
    { "mp_kowloon", "gun", 0x2A5A0E05, 0x7ABED },
    { "mp_kowloon", "shrp", 0x5ECBF130, 0x7B15C },
    { "mp_stadium", "tdm", 0xD2EB428C, 0x73D92 },
    { "mp_stadium", "dm", 0x781C98E0, 0x73C90 },
    { "mp_stadium", "sd", 0xBA90DFDF, 0x7553A },
    { "mp_stadium", "dom", 0x39462F00, 0x75AE5 },
    { "mp_stadium", "koth", 0x7C08F418, 0x754EA },
    { "mp_stadium", "dem", 0x21F9F800, 0x75A02 },
    { "mp_stadium", "ctf", 0xF905AF6E, 0x75F5E },
    { "mp_stadium", "sab", 0xFB82D1BA, 0x752EB },
    { "mp_stadium", "hlnd", 0x4949B53F, 0x73F34 },
    { "mp_stadium", "oic", 0x498C196C, 0x74108 },
    { "mp_stadium", "gun", 0xB01EAA2B, 0x74158 },
    { "mp_stadium", "shrp", 0xEAF53A56, 0x746C7 },
    { "mp_gridlock", "tdm", 0xC7C2AA03, 0x7624B },
    { "mp_gridlock", "dm", 0x2361BE57, 0x76149 },
    { "mp_gridlock", "sd", 0xDD25B3D6, 0x779F3 },
    { "mp_gridlock", "dom", 0xA2268477, 0x77F9E },
    { "mp_gridlock", "koth", 0xDE246D8F, 0x779A3 },
    { "mp_gridlock", "dem", 0x45D7CD77, 0x77EBB },
    { "mp_gridlock", "ctf", 0x244C09E5, 0x78417 },
    { "mp_gridlock", "sab", 0xA6525E31, 0x777A4 },
    { "mp_gridlock", "hlnd", 0x4C3C9936, 0x763ED },
    { "mp_gridlock", "oic", 0xAAFBD0E3, 0x765C1 },
    { "mp_gridlock", "gun", 0x03B8B022, 0x76611 },
    { "mp_gridlock", "shrp", 0x297870CD, 0x76B80 },
    { "mp_hotel", "tdm", 0x2491A0D8, 0x793DE },
    { "mp_hotel", "dm", 0x1F64C82C, 0x792DC },
    { "mp_hotel", "sd", 0x9B5ABBEB, 0x7AB86 },
    { "mp_hotel", "dom", 0x09B8464C, 0x7B131 },
    { "mp_hotel", "koth", 0x96C51964, 0x7AB36 },
    { "mp_hotel", "dem", 0xB4494F4C, 0x7B04E },
    { "mp_hotel", "ctf", 0xAD660C3A, 0x7B5AA },
    { "mp_hotel", "sab", 0xF5F36586, 0x7A937 },
    { "mp_hotel", "hlnd", 0xB8FF094B, 0x79580 },
    { "mp_hotel", "oic", 0x1BBACFB8, 0x79754 },
    { "mp_hotel", "gun", 0x9F9ABD37, 0x797A4 },
    { "mp_hotel", "shrp", 0x4D8F8922, 0x79D13 },
    { "mp_outskirts", "tdm", 0xFED19A74, 0x78893 },
    { "mp_outskirts", "dm", 0xBAEE13A0, 0x78791 },
    { "mp_outskirts", "sd", 0x15718041, 0x7A03B },
    { "mp_outskirts", "dom", 0xE6824980, 0x7A5E6 },
    { "mp_outskirts", "koth", 0x9A8B8D68, 0x79FEB },
    { "mp_outskirts", "dem", 0x4485E080, 0x7A503 },
    { "mp_outskirts", "ctf", 0xCE6C5A52, 0x7AA5F },
    { "mp_outskirts", "sab", 0xC1022486, 0x79DEC },
    { "mp_outskirts", "hlnd", 0x86FD2EE1, 0x78A35 },
    { "mp_outskirts", "oic", 0x81D7D794, 0x78C09 },
    { "mp_outskirts", "gun", 0x0615A275, 0x78C59 },
    { "mp_outskirts", "shrp", 0xEA4EC66A, 0x791C8 },
    { "mp_zoo", "tdm", 0x2C4D0712, 0x74F72 },
    { "mp_zoo", "dm", 0x0C7EA666, 0x74E70 },
    { "mp_zoo", "sd", 0xA748F425, 0x7671A },
    { "mp_zoo", "dom", 0x42CAE486, 0x76CC5 },
    { "mp_zoo", "koth", 0x599D479E, 0x766CA },
    { "mp_zoo", "dem", 0x1EB1ED86, 0x76BE2 },
    { "mp_zoo", "ctf", 0x1685FE74, 0x7713E },
    { "mp_zoo", "sab", 0xD50C9FC0, 0x764CB },
    { "mp_zoo", "hlnd", 0x8C298185, 0x75114 },
    { "mp_zoo", "oic", 0xAB2B75F2, 0x752E8 },
    { "mp_zoo", "gun", 0xB6723D71, 0x75338 },
    { "mp_zoo", "shrp", 0x5CC9EB5C, 0x758A7 },
    { "mp_drivein", "tdm", 0x2C3A33EF, 0x76A93 },
    { "mp_drivein", "dm", 0xB0752843, 0x76991 },
    { "mp_drivein", "sd", 0x7C8405C2, 0x7823B },
    { "mp_drivein", "dom", 0x7EECEE63, 0x787E6 },
    { "mp_drivein", "koth", 0x7F1D177B, 0x781EB },
    { "mp_drivein", "dem", 0x02763763, 0x78703 },
    { "mp_drivein", "ctf", 0xEF12C3D1, 0x78C5F },
    { "mp_drivein", "sab", 0xD5EC381D, 0x77FEC },
    { "mp_drivein", "hlnd", 0x637BEB22, 0x76C35 },
    { "mp_drivein", "oic", 0x52F85ACF, 0x76E09 },
    { "mp_drivein", "gun", 0x9FAA220E, 0x76E59 },
    { "mp_drivein", "shrp", 0xF78CEAB9, 0x773C8 },
    { "mp_area51", "tdm", 0xE074037F, 0x78EDB },
    { "mp_area51", "dm", 0xDEA655D3, 0x78DD9 },
    { "mp_area51", "sd", 0x530E19D2, 0x7A683 },
    { "mp_area51", "dom", 0x69C24BF3, 0x7AC2E },
    { "mp_area51", "koth", 0xFD78590B, 0x7A633 },
    { "mp_area51", "dem", 0x7B5114F3, 0x7AB4B },
    { "mp_area51", "ctf", 0x97E81661, 0x7B0A7 },
    { "mp_area51", "sab", 0x5DBE1CAD, 0x7A434 },
    { "mp_area51", "hlnd", 0x30570F32, 0x7907D },
    { "mp_area51", "oic", 0x6FE97A5F, 0x79251 },
    { "mp_area51", "gun", 0x39ECC81E, 0x792A1 },
    { "mp_area51", "shrp", 0xC7D45949, 0x79810 },
    { "mp_golfcourse", "tdm", 0xFDAB503E, 0x72E78 },
    { "mp_golfcourse", "dm", 0x8B7BA86A, 0x72D76 },
    { "mp_golfcourse", "sd", 0x0C94AC4B, 0x74620 },
    { "mp_golfcourse", "dom", 0x21A5764A, 0x74BCB },
    { "mp_golfcourse", "koth", 0x11424C32, 0x745D0 },
    { "mp_golfcourse", "dem", 0x3167CD4A, 0x74AE8 },
    { "mp_golfcourse", "ctf", 0x9D1A099C, 0x75044 },
    { "mp_golfcourse", "sab", 0xEA5F6CD0, 0x743D1 },
    { "mp_golfcourse", "hlnd", 0xAC6762EB, 0x7301A },
    { "mp_golfcourse", "oic", 0x3075B55E, 0x731EE },
    { "mp_golfcourse", "gun", 0xAAA6677F, 0x7323E },
    { "mp_golfcourse", "shrp", 0x37A4E3B4, 0x737AD },
    { "mp_silo", "tdm", 0xFC6613E2, 0x74F80 },
    { "mp_silo", "dm", 0x32BAB736, 0x74E7E },
    { "mp_silo", "sd", 0x162C87F5, 0x76728 },
    { "mp_silo", "dom", 0xD9B49556, 0x76CD3 },
    { "mp_silo", "koth", 0x220BB06E, 0x766D8 },
    { "mp_silo", "dem", 0xD9C09E56, 0x76BF0 },
    { "mp_silo", "ctf", 0x5A6F6544, 0x7714C },
    { "mp_silo", "sab", 0x3D992290, 0x764D9 },
    { "mp_silo", "hlnd", 0xDB5CF555, 0x75122 },
    { "mp_silo", "oic", 0x28CFE2C2, 0x752F6 },
    { "mp_silo", "gun", 0x6400ED41, 0x75346 },
    { "mp_silo", "shrp", 0x703E9A2C, 0x758B5 },
};

void CreateFilename(char* outBuffer, const char* filename)
{
    const char basePath[] = "/data/raw/";

    int i = 0;
    while (basePath[i] != '\0' && i < 255)
    {
        outBuffer[i] = basePath[i];
        i++;
    }

    int j = 0;
    while (filename[j] != '\0' && (i + j) < 255)
    {
        if (filename[j] == '\\')
            outBuffer[i + j] = '/';
        else
            outBuffer[i + j] = filename[j];

        j++;
    }

    outBuffer[i + j] = '\0';
}

static bool HasScriptExtension(const char* name)
{
    const char* dot = strrchr(name, '.');
    return dot && (!strcmp(dot, ".gsc") || !strcmp(dot, ".csc"));
}

ChecksumEntry* FindChecksum(const char* map, const char* gametype)
{
    for (int i = 0; i < sizeof(checksumTable) / sizeof(ChecksumEntry); i++)
    {
        if (!strcmp(checksumTable[i].mapname, map) && !strcmp(checksumTable[i].gametype, gametype))
        {
            return &checksumTable[i];
        }
    }
    return NULL;
}

static int64_t Scr_GetChecksum_Real(uint32_t* out, uint32_t inst)
{
    out[0] = *(uint32_t*)(SCR_VAR_PUB(inst) + 64);
    out[1] = *(uint32_t*)(REBASE_ADDRESS(0x2B96FE0) + (uint64_t)inst * 139336);
    out[2] = *(uint32_t*)(SCR_VAR_PUB(inst) + 96) - *(uint32_t*)(SCR_VAR_PUB(inst) + 88);
    return out[2];
}

int64_t Scr_GetCheckSum_h(uint32_t* out, uint32_t inst)
{
    int64_t result = Scr_GetChecksum_Real(out, inst);

    if (inst != SCRIPTINSTANCE_SERVER)
        return result;

    ChecksumEntry* entry = FindChecksum(Dvar_GetString_s("ui_mapname"), Dvar_GetString_s("g_gametype"));
    if (!entry)
        return result;

    out[0] = entry->checksum;
    out[1] = entry->programLen;
    return result;
}

static char* ReadStagedScript(int* outLen)
{
    if (!allocAddr)
        return NULL;

    int size = 0;
    memcpy(&size, (const void*)allocAddr, sizeof(int));
    if (size <= 0)
        return NULL;

    const char* src = (const char*)(allocAddr + sizeof(int));

    char* buf = (char*)Hunk_AllocateTempMemoryHigh_s(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, src, size);

    *outLen = size;
    return buf;
}

char* Scr_ReadFile_FastFile_h(T5_scriptInstance_t inst, const char* filename, const char* extFilename, char* codePos, int archive)
{
    char* result = NULL;

    if (archive && SCR_PARSER_GLOB_SAVELOOKUP(inst))
    {
        int idx = --SCR_PARSER_GLOB_SAVELOOKUPLEN(inst);
        char* entry = SCR_PARSER_GLOB_SAVELOOKUP(inst) + (uint64_t)idx * 16;
        const char* src = *(const char**)entry;
        int len = *(int*)(entry + 8);
        if (len >= 0)
        {
            result = (char*)Hunk_AllocateTempMemoryHigh_s(len + 1);
            for (int i = 0; i < len; i++)
                result[i] = src[i] ? src[i] : '\n';
            result[len] = '\0';
        }
        SCR_PARSER_PUB_SOURCEBUF(inst) = 0;
        return result;
    }

    bool isScript = HasScriptExtension(extFilename);

    if (isScript)
    {
        const char* stagedTarget = (g_id == T5_MP) ? "maps/mp/gametypes/_clientids.gsc" : "maps/_hud.gsc";

        if (!strcmp(extFilename, stagedTarget))
        {
            int stagedLen = 0;
            char* staged = ReadStagedScript(&stagedLen);
            if (staged)
            {
                SCR_PARSER_PUB_SOURCEBUF(inst) = 0;
                return staged;
            }
        }
    }

    RawFile* rawFile = T5_DB_FindXAssetHeader_s(ASSET_TYPE_RAWFILE, extFilename, true, -1).rawfile;
    if (rawFile)
    {
        if (!isScript)
        {
            result = rawFile->buffer;
        }
        else
        {
            uint64_t outLen = *(int*)(rawFile->buffer);
            uint64_t inLen = *(unsigned int*)(rawFile->buffer + 4);
            result = (char*)Hunk_AllocateTempMemoryHigh_s((int)outLen);
            Z_Uncompress_s((unsigned char*)result, &outLen, (const unsigned char*)rawFile->buffer + 8, inLen);
        }
    }

    SCR_PARSER_PUB_SOURCEBUF(inst) = 0;
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* debug_log_thread(void* arg) {
    return nullptr;
}

void* relay_title_thread(void* arg) {
    int listenSock = -1;

    while (true) 
    {
        if (listenSock < 0) {
            listenSock = sceNetSocket("TitleSocket", SCE_NET_AF_INET,
                SCE_NET_SOCK_STREAM, 0);
            if (listenSock < 0) {
                sceKernelUsleep(500000);
                continue;
            }

            SceNetSockaddrIn serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = SCE_NET_AF_INET;
            serverAddr.sin_port = sceNetHtons(6969);
            serverAddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);

            if (sceNetBind(listenSock, (SceNetSockaddr*)&serverAddr,
                sizeof(serverAddr)) < 0) {
                sceNetSocketClose(listenSock);
                listenSock = -1;
                sceKernelUsleep(500000);
                continue;
            }

            if (sceNetListen(listenSock, 1) < 0) {
                sceNetSocketClose(listenSock);
                listenSock = -1;
                sceKernelUsleep(500000);
                continue;
            }
        }

        int clientSock = sceNetAccept(listenSock, NULL, NULL);
        if (clientSock < 0) {
            sceKernelUsleep(100000);
            continue;
        }

        const char* code;

        switch (g_id) {
        case T5_SP:
        case T5_MP:
            code = "t5";
            break;

        case T6_SP:
        case T6_MP:
        case T6_ZM:
            code = "t6";
            break;

        case IW6_SP:
        case IW6_MP:
            code = "iw6";
            break;

        case S1_SP:
        case S1_MP:
            code = "s1";
            break;

        case T7:
            code = "t7";
            break;

        case IW7:
            code = "iw7";
            break;

        case H1_SP:
        case H1_MP:
            code = "h1";
            break;

        case S2_SP:
        case S2_MP:
            code = "s2";
            break;

        case T8:
            code = "t8";
            break;

        case IW8:
            code = "iw8";
            break;

        case H2:
            code = "h2";
            break;

        case T9:
            code = "t9";
            break;

        case S4:
            code = "s4";
            break;

        case IW9_SP:
        case IW9_MP:
            code = "iw9";
            break;

        case JUP_SP:
        case JUP_MP:
            code = "jup";
            break;

        case T10_SP:
        case T10_MP:
            code = "t10";
            break;

        default:
            code = "unknown";
            break;
        }

        char buffer[64];
        int len = snprintf(buffer, sizeof(buffer), "%s", code);
        sceNetSend(clientSock, buffer, len, 0);
        sceNetSocketClose(clientSock);
    }

    return NULL;
}

void* relay_alloc_thread(void* arg) {
    const uint64_t MEM_SIZE = 256 * 1024;

    void* ptr = nullptr;
    int r = sceKernelMmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON, -1, 0, &ptr);

    if (r < 0 || !ptr || ptr == MAP_FAILED)
        for (uint64_t hint = 0x200000000ULL; hint < 0x400000000ULL; hint += 0x10000000ULL) {
            ptr = nullptr;
            r = sceKernelMmap((void*)hint, MEM_SIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANON, -1, 0, &ptr);

            if (r >= 0 && ptr && ptr != MAP_FAILED)
                break;
        }

    if (r < 0 || !ptr || ptr == MAP_FAILED)
        return NULL;

    allocAddr = (unsigned long long)ptr;

	DbgPrint("[RELAY] allocated 0x%llX bytes at 0x%llX", (unsigned long long)MEM_SIZE, (unsigned long long)ptr);

    int listenSock =
        sceNetSocket("ListenSocket", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
    if (listenSock < 0) {
        sceKernelMunmap(ptr, MEM_SIZE);
        return NULL;
    }

    SceNetSockaddrIn serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = SCE_NET_AF_INET;
    serverAddr.sin_port = sceNetHtons(6767);
    serverAddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);

    if (sceNetBind(listenSock, (SceNetSockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        sceNetSocketClose(listenSock);
        sceKernelMunmap(ptr, MEM_SIZE);
        return NULL;
    }

    if (sceNetListen(listenSock, 1) < 0) {
        sceNetSocketClose(listenSock);
        sceKernelMunmap(ptr, MEM_SIZE);
        return NULL;
    }

    while (true) {
        int clientSock = sceNetAccept(listenSock, NULL, NULL);
        if (clientSock >= 0) {
            char buffer[32];
            int len = snprintf(buffer, sizeof(buffer), "0x%llX", (unsigned long long)ptr);
            sceNetSend(clientSock, buffer, len, 0);
            sceNetSocketClose(clientSock);
        }
        else
            sceKernelUsleep(100000);
    }

    sceNetSocketClose(listenSock);

    return NULL;
}

void* ncs_inject_thread(void*) {
    int tableStride = g_textPatch_vals.iw7Abi ? 16 : 24;
    int maxEntries = g_textPatch_vals.iw7Abi ? 23 : 96;

    uintptr_t cands[16];
    int nCand = 0;

    if (g_textPatch_vals.iw7Abi && g_textPatch_vals.ncsStringTable)
        cands[nCand++] = g_textPatch_vals.ncsStringTable;
    else
        nCand = FindNcsCandidates(cands, 16);

    if (!nCand)
        return nullptr;

    while (true) {
        sceKernelSleep(2);

        if (g_textPatch_vals.iw7Abi && g_ncsLoc.entry >= 0) {
            uintptr_t e = g_ncsLoc.table + g_ncsLoc.stride * g_ncsLoc.entry;

            if (!SafePtr(e, 24) ||
                *(uintptr_t*)e != g_ncsLoc.head ||
                !SafePtr(g_ncsLoc.head, 24) ||
                *(uintptr_t*)g_ncsLoc.head != g_ncsLoc.inner ||
                !SafePtr(g_ncsLoc.inner, 64))
            {
                DbgPrint("[NCS] location stale, re-probing");
                g_ncsLoc.entry = -1;
                g_ncsArr = 0;
                g_ncsNewCount = 0;
            }
        }

        if (g_ncsLoc.entry < 0) {
            if (g_textPatch_vals.iw7Abi) {
                uintptr_t table = cands[0];
                uintptr_t head = *(uintptr_t*)(table + tableStride * 7);
                if (!SafePtr(head, 16))
                    continue;

                uintptr_t inner = *(uintptr_t*)head;
                if (!SafePtr(inner, 64))
                    continue;

                g_ncsLoc.entry = 7;
                g_ncsLoc.arrOff = 24;
                g_ncsLoc.countOff = 20;
                g_ncsLoc.table = table;
                g_ncsLoc.stride = tableStride;
                g_ncsLoc.head = head;
                g_ncsLoc.inner = inner;
            }
            else {
                bool found = false;

                for (int i = 0; i < nCand && !found; i++)
                    found = NcsProbe(cands[i], tableStride, maxEntries, &g_ncsLoc);

                if (!found)
                    continue;
            }

            g_textPatch_vals.ncsStringTable = g_ncsLoc.table;
            g_textPatch_vals.ncsArrayOffset = g_ncsLoc.arrOff;
        }

        if (g_ncsArr && *(uintptr_t*)(g_ncsLoc.inner + g_ncsLoc.arrOff) == g_ncsArr)
            continue;

        NcsInject_Install();
    }

    return nullptr;
}

void* start_thread(void*) {
    remove("/data/GSCLabs/log.txt");

    GetBaseAddress();

    DetectAndNotifyGame();

    if (hasValidOffset(g_offsets.hudelems_pointer))
        if (g_id == H1_MP || g_id == S2_MP)
            g_hudelems = (void*)g_offsets.hudelems_pointer;
        else
            g_hudelems = *(void**)g_offsets.hudelems_pointer;

    if (hasValidOffset(g_offsets.SV_SetConfigString))
        SV_SetConfigString_s = (SV_SetConfigString_t)g_offsets.SV_SetConfigString;

    if (hasValidOffset(g_offsets.HECmd_ClearAllTextAfterHudElem))
        PatchValue(g_offsets.HECmd_ClearAllTextAfterHudElem, (uint64_t)HECmd_ClearAllTextAfterHudElem);

    if (hasValidOffset(g_offsets.PMem_AllocFromSource_NoDebug))
        PMem_AllocFromSource_NoDebug_s = (PMem_AllocFromSource_NoDebug_t)g_offsets.PMem_AllocFromSource_NoDebug;

    if (hasValidOffset(g_offsets.Dvar_GetString))
        Dvar_GetString_s = (Dvar_GetString_t)g_offsets.Dvar_GetString;

    if (hasValidOffset(g_offsets.HECmd_SetText) && hasValidOffset(g_textPatch_vals.scriptSystem) && hasValidOffset(g_textPatch_vals.hudElemBase))
    {
        CreateDetour(HECmd_SetText, g_offsets.HECmd_SetText);
        scePthreadCreate(&g_ncsInjectThread, nullptr, ncs_inject_thread, nullptr, "NCS Inject");

        for (int i = 0; i < 256; i++) 
            g_idxToSlot[i] = -1;
    }

    if (g_id == IW6_MP)
        PatchValue(REBASE_ADDRESS(0x2AB1C), (char)0x75);

    if (g_id == T5_SP || g_id == T5_MP)
    {
        T5_DB_FindXAssetHeader_s = (T5_DB_FindXAssetHeader_t)g_offsets.DB_FindXAssetHeader;
        Z_Uncompress_s = (Z_Uncompress_t)g_offsets.Z_Uncompress;
        Hunk_AllocateTempMemoryHigh_s = (Hunk_AllocateTempMemoryHigh_t)g_offsets.Hunk_AllocateTempMemoryHigh;

        CreateDetour(Scr_ReadFile_FastFile, g_offsets.Scr_ReadFile_FastFile);
        if (g_id == T5_MP)
            CreateDetour(Scr_GetCheckSum, g_offsets.Scr_GetChecksum);
    }
    else
    {
        CreateDetour(DB_FindXAssetHeader, g_offsets.DB_FindXAssetHeader);
    }

    if (g_id == T8 || g_id == T9)
        T89_Huds_Init();

    return nullptr;
}

void* stop_thread(void*) {
    RemoveDetour(DB_FindXAssetHeader, g_offsets.DB_FindXAssetHeader);
    RemoveDetour(HECmd_SetText, g_offsets.HECmd_SetText);
    RemoveDetour(Scr_ReadFile_FastFile, g_offsets.Scr_ReadFile_FastFile);
    RemoveDetour(Scr_GetCheckSum, g_offsets.Scr_GetCheckSum);

    return nullptr;
}

extern "C" {
    int module_start(size_t argc, const void* args) {
        ScePthread thread;

        scePthreadCreate(&thread, nullptr, start_thread, nullptr, "GSCLabs Init");
        scePthreadJoin(thread, nullptr);

        scePthreadCreate(&thread, nullptr, relay_alloc_thread, nullptr, "Mem Alloc Relay");
        scePthreadDetach(thread);

        scePthreadCreate(&thread, nullptr, relay_title_thread, nullptr, "Codename Relay");
        scePthreadDetach(thread);

        return 0;
    }

    int module_stop(size_t argc, const void* args) {
        ScePthread thread;
        scePthreadCreate(&thread, nullptr, stop_thread, nullptr, "GSCLabs Fini");
        scePthreadJoin(thread, nullptr);

        return 0;
    }
}
