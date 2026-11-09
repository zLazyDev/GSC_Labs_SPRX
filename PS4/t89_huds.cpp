#include "headers.hpp"

DECLARE_DETOUR(Scr_GetFunction)
DECLARE_DETOUR(Scr_GetMethod)
DECLARE_DETOUR(CScr_GetFunction)
DECLARE_DETOUR(CScr_GetMethod)
DECLARE_DETOUR(Renderer)
DECLARE_DETOUR(Scr_GetObjectField)
DECLARE_DETOUR(Scr_SetObjectField)
DECLARE_DETOUR(ScrVm_Error)
DECLARE_DETOUR(ScrVar_RemoveReference)

#pragma region Support

void DumpCode(const char* name, uintptr_t imageAddr, int before, int after)
{
    const uintptr_t rt = REBASE_ADDRESS(imageAddr);
    const uintptr_t lo = rt - (uintptr_t)before;
    const int       n  = before + after;

    if (!SafePtr(lo, (size_t)n))
    {
        DbgPrint("[Code] %s @ img 0x%llX - unreadable", name,
            (unsigned long long)imageAddr);
        return;
    }

    DbgPrint("[Code] %s img 0x%llX (error raised at this return address)",
        name, (unsigned long long)imageAddr);

    const unsigned char* p = (const unsigned char*)lo;

    for (int off = 0; off < n; off += 16)
    {
        char line[128];
        int  w = 0;

        for (int i = 0; i < 16 && (off + i) < n; ++i)
            w += snprintf(line + w, sizeof(line) - (unsigned)w, "%02X ", p[off + i]);

        DbgPrint("[Code]   %+05d img 0x%llX: %s",
            off - before,
            (unsigned long long)(imageAddr - (uintptr_t)before + (uintptr_t)off),
            line);
    }
}

void T89_Huds_Init()
{
    if (g_id != T8 && g_id != T9)
        return;

    R_AddCmdDrawQuadPicW_s = (R_AddCmdDrawQuadPicW_t)g_offsets.R_AddCmdDrawQuadPicW;
    R_AddCmdDrawText_s = (R_AddCmdDrawText_t)g_offsets.R_AddCmdDrawText;

    if (g_id == T8)
    {

        ScrVm_GetString_s = (ScrVm_GetString_t)REBASE_ADDRESS(0xAADCB0);
        ScrVm_GetInt_s = (ScrVm_GetInt_t)REBASE_ADDRESS(0xAB4D00);
        ScrVm_GetVector_s = (ScrVm_GetVector_t)REBASE_ADDRESS(0xAB6480);
        ScrVm_AddInt_s = (ScrVm_AddInt_t)REBASE_ADDRESS(0xAB6900);
        ScrVm_AddStruct_s = (ScrVm_AddStruct_t)REBASE_ADDRESS(0xAB7A50);
        ScrVm_AddEntity_s = (ScrVm_AddEntity_t)REBASE_ADDRESS(0xBEA510);
        ScrVar_EntityBridge_s = (ScrVar_EntityBridge_t)REBASE_ADDRESS(0xAA4810);

        g_vmBlock = REBASE_ADDRESS(0x3BE8FF0);
        g_vm.pubIsPointer = false;
        g_vm.stride = 0x16868;
        g_vm.tableOff = 0;
        g_vm.topOff = 0xE048;
        g_vm.numParamOff = 0x16860;
        g_vm.stackBeg = 0xE850;
        g_vm.stackEnd = 0x16850;

        g_scrPlaceArray = REBASE_ADDRESS(0x43BFEC0);
        g_scrPlaceStride = 0x74;

        g_varGlobals = REBASE_ADDRESS(0x3BDDC50);
        g_varBucketMask = 0x7FFFFu;
        g_var.instStride = 0x28;
        g_var.sharded = false;
        g_var.shardNodeOff = 0;
        g_var.bucketOff = 0x00;
        g_var.hash = VARHASH_T8;
        g_var.shardArrayOff = 0x08;
        g_var.nodeParentOff = 0x14;
        g_var.nodeNextOff = 0x18;
        g_var.probed = true;

        g_strTable = REBASE_ADDRESS(0x3686100);
        ScrStr_Follow_s = (ScrStr_Follow_t)REBASE_ADDRESS(0x1509330);
        g_strStride = 20;
        g_strTextOff = 16;
        g_strMaxIndex = 0x3FFFFu;

        R_TextWidth_s = (R_TextWidth_t)REBASE_ADDRESS(0x104CE50);

        CreateDetour(Renderer, REBASE_ADDRESS(0x15636D0));
        CreateDetour(Scr_GetFunction, REBASE_ADDRESS(0xF48F30));
        CreateDetour(Scr_GetMethod, REBASE_ADDRESS(0xF6F090));
        CreateDetour(CScr_GetFunction, REBASE_ADDRESS(0x935570));
        CreateDetour(CScr_GetMethod, REBASE_ADDRESS(0x936D40));

        CreateDetour(ScrVm_Error, REBASE_ADDRESS(0xAAA6D0));
        CreateDetour(ScrVar_RemoveReference, REBASE_ADDRESS(0xA9FEB0));
    }
    else if (g_id == T9)
    {
        CreateDetour(Renderer, REBASE_ADDRESS(0x1542050));
        CreateDetour(Scr_GetFunction, REBASE_ADDRESS(0x16BDA40));
        CreateDetour(Scr_GetMethod, REBASE_ADDRESS(0x16E6310));
        CreateDetour(CScr_GetFunction, REBASE_ADDRESS(0x1632FA0));
        CreateDetour(CScr_GetMethod, REBASE_ADDRESS(0x1635110));
        CreateDetour(ScrVm_Error, REBASE_ADDRESS(0x15E43C0));
        CreateDetour(ScrVar_RemoveReference, REBASE_ADDRESS(0x15D6870));

        ScrVm_GetString_s = (ScrVm_GetString_t)REBASE_ADDRESS(0x15E7330);
        ScrVm_GetInt_s = (ScrVm_GetInt_t)REBASE_ADDRESS(0x15ECE80);
        ScrVm_GetVector_s = (ScrVm_GetVector_t)REBASE_ADDRESS(0x15EDD90);
        ScrVm_AddInt_s = (ScrVm_AddInt_t)REBASE_ADDRESS(0x15EE2E0);
        ScrVm_AddStruct_s = (ScrVm_AddStruct_t)REBASE_ADDRESS(0x15D8610);
        ScrVm_AddEntity_s = (ScrVm_AddEntity_t)REBASE_ADDRESS(0x73B270);
        ScrVar_EntityBridge_s = (ScrVar_EntityBridge_t)REBASE_ADDRESS(0x15DDB30);

        g_vmBlock = REBASE_ADDRESS(0xCDFDCB0);
        g_vm.pubIsPointer = true;
        g_vm.stride = 0x199BD0;
        g_vm.tableOff = 0xA8058;
        g_vm.topOff = 0x08;
        g_vm.numParamOff = 0xC0CC;
        g_vm.stackBeg = 0;
        g_vm.stackEnd = 0;

        g_textBaselineNudge = 0.0f;

        g_scrPlaceArray = REBASE_ADDRESS(0x2E51310);
        g_scrPlaceStride = 0x74;

        g_varGlobals = REBASE_ADDRESS(0xCDFD610);
        g_varBucketMask = 0xFFFFFu;
        g_var.instStride = 0x228;
        g_var.sharded = true;
        g_var.shardNodeOff = 0x22220;
        g_var.bucketOff = 0x00;
        g_var.hash = VARHASH_T9;
        g_var.shardArrayOff = 0x08;
        g_var.nodeParentOff = 0x14;
        g_var.nodeNextOff = 0x18;
        g_var.probed = false;

        R_TextWidthT9_s = (R_TextWidthT9_t)REBASE_ADDRESS(0x90F3E0);

        g_strTable = REBASE_ADDRESS(0xC533EA0);
        ScrStr_Follow_s = (ScrStr_Follow_t)REBASE_ADDRESS(0xD22B40);
        g_strStride = 16;
        g_strTextOff = 24;
        g_strMaxIndex = 0x7FFFFu;

        CmdAlloc_s = (CmdAlloc_t)REBASE_ADDRESS(0x17DFC30);

        BuildQuad_s = (BuildQuad_t)REBASE_ADDRESS(0x1346D90);

    }

    DbgPrint("[HUD] build %s %s  builtins=%d methods=%d",
        __DATE__, __TIME__, g_t8_builtinCount, g_t8_methodCount);
    DbgPrint("[HUD Offset Values]");
    DbgPrint("  CScr_GetFunction                                   : 0x%p", (void*)CScr_GetFunction_s);
    DbgPrint("  CScr_GetMethod                                     : 0x%p", (void*)CScr_GetMethod_s);
    DbgPrint("  Scr_GetFunction                                    : 0x%p", (void*)Scr_GetFunction_s);
    DbgPrint("  Scr_GetMethod                                      : 0x%p", (void*)Scr_GetMethod_s);
    DbgPrint("  ScrVm_GetString                                    : 0x%p", (void*)ScrVm_GetString_s);
    DbgPrint("  ScrVm_GetInt                                       : 0x%p", (void*)ScrVm_GetInt_s);
    DbgPrint("  ScrVm_GetVector                                    : 0x%p", (void*)ScrVm_GetVector_s);
    DbgPrint("  ScrVm_AddInt                                       : 0x%p", (void*)ScrVm_AddInt_s);
    DbgPrint("  ScrVm_AddStruct                                    : 0x%p", (void*)ScrVm_AddStruct_s);
    DbgPrint("  g_vmBlock                                          : 0x%p", (void*)g_vmBlock);
    DbgPrint("  R_AddCmdDrawQuadPicW                               : 0x%p", (void*)R_AddCmdDrawQuadPicW_s);
    DbgPrint("  R_AddCmdDrawText                                   : 0x%p", (void*)R_AddCmdDrawText_s);
    DbgPrint("  CmdAlloc                                           : 0x%p", (void*)CmdAlloc_s);
    DbgPrint("  BuildQuad                                          : 0x%p", (void*)BuildQuad_s);
    DbgPrint("  R_TextWidth                                        : 0x%p", (void*)R_TextWidth_s);
    DbgPrint("  whiteMaterial                                      : 0x%p", (void*)REBASE_ADDRESS(g_id == T8 ? 0x42DDBA8 : 0xB766128));
    DbgPrint("  normalFont                                         : 0x%p", (void*)T89_Font());
    DbgPrint("  R_TextWidth (T9 form)                              : 0x%p", (void*)R_TextWidthT9_s);

    for (int i = 0; i < FONT_COUNT; ++i)
        DbgPrint("  font[%d] %-11s : 0x%p  pixelHeight=%d", i, FontNameOf(i),
            (void*)T89_FontRaw(i), *(const int*)(T89_FontRaw(i) + 0x10));

    if (Scr_GetFunction_s || Scr_GetMethod_s)
    {
        struct { uint32_t hash; const char* name; bool method; } checks[] = {
            { H_NEWCLIENTHUDELEM, "newClientHudElem", false },
            { H_CREATEFONTSTRING, "createFontString", true  },
            { H_SETPOINT,         "setPoint",         true  },
            { H_SETSHADER,        "setShader",        true  },
            { H_SETTEXT,          "setText",          true  },
            { H_SETPARENT,        "setParent",        true  },
            { H_DESTROY,          "destroy",          true  },
            { H_SETCOLOR,         "setColor",         true  },
            { H_SETALPHA,         "setAlpha",         true  },
            { H_SETSORT,          "setSort",          true  },
            { H_SETHIDDEN,        "setHidden",        true  },
            { H_FADEOVERTIME,            "fadeOverTime",            true },
            { H_MOVEOVERTIME,            "moveOverTime",            true },
            { H_SCALEOVERTIME,           "scaleOverTime",           true },
            { H_CHANGEFONTSCALEOVERTIME, "changeFontScaleOverTime", true },
            { H_SETALIGNX,               "setAlignX",               true },
            { H_SETALIGNY,               "setAlignY",               true },
            { H_SETHORZALIGN,            "setHorzAlign",            true },
            { H_SETVERTALIGN,            "setVertAlign",            true },
        };

        for (unsigned i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i)
        {
            int t = 0, mn = 0, mx = 0;
            GetFunction_t stock = checks[i].method ? Scr_GetMethod_s : Scr_GetFunction_s;
            if (!stock)
                continue;

            if (stock((int)checks[i].hash, &t, &mn, &mx))
            {

                t8_builtin_t* tbl = checks[i].method ? g_t8_methods : g_t8_builtins;
                const int      n   = checks[i].method ? g_t8_methodCount : g_t8_builtinCount;

                for (int k = 0; k < n; ++k)
                    if (tbl[k].hash == checks[i].hash)
                        tbl[k].hash = 0;

                DbgPrint("[HUD] NAME COLLISION: %s (%08X) is ALREADY an engine %s "
                    "(args %d..%d) - OURS DISABLED, engine keeps it, pick another name",
                    checks[i].name, checks[i].hash,
                    checks[i].method ? "method" : "function", mn, mx);
            }
            else
                DbgPrint("[HUD] name free: %s (%08X)", checks[i].name, checks[i].hash);
        }
    }

}

