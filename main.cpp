#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
#else
    #include "GTASA_STRUCTS_210.h"
#endif

MYMOD(net.rusjj.wantedradar, Wanted Radar, 1.1.1, RusJJ)

enum eGameLoaded
{
    GL_Unknown = 0,
    GL_SA,
    GL_VC,

    GL_GAMES
};
eGameLoaded loadedGame = eGameLoaded::GL_Unknown;

// Savings
uintptr_t pGame;
void* hGame;
Config* cfg;
float RadarColorProgress = 0.0f;

int cfgTimeToSwitch = 800;
float cfgRadarStartSpeed = 0.001f;
float cfgRadarStopSpeed = 0.003f;
rgba_t cfgRed = { 225, 30, 40, 0 }, cfgBlue = { 30, 40, 225, 0 };
rgba_t clrWhite = { 255, 255, 255, 255 };

// Vars
uint32_t *m_snTimeInMilliseconds;
float *ms_fTimeScale, *NearScreenZ;
void *maVertices;
CWidget **m_pWidgets;

// Funcs
CWanted* (*FindPlayerWanted)(int);
void (*RwRenderStateSet)(RwRenderState, void*);
void (*DrawAreaOnRadar)(CRect*, CRGBA*, bool);
void (*DrawRadarMask)();

inline uint8_t ProgressAlpha(float progress)
{
    return (uint8_t)(progress * 0x7F);
}

// Hooks
DECL_HOOKv(DrawRadarGangOverlay, bool bFullMap)
{
    DrawRadarGangOverlay(bFullMap);

    if(bFullMap) return; // We dont need it on a menu map.

    CWidget* radarWidget = m_pWidgets[WIDGET_RADAR];
    if(!radarWidget || !radarWidget->color.a) return; // No widget or its invisible

    CWanted* wanted = FindPlayerWanted(-1);
    if(!wanted || wanted->m_nWantedLevel <= 0)
    {
        RadarColorProgress -= cfgRadarStopSpeed / *ms_fTimeScale;
        if(RadarColorProgress < 0) RadarColorProgress = 0;
    }
    else
    {
        float maxprogress = wanted->m_nWantedLevel > 2 ? 1.0f : 0.6f;

        RadarColorProgress += cfgRadarStartSpeed / *ms_fTimeScale;
        if(RadarColorProgress > maxprogress) RadarColorProgress = maxprogress;
    }
    
    if(RadarColorProgress > 0)
    {
        CRect drawRect;
        drawRect.bottom = drawRect.right = 30000.0f;
        drawRect.top = drawRect.left = -30000.0f;
        CRGBA* rgbaColor;
        if((*m_snTimeInMilliseconds / cfgTimeToSwitch) % 2 == 0)
        {
            rgbaColor = (CRGBA*)&cfgRed;
        }
        else
        {
            rgbaColor = (CRGBA*)&cfgBlue;
        }
        rgbaColor->a = ProgressAlpha(RadarColorProgress);
        rgbaColor->a = ((int)rgbaColor->a * (int)radarWidget->color.a) >> 8;

        DrawAreaOnRadar(&drawRect, rgbaColor, bFullMap);
    }
}

DECL_HOOKv(ReInitGameObjectVariables)
{
    ReInitGameObjectVariables();

    RadarColorProgress = 0.0f;
}

// int main!
extern "C" void OnModLoad()
{
    logger->SetTag("Wanted Radar");
    if((pGame = aml->GetLib("libGTASA.so")))
    {
        hGame = aml->GetLibHandle("libGTASA.so");
        cfg = new Config("WantedRadar.SA");

        HOOK(DrawRadarGangOverlay, aml->GetSym(hGame, "_ZN6CRadar20DrawRadarGangOverlayEb"));
        HOOK(ReInitGameObjectVariables, aml->GetSym(hGame, "_ZN5CGame25ReInitGameObjectVariablesEv"));

        loadedGame = eGameLoaded::GL_SA;
    }
    else
    {
        logger->Info("Unsupported game moment");
        return; // Nuh-uh
    }

    SET_TO(m_snTimeInMilliseconds, aml->GetSym(hGame, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(ms_fTimeScale, aml->GetSym(hGame, "_ZN6CTimer13ms_fTimeScaleE"));
    SET_TO(NearScreenZ, aml->GetSym(hGame, "_ZN9CSprite2d11NearScreenZE"));
    SET_TO(maVertices, aml->GetSym(hGame, "_ZN9CSprite2d10maVerticesE"));
    SET_TO(m_pWidgets, *(void**)(pGame + BYBIT(0x67947C, 0x850910)));

    SET_TO(FindPlayerWanted, aml->GetSym(hGame, "_Z16FindPlayerWantedi"));
    SET_TO(RwRenderStateSet, aml->GetSym(hGame, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(DrawAreaOnRadar, aml->GetSym(hGame, "_ZN6CRadar15DrawAreaOnRadarERK5CRectRK5CRGBAb"));
    SET_TO(DrawRadarMask, aml->GetSym(hGame, "_ZN6CRadar13DrawRadarMaskEv"));

    cfgRed =  cfg->GetColor("RedColor", cfgRed);
    cfgBlue = cfg->GetColor("BlueColor", cfgBlue);
    cfgTimeToSwitch = cfg->GetInt("TimeToSwitch", cfgTimeToSwitch);
    cfgRadarStartSpeed = cfg->GetFloat("RadarStartSpeed", cfgRadarStartSpeed);
    cfgRadarStopSpeed = cfg->GetFloat("RadarStopSpeed", cfgRadarStopSpeed);
}