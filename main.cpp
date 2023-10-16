#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include "GTASA_STRUCTS.h"

MYMOD(net.rusjj.wantedradar, Wanted Radar, 1.0, RusJJ)

// Savings
uintptr_t pGame;
void* hGame;
Config* cfg;
float RadarColorProgress = 0.0f;

int cfgTimeToSwitch = 800;
float cfgRadarStartSpeed = 0.001f;
float cfgRadarStopSpeed = 0.003f;
rgba_t cfgRed = { 225, 30, 40, 0 }, cfgBlue = { 30, 40, 225, 0 };

// Vars
uint32_t *m_snTimeInMilliseconds;
float *ms_fTimeScale;

// Funcs
CWanted* (*FindPlayerWanted)(int);
inline rgba_t ProgressColor(rgba_t src, float progress)
{
    uint8_t r = src.r;
    uint8_t g = src.g;
    uint8_t b = src.b;
    float invprogress = 1.0f - progress;
    return rgba_t
    {
        (uint8_t)(r + (255 - r) * invprogress),
        (uint8_t)(g + (255 - g) * invprogress),
        (uint8_t)(b + (255 - b) * invprogress),
        0
    };
}

// Hooks
uintptr_t DrawRadarSection_BackTo;
extern "C" void DrawRadarSection_Patch(CRGBA* self, uint8_t alpha)
{
    CWanted* wanted;
    if((wanted = FindPlayerWanted(-1)) && wanted->m_nWantedLevel > 0)
    {
        float maxprogress = wanted->m_nWantedLevel > 2 ? 1.0f : 0.6f;

        RadarColorProgress += cfgRadarStartSpeed / *ms_fTimeScale;
        if(RadarColorProgress > maxprogress) RadarColorProgress = maxprogress;
    }
    else
    {
        RadarColorProgress -= cfgRadarStopSpeed / *ms_fTimeScale;
        if(RadarColorProgress < 0) RadarColorProgress = 0;
    }

    if(RadarColorProgress > 0)
    {
        rgba_t clr;
        if((*m_snTimeInMilliseconds / cfgTimeToSwitch) % 2 == 0)
        {
            clr = ProgressColor(cfgRed, RadarColorProgress);
        }
        else
        {
            clr = ProgressColor(cfgBlue, RadarColorProgress);
        }
        *self = { clr.r, clr.g, clr.b, alpha };
    }
    else
    {
        *self = { 255, 255, 255, alpha };
    }
}
__attribute__((optnone)) __attribute__((naked)) void DrawRadarSection_InjectSA(void)
{
    asm volatile(
        "UXTB.W R0, R8\n"
        "STR R0, [SP]\n"

        //"PUSH {R0-R11}\n"
        "ADD R0, SP, #0x8\n"
        "MOV R1, R8\n"
        "MOV R4, R0\n"
        "BL DrawRadarSection_Patch\n");
    asm volatile(
        "MOV R12, %0\n"
        //"POP {R0-R11}\n"
        "BX R12\n"
    :: "r" (DrawRadarSection_BackTo));
}

// int main!
extern "C" void OnModLoad()
{
    logger->SetTag("Wanted Radar");
    if((pGame = aml->GetLib("libGTASA.so")))
    {
        hGame = aml->GetLibHandle("libGTASA.so");
        cfg = new Config("WantedRadar.SA");
        DrawRadarSection_BackTo = pGame + 0x443ABE + 0x1;
        aml->Redirect(pGame + 0x443AAA + 0x1, (uintptr_t)DrawRadarSection_InjectSA);
    }
    else
    {
        logger->Info("Unsupported game moment");
        return; // Nuh-uh
    }

    SET_TO(m_snTimeInMilliseconds, aml->GetSym(hGame, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(ms_fTimeScale, aml->GetSym(hGame, "_ZN6CTimer13ms_fTimeScaleE"));
    SET_TO(FindPlayerWanted, aml->GetSym(hGame, "_Z16FindPlayerWantedi"));

    cfgRed =  cfg->GetColor("RedColor", cfgRed);
    cfgBlue = cfg->GetColor("BlueColor", cfgBlue);
    cfgTimeToSwitch = cfg->GetInt("TimeToSwitch", cfgTimeToSwitch);
    cfgRadarStartSpeed = cfg->GetFloat("RadarStartSpeed", cfgRadarStartSpeed);
    cfgRadarStopSpeed = cfg->GetFloat("RadarStopSpeed", cfgRadarStopSpeed);
}