#pragma endregion

#pragma region VM Access

char* ScrVm_Block(int inst)
{
    if (!g_vmBlock)
        return nullptr;

    const uint64_t slot =
        g_vmBlock + g_vm.stride * (unsigned)inst + g_vm.tableOff;

    if (!g_vm.pubIsPointer)
        return (char*)slot;

    if (!SafePtr(slot, sizeof(void*)))
        return nullptr;

    char* pub = *(char**)slot;

    if (!pub || !SafePtr((uintptr_t)pub, 16))
        return nullptr;

    return pub;
}

char* ScrVm_Top(int inst)
{
    char* blk = ScrVm_Block(inst);
    if (!blk)
        return nullptr;

    if (!SafePtr((uintptr_t)(blk + g_vm.topOff), sizeof(void*)))
        return nullptr;

    char* top = *(char**)(blk + g_vm.topOff);
    if (!top)
        return nullptr;

    if (g_vm.stackBeg && g_vm.stackEnd)
    {
        if (top < blk + g_vm.stackBeg || top >= blk + g_vm.stackEnd)
            return nullptr;
    }

    return top;
}

int ScrVm_GetNumParam(int inst)
{
    char* blk = ScrVm_Block(inst);

    if (!blk || !SafePtr((uintptr_t)(blk + g_vm.numParamOff), sizeof(int)))
        return 0;

    const int n = *(int*)(blk + g_vm.numParamOff);

    if (n < 0 || n > 256)
        return 0;

    return n;
}

bool ScrVm_SlotOk(int inst, const char* slot)
{
    if (!slot)
        return false;

    char* top = ScrVm_Top(inst);
    if (!top)
        return false;

    const int n = ScrVm_GetNumParam(inst);
    if (n <= 0)
        return false;

    if (slot > top || slot < top - 16 * n)
        return false;

    return SafePtr((uintptr_t)slot, 16);
}

float ScrVm_GetFloat(int inst, int index)
{
    if (ScrVm_GetNumParam(inst) <= index)
        return 0.0f;

    char* top = ScrVm_Top(inst);
    if (!top)
        return 0.0f;

    char* slot = top - 16 * index;
    if (!ScrVm_SlotOk(inst, slot))
        return 0.0f;

    int type = *(int*)(slot + 8);

    if (type == 5)
        return *(float*)slot;
    if (type == 6)
        return (float)*(int64_t*)slot;

    return 0.0f;
}

int ScrVm_ArgType(int inst, int index)
{
    if (ScrVm_GetNumParam(inst) <= index)
        return -1;

    char* top = ScrVm_Top(inst);
    if (!top)
        return -1;

    char* slot = top - 16 * index;
    if (!ScrVm_SlotOk(inst, slot))
        return -1;

    return *(int*)(slot + 8);
}

uint64_t ScrVm_ArgRaw(int inst, int index)
{
    if (ScrVm_GetNumParam(inst) <= index)
        return 0;

    char* top = ScrVm_Top(inst);
    if (!top)
        return 0;

    char* slot = top - 16 * index;
    if (!ScrVm_SlotOk(inst, slot))
        return 0;

    return *(uint64_t*)slot;
}

void StrCache_Add(uint32_t id, const char* text)
{
    if (!id || !text || !*text)
        return;

    for (int i = 0; i < g_strCacheCount; ++i)
        if (g_strCache[i].id == id)
            return;

    if (g_strCacheCount >= STR_CACHE_MAX)
        return;

    StrCacheEntry* e = &g_strCache[g_strCacheCount++];
    e->id = id;

    int i = 0;
    while (text[i] && i < STR_CACHE_LEN - 1) { e->text[i] = text[i]; ++i; }
    e->text[i] = 0;
}

void StrCache_Miss(const char* field, uint64_t raw)
{
    static uint32_t seen = 0;
    const uint32_t  bit  = (field[0] == 'a') ? 1u : (field[0] == 'r') ? 2u : 4u;

    if (seen & bit)
        return;

    seen |= bit;
    DbgPrint("[Hud] .%s string id 0x%llX not in cache - value left unchanged",
        field, (unsigned long long)(uint32_t)raw);
}

const char* ScrStr_Resolve(uint32_t id)
{
    if (!g_strTable || !id || (id - 1) > g_strMaxIndex)
        return nullptr;

    if (!SafePtr(g_strTable, sizeof(void*)))
        return nullptr;

    const uintptr_t table = *(const uintptr_t*)g_strTable;

    if (!table)
        return nullptr;

    const uintptr_t rec = table + (uintptr_t)g_strStride * id;

    if (!SafePtr(rec, g_strTextOff + sizeof(void*)))
        return nullptr;

    if (*(const uint8_t*)(rec + 3) != 7)
        return nullptr;

    const char* s = (const char*)(rec + g_strTextOff);

    if ((*(const uint8_t*)(rec + 2) & 0xC0) == 0x80)
    {
        if (!ScrStr_Follow_s)
            return nullptr;

        s = ScrStr_Follow_s((const char*)(rec + g_strTextOff));
    }

    if (!s || !SafePtr((uintptr_t)s, 1))
        return nullptr;

    return s;
}

#pragma endregion

#pragma region Script Arguments

const char* StrCache_Find(uint64_t raw)
{
    const uint32_t id = (uint32_t)raw;

    for (int i = 0; i < g_strCacheCount; ++i)
        if (g_strCache[i].id == id)
            return g_strCache[i].text;

    return ScrStr_Resolve(id);
}

int GetIntArg(int inst, int index, int def)
{
    if (!ScrVm_GetInt_s || ScrVm_GetNumParam(inst) <= index)
        return def;

    const int type = ScrVm_ArgType(inst, index);

    if (type != VAL_TYPE_INT && type != VAL_TYPE_FLOAT)
        return def;

    SCR_GETTER("GetInt", index);
    const int v = (int)ScrVm_GetInt_s(inst, index);
    SCR_GETTER_DONE();

    return v;
}

float GetFloatArg(int inst, int index, float def)
{
    const int type = ScrVm_ArgType(inst, index);

    if (type < 0)
        return def;

    if (type == VAL_TYPE_FLOAT)
        return ScrVm_GetFloat(inst, index);

    if (type == VAL_TYPE_INT && ScrVm_GetInt_s)
        return (float)ScrVm_GetInt_s(inst, index);

    return def;
}

const char* GetStringArg(int inst, int index, const char* def)
{
    if (!ScrVm_GetString_s || ScrVm_GetNumParam(inst) <= index)
        return def;

    const int type = ScrVm_ArgType(inst, index);

    if (type != VAL_TYPE_STRING)
    {

        static uint32_t seen = 0;

        if (type >= 0 && type < 32 && !(seen & (1u << type)))
        {
            seen |= (1u << type);
            DbgPrint("[Hud] %s arg %d type=%d, not a string (%s) - using default",
                g_scrWhere ? g_scrWhere : "?", index, type,
                type == VAL_TYPE_UNDEFINED ? "undefined" :
                type == VAL_TYPE_VECTOR    ? "vector"    :
                type == VAL_TYPE_FLOAT     ? "float"     :
                type == VAL_TYPE_INT       ? "int"       : "?");
        }

        return def;
    }

    SCR_GETTER("GetString", index);
    const char* s = ScrVm_GetString_s(inst, index);
    SCR_GETTER_DONE();

    const uint64_t raw = ScrVm_ArgRaw(inst, index);

    if (!s)
    {
        const char* alt = ScrStr_Resolve((uint32_t)raw);

        static int nullLogs = 0;

        if (nullLogs < 8)
        {
            ++nullLogs;
            DbgPrint("[Hud] %s arg %d: type=string but GetString returned null "
                "(raw=0x%llX idx=%u max=0x%X) table=0x%llX direct=%s",
                g_scrWhere ? g_scrWhere : "?", index,
                (unsigned long long)raw, (unsigned)(uint32_t)raw, g_strMaxIndex,
                (unsigned long long)(SafePtr(g_strTable, 8)
                    ? *(const uintptr_t*)g_strTable : 0),
                alt ? alt : "(null)");
        }

        if (alt)
        {
            StrCache_Add((uint32_t)raw, alt);
            return alt;
        }

        return def;
    }

    StrCache_Add((uint32_t)raw, s);

    return s;
}

const char* GetTextArg(int inst, int index, char* buf, int bufSize)
{
    const char* s = GetStringArg(inst, index, nullptr);

    if (s)
        return s;

    switch (ScrVm_ArgType(inst, index))
    {
    case VAL_TYPE_INT:
        snprintf(buf, bufSize, "%d", GetIntArg(inst, index, 0));
        return buf;

    case VAL_TYPE_FLOAT:
        snprintf(buf, bufSize, "%g", (double)GetFloatArg(inst, index, 0.0f));
        return buf;

    default:
        return "";
    }
}

#pragma endregion

#pragma region Layout

bool PointNameEq(const char* a, const char* b)
{
    while (*a || *b)
    {
        while (*a == '_') ++a;
        while (*b == '_') ++b;

        char ca = *a, cb = *b;

        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');

        if (ca != cb)
            return false;

        if (*a) ++a;
        if (*b) ++b;
    }

    return true;
}

static bool ScrPlace_FromEngine(ScrPlace* sp)
{
    if (!g_scrPlaceArray)
        return false;

    const float sw = T89_ScreenWidth();
    const float sh = T89_ScreenHeight();

    const float* best     = 0;
    float        bestArea = 0.0f;
    uint32_t     bestIdx  = 0;

    for (uint32_t i = 0; i < g_scrPlaceCount; ++i)
    {
        const float* f = (const float*)(g_scrPlaceArray + (uintptr_t)i * g_scrPlaceStride);

        const float vw = f[SPF_VIEWPORT_SIZE];
        const float vh = f[SPF_VIEWPORT_SIZE + 1];
        const float rx = f[SPF_VIRTUAL_TO_REAL];
        const float ry = f[SPF_VIRTUAL_TO_REAL + 1];
        const float ss = f[SPF_SPLIT_SCALE];

        if (!(vw > 16.0f && vw <= 16384.0f && vh > 16.0f && vh <= 16384.0f))
            continue;

        if (!(rx > 0.01f && rx < 64.0f && ry > 0.01f && ry < 64.0f))
            continue;

        if (!(ss > 0.0f && ss <= 4.0f))
            continue;

        const float area = vw * vh;

        if (area > bestArea)
        {
            bestArea = area;
            best     = f;
            bestIdx  = i;
        }
    }

    if (!best)
        return false;

    static bool logged = false;

    if (!logged)
    {
        logged = true;
        DbgPrint("[Hud] engine ScrPlace[%u]: viewport %.0f,%.0f %.0fx%.0f  videoOut %.0fx%.0f",
            bestIdx, best[SPF_VIEWPORT_BASE], best[SPF_VIEWPORT_BASE + 1],
            best[SPF_VIEWPORT_SIZE], best[SPF_VIEWPORT_SIZE + 1], sw, sh);
        DbgPrint("[Hud]   v2r %.4f,%.4f  v2f %.4f,%.4f  r2v %.4f,%.4f  split %.3f",
            best[SPF_VIRTUAL_TO_REAL], best[SPF_VIRTUAL_TO_REAL + 1],
            best[SPF_VIRTUAL_TO_FULL], best[SPF_VIRTUAL_TO_FULL + 1],
            best[SPF_REAL_TO_VIRTUAL], best[SPF_REAL_TO_VIRTUAL + 1],
            best[SPF_SPLIT_SCALE]);
        DbgPrint("[Hud]   viewable %.1f,%.1f .. %.1f,%.1f  sub %.1f,%.1f",
            best[SPF_VIEWABLE_MIN], best[SPF_VIEWABLE_MIN + 1],
            best[SPF_VIEWABLE_MAX], best[SPF_VIEWABLE_MAX + 1],
            best[SPF_SUBSCREEN], best[SPF_SUBSCREEN + 1]);
    }

    for (int i = 0; i < 2; ++i)
    {
        sp->viewportBase[i]  = best[SPF_VIEWPORT_BASE + i];
        sp->viewportSize[i]  = best[SPF_VIEWPORT_SIZE + i];
        sp->viewableMin[i]   = best[SPF_VIEWABLE_MIN + i];
        sp->viewableMax[i]   = best[SPF_VIEWABLE_MAX + i];
        sp->tweakableMin[i]  = best[SPF_TWEAKABLE_MIN + i];
        sp->tweakableMax[i]  = best[SPF_TWEAKABLE_MAX + i];
        sp->virtualToReal[i] = best[SPF_VIRTUAL_TO_REAL + i];
        sp->virtualToFull[i] = best[SPF_VIRTUAL_TO_FULL + i];
        sp->realToVirtual[i] = best[SPF_REAL_TO_VIRTUAL + i];
        sp->subScreen[i]     = best[SPF_SUBSCREEN + i];
    }

    sp->splitScale = best[SPF_SPLIT_SCALE];
    return true;
}

