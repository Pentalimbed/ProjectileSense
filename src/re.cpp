#include "re.h"
#include "config.h"

static std::mutex   func_mutex;
static std::jthread slowdown_thread;

inline void slowdown()
{
    REL::Relocation<float*> time_mult_1{RELOCATION_ID(511882, 388442)};
    REL::Relocation<float*> time_mult_2{RELOCATION_ID(511883, 388443)};
    *time_mult_1 *= Config::getSingleton()->timescale;
    *time_mult_2 = *time_mult_1;
}

inline void revert_slowdown()
{
    REL::Relocation<float*> time_mult_1{RELOCATION_ID(511882, 388442)};
    REL::Relocation<float*> time_mult_2{RELOCATION_ID(511883, 388443)};
    *time_mult_1 /= Config::getSingleton()->timescale;
    if (*time_mult_1 > 1.f)
        *time_mult_1 = 1.f;
    *time_mult_2 = *time_mult_1;
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

    // distance check
    auto dist = a_this->GetPosition().GetDistance(pos);
    if (dist > config->detect_range)
        return;

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

    // proj type check
    if (is_arrow && !config->sense_arrow)
        return;
    if (a_this->IsMissileProjectile() && !config->sense_missile)
        return;

    // min speed check
    auto velocity = a_this->GetProjectileRuntimeData().linearVelocity;
    if (velocity.Length() < config->min_speed)
        return;

    // live too long, must smth wrong
    if (a_this->GetProjectileRuntimeData().livingTime > 60.f)
        return;

    // shooter is hostile
    auto shooter_ref = a_this->GetProjectileRuntimeData().shooter;
    if (!shooter_ref || !shooter_ref.get() || !shooter_ref.get().get())
        return;
    auto shooter = shooter_ref.get().get()->As<RE::Actor>();
    if (!shooter)
        return;
    if (shooter && !shooter->IsHostileToActor(player))
        return;

    // angle check
    auto diff_vector = pos - a_this->GetPosition();
    auto angle       = std::acos(velocity.Dot(diff_vector) / velocity.Length() / diff_vector.Length());
    if (angle > config->detect_angle * std::_Pi / 180)
        return;

    // logger::info("los check! {}", angle * 180 / std::_Pi);
    // // LOS check
    // if (RequestLOS(player, a_this->AsReference(), 2 * std::_Pi) == 0)
    //     return;

    // apply spell
    player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(config->proj_spell, false, player->AsReference(), 1.f, false, 0.f, nullptr);

    player->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIERS::kDamage, RE::ActorValue::kStamina, -config->stamina_cost);
    player->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIERS::kDamage, RE::ActorValue::kMagicka, -config->magicka_cost);

    slowdown();
    slowdown_thread = std::jthread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(config->duration * 1000)));
        revert_slowdown();
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
