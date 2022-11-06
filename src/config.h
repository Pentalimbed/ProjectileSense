#pragma once

struct Config
{
    bool  sense_arrow   = true;
    bool  sense_missile = true;
    float timescale     = 0.05f;
    float duration      = 1.f;
    float cooldown      = 4.f;
    float detect_range  = 384.f;
    float detect_angle  = 30.f;
    float min_speed     = 2500.f;
    float stamina_cost  = 0.f;
    float magicka_cost  = 0.f;

    RE::BGSPerk* req_perk = nullptr;

    RE::SpellItem*     proj_spell       = nullptr;
    RE::EffectSetting* proj_cooldown_fx = nullptr;

    static Config* getSingleton()
    {
        static Config config;
        return std::addressof(config);
    }

    inline bool loaded() { return proj_spell && proj_cooldown_fx; }
    void        init();
    void        apply();
};