void ScrPlace_Update(ScrPlace* sp)
{
    const float sw = T89_ScreenWidth();
    const float sh = T89_ScreenHeight();

    float adjW = sh * (4.0f / 3.0f);
    float adjH = sh;

    if (adjW > sw)
    {
        adjW = sw;
        adjH = sw * (3.0f / 4.0f);
    }

    sp->viewportSize[0] = sw;
    sp->viewportSize[1] = sh;
    sp->viewportBase[0] = 0.0f;
    sp->viewportBase[1] = 0.0f;

    sp->virtualToReal[0] = adjW / VIRTUAL_W;
    sp->virtualToReal[1] = adjH / VIRTUAL_H;
    sp->virtualToFull[0] = sw   / VIRTUAL_W;
    sp->virtualToFull[1] = sh   / VIRTUAL_H;
    sp->realToVirtual[0] = VIRTUAL_W / adjW;
    sp->realToVirtual[1] = VIRTUAL_H / adjH;

    sp->viewableMin[0] = 0.0f;
    sp->viewableMin[1] = 0.0f;
    sp->viewableMax[0] = sw;
    sp->viewableMax[1] = sh;

    sp->tweakableMin[0] = sp->viewableMin[0];
    sp->tweakableMin[1] = sp->viewableMin[1];
    sp->tweakableMax[0] = sp->viewableMax[0];
    sp->tweakableMax[1] = sp->viewableMax[1];

    sp->subScreen[0] = (sw - adjW) * 0.5f;
    sp->subScreen[1] = (sh - adjH) * 0.5f;
    sp->splitScale   = 1.0f;
}

const ScrPlace* ScrPlace_Get()
{
    static ScrPlace sp;

    if (!ScrPlace_FromEngine(&sp))
        ScrPlace_Update(&sp);

    return &sp;
}

float ScrPlace_ApplyX(float x, int horzAlign)
{
    const ScrPlace* sp = ScrPlace_Get();

    x *= sp->splitScale;

    switch (horzAlign)
    {
    case HHA_LEFT:            return x * sp->virtualToReal[0] + sp->viewableMin[0];
    case HHA_CENTER:          return x * sp->virtualToReal[0] + sp->viewportSize[0] * 0.5f + sp->viewportBase[0];
    case HHA_RIGHT:           return x * sp->virtualToReal[0] + sp->viewableMax[0];
    case HHA_FULLSCREEN:      return x * sp->virtualToFull[0] + sp->viewportBase[0];
    case HHA_NOSCALE:         return x;
    case HHA_ALIGNTO640:      return x * sp->realToVirtual[0];
    case HHA_CENTER_SAFEAREA: return x * sp->virtualToReal[0] + (sp->viewableMax[0] + sp->viewableMin[0]) * 0.5f;
    case HHA_USER_CENTER:     return x * sp->virtualToReal[0] + (sp->tweakableMax[0] + sp->tweakableMin[0]) * 0.5f;
    case HHA_USER_LEFT:       return x * sp->virtualToReal[0] + sp->tweakableMin[0];
    case HHA_USER_RIGHT:      return x * sp->virtualToReal[0] + sp->tweakableMax[0];
    default:                  return x * sp->virtualToReal[0] + sp->subScreen[0];
    }
}

float ScrPlace_ApplyY(float y, int vertAlign)
{
    const ScrPlace* sp = ScrPlace_Get();

    y *= sp->splitScale;

    switch (vertAlign)
    {
    case HVA_TOP:             return y * sp->virtualToReal[1] + sp->viewableMin[1];
    case HVA_MIDDLE:          return y * sp->virtualToReal[1] + sp->viewportSize[1] * 0.5f + sp->viewportBase[1];
    case HVA_BOTTOM:          return y * sp->virtualToReal[1] + sp->viewableMax[1];
    case HVA_FULLSCREEN:      return y * sp->virtualToFull[1] + sp->viewportBase[1];
    case HVA_NOSCALE:         return y;
    case HVA_ALIGNTO480:      return y * sp->realToVirtual[1];
    case HVA_CENTER_SAFEAREA: return y * sp->virtualToReal[1] + (sp->viewableMax[1] + sp->viewableMin[1]) * 0.5f;
    case HVA_USER_CENTER:     return y * sp->virtualToReal[1] + (sp->tweakableMax[1] + sp->tweakableMin[1]) * 0.5f;
    case HVA_USER_TOP:        return y * sp->virtualToReal[1] + sp->tweakableMin[1];
    case HVA_USER_BOTTOM:     return y * sp->virtualToReal[1] + sp->tweakableMax[1];
    default:                  return y * sp->virtualToReal[1] + sp->subScreen[1];
    }
}

float ScrPlace_ApplyW(float w, int horzAlign)
{
    const ScrPlace* sp = ScrPlace_Get();

    w *= sp->splitScale;

    switch (horzAlign)
    {
    case HHA_FULLSCREEN: return w * sp->virtualToFull[0];
    case HHA_NOSCALE:    return w;
    case HHA_ALIGNTO640: return w * sp->realToVirtual[0];
    default:             return w * sp->virtualToReal[0];
    }
}

float ScrPlace_ApplyH(float h, int vertAlign)
{
    const ScrPlace* sp = ScrPlace_Get();

    h *= sp->splitScale;

    switch (vertAlign)
    {
    case HVA_FULLSCREEN: return h * sp->virtualToFull[1];
    case HVA_NOSCALE:    return h;
    case HVA_ALIGNTO480: return h * sp->realToVirtual[1];
    default:             return h * sp->virtualToReal[1];
    }
}

float VirtScale()
{
    return ScrPlace_Get()->virtualToReal[1];
}

int GetPointArg(int inst, int index, int def)
{
    const int type = ScrVm_ArgType(inst, index);

    if (type < 0)
        return def;

    if (type == VAL_TYPE_INT)
        return GetIntArg(inst, index, def);

    if (type == VAL_TYPE_FLOAT)
        return (int)GetFloatArg(inst, index, (float)def);

    return PointFromName(GetStringArg(inst, index, nullptr), def);
}

struct FontSlot
{
    uint32_t t8;
    uint32_t t9;
    int      px;
};

static const FontSlot kFontSlots[FONT_COUNT] =
{
    { 0x1CF0DC0, 0x2479518, 16 },
    { 0x1CF0E00, 0x2479558, 20 },
    { 0x1CF0E40, 0x2479598, 25 },
    { 0x1CF0E20, 0x2479578, 30 },
    { 0x1CF0DA0, 0x24794F8, 38 },
    { 0x1CF0D80, 0x24794D8, 48 },
    { 0x1CF0DE0, 0x2479538, 60 },
    { 0x1CF0D60, 0x24794B0, 22 },
    { 0x1CF0D40, 0x2479490, 22 },
    { 0x1CF0D20, 0x2479470, 24 },
};

struct FontAlias
{
    const char* name;
    int         slot;
};

static const FontAlias kFontAliases[] =
{
    { "default",    3 }, 
    { "normal",     3 },
    { "objective",  2 }, { "bold",       2 },
    { "big",        4 }, { "hudbig",     4 },
    { "bigfixed",   5 }, { "extrabig",   5 },
    { "title",      6 },
    { "small",      1 }, { "hudsmall",   1 },
    { "smallfixed", 0 }, { "extrasmall", 0 },
    { "console",    9 }, { "dev",        9 }, { "devsmall", 7 },
};

uintptr_t T89_FontRaw(int idx)
{
    if (idx < 0 || idx >= FONT_COUNT)
        idx = FONT_DEFAULT;

    const uint32_t off = (g_id == T8) ? kFontSlots[idx].t8
                       : (g_id == T9) ? kFontSlots[idx].t9
                       : 0u;

    return off ? REBASE_ADDRESS(off) : 0;
}

int T89_FontEffective(int idx)
{
    if (idx < 0 || idx >= FONT_COUNT)
        idx = FONT_DEFAULT;

    const uintptr_t d = T89_FontRaw(idx);

    if (!d)
        return FONT_DEFAULT;

    if (*(void* const*)(d + 0x08))
        return idx;

    const void* const name = *(const void* const*)(d + 0x00);

    for (int j = 0; j < FONT_COUNT; ++j)
    {
        const uintptr_t o = T89_FontRaw(j);

        if (!o || o == d)
            continue;

        if (*(const void* const*)(o + 0x00) != name)
            continue;

        void* const handle = *(void* const*)(o + 0x08);

        if (!handle)
            continue;

        *(void**)(d + 0x08) = handle;
        return idx;
    }

    return FONT_DEFAULT;
}

uintptr_t T89_FontAt(int idx)
{
    return T89_FontRaw(T89_FontEffective(idx));
}

int T89_FontPx(int idx)
{
    const int  eff = T89_FontEffective(idx);
    const uintptr_t d = T89_FontRaw(eff);

    if (d)
    {
        const int px = *(const int*)(d + 0x10);

        if (px > 4 && px < 256)
            return px;
    }

    return kFontSlots[eff].px;
}

const char* FontNameOf(int idx)
{
    for (unsigned i = 0; i < sizeof(kFontAliases) / sizeof(kFontAliases[0]); ++i)
        if (kFontAliases[i].slot == idx)
            return kFontAliases[i].name;

    return "?";
}

int FontIndexFromName(const char* name, int def)
{
    if (!name || !*name)
        return def;

    for (unsigned i = 0; i < sizeof(kFontAliases) / sizeof(kFontAliases[0]); ++i)
        if (PointNameEq(name, kFontAliases[i].name))
            return kFontAliases[i].slot;

    return def;
}

uintptr_t T89_Font()
{
    return T89_FontAt(FONT_DEFAULT);
}

float T89_FontPixelHeight()
{
    return (float)T89_FontPx(FONT_DEFAULT);
}

float FontScaleFactorFor(int idx)
{
    (void)idx;
    return FONT_VIRT_PER_POINT * VirtScale() / (float)T89_FontPx(FONT_DEFAULT);
}

uint64_t T89_AssetHash(const char* name)
{
    uint64_t h = 0xCBF29CE484222325ull;

    for (const char* p = name; p && *p; ++p)
    {
        char c = *p;

        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c == 0x5C)            c = '/';

        h = (h ^ (uint64_t)(unsigned char)c) * 0x100000001B3ull;
    }

    return h & 0x7FFFFFFFFFFFFFFFull;
}

void* T89_WhiteMaterial()
{
    const uintptr_t g = REBASE_ADDRESS(g_id == T8 ? 0x42DDBA8 : 0xB766128);
    return g ? *(void**)g : nullptr;
}

using DB_FindT8_t = void* (*)(int type, uint64_t hash, int a3, int a4, int a5);
using DB_FindT9_t = void* (*)(int type, uint64_t hash, int a3, int a4);

void* T89_FindMaterial(const char* name)
{
    if (!name || !*name || !DB_FindXAssetHeader_s)
        return nullptr;

    struct Entry { uint64_t hash; void* mat; };
    static Entry cache[24];
    static int   used = 0;

    const uint64_t hash = T89_AssetHash(name);

    for (int i = 0; i < used; ++i)
        if (cache[i].hash == hash)
            return cache[i].mat;

    void* mat = nullptr;

    if (g_id == T8)
        mat = ((DB_FindT8_t)DB_FindXAssetHeader_s)(6, hash, 0, 1, -1);
    else if (g_id == T9)
        mat = ((DB_FindT9_t)DB_FindXAssetHeader_s)(10, hash, 1, -1);

    if (used < (int)(sizeof(cache) / sizeof(cache[0])))
    {
        cache[used].hash = hash;
        cache[used].mat  = mat;
        ++used;
    }

    DbgPrint("[Hud] material \"%s\" hash=0x%llX -> 0x%p", name,
        (unsigned long long)hash, mat);

    return mat;
}

void T89_DrawRectangle(float x, float y, float w, float h, const float color[4], void* material)
{
    if (g_id == T8)
    {
        void* mat = material ? material : T89_WhiteMaterial();
        if (!mat || !R_AddCmdDrawQuadPicW_s)
            return;

        vec2_t q[4] = { { x,     y     },
                        { x + w, y     },
                        { x + w, y + h },
                        { x,     y + h } };

        R_AddCmdDrawQuadPicW_s(q, 1.0f, color, (uint64_t)mat, nullptr);
        return;
    }

    if (g_id != T9)
        return;

    void* mat = material ? material : T89_WhiteMaterial();
    if (!mat || !CmdAlloc_s || !BuildQuad_s)
        return;

    char q[96];
    BuildQuad_s(q, 0, x, y, x + w, y + h);

    char* cmd = (char*)CmdAlloc_s(12, 8, 64);
    if (!cmd)
        return;

    *(uint64_t*)(cmd + 0x08) = (uint64_t)mat;
    *(uint64_t*)(cmd + 0x10) = 0;
    *(uint64_t*)(cmd + 0x18) = *(uint64_t*)(q + 0);
    *(uint64_t*)(cmd + 0x20) = *(uint64_t*)(q + 12);
    *(uint64_t*)(cmd + 0x28) = *(uint64_t*)(q + 36);
    *(uint64_t*)(cmd + 0x30) = *(uint64_t*)(q + 24);
    *(uint32_t*)(cmd + 0x38) = 0x3F800000;

    for (int i = 0; i < 4; ++i)
    {
        int n = (int)lrintf(color[i] * 255.0f);
        if (n < 0) n = 0;
        if (n > 255) n = 255;
        cmd[0x3C + i] = (char)(unsigned char)n;
    }
}

