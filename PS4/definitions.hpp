#pragma once

#include "utilities.hpp"

enum game 
{
    NONE,

    T5_SP,      // BO1     1.08
    T5_MP,      // BO1     1.08

    T6_SP,      // BO2     1.10
    T6_MP,      // BO2     1.10
    T6_ZM,      // BO2     1.10

    IW6_SP,     // GHO     1.21
    IW6_MP,     // GHO     1.21

    S1_SP,      // AW      1.24
    S1_MP,      // AW      1.24

    T7,         // BO3     1.33

    IW7,        // IW      1.25

    H1_SP,      // MWR     1.15
    H1_MP,      // MWR     1.15

    S2_SP,      // WWII    1.25
    S2_MP,      // WWII    1.25

    T8,         // BO4     1.26

    IW8,        // MW19    1.67 

    H2,         // MW2CR   1.00

    T9,         // CW      1.41

    S4,         // VG      1.27

    IW9_SP,     // MWII    ANY
    IW9_MP,     // MWII    ANY

    JUP_SP,     // MWIII   ANY
    JUP_MP,     // MWIII   ANY

    T10_SP,     // BO6     ALPHA
    T10_MP,     // BO6     ALPHA
};

extern game g_id;

struct GameDetectionEntry
{
    uintptr_t address;
    game id;
};

static GameDetectionEntry gameDetectionTable[] = {
    {REBASE_ADDRESS(0x6908F0), T5_SP},   // multiplayer
    {REBASE_ADDRESS(0x70C268), T5_MP},   // multiplayer

    {REBASE_ADDRESS(0x706FDA), IW6_SP},  // IsMultiplayer
    {REBASE_ADDRESS(0x8251D7), IW6_MP},  // IsMultiplayer

    {REBASE_ADDRESS(0x6949BB), S1_SP},   // IsMultiplayer
    {REBASE_ADDRESS(0x80B0E7), S1_MP},   // IsMultiplayer

    {REBASE_ADDRESS(0x1312346), T7},     // IsMultiplayerGame

    {REBASE_ADDRESS(0x1364BAB), IW7},    // IsMultiplayer

    {REBASE_ADDRESS(0x95E106), H1_SP},   // IsMultiplayer
    {REBASE_ADDRESS(0xB991A9), H1_MP},   // IsMultiplayer

    {REBASE_ADDRESS(0xB97652), S2_SP},   // IsMultiplayer
    {REBASE_ADDRESS(0xFD7939), S2_MP},   // IsMultiplayer

    {REBASE_ADDRESS(0x199A0B7), T8},     // MultiProgress

    {REBASE_ADDRESS(0x19EB26C), IW8},    // MultiProgress

    {REBASE_ADDRESS(0x946DEE), H2},      // IsMultiplayer

    {REBASE_ADDRESS(0x1D84C10), T9},     // Multiplayer

    {REBASE_ADDRESS(0x2620BA2), S4},     // MultiProgress

    {REBASE_ADDRESS(0x2BB28A0), IW9_SP}, // MultiProgress
    {REBASE_ADDRESS(0x2E6ACB7), IW9_MP}, // MultiProgress

    {REBASE_ADDRESS(0x2BF4006), JUP_SP}, // MultiProgress
    {REBASE_ADDRESS(0x2C3A5EA), JUP_MP}, // MultiProgress

    {REBASE_ADDRESS(0x29C4CB3), T10_SP}, // MultiProgress
    {REBASE_ADDRESS(0x2C55743), T10_MP}  // MultiProgress
};

inline bool g_legacy = false;

struct offsets
{
    uint64_t hudelems_pointer = 0xDEADBEEF;
    uint64_t SV_SetConfigString = 0xDEADBEEF;
    uint64_t HECmd_ClearAllTextAfterHudElem = 0xDEADBEEF;
    uint64_t HECmd_SetText = 0xDEADBEEF;

    uint64_t Com_GetCurrentCoDPlayMode = 0xDEADBEEF;

    uint64_t Scr_VarPub_ProgramBuffer = 0xDEADBEEF;
    uint64_t Scr_CompilePub_ProgramLen = 0xDEADBEEF;

    uint64_t PMem_AllocFromSource_NoDebug = 0xDEADBEEF;
    uint64_t DB_FindXAssetHeader = 0xDEADBEEF;

