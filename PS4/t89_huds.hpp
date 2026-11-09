#pragma once

#pragma region Constants
#define SCREEN_W_DEFAULT   1920.0f
#define SCREEN_H_DEFAULT   1080.0f
#define MAX_DRAWS  128
#define MAX_ELEMS  128
#define MAX_TEXT   256
#define DRAW_RECT  0
#define DRAW_TEXT  1
#define H_HUDNEWELEM        0x3CA4F640
#define H_HUDCREATETEXT     0x27AAF887
#define H_HUDSETSHADER      0x7B61E0F9
#define H_HUDSETTEXT        0xB173783C
#define H_HUDSETPOINT       0x5691AF51
#define H_HUDSETPARENT      0x51BCEB63
#define H_HUDSETCOLOR       0xE1180851
#define H_HUDSETSORT        0x949936E9
#define H_HUDSETHIDDEN      0x41193716
#define H_HUDDESTROY        0x7928078F
#define H_HUDLOG            0x50D51742
#define H_HUDCLEARALL       0x61DE9DE8
#define H_HUDSETSCALE       0xE75C566A
#define CLASS_NUM_ENTITY   0
#define CLASS_NUM_HUDELEM  1
#define SCRVAR_INDEX_MAX 0x7FFFF
#define H_NEWCLIENTHUDELEM  0x2C66E134
#define H_CREATEFONTSTRING  0x16D39751
#define H_SETPOINT          0x16461F87
#define H_SETPARENT         0x84B77663
#define H_SETSHADER         0x89FBE184
#define H_SETTEXT           0xEC9B7A6B   
#define H_DESTROY           0xD4C79008
#define H_SETCOLOR          0x734BD101
#define H_SETALPHA          0x8E0E48ED
#define H_SETSORT           0x06CF9C77
#define H_SETHIDDEN         0x59807FEE
#define H_FADEOVERTIME             0x4589E67A
#define H_MOVEOVERTIME             0x110229AF
#define H_SCALEOVERTIME            0x3C03A473
#define H_CHANGEFONTSCALEOVERTIME  0x34F32D99
#define H_SETALIGNX                0x427D1643
#define H_SETALIGNY                0x50C2B2CE
#define H_SETHORZALIGN             0xBF0BEF40
#define H_SETVERTALIGN             0xE481E9A1

#define GSC_INST 0
#define VAL_TYPE_UNDEFINED 0
#define VAL_TYPE_STRING    2
#define VAL_TYPE_VECTOR    3
#define VAL_TYPE_FLOAT     5
#define VAL_TYPE_INT       6
#define SCR_WHERE(name) ScrWhere scrWhere_(name)
#define SCR_GETTER(name, index)  g_scrGetter = (name); g_scrArg = (index)
#define SCR_GETTER_DONE()        g_scrGetter = nullptr; g_scrArg = -1
#define STR_CACHE_MAX  64
#define STR_CACHE_LEN  32
#define P_TOPLEFT      0
#define P_TOP          1
#define P_TOPRIGHT     2
#define P_LEFT         3
#define P_CENTER       4
#define P_RIGHT        5
#define P_BOTTOMLEFT   6
#define P_BOTTOM       7
#define P_BOTTOMRIGHT  8
#define VIRTUAL_W 640.0f
#define VIRTUAL_H 480.0f
#define TEXT_LINE_H  22.0f

#define FONT_VIRT_PER_POINT  12.0f