static float CharAdvance(char c, int fontIdx, float scale)
{
    char one[2] = { c, 0 };

    const uintptr_t desc   = T89_FontAt(fontIdx);
    const uint64_t  cached = desc ? *(const uint64_t*)(desc + 0x08) : 0;

    if (R_TextWidthT9_s && desc && cached)
    {
        const float w = R_TextWidthT9_s(one, 0, desc, 1.0f);

        if (w > 0.0f && w < 4096.0f)
            return w * scale;
    }

    if (R_TextWidth_s && cached)
    {
        const int w = R_TextWidth_s(one, 0, cached, T89_FontPx(fontIdx));

        if (w > 0 && w < 4096)
            return (float)w * scale;
    }

    return GlyphWidth(c) * scale;
}

#define RAINBOW_FRAMES 4

static const float kRainbow[][3] =
{
    { 1.00f, 0.36f, 0.36f },   
    { 0.00f, 1.00f, 0.00f },   
    { 1.00f, 1.00f, 0.00f },   
    { 0.00f, 0.00f, 1.00f },   
    { 0.00f, 1.00f, 1.00f },   
    { 1.00f, 0.36f, 1.00f },   
};

#define RAINBOW_STEPS ((int)(sizeof(kRainbow) / sizeof(kRainbow[0])))

static void DrawTextLine(const char* line, float x, float y, float scale,
    const float color[4], int fontIdx, uintptr_t font)
{
    int r = 0;
    bool hasEffect = false;

    while (line[r])
    {
        if (line[r] == '^' && (line[r + 1] == 'R' || line[r + 1] == 'C'))
        {
            hasEffect = true;
            break;
        }
        ++r;
    }

    if (!hasEffect)
    {
        R_AddCmdDrawText_s(line, 0x7FFFFFFF, font, x, y,
            scale, scale, 0.0f, color, 0);
        return;
    }

    char seg[MAX_TEXT];
    int  n       = 0;
    int  i       = 0;
    int  hue     = 0;
    bool rainbow = false;
    float pen    = x;

    bool  cycling    = false;
    int   cyclePeriod = 6;
    float cycleCol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    const int tick = (int)(g_frameTick / RAINBOW_FRAMES);

    for (;;)
    {
        const bool endOfRun = (line[i] == 0)
                           || (line[i] == '^' && line[i + 1] == 'R')
                           || (line[i] == '^' && line[i + 1] == 'C');

        if (endOfRun && n)
        {
            const float* useCol = color;

            if (cycling)
            {
                const int step =
                    (int)(g_frameTick / (uint32_t)cyclePeriod) % RAINBOW_STEPS;
                const float* h = kRainbow[step];

                cycleCol[0] = h[0];
                cycleCol[1] = h[1];
                cycleCol[2] = h[2];
                cycleCol[3] = color[3];
                useCol      = cycleCol;
            }

            seg[n] = 0;
            R_AddCmdDrawText_s(seg, 0x7FFFFFFF, font, pen, y,
                scale, scale, 0.0f, useCol, 0);

            for (int k = 0; k < n; ++k)
                pen += CharAdvance(seg[k], fontIdx, scale);

            n = 0;
        }

        if (!line[i])
            break;

        if (line[i] == '^' && line[i + 1] == 'R')
        {
            rainbow = !rainbow;
            i += 2;
            continue;
        }

        if (line[i] == '^' && line[i + 1] == 'C' &&
            line[i + 2] >= '0' && line[i + 2] <= '9')
        {
            cycling     = true;
            cyclePeriod = 3 + (line[i + 2] - '0') * 3;
            i += 3;
            continue;
        }

        if (!rainbow)
        {
            if (n < MAX_TEXT - 1)
                seg[n++] = line[i];

            ++i;
            continue;
        }

        const float* hueRgb = kRainbow[((hue + tick) % RAINBOW_STEPS + RAINBOW_STEPS) % RAINBOW_STEPS];
        const float  col[4] = { hueRgb[0], hueRgb[1], hueRgb[2], color[3] };

        char one[2] = { line[i], 0 };

        R_AddCmdDrawText_s(one, 0x7FFFFFFF, font, pen, y,
            scale, scale, 0.0f, col, 0);

        pen += CharAdvance(line[i], fontIdx, scale);
        ++hue;
        ++i;
    }
}

void T89_DrawText(const char* text, float x, float y, float scale, const float color[4], int fontIdx)
{
    if (!text || !R_AddCmdDrawText_s)
        return;

    uintptr_t font = T89_FontAt(fontIdx);
    if (!font)
        return;

    const float lineH = (float)T89_FontPx(fontIdx) * scale;

    char line[MAX_TEXT];
    int  i  = 0;
    int  ln = 0;

    while (text[i])
    {
        int n = 0;

        while (text[i] && text[i] != '\n' && n < MAX_TEXT - 1)
            line[n++] = text[i++];

        line[n] = 0;

        if (n)
        {
            DrawTextLine(line, x, y + (lineH * (float)ln),
                scale, color, fontIdx, font);
        }

        if (text[i] == '\n')
        {
            ++i;
            ++ln;
        }
    }
}

void DrawLock()
{
    while (__sync_lock_test_and_set(&g_drawLock, 1))
        scePthreadYield();
}

void DrawUnlock()
{
    __sync_lock_release(&g_drawLock);
}

DrawItem* Draw_Find(int id)
{
    for (int i = 0; i < g_drawCount; ++i)
        if (g_draws[i].id == id)
            return &g_draws[i];

    return nullptr;
}

#pragma endregion

#pragma region Public API

int T89_Create()
{
    DrawLock();

    if (g_drawCount >= MAX_DRAWS)
    {
        DrawUnlock();
        return 0;
    }

    DrawItem* d = &g_draws[g_drawCount];
    d->type = DRAW_RECT;
    d->x = d->y = d->w = d->h = 0.0f;
    d->scale = 1.0f;
    d->color[0] = d->color[1] = d->color[2] = 1.0f;
    d->color[3] = 0.0f;
    d->text[0] = 0;
    d->layer = 0;
    d->seq = g_drawCount;

    int id = 0;

    for (int cand = 1; cand <= MAX_DRAWS; ++cand)
    {
        bool used = false;

        for (int i = 0; i < g_drawCount; ++i)
        {
            if (g_draws[i].id == cand)
            {
                used = true;
                break;
            }
        }

        if (!used)
        {
            id = cand;
            break;
        }
    }

    if (!id)
    {
        DrawUnlock();
        return 0;
    }

    d->id = id;
    ++g_drawCount;

    DrawUnlock();
    return id;
}

bool T89_Apply(int id, int type, float x, float y, float w, float h,
    float scale, int layer, const float color[4], const char* text, int font, void* material)
{
    DrawLock();

    DrawItem* d = Draw_Find(id);
    if (d)
    {
        d->type = type;
        d->x = x;
        d->y = y;
        d->w = w;
        d->h = h;
        d->scale = scale;
        d->font  = font;
        d->material = material;
        d->layer = layer;

        d->color[0] = color[0];
        d->color[1] = color[1];
        d->color[2] = color[2];
        d->color[3] = color[3];

        int i = 0;
        if (text)
        {
            while (text[i] && i < MAX_TEXT - 1)
            {
                d->text[i] = text[i];
                ++i;
            }
        }
        d->text[i] = 0;
    }

    DrawUnlock();
    return d != nullptr;
}

void T89_Remove(int id)
{
    DrawLock();

    for (int i = 0; i < g_drawCount; ++i)
    {
        if (g_draws[i].id != id)
            continue;

        for (int j = i; j < g_drawCount - 1; ++j)
            g_draws[j] = g_draws[j + 1];

        --g_drawCount;

        for (int j = 0; j < g_drawCount; ++j)
            g_draws[j].seq = j;

        break;
    }

    DrawUnlock();
}

void T89_Clear()
{
    DrawLock();
    g_drawCount = 0;
    DrawUnlock();
}

void T89_FlushDraws()
{
    static DrawItem local[MAX_DRAWS];
    int count;

    DrawLock();

    for (int i = 1; i < g_drawCount; ++i)
    {
        DrawItem key = g_draws[i];
        int j = i - 1;
        while (j >= 0 &&
            (g_draws[j].layer > key.layer ||
                (g_draws[j].layer == key.layer && g_draws[j].seq > key.seq)))
        {
            g_draws[j + 1] = g_draws[j];
            --j;
        }
        g_draws[j + 1] = key;
    }

    count = g_drawCount;
    for (int i = 0; i < count; ++i)
        local[i] = g_draws[i];

    DrawUnlock();

    for (int i = 0; i < count; ++i)
    {
        if (local[i].color[3] <= 0.0f)
            continue;

        if (local[i].type == DRAW_RECT)
            T89_DrawRectangle(local[i].x, local[i].y, local[i].w, local[i].h, local[i].color, local[i].material);
        else
            T89_DrawText(local[i].text, local[i].x, local[i].y, local[i].scale, local[i].color, local[i].font);
    }
}

#pragma endregion

#pragma region Layout

int PointClamp(int p)
{
    return (p < 0 || p > P_BOTTOMRIGHT) ? P_TOPLEFT : p;
}

float HorzAlignFrac(int ha)
{
    switch (ha)
    {
    case HHA_CENTER: case HHA_CENTER_SAFEAREA: case HHA_USER_CENTER: return 0.5f;
    case HHA_RIGHT:  case HHA_USER_RIGHT:                            return 1.0f;
    default:                                                         return 0.0f;
    }
}

float VertAlignFrac(int va)
{
    switch (va)
    {
    case HVA_MIDDLE: case HVA_CENTER_SAFEAREA: case HVA_USER_CENTER: return 0.5f;
    case HVA_BOTTOM: case HVA_USER_BOTTOM:                           return 1.0f;
    default:                                                         return 0.0f;
    }
}

float SelfAlignFrac(int a)
{
    return (a < 0 || a > 2) ? 0.0f : (float)a * 0.5f;
}

int PointFromName(const char* s, int def)
{
    if (!s || !*s)
        return def;

    static const char* const kNames[9] =
    {
        "topleft", "top", "topright",
        "left",    "center", "right",
        "bottomleft", "bottom", "bottomright"
    };

    for (int i = 0; i < 9; ++i)
        if (PointNameEq(s, kNames[i]))
            return i;

    if (PointNameEq(s, "centre"))
        return P_CENTER;

    static const struct { const char* name; int point; } kAliases[] =
    {
        { "topcenter",    P_TOP          },
        { "topcentre",    P_TOP          },
        { "bottomcenter", P_BOTTOM       },
        { "bottomcentre", P_BOTTOM       },
        { "centertop",    P_TOP          },
        { "centerbottom", P_BOTTOM       },
        { "centerleft",   P_LEFT         },
        { "centerright",  P_RIGHT        },
        { "lefttop",      P_TOPLEFT      },
        { "righttop",     P_TOPRIGHT     },
        { "leftbottom",   P_BOTTOMLEFT   },
        { "rightbottom",  P_BOTTOMRIGHT  },
        { "middle",       P_CENTER       },
    };

    for (unsigned i = 0; i < sizeof(kAliases) / sizeof(kAliases[0]); ++i)
        if (PointNameEq(s, kAliases[i].name))
            return kAliases[i].point;

    DbgPrint("[Hud] unknown alignment \"%s\", using %d", s, def);
    return def;
}

int HorzAlignClamp(int v) { return (v < 0 || v >= HHA_COUNT) ? HHA_LEFT : v; }

int VertAlignClamp(int v) { return (v < 0 || v >= HVA_COUNT) ? HVA_TOP  : v; }

int SelfAlignClamp(int v) { return (v < 0 || v > 2) ? 0 : v; }

#pragma endregion

#pragma region Hud Elements

void ElemLock()
{
    while (__sync_lock_test_and_set(&g_elemLock, 1))
        scePthreadYield();
}

void ElemUnlock()
{
    __sync_lock_release(&g_elemLock);
}

HudElem* Elem_Find(int handle)
{
    if (!handle)
        return nullptr;

    for (int i = 0; i < g_elemCount; ++i)
        if (g_elems[i].draw == handle)
            return &g_elems[i];

    return nullptr;
}

void Elem_ApplyPointPair(HudElem* e, int align, int relative)
{
    e->alignX = SelfAlignClamp(align % 3);
    e->alignY = SelfAlignClamp(align / 3);
    e->horzAlign = HorzAlignClamp((relative % 3) + HHA_LEFT);
    e->vertAlign = VertAlignClamp((relative / 3) + HVA_TOP);
}