    uint64_t Scr_GetFunction = 0xDEADBEEF;
    uint64_t R_AddCmdDrawQuadPicW = 0xDEADBEEF;
    uint64_t R_AddCmdDrawText = 0xDEADBEEF;

    uint64_t Scr_ReadFile_FastFile = 0xDEADBEEF;
    uint64_t Dvar_GetString = 0xDEADBEEF;
    uint64_t Z_Uncompress = 0xDEADBEEF;
    uint64_t Hunk_AllocateTempMemoryHigh = 0xDEADBEEF;
    uint64_t Scr_GetChecksum = 0xDEADBEEF;

    void init(int id) 
    {
        switch (int(g_id)) 
        {

        case T5_MP:
        {
            Scr_ReadFile_FastFile = REBASE_ADDRESS(0x468A90);
            Dvar_GetString = REBASE_ADDRESS(0x3C5670);
            Z_Uncompress = REBASE_ADDRESS(0x3FBCE0);
            Hunk_AllocateTempMemoryHigh = REBASE_ADDRESS(0x3BC870);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x1CEA30);
            Scr_GetChecksum = REBASE_ADDRESS(0x479340);
            break;
        }

        case T5_SP:
        {
            Scr_ReadFile_FastFile = REBASE_ADDRESS(0x475440);
            Z_Uncompress = REBASE_ADDRESS(0x3D2240);
            Hunk_AllocateTempMemoryHigh = REBASE_ADDRESS(0x3841A0);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x161A80);
            break;
        }