inline float g_textBaselineNudge = 0.0f;
#define HXA_LEFT    0
#define HXA_CENTER  1
#define HXA_RIGHT   2
#define HYA_TOP     0
#define HYA_MIDDLE  1
#define HYA_BOTTOM  2
#define HHA_SUBLEFT         0
#define HHA_LEFT            1
#define HHA_CENTER          2
#define HHA_RIGHT           3
#define HHA_FULLSCREEN      4
#define HHA_NOSCALE         5
#define HHA_ALIGNTO640      6
#define HHA_CENTER_SAFEAREA 7
#define HHA_USER_LEFT       8
#define HHA_USER_CENTER     9
#define HHA_USER_RIGHT      10
#define HHA_COUNT           11
#define HVA_SUBTOP          0
#define HVA_TOP             1
#define HVA_MIDDLE          2
#define HVA_BOTTOM          3
#define HVA_FULLSCREEN      4
#define HVA_NOSCALE         5
#define HVA_ALIGNTO480      6
#define HVA_CENTER_SAFEAREA 7
#define HVA_USER_TOP        8
#define HVA_USER_CENTER     9
#define HVA_USER_BOTTOM     10
#define HVA_COUNT           11
#define SYNCF_X          0
#define SYNCF_Y          1
#define SYNCF_WIDTH      2
#define SYNCF_HEIGHT     3
#define SYNCF_XOFFSET    4
#define SYNCF_YOFFSET    5
#define SYNCF_FONTSCALE  6
#define SYNCF_ALIGNX     7
#define SYNCF_ALIGNY     8
#define SYNCF_HORZALIGN  9
#define SYNCF_VERTALIGN  10
#define SYNCF_COUNT      11
#define ANIM_FPS 60.0f
#define HUDF_COLOR      0x14F72AD2ULL
#define HUDF_ALPHA      0x6B0D2DAAULL
#define HUDF_SORT       0x9F6C4424ULL
#define HUDF_HIDDEN     0x8F490AE1ULL
#define HUDF_X          0x4467B457ULL
#define HUDF_Y          0xC45D343CULL
#define HUDF_WIDTH      0x7848D0D1ULL
#define HUDF_HEIGHT     0x2F32BB4EULL
#define HUDF_ALIGNX     0x4F4B29E5ULL
#define HUDF_ALIGNY     0xA0B04CB2ULL
#define HUDF_HORZALIGN  0xA0CCBA92ULL
#define HUDF_VERTALIGN  0x3DE0F65AULL
#define HUDF_FONTSCALE  0x2B509879ULL
#define HUDF_LABEL          0x8274726FULL
#define HUDF_ARCHIVED       0x6809E678ULL
#define HUDF_FOREGROUND     0x1A82EE61ULL
#define HUDF_ELEMTYPE       0x0BC40290ULL
#define HUDF_CHILDREN       0x3C312BEDULL
#define HUDF_SHADER         0xCB52416DULL
#define HUDF_RELATIVE       0x38FD7EBBULL
#define HUDF_ALIGN          0x84246D07ULL
#define HUDF_XOFFSET        0x584DC12CULL
#define HUDF_YOFFSET        0xE9389928ULL
#define HUDF_HIDEWHENINMENU 0x34AD5F0EULL
#define SYNC_ALPHA 1
#define SYNC_SORT 2
#define SYNC_HIDDEN 4
#define SYNC_COLOR 8
#define OF_LOG_BUDGET 12
#define ERR_SITE_MAX    32
#define ERR_SITE_FIRST  4
#define ERR_SITE_EVERY  256
#define SCR_ERR_INDEX_ON_UNDEF 0xCFEE94F2ull
#pragma endregion

#pragma region Types and Globals
float T89_ScreenWidth();

float T89_ScreenHeight();

struct vec2_t
{
    float x;
    float y;
};

struct vec3_t
{
    float x, y, z;
};

struct DrawItem
{
    int   type;
    float x, y, w, h;
    float scale;
    int   font;
    void* material;
    float color[4];
    char  text[MAX_TEXT];
    int   layer;
    int   seq;
    int   id;
};

struct t8_builtin_t
{
    uint32_t hash;
    void* func;
    int      minArgs;
    int      maxArgs;
};

using GetFunction_t = void* (*)(int nameHash, int* type, int* minArgs, int* maxArgs);

inline GetFunction_t Scr_GetFunction_s = nullptr;

inline GetFunction_t Scr_GetMethod_s = nullptr;

inline GetFunction_t CScr_GetFunction_s = nullptr;

inline GetFunction_t CScr_GetMethod_s = nullptr;