HudElem* Elem_New()
{
    if (g_elemCount >= MAX_ELEMS)
        return nullptr;

    HudElem* e = &g_elems[g_elemCount++];

    for (unsigned b = 0; b < sizeof(HudElem); ++b)
        ((volatile char*)e)[b] = 0;

    e->draw = T89_Create();
    e->isText = 0;
    e->x = e->y = 0.0f;

    e->alignX = HXA_LEFT;
    e->alignY = HYA_TOP;
    e->horzAlign = HHA_SUBLEFT;
    e->vertAlign = HVA_SUBTOP;
    e->xOffset = e->yOffset = 0.0f;
    e->foreground = 0;
    e->archived = 0;
    e->hideWhenInMenu = 0;
    e->w = e->h = 0.0f;
    e->scale = 1.0f;
    e->font = FONT_DEFAULT;
    e->sort = 0;
    e->color[0] = e->color[1] = e->color[2] = 1.0f;
    e->color[3] = 1.0f;
    e->text[0] = 0;
    e->parent = 0;

    return e;
}

float GlyphWidth(char c)
{
    switch (c)
    {
    case 'i': case 'j': case 'l': case 'I':
    case '.': case ',': case '\'': case '!':
    case '|': case ':': case ';': case '`':
        return 6.0f;

    case 'f': case 't': case 'r': case '(': case ')':
    case '[': case ']': case '{': case '}':
        return 8.0f;

    case ' ':
        return 8.0f;

    case 'm': case 'w': case 'M': case 'W':
        return 23.0f;

    default:
        break;
    }

    if (c >= 'A' && c <= 'Z')
        return 17.0f;

    return 14.0f;
}

float Elem_FontHeight(HudElem* e)
{
    return (float)T89_FontPx(e->font) * e->scale;
}

void Elem_Extent(HudElem* e, float* w, float* h)
{
    if (!e->isText)
    {
        *w = ScrPlace_ApplyW(e->w, e->horzAlign);
        *h = ScrPlace_ApplyH(e->h, e->vertAlign);
        return;
    }


    const uintptr_t desc = T89_FontAt(e->font);
    const uint64_t cached = desc ? *(const uint64_t*)(desc + 0x08) : 0;
    const uint64_t font   = (R_TextWidth_s && cached) ? cached : 0;
    const int      fontH  = desc ? *(const int*)(desc + 0x10) : 0;

    float px = 0.0f;

    char  line[MAX_TEXT];
    int   i = 0;

    for (;;)
    {
        int   n     = 0;
        float lpx   = 0.0f;

        while (i < MAX_TEXT && e->text[i] && e->text[i] != '\n')
        {
            if (e->text[i] == '^' && e->text[i + 1] == 'R')
            {
                i += 2;
                continue;
            }

            if (e->text[i] == '^' && e->text[i + 1] == 'C' &&
                e->text[i + 2] >= '0' && e->text[i + 2] <= '9')
            {
                i += 3;
                continue;
            }

            if (e->text[i] == '^' && e->text[i + 1] == 'F')
            {
                if (n < MAX_TEXT - 2)
                {
                    line[n++] = e->text[i];
                    line[n++] = e->text[i + 1];
                }
                i += 2;
                continue;
            }
            if (e->text[i] == '^' && e->text[i + 1] == 'A' && e->text[i + 2])
            {
                for (int c = i; c < i + 3; ++c)
                    if (n < MAX_TEXT - 1)
                        line[n++] = e->text[c];

                i += 3;
                continue;
            }

            if (e->text[i] == '^' &&
                (e->text[i + 1] == 'H' || e->text[i + 1] == 'I'))
            {
                const unsigned char wb  = (unsigned char)e->text[i + 2];
                const unsigned char hb  = (unsigned char)e->text[i + 3];
                const unsigned char len = (unsigned char)e->text[i + 4];

                if (!wb || !hb || !len || !e->text[i + 5])
                {
                    lpx += 32.0f;
                    break;
                }

                const int total = 5 + (int)len;

                for (int c = i; c < i + total && e->text[c]; ++c)
                    if (n < MAX_TEXT - 1)
                        line[n++] = e->text[c];

                lpx += (float)((T89_FontPx(e->font) * ((int)wb - 0x10) + 0x10) / 32);
                i += total;
                continue;
            }

            if (e->text[i] == '^' && e->text[i + 1] == 'B')
            {
                int k = i + 2;

                while (e->text[k] && e->text[k] != '^' && (k - i) < 0x3E)
                    ++k;

                if (e->text[k] == '^')
                {
                    for (int c = i; c <= k; ++c)
                        if (n < MAX_TEXT - 1)
                            line[n++] = e->text[c];

                    lpx += (float)T89_FontPx(e->font);
                    i = k + 1;
                    continue;
                }
            }

            if (e->text[i] == '^' && e->text[i + 1] &&
                ((g_id == T9 && (unsigned char)e->text[i + 1] >= 0x2D
                              && (unsigned char)e->text[i + 1] <= 0x40)
                 || (e->text[i + 1] >= '0' && e->text[i + 1] <= '9')))
            {
                if (n < MAX_TEXT - 2)
                {
                    line[n++] = e->text[i];
                    line[n++] = e->text[i + 1];
                }
                i += 2;
                continue;
            }

            lpx += GlyphWidth(e->text[i]);

            if (n < MAX_TEXT - 1)
                line[n++] = e->text[i];

            ++i;
        }

        line[n] = 0;

        if (R_TextWidthT9_s && desc && cached && n)
        {
            const float measured = R_TextWidthT9_s(line, 0, desc, 1.0f);

            if (measured > 0.0f && measured < T89_ScreenWidth() * 4.0f)
                lpx = measured;
        }
        else if (font && fontH > 0 && n)
        {
            const int measured = R_TextWidth_s(line, 0, font, fontH);

            if (measured > 0 && (float)measured < T89_ScreenWidth() * 4.0f)
                lpx = (float)measured;
        }

        if (lpx > px)
            px = lpx;

        if (i < MAX_TEXT && e->text[i] == '\n')
        {
            ++i;
            continue;
        }

        break;
    }

    *w = px * e->scale;
    *h = Elem_FontHeight(e);
}

void Elem_Resolve(HudElem* e, int depth, float* outX, float* outY, float* outW, float* outH)
{
    float ew, eh;
    Elem_Extent(e, &ew, &eh);

    float ax = ScrPlace_ApplyX(e->x + e->xOffset, e->horzAlign);
    float ay = ScrPlace_ApplyY(e->y + e->yOffset, e->vertAlign);

    if (e->parent && depth < 8)
    {
        HudElem* p = Elem_Find(e->parent);

        if (p && p != e)
        {
            float px, py, pw, ph;
            Elem_Resolve(p, depth + 1, &px, &py, &pw, &ph);

            ax = px + (pw * HorzAlignFrac(e->horzAlign))
               + ScrPlace_ApplyW(e->x + e->xOffset, e->horzAlign);
            ay = py + (ph * VertAlignFrac(e->vertAlign))
               + ScrPlace_ApplyH(e->y + e->yOffset, e->vertAlign);
        }
    }

    *outX = ax - (ew * SelfAlignFrac(e->alignX));
    *outY = ay - (eh * SelfAlignFrac(e->alignY));
    *outW = ew;
    *outH = eh;
}

static float HudQuantise(float v)
{
    return (float)(int)(v + 9.3132257461547852e-10f);
}

void Elem_Push(HudElem* e)
{
    if (e->isText && e->fontScaleRaw > 0.0f && e->animFontLeft <= 0)
        e->scale = e->fontScaleRaw * FontScaleFactorFor(e->font);

    float x, y, w, h;
    Elem_Resolve(e, 0, &x, &y, &w, &h);

    x = HudQuantise(x);
    y = HudQuantise(y);

    float drawY = e->isText ? (y + __builtin_floorf(h + 0.5f) + g_textBaselineNudge * VirtScale()) : y;
    {
        static int   traced = 0;
        static float seenY[MAX_ELEMS] = {};

        const int slot = (e->draw > 0 && e->draw < MAX_ELEMS) ? e->draw : 0;

        if (e->isText && seenY[slot] != drawY && traced < 40)
        {
            ++traced;
            seenY[slot] = drawY;
            DbgPrint("[Text] h=%d virt(%.1f,%.1f) off(%.1f,%.1f) va=%d ay=%d "
                "font=%d px=%d scale=%.4f fh=%.2f -> y=%.1f drawY=%.1f \"%.16s\"",
                e->draw, e->x, e->y, e->xOffset, e->yOffset,
                e->vertAlign, e->alignY, e->font, T89_FontPx(e->font),
                e->scale, h, y, drawY, e->text);
        }
    }

    float col[4] =
    {
        e->color[0],
        e->color[1],
        e->color[2],
        e->hidden ? 0.0f : e->color[3],
    };

    T89_Apply(e->draw, e->isText ? DRAW_TEXT : DRAW_RECT,
        x, drawY, w, h, e->scale, e->sort, col, e->text, e->font, e->material);
}

void Elem_Free(int handle)
{
    if (!handle)
        return;

    ElemLock();

    for (int i = 0; i < g_elemCount; ++i)
    {
        if (g_elems[i].draw != handle)
            continue;

        T89_Remove(g_elems[i].draw);

        for (int j = i; j < g_elemCount - 1; ++j)
            g_elems[j] = g_elems[j + 1];

        --g_elemCount;
        break;
    }

    ElemUnlock();
}

void T89_ClearAll()
{
    ElemLock();
    g_elemCount = 0;
    ElemUnlock();

    T89_Clear();
}

struct ButtonGlyph
{
    const char* token;
    const char* icon;
};

static const ButtonGlyph kButtonGlyphs[] =
{
    { "cross",     "^BPS3ButtonXBinding^"         },
    { "x",         "^BPS3ButtonXBinding^"         },
    { "circle",    "^BPS3ButtonCircleBinding^"    },
    { "square",    "^BPS3ButtonSquareBinding^"    },
    { "triangle",  "^BPS3ButtonTriangleBinding^"  },
    { "l1",        "^BPS3ButtonShoulderLBinding^" },
    { "r1",        "^BPS3ButtonShoulderRBinding^" },
    { "l2",        "^BPS3ButtonTrigLBinding^"     },
    { "r2",        "^BPS3ButtonTrigRBinding^"     },
    { "l3",        "^BPS3ButtonStickAnimatedL^"   },
    { "r3",        "^BPS3ButtonStickAnimatedR^"   },
    { "options",   "^BPS3ButtonStart^"            },
    { "start",     "^BPS3ButtonStart^"            },
    { "share",     "^BPS3ButtonSelect^"           },
    { "touchpad",  "^BPS3ButtonSelect^"           },
    { "dpadup",    "^BPS3ButtonDpadU^"            },
    { "dpaddown",  "^BPS3ButtonDpadD^"            },
    { "dpadleft",  "^BPS3ButtonDpadL^"            },
    { "dpadright", "^BPS3ButtonDpadR^"            },

    { "+attack",       "^BPS3ButtonTrigRBinding^"     },
    { "+speedthrow",   "^BPS3ButtonTrigLBinding^"     },
    { "+usereload",    "^BPS3ButtonSquareBinding^"    },
    { "+activate",     "^BPS3ButtonSquareBinding^"    },
    { "+reload",       "^BPS3ButtonSquareBinding^"    },
    { "+use",          "^BPS3ButtonSquareBinding^"    },
    { "+gostand",      "^BPS3ButtonXBinding^"         },
    { "+jump",         "^BPS3ButtonXBinding^"         },
    { "+melee",        "^BPS3ButtonStickAnimatedR^"   },
    { "+stance",       "^BPS3ButtonCircleBinding^"    },
    { "+gocrouch",     "^BPS3ButtonCircleBinding^"    },
    { "+goprone",      "^BPS3ButtonCircleBinding^"    },
    { "+frag",         "^BPS3ButtonShoulderRBinding^" },
    { "+smoke",        "^BPS3ButtonShoulderLBinding^" },
    { "+breathsprint", "^BPS3ButtonStickAnimatedL^"   },
    { "+sprint",       "^BPS3ButtonStickAnimatedL^"   },
    { "weapnext",      "^BPS3ButtonTriangleBinding^"  },
    { "+actionslot1",  "^BPS3ButtonDpadU^"            },
    { "+actionslot2",  "^BPS3ButtonDpadD^"            },
    { "+actionslot3",  "^BPS3ButtonDpadL^"            },
    { "+actionslot4",  "^BPS3ButtonDpadR^"            },
};

static bool BindNameEq(const char* a, int alen, const char* b)
{
    int i = 0;

    for (;;)
    {
        while (i < alen && (a[i] == ' ' || a[i] == '_' || a[i] == '-')) ++i;
        while (*b == ' ' || *b == '_' || *b == '-') ++b;

        if (i >= alen || !a[i])
            return *b == 0;

        if (!*b)
            return false;

        char ca = a[i];
        char cb = *b;

        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');

        if (ca != cb)
            return false;

        ++i;
        ++b;
    }
}

static int TokenPrefix(const char* name, int len, const char* pfx)
{
    int k = 0;

    while (pfx[k])
    {
        if (k >= len)
            return 0;

        char a = name[k];
        char b = pfx[k];

        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');

        if (a != b)
            return 0;

        ++k;
    }

    return k;
}

