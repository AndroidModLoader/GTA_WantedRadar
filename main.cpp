#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#ifdef AML32
    #include "GTASA_STRUCTS.h"
#else
    #include "GTASA_STRUCTS_210.h"
#endif

MYMOD(net.rusjj.wantedradar, Wanted Radar, 1.2, RusJJ)

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
float *ms_fTimeScale;
void *maVertices;
CWidget **m_pWidgets;

// Funcs
CWanted* (*FindPlayerWanted)(int);
void (*RwRenderStateSet)(RwRenderState, void*);
void (*DrawRect)(CRect*, CRGBA*);
inline rgba_t ProgressColor(rgba_t dest, rgba_t src, float progress)
{
    uint8_t r = dest.r;
    uint8_t g = dest.g;
    uint8_t b = dest.b;
    float invprogress = 1.0f - progress;
    return rgba_t
    {
        (uint8_t)(r + (src.r - r) * invprogress),
        (uint8_t)(g + (src.g - g) * invprogress),
        (uint8_t)(b + (src.b - b) * invprogress),
        0
    };
}
inline uint8_t ProgressAlpha(float progress)
{
    return (uint8_t)(progress * 0xFF);
}

// Hooks
DECL_HOOKv(DrawRadarGangOverlay, bool bFullMap)
{
    DrawRadarGangOverlay(bFullMap);

    if(m_pWidgets[WIDGET_RADAR])
    {
        CWanted* wanted = FindPlayerWanted(-1);
        if(!wanted || wanted->m_nWantedLevel <= 0)
        {
            RadarColorProgress -= cfgRadarStopSpeed / *ms_fTimeScale;
            if(RadarColorProgress < 0) RadarColorProgress = 0;
        }
        else
        {
            float maxprogress = wanted->m_nWantedLevel > 2 ? 0.5f : 0.3f;

            RadarColorProgress += cfgRadarStartSpeed / *ms_fTimeScale;
            if(RadarColorProgress > maxprogress) RadarColorProgress = maxprogress;
        }
        
        if(RadarColorProgress > 0)
        {
            CRect* drawRect = &m_pWidgets[WIDGET_RADAR]->screenRect;
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
            DrawRect(drawRect, rgbaColor);
        }
    }
}

// int main!
extern "C" void OnModLoad()
{
    logger->SetTag("Wanted Radar");
    if((pGame = aml->GetLib("libGTASA.so")))
    {
        hGame = aml->GetLibHandle("libGTASA.so");
        cfg = new Config("WantedRadar.SA");

      #ifdef AML32
        HOOKBLX(DrawRadarGangOverlay, pGame + 0x43E730 + 0x1);
      #else
        HOOKBL(DrawRadarGangOverlay, pGame + 0x523BD0);
      #endif

        loadedGame = eGameLoaded::GL_SA;
    }
    else
    {
        logger->Info("Unsupported game moment");
        return; // Nuh-uh
    }

    SET_TO(m_snTimeInMilliseconds, aml->GetSym(hGame, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(ms_fTimeScale, aml->GetSym(hGame, "_ZN6CTimer13ms_fTimeScaleE"));
    SET_TO(maVertices, aml->GetSym(hGame, "_ZN9CSprite2d10maVerticesE"));
    SET_TO(m_pWidgets, *(void**)(pGame + BYBIT(0x67947C, 0x850910)));

    SET_TO(FindPlayerWanted, aml->GetSym(hGame, "_Z16FindPlayerWantedi"));
    SET_TO(RwRenderStateSet, aml->GetSym(hGame, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(DrawRect, aml->GetSym(hGame, "_ZN9CSprite2d8DrawRectERK5CRectRK5CRGBA"));

    cfgRed =  cfg->GetColor("RedColor", cfgRed);
    cfgBlue = cfg->GetColor("BlueColor", cfgBlue);
    cfgTimeToSwitch = cfg->GetInt("TimeToSwitch", cfgTimeToSwitch);
    cfgRadarStartSpeed = cfg->GetFloat("RadarStartSpeed", cfgRadarStartSpeed);
    cfgRadarStopSpeed = cfg->GetFloat("RadarStopSpeed", cfgRadarStopSpeed);
}