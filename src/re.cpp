#include "re.h"
#include "config.h"

static std::mutex   func_mutex;
static std::jthread slowdown_thread;

// why tho
template <>
RE::BGSProjectile* RE::TESForm::As<RE::BGSProjectile, void>() noexcept
{
    return const_cast<RE::BGSProjectile*>(
        static_cast<const RE::TESForm*>(this)->As<RE::BGSProjectile>());
}

inline void slowdown()
{
    REL::Relocation<float*> time_mult_1{REL::VariantID(511882, 388442, 0x1EC5698)};
    REL::Relocation<float*> time_mult_2{REL::VariantID(511883, 388443, 0x1EC569C)};
    *time_mult_1 *= Config::getSingleton()->timescale;
    *time_mult_2 = *time_mult_1;
}

inline void revertSlowdown()
{
    REL::Relocation<float*> time_mult_1{REL::VariantID(511882, 388442, 0x1EC5698)};
    REL::Relocation<float*> time_mult_2{REL::VariantID(511883, 388443, 0x1EC569C)};
    *time_mult_1 /= Config::getSingleton()->timescale;
    if (*time_mult_1 > 1.f)
        *time_mult_1 = 1.f;
    *time_mult_2 = *time_mult_1;
}

inline void logProjectile(RE::Projectile* a_this)
{
    auto base = a_this->GetBaseObject();
    logger::debug("Projectile Name: {} ({:08x} | {})",
                  a_this->GetName(), base->GetFormID(), base->GetFile()->fileName);
    auto shooter_ref = a_this->GetProjectileRuntimeData().shooter;
    if (shooter_ref && shooter_ref.get() && shooter_ref.get().get())
    {
        auto shooter = shooter_ref.get().get();
        logger::debug("Source: {}",
                      shooter->GetName() ? shooter->GetName() : "");
    }
    logger::debug("Flags: {:032b}",
                  a_this->GetProjectileRuntimeData().flags);
}

void ProjectileHook::Process(RE::Projectile* a_this, bool is_arrow)
{
    if (slowdown_thread.joinable())
        return;

    auto config = Config::getSingleton();

    if (!config->loaded())
        return;

    if (!a_this->Get3D2())
        return;

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player)
        return;

    auto spine = player->GetNodeByName("NPC Spine [Spn0]");
    if (!spine)
        return;
    auto pos = spine->world.translate;

    // ragdoll check
    if (player->IsInRagdollState())
        return;

    // distance check
    auto& runtime_data = a_this->GetProjectileRuntimeData();
    auto  velocity     = runtime_data.linearVelocity;
    auto  dist         = a_this->GetPosition().GetDistance(pos);
    if ((dist > config->detect_range) && (dist > config->detect_time * velocity.Length()))
        return;

    // logProjectile(a_this);

    // cooldown check
    if (player->AsMagicTarget()->HasMagicEffect(config->proj_cooldown_fx))
        return;

    // stamina/magicka check
    if ((player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kStamina) < config->stamina_cost) ||
        (player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka) < config->magicka_cost))
        return;

    // perk check
    if (config->req_perk && !player->HasPerk(config->req_perk))
        return;

    // toggling check
    if (!player->HasSpell(config->proj_enabled_spell))
        return;

    // proj type check
    if (is_arrow && !config->sense_arrow)
        return;
    if (a_this->IsMissileProjectile())
    {
        if (!config->sense_missile)
            return;
    }
    else
        return;

    // min speed check
    if (velocity.Length() < config->min_speed)
        return;

    // live too long, must smth wrong
    if (runtime_data.livingTime > 60.f)
        return;

    // potential flag check for buggy dropped arrow (?)
    if (runtime_data.flags & (1 << 2))
        return;

    // shooter is hostile
    auto shooter_ref = runtime_data.shooter;
    if (!shooter_ref || !shooter_ref.get() || !shooter_ref.get().get())
        return;
    auto shooter = shooter_ref.get().get()->As<RE::Actor>();
    if (!shooter)
        return;
    if (!shooter->IsHostileToActor(player))
        return;

    // angle check
    auto diff_vector    = pos - a_this->GetPosition();
    auto expected_angle = std::asin(config->detect_margin / dist);
    auto angle          = std::acos(velocity.Dot(diff_vector) / velocity.Length() / diff_vector.Length());
    if (angle > expected_angle)
        return;

    // logger::info("los check! {}", angle * 180 / std::_Pi);
    // // LOS check
    // if (RequestLOS(player, a_this->AsReference(), 2 * std::_Pi) == 0)
    //     return;

    // apply spell
    logger::debug("Triggering!");
    if (spdlog::get_level() < spdlog::level::info)
        logProjectile(a_this);

    player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(config->proj_spell, false, player->AsReference(), 1.f, false, 0.f, nullptr);

    // dispeled cooldown fx fix
    if (!player->AsMagicTarget()->HasMagicEffect(config->proj_cooldown_fx))
        return;

    player->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIERS::kDamage, RE::ActorValue::kStamina, -config->stamina_cost);
    player->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIERS::kDamage, RE::ActorValue::kMagicka, -config->magicka_cost);

    slowdown();
    slowdown_thread = std::jthread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(config->duration * 1000)));
        revertSlowdown();
    });
    slowdown_thread.detach();
}

void ProjectileHook::GetLinearVelocityProjectile(RE::Projectile* a_this, RE::NiPoint3& a_outVelocity)
{
    _GetLinearVelocityProjectile(a_this, a_outVelocity);

    if (func_mutex.try_lock())
    {
        Process(a_this, false);
        func_mutex.unlock();
    }
}

void ProjectileHook::GetLinearVelocityArrow(RE::Projectile* a_this, RE::NiPoint3& a_outVelocity)
{
    _GetLinearVelocityArrow(a_this, a_outVelocity);

    if (func_mutex.try_lock())
    {
        Process(a_this, true);
        func_mutex.unlock();
    }
}

void ProjectileHook::GetLinearVelocityMissile(RE::Projectile* a_this, RE::NiPoint3& a_outVelocity)
{
    _GetLinearVelocityMissile(a_this, a_outVelocity);

    if (func_mutex.try_lock())
    {
        Process(a_this, false);
        func_mutex.unlock();
    }
}
