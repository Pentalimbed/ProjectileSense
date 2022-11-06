#include "config.h"

#include <toml++/toml.h>

void Config::init()
{
    proj_spell       = RE::TESForm::LookupByEditorID<RE::SpellItem>("ProjectileSense");
    proj_cooldown_fx = RE::TESForm::LookupByEditorID<RE::EffectSetting>("ProjectileSenseCooldown");

    toml::table tbl;
    try
    {
        tbl = toml::parse_file("data/skse/plugins/ProjectileSense.toml");

        sense_arrow   = tbl["sense_arrow"].value_or(true);
        sense_missile = tbl["sense_missile"].value_or(true);
        timescale     = tbl["timescale"].value_or(0.05f);
        duration      = tbl["duration"].value_or(1.f);
        cooldown      = tbl["cooldown"].value_or(4.f);
        detect_range  = tbl["detect_range"].value_or(384.f);
        detect_angle  = tbl["detect_angle"].value_or(30.f);
        min_speed     = tbl["min_speed"].value_or(2500.f);
        stamina_cost  = tbl["stamina_cost"].value_or(0.f);
        magicka_cost  = tbl["magicka_cost"].value_or(0.f);

        std::string required_perk = tbl["required_perk"].value_or("");
        req_perk                  = RE::TESForm::LookupByEditorID<RE::BGSPerk>(required_perk);
    }
    catch (const toml::parse_error& err)
    {
        logger::error("Failed to parse ProjectileSense.toml. Error: {}", err.description());
    }

    if (loaded())
        apply();
}

void Config::apply()
{
    // sound
    proj_spell->effects[0]->effectItem.duration = timescale * duration;
    //cooldown
    proj_spell->effects[1]->effectItem.duration = cooldown;
}