using R_AddCmdDrawQuadPicW_t = void(*)(const vec2_t* verts, float w, const float* color, uint64_t material, const void* image);

using R_AddCmdDrawText_t = void(*)(const char* text, int maxChars, uintptr_t font, float x, float y, float xScale, float yScale, float rotation, const float* color, int style);

using R_TextWidth_t = int(*)(const char* text, int maxChars, uint64_t font, int scale);

inline R_TextWidth_t R_TextWidth_s = nullptr;

using R_TextWidthT9_t = float(*)(const char* text, int maxChars, uintptr_t descriptor, float scale);
inline R_TextWidthT9_t R_TextWidthT9_s = nullptr;

struct ObjFieldRet { uint64_t a; uint64_t b; };

using Scr_GetObjectField_t = ObjFieldRet(*)(int inst, int classnum, uint64_t entnum,
    int clientNum, int fieldIndex);

using Scr_SetObjectField_t = bool(*)(int inst, int classnum, uint64_t entnum,
    int clientNum, int fieldIndex, uint64_t value);

inline Scr_GetObjectField_t Scr_GetObjectField_s = nullptr;   

inline Scr_SetObjectField_t Scr_SetObjectField_s = nullptr;   

using ScrVm_AddEntity_t = void(*)(int inst, const uint32_t* entnum, int classnum);

inline ScrVm_AddEntity_t ScrVm_AddEntity_s = nullptr;         

using CmdAlloc_t = void* (*)(int type, int align, int size);

using BuildQuad_t = void(*)(void* out96, int flags, float x0, float y0, float x1, float y1);

using Renderer_t = int(*)();

using ScrVm_GetString_t = const char* (*)(int inst, int index);

using ScrVm_GetInt_t = int64_t(*)(int inst, int index);

using ScrVm_GetVector_t = void(*)(int inst, int index, float* out, const float* def);

using ScrVm_AddInt_t = void(*)(int inst, int64_t value);

using ScrVm_AddStruct_t = void(*)(int inst);

inline R_AddCmdDrawQuadPicW_t R_AddCmdDrawQuadPicW_s = nullptr;

inline R_AddCmdDrawText_t     R_AddCmdDrawText_s = nullptr;

inline CmdAlloc_t             CmdAlloc_s = nullptr;

inline BuildQuad_t            BuildQuad_s = nullptr;

inline Renderer_t             Renderer_s = nullptr;

inline ScrVm_GetString_t      ScrVm_GetString_s = nullptr;

inline ScrVm_GetInt_t         ScrVm_GetInt_s = nullptr;

inline ScrVm_GetVector_t      ScrVm_GetVector_s = nullptr;

inline ScrVm_AddInt_t         ScrVm_AddInt_s = nullptr;

inline ScrVm_AddStruct_t      ScrVm_AddStruct_s = nullptr;

inline uintptr_t              g_vmBlock = 0;

int   ScrVm_GetNumParam(int inst);

float ScrVm_GetFloat(int inst, int index);

#define FONT_COUNT   10
#define FONT_DEFAULT 3

uint64_t  T89_AssetHash(const char* name);
void*     T89_FindMaterial(const char* name);
void*     T89_WhiteMaterial();

uintptr_t T89_FontRaw(int idx);
int       T89_FontEffective(int idx);
uintptr_t T89_FontAt(int idx);
int       T89_FontPx(int idx);
int       FontIndexFromName(const char* name, int def);
const char* FontNameOf(int idx);
float     FontScaleFactorFor(int idx);

uintptr_t T89_Font();
float T89_FontPixelHeight();

void T89_DrawRectangle(float x, float y, float w, float h, const float color[4], void* material);

void T89_DrawText(const char* text, float x, float y, float scale, const float color[4], int font);

int  T89_Create();

bool T89_Apply(int id, int type, float x, float y, float w, float h,
    float scale, int layer, const float color[4], const char* text, int font, void* material);

void T89_Remove(int id);

void T89_Clear();

void T89_ClearAll();

void T89_FlushDraws();

void* Scr_GetFunction_h(int nameHash, int* type, int* minArgs, int* maxArgs);