        case IW6_SP: // GHO
        {
            PMem_AllocFromSource_NoDebug = REBASE_ADDRESS(0x39E8C0);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x1A1250);
            break;
        }

        case IW6_MP: // GHO
        {
            hudelems_pointer = REBASE_ADDRESS(0x34FE790);
            SV_SetConfigString = REBASE_ADDRESS(0x3E9730);
            HECmd_ClearAllTextAfterHudElem = REBASE_ADDRESS(0x34E5750);
            PMem_AllocFromSource_NoDebug = REBASE_ADDRESS(0x4820B0);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x25FC10);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case S1_SP: // AW
        {
            PMem_AllocFromSource_NoDebug = REBASE_ADDRESS(0x3B6A40);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x1981F0);
            break;
        }

        case S1_MP: // AW
        {
            hudelems_pointer = REBASE_ADDRESS(0x97F990);
            SV_SetConfigString = REBASE_ADDRESS(0x49E050);
            HECmd_ClearAllTextAfterHudElem = REBASE_ADDRESS(0x960B20);
            PMem_AllocFromSource_NoDebug = REBASE_ADDRESS(0x534F40);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x2A4570);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case T7: // BO3
        {
            DB_FindXAssetHeader = REBASE_ADDRESS(0x8591B0);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case IW7: // IW
        {
            Scr_VarPub_ProgramBuffer = REBASE_ADDRESS(0x3A9B8F8);
            Scr_CompilePub_ProgramLen = REBASE_ADDRESS(0x6D7B624);
            Com_GetCurrentCoDPlayMode = REBASE_ADDRESS(0x4E92DD4);
            HECmd_SetText = REBASE_ADDRESS(0xBFC0A0);
            DB_FindXAssetHeader = REBASE_ADDRESS(0xB39B40);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case H1_SP: // MWR
        {
            PMem_AllocFromSource_NoDebug = REBASE_ADDRESS(0x640000);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x2F4C30);
            break;
        }

        case H1_MP: // MWR
        {
            hudelems_pointer = REBASE_ADDRESS(0xACBC840);
            SV_SetConfigString = REBASE_ADDRESS(0x84D8F0);
            HECmd_ClearAllTextAfterHudElem = REBASE_ADDRESS(0xC5FDC0);
            PMem_AllocFromSource_NoDebug = REBASE_ADDRESS(0x8C69A0);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x5B6590);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case S2_SP: // WWII
        {
            DB_FindXAssetHeader = REBASE_ADDRESS(0x546840);
            break;
        }

        case S2_MP: // WWII
        {
            hudelems_pointer = REBASE_ADDRESS(0xBD09700);
            SV_SetConfigString = REBASE_ADDRESS(0xC2B3F0);
            HECmd_ClearAllTextAfterHudElem = REBASE_ADDRESS(0x11359E0);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x323440);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case T8: // BO4
        {
            DB_FindXAssetHeader = REBASE_ADDRESS(0xDA44B0);
            Scr_GetFunction = REBASE_ADDRESS(0xF48F30);
            R_AddCmdDrawQuadPicW = REBASE_ADDRESS(0x108DDD0);
            R_AddCmdDrawText = REBASE_ADDRESS(0x108DBA0);

            break;
        }

        /* --------------------------------------------------------------------------------- */

        case IW8: // MW (2019)
        {
            HECmd_SetText = FindSetText();

            uintptr_t wrapper = FindCalleeAfterString("ui_scrollbar");
            DB_FindXAssetHeader = ResolveFirstCallee(wrapper);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case H2: // MW2CR
        {
            PMem_AllocFromSource_NoDebug = REBASE_ADDRESS(0x654180);
            DB_FindXAssetHeader = REBASE_ADDRESS(0x336480);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case T9: // CW
        {
			DB_FindXAssetHeader = REBASE_ADDRESS(0xB3B680);
            Scr_GetFunction = REBASE_ADDRESS(0x16BDA40);
            R_AddCmdDrawQuadPicW = REBASE_ADDRESS(0x148CB80);
            R_AddCmdDrawText = REBASE_ADDRESS(0x17DFF60);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case S4: // VG
        {
            //HECmd_SetText = FindSetText();

            auto scan = FindCalleeAfterString("ui_scrollbar");
            DB_FindXAssetHeader = FindFuncStart(scan);

            //scan = AobScan("80 3D ?? ?? ?? ?? 01 75 ?? 48 69 CB A0 76 00 00");
            //Com_GetCurrentCoDPlayMode = ResolveRip(scan, 2, 7);
            break;
        }

        /* --------------------------------------------------------------------------------- */

        case IW9_SP: // MWII
        {
            break;
        }

        case IW9_MP: // MWII
        {
            HECmd_SetText = FindSetText();

            uintptr_t parser = FindCalleeAfterString("ui_scrollbar");
            DB_FindXAssetHeader = FindFuncStart(ResolveTailCallee(parser, 0x400));

            break;
        }

        /* --------------------------------------------------------------------------------- */

        case JUP_SP: // MWIII
        {
            break;
        }

        case JUP_MP: // MWIII
        {
            HECmd_SetText = FindSetText();

            uintptr_t parser = FindCalleeAfterString("ui_scrollbar");
            DB_FindXAssetHeader = FindFuncStart(ResolveTailCallee(parser, 0x400));

            break;
        }

        /* --------------------------------------------------------------------------------- */

        case T10_SP: // BO6
        {
            break;
        }

        case T10_MP: // BO6
        {
            HECmd_SetText = FindSetText();
            DB_FindXAssetHeader = REBASE_ADDRESS(0x138550);

            break;
        }
        }

        DbgPrint("[Offset Values]");
        DbgPrint("  hudelems_pointer                                   : 0x%p", (void*)hudelems_pointer);
        DbgPrint("  SV_SetConfigString                                 : 0x%p", (void*)SV_SetConfigString);
        DbgPrint("  HECmd_ClearAllTextAfterHudElem                     : 0x%p", (void*)HECmd_ClearAllTextAfterHudElem);
        DbgPrint("  HECmd_SetText                                      : 0x%p", (void*)HECmd_SetText);

        DbgPrint("  Com_GetCurrentCoDPlayMode                          : 0x%p", (void*)Com_GetCurrentCoDPlayMode);

        DbgPrint("  Scr_VarPub_ProgramBuffer                           : 0x%p", (void*)Scr_VarPub_ProgramBuffer);
        DbgPrint("  Scr_CompilePub_ProgramLen                          : 0x%p", (void*)Scr_CompilePub_ProgramLen);

        DbgPrint("  PMem_AllocFromSource_NoDebug                       : 0x%p", (void*)PMem_AllocFromSource_NoDebug);
        DbgPrint("  DB_FindXAssetHeader                                : 0x%p", (void*)DB_FindXAssetHeader);

        DbgPrint("  Scr_GetFunction                                    : 0x%p", (void*)Scr_GetFunction);
        DbgPrint("  R_AddCmdDrawQuadPicW                               : 0x%p", (void*)R_AddCmdDrawQuadPicW);
        DbgPrint("  R_AddCmdDrawText                                   : 0x%p", (void*)R_AddCmdDrawText);
    }
};

inline offsets g_offsets;

struct overflow_vals
{
    uint16_t count = 0;
    uint16_t start = 0;
    uint16_t end = 0;

    void init(int id) {
        switch (int(g_id)) {

        case IW6_MP: // GHO
            count = 511;
            start = 541;
            break;

        case S1_MP: // AW
        case H1_MP: // MWR
            count = 650;
            start = 541;
            break;

        case S2_MP: // WWII
            count = 650;
            start = 570;
            break;

        }

        if (count != 0 || start != 0 || end != 0)
            end = start + count - 1;

        DbgPrint("[Overflow Values]");
        DbgPrint("  Count : %d", count);
        DbgPrint("  Start : %d", start);
        DbgPrint("  End   : %d", end);
    }
};

inline overflow_vals g_overflow_vals;

struct findXAsset_vals {
    const char* name;

    void init(int id) {
        switch (int(g_id)) {
        case IW6_SP: // GHO
            name = "988";
            break;

        case IW6_MP: // GHO
            name = "1499";
            break;

            /* --------------------------------------------------------------------------------- */

        case S1_SP: // AW
            name = "42299";
            break;

        case S1_MP: // AW
            name = "42918";
            break;

            /* --------------------------------------------------------------------------------- */

        case T7: // BO3
            name = "scripts/shared/duplicaterender_mgr.gsc";
            break;

            /* --------------------------------------------------------------------------------- */

        case IW7: // IW
            name = "";
            break;

            /* --------------------------------------------------------------------------------- */

        case H1_SP: // MWR
            name = "42309";
            break;

        case H1_MP: // MWR
            name = "42928";
            break;

            /* --------------------------------------------------------------------------------- */

        case S2_SP: // WWII
            name = "709";
            break;

        case S2_MP: // WWII
            name = "1262";
            break;

            /* --------------------------------------------------------------------------------- */

        case T8: // BO4
            name = "";
            break;

            /* --------------------------------------------------------------------------------- */

        case IW8: // MW (2019)
            name = "1892";
            break;

            /* --------------------------------------------------------------------------------- */

        case H2: // MW2CR
            name = "42312";
            break;

            /* --------------------------------------------------------------------------------- */

        case T9: // CW
            name = "";
            break;

            /* --------------------------------------------------------------------------------- */

        case S4: // VG
            name = "";
            break;

            /* --------------------------------------------------------------------------------- */

        case IW9_SP: // MW2 (2022)
            name = "";
            break;

        case IW9_MP: // MW2 (2022)
            name = "";
            break;

            /* --------------------------------------------------------------------------------- */

        case JUP_SP: // MW3 (2023)
            name = "";
            break;

        case JUP_MP: // MW3 (2023)
            name = "";
            break;

            /* --------------------------------------------------------------------------------- */

        case T10_SP: // BO6
            name = "";
            break;

        case T10_MP: // BO6
            name = "";
            break;
        }

        /*DbgPrint("[DB_FindXAssetHeader Values]");
        DbgPrint("  Name       : %s", name);*/
    }
};

inline findXAsset_vals g_findXAsset_vals;

struct textPatch_vals
{
    uintptr_t hudElemBase;
    uintptr_t scriptSystem;
    uintptr_t ncsStringTable;

    int  hudElemStride;
    int  constructVtSlot;
    int  ncsArrayOffset;
    bool iw7Abi = false;

    void init(int id)
    {
        switch (id)
        {
        case IW7:
            hudElemBase = REBASE_ADDRESS(0x60F6300);
            scriptSystem = REBASE_ADDRESS(0x61A5F20);
            ncsStringTable = REBASE_ADDRESS(0x54290D0);
            iw7Abi = true;
            break;
        }

        if (g_id == IW7 || g_id > S2_MP)
            ResolveTextPatch(g_offsets.HECmd_SetText);
    }
};

inline textPatch_vals g_textPatch_vals;

/* -------------------------------------------------------------------------------- */

enum he_type_t : int
{
    HE_TYPE_FREE,
    HE_TYPE_TEXT,
    HE_TYPE_VALUE,
    HE_TYPE_PLAYERNAME,
    HE_TYPE_MATERIAL,
    HE_TYPE_TIMER_DOWN,
    HE_TYPE_TIMER_UP,
    HE_TYPE_TIMER_STATIC,
    HE_TYPE_TENTHS_TIMER_DOWN,
    HE_TYPE_TENTHS_TIMER_UP,
    HE_TYPE_TENTHS_TIMER_STATIC,
    HE_TYPE_CLOCK_DOWN,
    HE_TYPE_CLOCK_UP,
    HE_TYPE_WAYPOINT,
    HE_TYPE_COUNT
};

struct IW6_MP_hudelem_s
{
    he_type_t type;
    char pad[0x80];
    int text;
    char _pad[0x30];
};

struct S1_MP_hudelem_s
{
    char pad[0x1C];
    he_type_t type;
    char _pad[0x64];
    int text;
    char __pad[0x34];
};

struct H1_MP_hudelem_s
{
    char pad[0x1C];
    he_type_t type;
    char _pad[0x64];
    int text;
    char __pad[0x48];
};

struct S2_MP_hudelem_s
{
    uint8_t _pad0[0x1C];
    he_type_t type;
    uint8_t _pad1[0x74];
    int text;
    uint8_t _pad2[0x38];
};

struct scr_entref_t
{
    unsigned short entnum;
    unsigned short classnum;
};

struct ScriptFile
{
    const char* name;
    int compressedLen;
    int len;
    int bytecodeLen;
    const char* buffer;
    unsigned char* bytecode;
};

struct ScriptFile_S4
{
    const char* name;
    const char* pad0;
    int compressedLen;
    int len;
    int bytecodeLen;
    int pad1;
    const char* buffer;
    unsigned char* bytecode;
};

struct ScriptFile_IW9
{
    uint64_t hash;
    int compressedLen;
    int len;
    int bytecodeLen;
    const char* buffer;
    unsigned char* bytecode;
};

struct ScriptParseTree
{
    uint64_t name;
    uint32_t len;
    void* buffer;
};

struct ScriptParseTree_T8
{
    uint64_t  name;
    uint64_t  pad0;
    char* buffer;
    uint32_t  len;
    uint32_t  pad02;
};

struct ScriptParseTree_T9
{
    uint64_t name;
    void* buffer;
    int32_t  len;
    int32_t  pad14;
};

/* ------------------------------------------------------------------------------------- */

typedef unsigned char byte;

struct RawFile
{
    const char* name;
    int len;
    char* buffer;
};

enum T5_XAssetType
{
    ASSET_TYPE_XMODELPIECES,
    ASSET_TYPE_PHYSPRESET,
    ASSET_TYPE_PHYSCONSTRAINTS,
    ASSET_TYPE_DESTRUCTIBLEDEF,
    ASSET_TYPE_XANIM,
    ASSET_TYPE_XMODEL,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_TECHNIQUE_SET,
    ASSET_TYPE_IMAGE,
    ASSET_TYPE_SOUND,
    ASSET_TYPE_SOUND_PATCH,
    ASSET_TYPE_COL_MAP_SP,
    ASSET_TYPE_COL_MAP_MP,
    ASSET_TYPE_COMWORLD,
    ASSET_TYPE_GAMEWORLD_SP,
    ASSET_TYPE_GAMEWORLD_MP,
    ASSET_TYPE_MAP_ENTS,
    ASSET_TYPE_GFXWORLD,
    ASSET_TYPE_LIGHT_DEF,
    ASSET_TYPE_UI_MAP,
    ASSET_TYPE_FONT,
    ASSET_TYPE_MENULIST,
    ASSET_TYPE_MENU,
    ASSET_TYPE_LOCALIZE_ENTRY,
    ASSET_TYPE_WEAPON,
    ASSET_TYPE_WEAPONDEF,
    ASSET_TYPE_WEAPON_VARIANT,
    ASSET_TYPE_SNDDRIVER_GLOBALS,
    ASSET_TYPE_FX,
    ASSET_TYPE_IMPACT_FX,
    ASSET_TYPE_AITYPE,
    ASSET_TYPE_MPTYPE,
    ASSET_TYPE_MPBODY,
    ASSET_TYPE_MPHEAD,
    ASSET_TYPE_CHARACTER,
    ASSET_TYPE_XMODELALIAS,
    ASSET_TYPE_RAWFILE,
    ASSET_TYPE_STRINGTABLE,
    ASSET_TYPE_PACKINDEX,
    ASSET_TYPE_XGLOBALS,
    ASSET_TYPE_DDL,
    ASSET_TYPE_GLASSES,
    ASSET_TYPE_TEXTURELIST,
    ASSET_TYPE_EMBLEMSET
};

union T5_XAssetHeader
{
    RawFile* rawfile;
};

enum T5_scriptInstance_t
{
    SCRIPTINSTANCE_SERVER = 0,
    SCRIPTINSTANCE_CLIENT = 1,
    SCRIPT_INSTANCE_MAX = 2
};

enum DvarSetSource 
{
    DVAR_SOURCE_INTERNAL = 0,
    DVAR_SOURCE_EXTERNAL = 1,
    DVAR_SOURCE_SCRIPT = 2,
    DVAR_SOURCE_DEVGUI = 3
};

union DvarLimits 
{
    struct enumeration
    {
        int stringCount;
        const char** strings;
    };
    struct integer
    {
        int min;
        int max;
    };
    struct integer64
    {
        int64_t min;
        int64_t max;
    };
    struct value
    {
        float min;
        float max;
    };
    struct vector
    {
        float min;
        float max;
    };
};

enum dvarType_t : int
{
    DVAR_TYPE_BOOL = 0,
    DVAR_TYPE_FLOAT = 1,
    DVAR_TYPE_FLOAT_2 = 2,
    DVAR_TYPE_FLOAT_3 = 3,
    DVAR_TYPE_FLOAT_4 = 4,
    DVAR_TYPE_INT = 5,
    DVAR_TYPE_ENUM = 6,
    DVAR_TYPE_STRING = 7,
    DVAR_TYPE_COLOR = 8,
    DVAR_TYPE_INT64 = 9,
    DVAR_TYPE_LINEAR_COLOR_RGB = 10,
    DVAR_TYPE_COLOR_XYZ = 11,
    DVAR_TYPE_COUNT = 12
};

union DvarValue 
{
    bool enabled;
    int integer;
    unsigned int unsignedInt;
    float value;
    float vector[4];
    const char* string;
    byte color[4];
};

struct dvar_s 
{
    const char* name;
    const char* description;
    uint64_t hash;
    unsigned int flags;
    dvarType_t type;
    bool modified;
    bool loadedFromSaveGame;
    char _pad[6];
    DvarValue current;
    DvarValue latched;
    DvarValue reset;
    DvarValue saved;
    DvarLimits domain;
    dvar_s* hashNext;
};

struct ChecksumEntry 
{
    const char* mapname;
    const char* gametype;
    uint32_t checksum;
    uint32_t programLen;
};

/* ------------------------------------------------------------------------------------- */

using SV_SetConfigString_t = void (*)(uint32_t, const char*);
inline SV_SetConfigString_t SV_SetConfigString_s;

using PMem_AllocFromSource_NoDebug_t = void* (*)(uint32_t, uint32_t, uint32_t, int32_t);
inline PMem_AllocFromSource_NoDebug_t PMem_AllocFromSource_NoDebug_s;

using DB_FindXAssetHeader_t = void* (*)(int, void*);
inline DB_FindXAssetHeader_t DB_FindXAssetHeader_s;

using ConstructMsg_t = void (*)(int64_t, int64_t, int64_t, int64_t, const char*, void*, int);
using ConstructMsg_IW7_t = void (*)(int64_t, int64_t, int64_t, const char*, void*, int);

using HECmd_SetText_t = void (*)(int64_t, int64_t);
inline HECmd_SetText_t HECmd_SetText_s;

using Hunk_AllocateTempMemoryHigh_t = void* (*)(int size);
inline Hunk_AllocateTempMemoryHigh_t Hunk_AllocateTempMemoryHigh_s;

using Dvar_GetString_t = const char* (*)(const char* dvarName);
inline Dvar_GetString_t Dvar_GetString_s;

using Z_Uncompress_t = int (*)(unsigned char* dest, uint64_t* destLen, const unsigned char* source, uint64_t sourceLen);
inline Z_Uncompress_t Z_Uncompress_s;

using T5_DB_FindXAssetHeader_t = T5_XAssetHeader (*)(T5_XAssetType type, const char* name, bool errorIfMissing, int waitTime);
inline T5_DB_FindXAssetHeader_t T5_DB_FindXAssetHeader_s;

using Scr_ReadFile_FastFile_t  = char* (*)(T5_scriptInstance_t inst, const char* filename, const char* extFilename, char* codePos, int archive);
inline Scr_ReadFile_FastFile_t Scr_ReadFile_FastFile_s;

using Scr_GetChecksum_t = uint64_t(*)(uint32_t* out, uint32_t inst);
inline Scr_GetChecksum_t Scr_GetCheckSum_s;