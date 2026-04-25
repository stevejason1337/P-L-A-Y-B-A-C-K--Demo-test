#pragma once
// ═══════════════════════════════════════════════════════════════
//  Engine/Core/Soundmanager.h
//  Зависимости: только miniaudio.h — никакого Player.h
// ═══════════════════════════════════════════════════════════════

#include "miniaudio.h"

static constexpr int SND_SHOT_VOICES = 4;
static constexpr int SND_STEP_VOICES = 3;

class SoundManager
{
public:
    // ── Lifecycle ─────────────────────────────────────────────
    void init();
    void shutdown();

    // ── Playback ──────────────────────────────────────────────
    void playShot(int weaponIndex);       // 0=glock, 1=glock2, 2=ak74
    void playReload(int weaponIndex);
    void playEmpty();

    // sprinting передаётся из Game — SoundManager не знает про Player
    void playFootstep(float dt, bool moving, bool onGround, bool sprinting = false);

private:
    ma_engine engine{};
    bool ready = false;

    // ── Shot voices (polyphony) ───────────────────────────────
    ma_sound shotSounds[3][SND_SHOT_VOICES]{};
    bool     shotLoaded[3] = {};
    int      shotVoice[3] = {};

    // ── Reload ────────────────────────────────────────────────
    ma_sound reloadSounds[3]{};
    bool     reloadLoaded[3] = {};

    // ── Empty click ───────────────────────────────────────────
    ma_sound emptySound{};
    bool     emptyLoaded = false;

    // ── Footstep voices ───────────────────────────────────────
    ma_sound stepSound[SND_STEP_VOICES]{};
    bool     stepLoaded = false;
    int      stepVoice = 0;
    float    stepTimer = 0.f;

    void _play(ma_sound& s);
};

// Глобальный экземпляр — определён в Soundmanager.cpp
extern SoundManager soundManager;