void* Scr_GetMethod_h(int nameHash, int* type, int* minArgs, int* maxArgs);

void* CScr_GetFunction_h(int nameHash, int* type, int* minArgs, int* maxArgs);

void* CScr_GetMethod_h(int nameHash, int* type, int* minArgs, int* maxArgs);

int   Renderer_h();

ObjFieldRet Scr_GetObjectField_h(int inst, int classnum, uint64_t entnum,
    int clientNum, int fieldIndex);

bool Scr_SetObjectField_h(int inst, int classnum, uint64_t entnum,
    int clientNum, int fieldIndex, uint64_t value);

using ScrVar_RemoveReference_t = void(*)(int inst, int varIndex);

inline ScrVar_RemoveReference_t ScrVar_RemoveReference_s = nullptr;   

void ScrVar_RemoveReference_h(int inst, int varIndex);

using ScrVm_Error_t = void(*)(uint32_t code, int inst, int terminal);

inline ScrVm_Error_t ScrVm_Error_s = nullptr;                 

void ScrVm_Error_h(uint32_t code, int inst, int terminal);

using ScrMethod_t = void(*)(int inst, uint64_t entnum, uint64_t classnum);

using ScrVar_EntityBridge_t = int(*)(int inst, uint64_t key, int classnum, int clientNum);

inline ScrVar_EntityBridge_t ScrVar_EntityBridge_s = nullptr;   

#define VARHASH_T8  0
#define VARHASH_T9  1

struct VarLayout
{
    uint32_t instStride;
    bool     sharded;
    uint32_t shardNodeOff;
    uint32_t bucketOff;
    int      hash;
    uint32_t shardArrayOff;
    uint32_t nodeParentOff;
    uint32_t nodeNextOff;
    bool     probed;
};

inline VarLayout g_var = { 0x28, false, 0, 0x00, VARHASH_T8, 0, 0x14, 0x18, false };

inline uintptr_t g_varGlobals = 0;                              

struct VmLayout
{
    bool     pubIsPointer;
    uint64_t stride;
    uint64_t tableOff;
    uint32_t topOff;
    uint32_t numParamOff;
    uint32_t stackBeg;
    uint32_t stackEnd;
};

inline VmLayout g_vm =
{
    false,
    0x16868,
    0,
    0xE048,
    0x16860,
    0xE850,
    0x16850,
};
using ScrStr_Follow_t = const char* (*)(const char* body);
inline ScrStr_Follow_t ScrStr_Follow_s = nullptr;
inline uintptr_t g_strTable = 0;
inline uint32_t  g_strStride = 20;
inline uint32_t  g_strTextOff = 16;
inline uint32_t  g_strMaxIndex = 0x3FFFFu;
inline uint32_t g_varBucketMask = 0x7FFFFu;
inline const char* g_scrWhere = nullptr;
inline const char* g_scrGetter = nullptr;
inline int         g_scrArg = -1;
struct ScrWhere
{
    const char* prev;

    explicit ScrWhere(const char* what) : prev(g_scrWhere) { g_scrWhere = what; }
    ~ScrWhere() { g_scrWhere = prev; g_scrGetter = nullptr; g_scrArg = -1; }
};

struct StrCacheEntry
{
    uint32_t id;
    char     text[STR_CACHE_LEN];
};

inline StrCacheEntry g_strCache[STR_CACHE_MAX];
inline int           g_strCacheCount = 0;
inline DrawItem     g_draws[MAX_DRAWS];
inline int          g_drawCount = 0;
inline volatile int g_drawLock = 0;
struct HudElem
{
    int      draw;
    int      isText;

    float    x, y;

    int      alignX, alignY;
    int      horzAlign, vertAlign;

    float    xOffset, yOffset;

    int      foreground;
    int      archived;
    int      hideWhenInMenu;

    float    w, h;
    float    scale;
    float    fontScaleRaw;
    int      font;
    void*    material;
    int      sort;
    float    color[4];
    char     text[MAX_TEXT];