static const char* ButtonGlyphFor(const char* name, int len)
{
    for (unsigned i = 0; i < sizeof(kButtonGlyphs) / sizeof(kButtonGlyphs[0]); ++i)
        if (BindNameEq(name, len, kButtonGlyphs[i].token))
            return kButtonGlyphs[i].icon;

    return nullptr;
}

void CopyText(char* dst, const char* src)
{
    int i = 0;

    if (src)
    {
        int r = 0;

        while (src[r] && i < MAX_TEXT - 1)
        {
            const bool bracketed = src[r] == '[' && src[r + 1] == '{';

            if (bracketed || src[r] == '{')
            {
                const int  open  = bracketed ? 2 : 1;
                const char* name = src + r + open;

                int n = 0;
                while (name[n] && name[n] != '}' && n < 64)
                    ++n;

                const bool closed = name[n] == '}'
                                 && (!bracketed || name[n + 1] == ']');

                if (closed)
                {
                    int pfx = TokenPrefix(name, n, "shader:");

                    if (!pfx)
                        pfx = TokenPrefix(name, n, "img:");

                    if (pfx && n > pfx)
                    {
                        const int mlen  = n - pfx;
                        const int need  = 5 + mlen;

                        if (mlen <= 0x3F && i + need < MAX_TEXT - 1)
                        {
                            dst[i++] = '^';
                            dst[i++] = 'H';
                            dst[i++] = (char)0x30;
                            dst[i++] = (char)0x30;
                            dst[i++] = (char)mlen;

                            for (int c = 0; c < mlen; ++c)
                                dst[i++] = name[pfx + c];

                            r += open + n + (bracketed ? 2 : 1);
                            continue;
                        }
                    }

                    const char* g = ButtonGlyphFor(name, n);

                    if (g)
                    {
                        int k = 0;

                        while (g[k] && i < MAX_TEXT - 1)
                            dst[i++] = g[k++];

                        if (g[k])
                        {
                            i -= k;
                            break;
                        }

                        r += open + n + (bracketed ? 2 : 1);
                        continue;
                    }
                }
            }

            dst[i++] = src[r++];
        }
    }

    dst[i] = 0;
}

void ResolveScreen()
{
    if (s_resCached)
        return;

    for (int handle = 0; handle < 16; ++handle)
    {
        SceVideoOutResolutionStatus status{};

        if (sceVideoOutGetResolutionStatus(handle, &status) != SCE_VIDEO_OUT_OK)
            continue;

        uint32_t w = status.paneWidth  ? status.paneWidth  : status.fullWidth;
        uint32_t h = status.paneHeight ? status.paneHeight : status.fullHeight;

        if (!w || !h)
            continue;

        s_screenWidth = (float)w;
        s_screenHeight = (float)h;
        s_resCached = true;

        DbgPrint("[Hud] resolution handle=%d pane %ux%u full %ux%u -> using %ux%u",
            handle, status.paneWidth, status.paneHeight,
            status.fullWidth, status.fullHeight, w, h);
        return;
    }

    static bool warned = false;
    if (!warned)
    {
        warned = true;
        DbgPrint("[Hud] no video out handle answered, using %.0fx%.0f",
            s_screenWidth, s_screenHeight);
    }
}

float T89_ScreenWidth()  { ResolveScreen(); return s_screenWidth; }

float T89_ScreenHeight() { ResolveScreen(); return s_screenHeight; }

#pragma endregion

#pragma region VM Access

void Scr_NewClientHudElem(int inst)
{
    int handle = 0;

    ElemLock();
    HudElem* e = Elem_New();
    if (e)
        handle = e->draw;
    ElemUnlock();

    if (!e)
        DbgPrint("[Hud] OUT OF ELEMENTS (max %d) - newClientHudElem returned 0; "
            "something is creating without destroying", MAX_ELEMS);

    if (ScrVm_AddInt_s)
        ScrVm_AddInt_s(inst, handle);
}

void Scr_CreateFontString(int inst)
{
    const char* fname   = GetStringArg(inst, 0, nullptr);
    const int   fontIdx = FontIndexFromName(fname, FONT_DEFAULT);

    float scale = (fname ? GetFloatArg(inst, 1, 1.0f) : GetFloatArg(inst, 0, 1.0f))
        * FontScaleFactorFor(fontIdx);
    int   handle = 0;

    ElemLock();
    HudElem* e = Elem_New();
    if (e)
    {
        e->isText = 1;
        e->font = fontIdx;
        e->fontScaleRaw = (fname ? GetFloatArg(inst, 1, 1.0f) : GetFloatArg(inst, 0, 1.0f));
        e->scale = scale;
        handle = e->draw;
        Elem_Push(e);
    }
    ElemUnlock();

    if (ScrVm_AddInt_s)
        ScrVm_AddInt_s(inst, handle);
}