    int      parent;
    int      hidden;

    uint64_t lastField[SYNCF_COUNT];
    uint32_t lastSeen;

    int      varId;

    int      animAlphaLeft,  animAlphaTotal;   float animAlphaFrom, animAlphaTo;
    int      animMoveLeft,   animMoveTotal;    float animMoveFromX, animMoveFromY,
                                                     animMoveToX,   animMoveToY;
    int      animScaleLeft,  animScaleTotal;   float animScaleFromW, animScaleFromH,
                                                     animScaleToW,   animScaleToH;
    int      animFontLeft,   animFontTotal;    float animFontFrom,  animFontTo;
};

inline HudElem      g_elems[MAX_ELEMS];
inline int          g_elemCount = 0;
inline volatile int g_elemLock = 0;
inline volatile int g_hudResetPending = 0;
inline float s_screenWidth = SCREEN_W_DEFAULT;
inline float s_screenHeight = SCREEN_H_DEFAULT;
inline bool  s_resCached = false;
inline const bool g_useEntityBridge = true;
inline volatile int g_ofGetLogs = 0;
inline volatile int g_ofSetLogs = 0;
inline const bool g_scrNonFatal = true;
struct ErrSite
{
    uint64_t  code;
    uintptr_t site;
    uint32_t  hits;
};

inline ErrSite      g_errSites[ERR_SITE_MAX];
inline volatile int g_errSiteCount = 0;
inline volatile int g_errSiteLock = 0;
inline volatile int g_rrBadLogs = 0;
inline volatile int g_rrSiteLogs = 0;
inline const bool g_scrUndefTolerant = false;
#pragma endregion

#pragma region Forward Declarations
void DumpCode(const char* name, uintptr_t imageAddr, int before, int after);
void T89_Huds_Init();
char* ScrVm_Block(int inst);
char* ScrVm_Top(int inst);
int ScrVm_GetNumParam(int inst);
bool ScrVm_SlotOk(int inst, const char* slot);
float ScrVm_GetFloat(int inst, int index);
int ScrVm_ArgType(int inst, int index);
uint64_t ScrVm_ArgRaw(int inst, int index);
void StrCache_Add(uint32_t id, const char* text);
void StrCache_Miss(const char* field, uint64_t raw);
const char* ScrStr_Resolve(uint32_t id);
const char* StrCache_Find(uint64_t raw);
int GetIntArg(int inst, int index, int def);
float GetFloatArg(int inst, int index, float def);
const char* GetStringArg(int inst, int index, const char* def);
const char* GetTextArg(int inst, int index, char* buf, int bufSize);
bool PointNameEq(const char* a, const char* b);

struct ScrPlace
{
    float viewportBase[2];
    float viewportSize[2];
    float viewableMin[2];
    float viewableMax[2];
    float tweakableMin[2];
    float tweakableMax[2];
    float virtualToReal[2];
    float virtualToFull[2];
    float realToVirtual[2];
    float subScreen[2];
    float splitScale;
};

#define SPF_VIRTUAL_TO_REAL  0
#define SPF_VIRTUAL_TO_FULL  2
#define SPF_REAL_TO_VIRTUAL  4
#define SPF_VIEWPORT_BASE    14
#define SPF_VIEWPORT_SIZE    16
#define SPF_VIEWABLE_MIN     18
#define SPF_VIEWABLE_MAX     20
#define SPF_TWEAKABLE_MIN    22
#define SPF_TWEAKABLE_MAX    24
#define SPF_SUBSCREEN        26
#define SPF_SPLIT_SCALE      28

inline uint32_t  g_frameTick      = 0;
inline uintptr_t g_scrPlaceArray  = 0;
inline uint32_t  g_scrPlaceStride = 0x74;
inline uint32_t  g_scrPlaceCount  = 12;

void ScrPlace_Update(ScrPlace* sp);
const ScrPlace* ScrPlace_Get();
float ScrPlace_ApplyX(float x, int horzAlign);
float ScrPlace_ApplyY(float y, int vertAlign);
float ScrPlace_ApplyW(float w, int horzAlign);
float ScrPlace_ApplyH(float h, int vertAlign);