void Scr_SetShader(int inst)
{
    int handle = GetIntArg(inst, 0, 0);

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e)
    {
        e->w = GetFloatArg(inst, 1, 0.0f);
        e->h = GetFloatArg(inst, 2, 0.0f);
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetText(int inst)
{
    int         handle = GetIntArg(inst, 0, 0);
    const char* text = GetStringArg(inst, 1, "");

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e)
    {
        e->isText = 1;
        CopyText(e->text, text);
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetPoint(int inst)
{
    int   handle = GetIntArg(inst, 0, 0);
    int   align = GetPointArg(inst, 1, P_TOPLEFT);
    int   relative = GetPointArg(inst, 2, P_TOPLEFT);
    float x = GetFloatArg(inst, 3, 0.0f);
    float y = GetFloatArg(inst, 4, 0.0f);

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e)
    {
        Elem_ApplyPointPair(e, PointClamp(align), PointClamp(relative));
        e->x = x;
        e->y = y;
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetParent(int inst)
{
    int handle = GetIntArg(inst, 0, 0);
    int parent = GetIntArg(inst, 1, 0);

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e && parent != handle)
    {
        e->parent = parent;
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetColor(int inst)
{
    int handle = GetIntArg(inst, 0, 0);

    float rgb[3] = { 1.0f, 1.0f, 1.0f };

    if (ScrVm_GetVector_s)
    {
        static const float kWhite[3] = { 1.0f, 1.0f, 1.0f };
        SCR_GETTER("GetVector", 1);
        ScrVm_GetVector_s(inst, 1, rgb, kWhite);
        SCR_GETTER_DONE();
    }

    float r = rgb[0];
    float g = rgb[1];
    float b = rgb[2];

    float a = GetFloatArg(inst, 2, 1.0f);

    if (a > 1.0f)
        a = a / 255.0f;

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e)
    {
        e->color[0] = r;
        e->color[1] = g;
        e->color[2] = b;
        e->color[3] = a;
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetSort(int inst)
{
    int handle = GetIntArg(inst, 0, 0);
    int sort = GetIntArg(inst, 1, 0);

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e)
    {
        e->sort = sort;
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetScale(int inst)
{
    int   handle = GetIntArg(inst, 0, 0);
    float raw    = GetFloatArg(inst, 1, 1.0f);

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e)
    {
        e->fontScaleRaw = raw;
        e->scale = raw * FontScaleFactorFor(e->font);
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetHidden(int inst)
{
    int handle = GetIntArg(inst, 0, 0);
    int hidden = GetIntArg(inst, 1, 0);

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e)
    {
        e->hidden = hidden ? 1 : 0;
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_Destroy(int inst)
{
    Elem_Free(GetIntArg(inst, 0, 0));
}

void Scr_ClearAllHuds(int)
{
    T89_ClearAll();
    DbgPrint("[Hud] clearAllHuds from script");
}

static bool VarSlot(int inst, uint32_t id, const char** outNode,
    const char** outValue)
{
    const uintptr_t base = g_varGlobals
        + (uintptr_t)(unsigned)inst * g_var.instStride;

    if (!g_var.sharded)
    {
        const char* nodes  = *(const char**)(base + 0x08);
        const char* values = *(const char**)(base + 0x20);

        if (!nodes || !values)
            return false;

        *outNode  = nodes + (uintptr_t)id * 32;
        *outValue = values + (uintptr_t)id * 16;
        return true;
    }

    const uintptr_t slot = base + g_var.shardArrayOff + 16u * (id & 0x1Fu);

    if (!SafePtr(slot, sizeof(void*)))
        return false;

    const char* shard = *(const char* const*)slot;

    if (!shard)
        return false;

    const uintptr_t idx = id >> 5;

    *outNode  = shard + g_var.shardNodeOff + idx * 32;
    *outValue = shard + idx * 16;
    return true;
}


bool VarField(int inst, int parentVarId, uint64_t canonId,
    uint64_t* outVal, int* outType)
{
    if (!g_varGlobals || !parentVarId)
        return false;

    const uintptr_t base = g_varGlobals
        + (uintptr_t)(unsigned)inst * g_var.instStride;

    if (!SafePtr(base, sizeof(void*)))
        return false;

    const uint32_t* buckets = *(const uint32_t**)(base + g_var.bucketOff);

    if (!buckets)
        return false;

    uint32_t bucket;

    if (g_var.hash == VARHASH_T9)
    {
        const uint32_t folded =
            (uint32_t)(canonId & 0xFFFFFFFFull) ^ (uint32_t)(canonId >> 32);

        bucket = (uint32_t)(0x84222325u * (uint32_t)parentVarId
               + 0x1B3u * folded) & g_varBucketMask;
    }
    else
    {
        bucket = (uint32_t)(((uint32_t)parentVarId * 0x84222325u)
               + (uint32_t)canonId) & g_varBucketMask;
    }

    if (!SafePtr((uintptr_t)(buckets + bucket), 4))
        return false;

    for (uint32_t n = buckets[bucket], guard = 0; n && guard < 256; ++guard)
    {
        const char* node = nullptr;
        const char* v = nullptr;

        if (!VarSlot(inst, n, &node, &v))
            return false;

        if (!SafePtr((uintptr_t)node, 32))
            return false;

        if (*(const uint32_t*)(node + g_var.nodeParentOff) == (uint32_t)parentVarId &&
            *(const uint64_t*)(node + 0x00) == canonId)
        {
            if (!SafePtr((uintptr_t)v, 16))
                return false;

            if (outVal)  *outVal  = *(const uint64_t*)v;
            if (outType) *outType = *(const int*)(v + 8);
            return true;
        }

        n = *(const uint32_t*)(node + g_var.nodeNextOff);
    }

    return false;
}

bool VarFieldFloat(int inst, int varId, uint64_t canonId, float* out)
{
    uint64_t raw = 0;
    int      type = 0;

    if (!VarField(inst, varId, canonId, &raw, &type))
        return false;

    if (type == VAL_TYPE_FLOAT) { *out = *(const float*)&raw;  return true; }
    if (type == VAL_TYPE_INT)   { *out = (float)(int64_t)raw;  return true; }

    return false;
}

#pragma endregion

#pragma region Hud Elements

int Elem_SyncFromObject(int inst, HudElem* e)
{

    if (!e)
        return 0;

    const int varId = e->varId;
    if (!varId)
        return 0;

    int got = 0;
    float f;
    if (VarFieldFloat(inst, varId, HUDF_ALPHA, &f)) { e->color[3] = f;      got |= SYNC_ALPHA; }
    if (VarFieldFloat(inst, varId, HUDF_SORT,  &f)) { e->sort     = (int)f; got |= SYNC_SORT;  }

    uint64_t raw = 0;
    int      type = 0;

    if (VarField(inst, varId, HUDF_HIDDEN, &raw, &type))
    {
        e->hidden = (raw != 0);
        got |= SYNC_HIDDEN;
    }

    if (VarField(inst, varId, HUDF_COLOR, &raw, &type))
    {
        static int logged = 0;
        if (!logged)
        {
            ++logged;
        }

        if (raw > 0x10000 && raw < 0x0000800000000000ULL && SafePtr((uintptr_t)raw, 12))
        {
            const float* v = (const float*)raw;
            if (v[0] >= 0.0f && v[0] <= 1.0f &&
                v[1] >= 0.0f && v[1] <= 1.0f &&
                v[2] >= 0.0f && v[2] <= 1.0f)
            {
                e->color[0] = v[0];
                e->color[1] = v[1];
                e->color[2] = v[2];
                got |= SYNC_COLOR;
            }
        }
    }

    return got;
}

bool Elem_FieldChanged(HudElem* e, int slot, uint64_t raw)
{
    if ((e->lastSeen & (1u << slot)) && e->lastField[slot] == raw)
        return false;

    e->lastSeen |= (1u << slot);
    e->lastField[slot] = raw;
    return true;
}

void Elem_SyncLayout(int inst, HudElem* e)
{
    if (!e || !e->varId)
        return;

    const int varId = e->varId;
    float f;

    uint64_t raw = 0;
    int      type = 0;

    #define SYNC_NUM(canon, slot, expr)                                        \
        if (VarField(inst, varId, (canon), &raw, &type) &&                     \
            (type == VAL_TYPE_FLOAT || type == VAL_TYPE_INT) &&                \
            Elem_FieldChanged(e, (slot), raw))                                 \
        {                                                                      \
            f = (type == VAL_TYPE_FLOAT) ? *(const float*)&raw                 \
                                         : (float)(int64_t)raw;                \
            (expr);                                                            \
        }

    SYNC_NUM(HUDF_X,         SYNCF_X,         e->x       = f);
    SYNC_NUM(HUDF_Y,         SYNCF_Y,         e->y       = f);
    SYNC_NUM(HUDF_WIDTH,     SYNCF_WIDTH,     e->w       = f);
    SYNC_NUM(HUDF_HEIGHT,    SYNCF_HEIGHT,    e->h       = f);
    SYNC_NUM(HUDF_XOFFSET,   SYNCF_XOFFSET,   e->xOffset = f);
    SYNC_NUM(HUDF_YOFFSET,   SYNCF_YOFFSET,   e->yOffset = f);
    SYNC_NUM(HUDF_FONTSCALE, SYNCF_FONTSCALE, (e->fontScaleRaw = f, e->scale = f * FontScaleFactorFor(e->font)));
    SYNC_NUM(HUDF_ALIGNX,    SYNCF_ALIGNX,    e->alignX    = SelfAlignClamp((int)f));
    SYNC_NUM(HUDF_ALIGNY,    SYNCF_ALIGNY,    e->alignY    = SelfAlignClamp((int)f));
    SYNC_NUM(HUDF_HORZALIGN, SYNCF_HORZALIGN, e->horzAlign = HorzAlignClamp((int)f));
    SYNC_NUM(HUDF_VERTALIGN, SYNCF_VERTALIGN, e->vertAlign = VertAlignClamp((int)f));

    #undef SYNC_NUM

    if (VarField(inst, varId, HUDF_FOREGROUND, &raw, &type))     e->foreground     = (raw != 0);
    if (VarField(inst, varId, HUDF_ARCHIVED, &raw, &type))       e->archived       = (raw != 0);
    if (VarField(inst, varId, HUDF_HIDEWHENINMENU, &raw, &type)) e->hideWhenInMenu = (raw != 0);

    if (VarField(inst, varId, HUDF_ALIGN, &raw, &type) && type == VAL_TYPE_STRING)
    {
        const char* n = StrCache_Find(raw);

        if (n)
        {
            const int pt = PointFromName(n, -1);
            if (pt >= 0)
            {
                e->alignX = SelfAlignClamp(pt % 3);
                e->alignY = SelfAlignClamp(pt / 3);
            }
        }
        else
        {
            StrCache_Miss("align", raw);
        }
    }

    if (VarField(inst, varId, HUDF_RELATIVE, &raw, &type) && type == VAL_TYPE_STRING)
    {
        const char* n = StrCache_Find(raw);

        if (n)
        {
            const int pt = PointFromName(n, -1);
            if (pt >= 0)
            {
                e->horzAlign = HorzAlignClamp((pt % 3) + HHA_LEFT);
                e->vertAlign = VertAlignClamp((pt / 3) + HVA_TOP);
            }
        }
        else
        {
            StrCache_Miss("relative", raw);
        }
    }



}

void Elem_SyncAllLayout(int inst)
{
    if (!g_varGlobals)
        return;

    ElemLock();

    for (int i = 0; i < g_elemCount; ++i)
    {
        if (!g_elems[i].varId)
            continue;

        Elem_SyncLayout(inst, &g_elems[i]);
        Elem_Push(&g_elems[i]);
    }

    ElemUnlock();
}

bool MethodElem(uint64_t entnum, uint64_t classnum, int* outId)
{
    if (classnum != CLASS_NUM_HUDELEM && classnum != 0)
    {
        static int logged = 0;

        if (logged < 8)
        {
            ++logged;
            DbgPrint("[Hud] %s: self classnum=%llu (expected %d), entnum=%llu "
                "- treating as element handle anyway",
                g_scrWhere ? g_scrWhere : "?",
                (unsigned long long)classnum, CLASS_NUM_HUDELEM,
                (unsigned long long)entnum);
        }
    }

    *outId = (int)(uint32_t)entnum;
    return *outId != 0;
}

void MintElemStruct(int inst, int handle)
{
    if (!ScrVm_AddEntity_s)
        return;

    if (g_useEntityBridge && !ScrVar_EntityBridge_s)
        return;

    const uint32_t entnumTmp = (uint32_t)handle;

    ScrVm_AddEntity_s(inst, &entnumTmp, CLASS_NUM_HUDELEM);

    int varId = 0;

    const int bridgeClass = g_var.sharded ? 0 : CLASS_NUM_HUDELEM;

    if (g_useEntityBridge)
        varId = ScrVar_EntityBridge_s(inst, (uint64_t)(uint32_t)handle,
            bridgeClass, 0);

    {
        static int logged = 0;

        const bool sane = (varId > 0 && (uint32_t)varId <= 0x7FFFFu);

        if (!sane || logged < 8)
        {
            ++logged;
        }
    }

    ElemLock();
    HudElem* e = Elem_Find(handle);
    if (e)
        e->varId = varId;
    ElemUnlock();
}

#pragma endregion

#pragma region VM Access

void Scr_NewClientHudElem_Obj(int inst)
{
    SCR_WHERE("newClientHudElem");

    int handle = 0;

    ElemLock();
    HudElem* e = Elem_New();
    if (e)
        handle = e->draw;
    ElemUnlock();

    if (!handle)
    {
        DbgPrint("[Hud] OUT OF ELEMENTS (max %d) - newClientHudElem", MAX_ELEMS);
        return;
    }

    MintElemStruct(inst, handle);
}

void Scr_CreateFontString_M(int inst, uint64_t entnum, uint64_t classnum)
{
    SCR_WHERE("createFontString");
    Elem_SyncAllLayout(inst);

    (void)entnum; (void)classnum;

    const char* fname   = GetStringArg(inst, 0, nullptr);
    const int   fontIdx = FontIndexFromName(fname, FONT_DEFAULT);
    const float raw     = GetFloatArg(inst, 1, 1.0f);
    const float scale   = raw * FontScaleFactorFor(fontIdx);


    int handle = 0;

    ElemLock();
    HudElem* e = Elem_New();
    if (e)
    {
        e->isText = 1;
        e->font = fontIdx;
        e->fontScaleRaw = raw;
        e->scale = scale;
        handle = e->draw;
        Elem_Push(e);
    }
    ElemUnlock();

    if (!handle)
    {
        DbgPrint("[Hud] OUT OF ELEMENTS (max %d) - createFontString", MAX_ELEMS);
        return;
    }

    MintElemStruct(inst, handle);
}

void Scr_SetPoint_M(int inst, uint64_t entnum, uint64_t classnum)
{
    SCR_WHERE("setPoint");
    Elem_SyncAllLayout(inst);

    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const int   align    = GetPointArg(inst, 0, P_TOPLEFT);
    const int   relative = GetPointArg(inst, 1, P_TOPLEFT);
    const float x        = GetFloatArg(inst, 2, 0.0f);
    const float y        = GetFloatArg(inst, 3, 0.0f);

    HudElem tmp;
    tmp.draw = id;
    tmp.varId = 0;
    ElemLock();
    { HudElem* src = Elem_Find(id); if (src) tmp.varId = src->varId; }
    ElemUnlock();


    const int got = Elem_SyncFromObject(inst, &tmp);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e)
    {
        Elem_ApplyPointPair(e, PointClamp(align), PointClamp(relative));
        e->x = x;
        e->y = y;
        if (got & SYNC_ALPHA)  e->color[3] = tmp.color[3];
        if (got & SYNC_SORT)   e->sort     = tmp.sort;
        if (got & SYNC_HIDDEN) e->hidden   = tmp.hidden;
        if (got & SYNC_COLOR)
        {
            e->color[0] = tmp.color[0];
            e->color[1] = tmp.color[1];
            e->color[2] = tmp.color[2];
        }
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetShader_M(int inst, uint64_t entnum, uint64_t classnum)
{
    SCR_WHERE("setShader");
    Elem_SyncAllLayout(inst);

    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    HudElem tmp;
    tmp.draw = id;
    tmp.varId = 0;
    ElemLock();
    { HudElem* src = Elem_Find(id); if (src) tmp.varId = src->varId; }
    ElemUnlock();

    const int got = Elem_SyncFromObject(inst, &tmp);

    void* mat = T89_FindMaterial(GetStringArg(inst, 0, nullptr));

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e)
    {
        e->w = GetFloatArg(inst, 1, 0.0f);
        e->h = GetFloatArg(inst, 2, 0.0f);
        e->material = mat;
        if (got & SYNC_ALPHA)  e->color[3] = tmp.color[3];
        if (got & SYNC_SORT)   e->sort     = tmp.sort;
        if (got & SYNC_HIDDEN) e->hidden   = tmp.hidden;
        if (got & SYNC_COLOR)
        {
            e->color[0] = tmp.color[0];
            e->color[1] = tmp.color[1];
            e->color[2] = tmp.color[2];
        }
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetText_M(int inst, uint64_t entnum, uint64_t classnum)
{
    SCR_WHERE("setText");
    Elem_SyncAllLayout(inst);

    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    char        numbuf[32];
    const char* text = GetTextArg(inst, 0, numbuf, sizeof(numbuf));

    {
        static int traced = 0;
        if (traced < 24)
        {
            ++traced;
        }
    }

    HudElem tmp;
    tmp.draw = id;
    tmp.varId = 0;
    ElemLock();
    { HudElem* src = Elem_Find(id); if (src) tmp.varId = src->varId; }
    ElemUnlock();

    const int got = Elem_SyncFromObject(inst, &tmp);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e)
    {
        e->isText = 1;
        CopyText(e->text, text);
        if (got & SYNC_ALPHA)  e->color[3] = tmp.color[3];
        if (got & SYNC_SORT)   e->sort     = tmp.sort;
        if (got & SYNC_HIDDEN) e->hidden   = tmp.hidden;
        if (got & SYNC_COLOR)
        {
            e->color[0] = tmp.color[0];
            e->color[1] = tmp.color[1];
            e->color[2] = tmp.color[2];
        }
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_Destroy_M(int inst, uint64_t entnum, uint64_t classnum)
{
    SCR_WHERE("destroy");

    (void)inst;

    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    Elem_Free(id);
}

void Scr_SetColor_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    float rgb[3] = { 1.0f, 1.0f, 1.0f };

    if (ScrVm_GetNumParam(inst) >= 3)
    {
        rgb[0] = GetFloatArg(inst, 0, 1.0f);
        rgb[1] = GetFloatArg(inst, 1, 1.0f);
        rgb[2] = GetFloatArg(inst, 2, 1.0f);
    }
    else if (ScrVm_GetVector_s)
    {
        const float def[3] = { 1.0f, 1.0f, 1.0f };
        SCR_GETTER("GetVector", 0);
        ScrVm_GetVector_s(inst, 0, rgb, def);
        SCR_GETTER_DONE();
    }

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e)
    {
        e->color[0] = rgb[0];
        e->color[1] = rgb[1];
        e->color[2] = rgb[2];
        Elem_Push(e);
    }
    ElemUnlock();
}

void Scr_SetAlpha_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const float a = GetFloatArg(inst, 0, 1.0f);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e) { e->color[3] = a; Elem_Push(e); }
    ElemUnlock();
}

void Scr_SetSort_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const int sort = (int)GetFloatArg(inst, 0, 0.0f);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e) { e->sort = sort; Elem_Push(e); }
    ElemUnlock();
}

void Scr_SetHidden_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const int hide = GetIntArg(inst, 0, 0);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e) { e->hidden = hide ? 1 : 0; Elem_Push(e); }
    ElemUnlock();
}

void Scr_SetParent_M(int inst, uint64_t entnum, uint64_t classnum)
{
    SCR_WHERE("setParent");
    Elem_SyncAllLayout(inst);

    (void)inst; (void)entnum; (void)classnum;
}

void Elem_TickAnims()
{
    ElemLock();

    for (int i = 0; i < g_elemCount; ++i)
    {
        HudElem* e = &g_elems[i];
        bool changed = false;

        if (e->animAlphaLeft > 0)
        {
            --e->animAlphaLeft;
            const float t = 1.0f - (float)e->animAlphaLeft / (float)e->animAlphaTotal;
            e->color[3] = e->animAlphaFrom + (e->animAlphaTo - e->animAlphaFrom) * t;
            changed = true;
        }

        if (e->animMoveLeft > 0)
        {
            --e->animMoveLeft;
            const float t = 1.0f - (float)e->animMoveLeft / (float)e->animMoveTotal;
            e->x = e->animMoveFromX + (e->animMoveToX - e->animMoveFromX) * t;
            e->y = e->animMoveFromY + (e->animMoveToY - e->animMoveFromY) * t;
            changed = true;
        }

        if (e->animScaleLeft > 0)
        {
            --e->animScaleLeft;
            const float t = 1.0f - (float)e->animScaleLeft / (float)e->animScaleTotal;
            e->w = e->animScaleFromW + (e->animScaleToW - e->animScaleFromW) * t;
            e->h = e->animScaleFromH + (e->animScaleToH - e->animScaleFromH) * t;
            changed = true;
        }

        if (e->animFontLeft > 0)
        {
            --e->animFontLeft;
            const float t = 1.0f - (float)e->animFontLeft / (float)e->animFontTotal;
            e->scale = e->animFontFrom + (e->animFontTo - e->animFontFrom) * t;
            changed = true;
        }

        if (changed)
            Elem_Push(e);
    }

    ElemUnlock();
}

int AnimFrames(float seconds)
{
    int f = (int)(seconds * ANIM_FPS);
    return f < 1 ? 1 : f;
}

void Scr_FadeOverTime_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const float secs = GetFloatArg(inst, 0, 0.0f);
    const float to   = GetFloatArg(inst, 1, 0.0f);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e)
    {
        e->animAlphaFrom  = e->color[3];
        e->animAlphaTo    = to;
        e->animAlphaTotal = AnimFrames(secs);
        e->animAlphaLeft  = e->animAlphaTotal;
    }
    ElemUnlock();
}

void Scr_MoveOverTime_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const float secs = GetFloatArg(inst, 0, 0.0f);
    const float tx   = GetFloatArg(inst, 1, 0.0f);
    const float ty   = GetFloatArg(inst, 2, 0.0f);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e)
    {
        e->animMoveFromX = e->x;
        e->animMoveFromY = e->y;
        e->animMoveToX   = tx;
        e->animMoveToY   = ty;
        e->animMoveTotal = AnimFrames(secs);
        e->animMoveLeft  = e->animMoveTotal;
    }
    ElemUnlock();
}

void Scr_ScaleOverTime_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const float secs = GetFloatArg(inst, 0, 0.0f);
    const float tw   = GetFloatArg(inst, 1, 0.0f);
    const float th   = GetFloatArg(inst, 2, 0.0f);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e)
    {
        e->animScaleFromW = e->w;
        e->animScaleFromH = e->h;
        e->animScaleToW   = tw;
        e->animScaleToH   = th;
        e->animScaleTotal = AnimFrames(secs);
        e->animScaleLeft  = e->animScaleTotal;
    }
    ElemUnlock();
}

void Scr_ChangeFontScaleOverTime_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const float secs = GetFloatArg(inst, 0, 0.0f);
    const float raw  = GetFloatArg(inst, 1, 1.0f);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e)
    {
        e->animFontFrom  = e->scale;
        e->animFontTo    = raw * FontScaleFactorFor(e->font);
        e->animFontTotal = AnimFrames(secs);
        e->animFontLeft  = e->animFontTotal;
    }
    ElemUnlock();
}

void Scr_SetAlignX_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const int a = GetPointArg(inst, 0, P_TOPLEFT);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e) { e->alignX = (PointClamp(a) % 3); e->alignY = (PointClamp(a) / 3); Elem_Push(e); }
    ElemUnlock();
}

void Scr_SetAlignY_M(int inst, uint64_t entnum, uint64_t classnum)
{
    Scr_SetAlignX_M(inst, entnum, classnum);
}

void Scr_SetHorzAlign_M(int inst, uint64_t entnum, uint64_t classnum)
{
    int id;
    if (!MethodElem(entnum, classnum, &id))
        return;

    const int a = GetPointArg(inst, 0, P_TOPLEFT);

    ElemLock();
    HudElem* e = Elem_Find(id);
    if (e) { e->horzAlign = HorzAlignClamp((PointClamp(a) % 3) + HHA_LEFT);
             e->vertAlign = VertAlignClamp((PointClamp(a) / 3) + HVA_TOP); Elem_Push(e); }
    ElemUnlock();
}

void Scr_SetVertAlign_M(int inst, uint64_t entnum, uint64_t classnum)
{
    Scr_SetHorzAlign_M(inst, entnum, classnum);
}

void Scr_HudLog(int inst)
{
    SCR_WHERE("hudLog");

    static volatile int logged = 0;
    if (logged >= 200)
        return;
    ++logged;

    DbgPrint("[GSC] %s", GetStringArg(inst, 0, "(no arg)"));
}

void* ResolveCustom(const char* vm, const char* kind,
    t8_builtin_t* table, int count,
    GetFunction_t fallback, bool preferEngine,
    int nameHash, int* type, int* minArgs, int* maxArgs)
{
    if (preferEngine && fallback)
    {
        void* stock = fallback(nameHash, type, minArgs, maxArgs);
        if (stock)
            return stock;
    }

    for (int i = 0; i < count; ++i)
    {
        if (table[i].hash != (uint32_t)nameHash)
            continue;

        if (type)    *type = 0;
        if (minArgs) *minArgs = table[i].minArgs;
        if (maxArgs) *maxArgs = table[i].maxArgs;

        DbgPrint("[%s] CUSTOM %s %08X -> 0x%p (args %d..%d)",
            vm, kind, (uint32_t)nameHash, table[i].func,
            table[i].minArgs, table[i].maxArgs);

        return table[i].func;
    }

    void* r = preferEngine
        ? nullptr
        : (fallback ? fallback(nameHash, type, minArgs, maxArgs) : nullptr);

    if (!r)
    {
        const int slot = preferEngine ? 1 : 0;
        static int logged[2] = { 0, 0 };
        const int kMax = 40;

        if (logged[slot] < kMax)
            DbgPrint("[%s] unresolved %s %08X", vm, kind, (uint32_t)nameHash);
        else if (logged[slot] == kMax)
            DbgPrint("[%s] further unresolved lines suppressed", vm);

        ++logged[slot];
    }

    return r;
}

#pragma endregion

#pragma region Hooks

void* Scr_GetFunction_h(int nameHash, int* type, int* minArgs, int* maxArgs)
{
    return ResolveCustom("Scr", "FUNCTION", g_t8_builtins, g_t8_builtinCount,
        Scr_GetFunction_s, false, nameHash, type, minArgs, maxArgs);
}

void* Scr_GetMethod_h(int nameHash, int* type, int* minArgs, int* maxArgs)
{
    return ResolveCustom("Scr", "METHOD", g_t8_methods, g_t8_methodCount,
        Scr_GetMethod_s, false, nameHash, type, minArgs, maxArgs);
}

void* CScr_GetFunction_h(int nameHash, int* type, int* minArgs, int* maxArgs)
{
    return ResolveCustom("CScr", "FUNCTION", g_t8_builtins, g_t8_builtinCount,
        CScr_GetFunction_s, true, nameHash, type, minArgs, maxArgs);
}

void* CScr_GetMethod_h(int nameHash, int* type, int* minArgs, int* maxArgs)
{
    return ResolveCustom("CScr", "METHOD", g_t8_methods, g_t8_methodCount,
        CScr_GetMethod_s, true, nameHash, type, minArgs, maxArgs);
}

void ScrVar_RemoveReference_h(int inst, int varIndex)
{
    if (varIndex <= 0 || (unsigned)varIndex > g_varBucketMask)
    {
        if (g_rrBadLogs < 8)
        {
            ++g_rrBadLogs;

            const uintptr_t ret  = (uintptr_t)__builtin_return_address(0);
            const uintptr_t base = GetBaseAddress();

            DbgPrint("[Var] REFUSED release inst=%d index=0x%X (max 0x%X) caller=rt 0x%llX",
                inst, (unsigned)varIndex, g_varBucketMask,
                (unsigned long long)((ret > base) ? (ret - base) : 0));
        }

        return;
    }

    ScrVar_RemoveReference_s(inst, varIndex);
}

bool Scr_YieldUndefined(int inst)
{
    char* top = ScrVm_Top(inst);
    if (!top)
        return false;

    char* base = top - 16;
    if (!ScrVm_SlotOk(inst, base))
        return false;

    static int logged = 0;
    if (logged < 4)
    {
        ++logged;
        DbgPrint("[Scr] undef-index fixup: base type=%d raw=0x%llX, index type=%d",
            *(const int*)(base + 8), (unsigned long long)*(const uint64_t*)base,
            *(const int*)(top + 8));
    }

    *(uint64_t*)base   = 0;
    *(int*)(base + 8)  = VAL_TYPE_UNDEFINED;
    return true;
}

void ScrVm_Error_h(uint32_t code, int inst, int terminal)
{
    const uintptr_t ret   = (uintptr_t)__builtin_return_address(0);
    const uintptr_t base  = GetBaseAddress();
    const uintptr_t rt    = (ret > base) ? (ret - base) : 0;
    const uintptr_t image = (rt > 0x400000) ? (rt - 0x400000) : 0;

    while (__sync_lock_test_and_set(&g_errSiteLock, 1))
        scePthreadYield();

    ErrSite* s = nullptr;

    for (int i = 0; i < g_errSiteCount; ++i)
    {
        if (g_errSites[i].code == code && g_errSites[i].site == rt)
        {
            s = &g_errSites[i];
            break;
        }
    }

    if (!s && g_errSiteCount < ERR_SITE_MAX)
    {
        s = &g_errSites[g_errSiteCount++];
        s->code = code;
        s->site = rt;
        s->hits = 0;
    }

    const uint32_t hits = s ? ++s->hits : 0;

    __sync_lock_release(&g_errSiteLock);

    if (!s)
    {
        static bool warnedFull = false;
        if (!warnedFull)
        {
            warnedFull = true;
            DbgPrint("[Err] site table full (%d) - further NEW sites unlogged", ERR_SITE_MAX);
        }
    }
    else if (hits <= ERR_SITE_FIRST || (hits % ERR_SITE_EVERY) == 0)
    {

        if (g_scrGetter)
        {
            DbgPrint("[Err] code=0x%llX inst=%d %s rt 0x%llX hit #%u  where=%s/%s arg %d",
                (unsigned long long)code, inst,
                terminal ? "TERMINAL" : "warn",
                (unsigned long long)rt, hits,
                g_scrWhere ? g_scrWhere : "?", g_scrGetter, g_scrArg);
        }
        else
        {
            DbgPrint("[Err] code=0x%llX inst=%d %s rt 0x%llX img 0x%llX hit #%u  where=%s",
                (unsigned long long)code, inst,
                terminal ? "TERMINAL" : "warn",
                (unsigned long long)rt, (unsigned long long)image, hits,
                g_scrWhere ? g_scrWhere : "script");
        }
    }

    if (terminal)
    {
        g_scrWhere = nullptr;
        g_scrGetter = nullptr;
        g_scrArg = -1;
    }

    if (g_scrUndefTolerant && code == SCR_ERR_INDEX_ON_UNDEF)
    {
        static uint32_t tolerated = 0;
        ++tolerated;

        if (Scr_YieldUndefined(inst))
        {
            if ((tolerated % 512) == 0)
                DbgPrint("[Scr] undef-index tolerated total=%u", tolerated);

            return;
        }
    }

    if (terminal && g_scrNonFatal)
    {
        static uint32_t downgrades = 0;
        ++downgrades;

        if (hits <= ERR_SITE_FIRST || (downgrades % 256) == 0)
            DbgPrint("[Err] code=0x%llX downgraded (rt 0x%llX) total=%u",
                (unsigned long long)code, (unsigned long long)rt, downgrades);

        ScrVm_Error_s(code, inst, 0);
        return;
    }

    ScrVm_Error_s(code, inst, terminal);

}

ObjFieldRet Scr_GetObjectField_h(int inst, int classnum, uint64_t entnum,
    int clientNum, int fieldIndex)
{

    if (classnum == CLASS_NUM_HUDELEM)
    {
        DbgPrint("[OF] GET  *** HUDELEM *** inst=%d ent=%llu client=%d field=%d",
            inst, (unsigned long long)entnum, clientNum, fieldIndex);
    }
    else if (g_ofGetLogs < OF_LOG_BUDGET)
    {
        ++g_ofGetLogs;
        DbgPrint("[OF] GET  inst=%d class=%d ent=%llu client=%d field=%d",
            inst, classnum, (unsigned long long)entnum, clientNum, fieldIndex);
    }

    return Scr_GetObjectField_s(inst, classnum, entnum, clientNum, fieldIndex);
}

bool Scr_SetObjectField_h(int inst, int classnum, uint64_t entnum,
    int clientNum, int fieldIndex, uint64_t value)
{
    if (classnum == CLASS_NUM_HUDELEM)
    {
        DbgPrint("[OF] SET  *** HUDELEM *** inst=%d ent=%llu client=%d field=%d",
            inst, (unsigned long long)entnum, clientNum, fieldIndex);
    }
    else if (g_ofSetLogs < OF_LOG_BUDGET)
    {
        ++g_ofSetLogs;
        DbgPrint("[OF] SET  inst=%d class=%d ent=%llu client=%d field=%d",
            inst, classnum, (unsigned long long)entnum, clientNum, fieldIndex);
    }

    return Scr_SetObjectField_s(inst, classnum, entnum, clientNum, fieldIndex, value);
}

int Renderer_h()
{
    ++g_frameTick;

    if (g_hudResetPending)
    {
        g_hudResetPending = 0;
        T89_ClearAll();
        DbgPrint("[Hud] reset on script install - elements cleared");
    }

    Elem_SyncAllLayout(GSC_INST);

    Elem_TickAnims();
    T89_FlushDraws();

    return Renderer_s();
}

#pragma endregion