float VirtScale();
int PointFromName(const char* s, int def);
int GetPointArg(int inst, int index, int def);
uintptr_t T89_Font();
void T89_DrawRectangle(float x, float y, float w, float h, const float color[4], void* material);
void T89_DrawText(const char* text, float x, float y, float scale, const float color[4], int font);
void DrawLock();
void DrawUnlock();
DrawItem* Draw_Find(int id);
int T89_Create();
bool T89_Apply(int id, int type, float x, float y, float w, float h, float scale, int layer, const float color[4], const char* text, int font, void* material);
void T89_Remove(int id);
void T89_Clear();
void T89_FlushDraws();
int PointClamp(int p);
float HorzAlignFrac(int ha);
float VertAlignFrac(int va);
float SelfAlignFrac(int a);
int HorzAlignClamp(int v);
int VertAlignClamp(int v);
int SelfAlignClamp(int v);
void ElemLock();
void ElemUnlock();
HudElem* Elem_Find(int handle);
void Elem_ApplyPointPair(HudElem* e, int align, int relative);
HudElem* Elem_New();
float GlyphWidth(char c);
float Elem_FontHeight(HudElem* e);
void Elem_Extent(HudElem* e, float* w, float* h);
void Elem_Resolve(HudElem* e, int depth, float* outX, float* outY, float* outW, float* outH);
void Elem_Push(HudElem* e);
void Elem_Free(int handle);
void T89_ClearAll();
void CopyText(char* dst, const char* src);
void ResolveScreen();
float T89_ScreenWidth();
float T89_ScreenHeight();
void Scr_NewClientHudElem(int inst);
void Scr_CreateFontString(int inst);
void Scr_SetShader(int inst);
void Scr_SetText(int inst);
void Scr_SetPoint(int inst);
void Scr_SetParent(int inst);
void Scr_SetColor(int inst);
void Scr_SetSort(int inst);
void Scr_SetScale(int inst);
void Scr_SetHidden(int inst);
void Scr_Destroy(int inst);
void Scr_ClearAllHuds(int);
bool VarField(int inst, int parentVarId, uint64_t canonId, uint64_t* outVal, int* outType);
bool VarFieldFloat(int inst, int varId, uint64_t canonId, float* out);
int Elem_SyncFromObject(int inst, HudElem* e);
bool Elem_FieldChanged(HudElem* e, int slot, uint64_t raw);
void Elem_SyncLayout(int inst, HudElem* e);
void Elem_SyncAllLayout(int inst);
bool MethodElem(uint64_t entnum, uint64_t classnum, int* outId);
void MintElemStruct(int inst, int handle);
void Scr_NewClientHudElem_Obj(int inst);
void Scr_CreateFontString_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetPoint_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetShader_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetText_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_Destroy_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetColor_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetAlpha_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetSort_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetHidden_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetParent_M(int inst, uint64_t entnum, uint64_t classnum);
void Elem_TickAnims();
int AnimFrames(float seconds);
void Scr_FadeOverTime_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_MoveOverTime_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_ScaleOverTime_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_ChangeFontScaleOverTime_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetAlignX_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetAlignY_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetHorzAlign_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_SetVertAlign_M(int inst, uint64_t entnum, uint64_t classnum);
void Scr_HudLog(int inst);
void* ResolveCustom(const char* vm, const char* kind, t8_builtin_t* table, int count, GetFunction_t fallback, bool preferEngine, int nameHash, int* type, int* minArgs, int* maxArgs);
void* Scr_GetFunction_h(int nameHash, int* type, int* minArgs, int* maxArgs);
void* Scr_GetMethod_h(int nameHash, int* type, int* minArgs, int* maxArgs);
void* CScr_GetFunction_h(int nameHash, int* type, int* minArgs, int* maxArgs);
void* CScr_GetMethod_h(int nameHash, int* type, int* minArgs, int* maxArgs);
void ScrVar_RemoveReference_h(int inst, int varIndex);
bool Scr_YieldUndefined(int inst);
void ScrVm_Error_h(uint32_t code, int inst, int terminal);
ObjFieldRet Scr_GetObjectField_h(int inst, int classnum, uint64_t entnum, int clientNum, int fieldIndex);
bool Scr_SetObjectField_h(int inst, int classnum, uint64_t entnum, int clientNum, int fieldIndex, uint64_t value);
int Renderer_h();
#pragma endregion

#pragma region Dispatch Tables
inline t8_builtin_t g_t8_builtins[] =
{
    { H_HUDNEWELEM,      (void*)Scr_NewClientHudElem, 0, 1 },
    { H_HUDCREATETEXT,   (void*)Scr_CreateFontString, 0, 1 },
    { H_HUDSETSHADER,    (void*)Scr_SetShader,        0, 3 },
    { H_HUDSETTEXT,      (void*)Scr_SetText,          0, 2 },
    { H_HUDSETPOINT,     (void*)Scr_SetPoint,         0, 5 },
    { H_HUDSETPARENT,    (void*)Scr_SetParent,        0, 2 },
    { H_HUDSETCOLOR,     (void*)Scr_SetColor,         0, 3 },
    { H_HUDSETSORT,      (void*)Scr_SetSort,          0, 2 },
    { H_HUDSETHIDDEN,    (void*)Scr_SetHidden,        0, 2 },
    { H_HUDSETSCALE,     (void*)Scr_SetScale,         0, 2 },
    { H_HUDDESTROY,      (void*)Scr_Destroy,          0, 1 },
    { H_HUDLOG,          (void*)Scr_HudLog,           0, 1 },
    { H_HUDCLEARALL,     (void*)Scr_ClearAllHuds,     0, 0 },

    { H_NEWCLIENTHUDELEM,(void*)Scr_NewClientHudElem_Obj, 0, 1 },

};

inline t8_builtin_t g_t8_methods[] =
{

    { H_CREATEFONTSTRING,(void*)Scr_CreateFontString_M, 0, 2 },
    { H_SETPOINT,        (void*)Scr_SetPoint_M,         0, 4 },
    { H_SETSHADER,       (void*)Scr_SetShader_M,        0, 3 },
    { H_SETTEXT,         (void*)Scr_SetText_M,          0, 1 },
    { H_SETPARENT,       (void*)Scr_SetParent_M,        0, 1 },

    { H_DESTROY,         (void*)Scr_Destroy_M,          0, 0 },
    { H_SETCOLOR,        (void*)Scr_SetColor_M,         0, 3 },
    { H_SETALPHA,        (void*)Scr_SetAlpha_M,         0, 1 },
    { H_SETSORT,         (void*)Scr_SetSort_M,          0, 1 },
    { H_SETHIDDEN,       (void*)Scr_SetHidden_M,        0, 1 },

    { H_FADEOVERTIME,            (void*)Scr_FadeOverTime_M,            0, 2 },
    { H_MOVEOVERTIME,            (void*)Scr_MoveOverTime_M,            0, 3 },
    { H_SCALEOVERTIME,           (void*)Scr_ScaleOverTime_M,           0, 3 },
    { H_CHANGEFONTSCALEOVERTIME, (void*)Scr_ChangeFontScaleOverTime_M, 0, 2 },
    { H_SETALIGNX,               (void*)Scr_SetAlignX_M,               0, 1 },
    { H_SETALIGNY,               (void*)Scr_SetAlignY_M,               0, 1 },
    { H_SETHORZALIGN,            (void*)Scr_SetHorzAlign_M,            0, 1 },
    { H_SETVERTALIGN,            (void*)Scr_SetVertAlign_M,            0, 1 },
};
inline const int g_t8_builtinCount = sizeof(g_t8_builtins) / sizeof(g_t8_builtins[0]);
inline const int g_t8_methodCount = sizeof(g_t8_methods) / sizeof(g_t8_methods[0]);

#pragma endregion

#pragma region Public API

void T89_Huds_Init();

#pragma endregion
