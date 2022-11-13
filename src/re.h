#pragma once

// Ersh
class ProjectileHook
{
public:
    static void Hook()
    {
        REL::Relocation<std::uintptr_t> ProjectileVtbl{RE::VTABLE_Projectile[0]};               // 167C888
        REL::Relocation<std::uintptr_t> ArrowProjectileVtbl{RE::VTABLE_ArrowProjectile[0]};     // 1676318
        REL::Relocation<std::uintptr_t> MissileProjectileVtbl{RE::VTABLE_MissileProjectile[0]}; // 167AE78
        REL::Relocation<std::uintptr_t> BeamProjectileVtbl{RE::VTABLE_BeamProjectile[0]};       // 1677660
        // _GetLinearVelocityProjectile = ProjectileVtbl.write_vfunc(0x86, GetLinearVelocityProjectile);
        _GetLinearVelocityArrow   = ArrowProjectileVtbl.write_vfunc(0x86, GetLinearVelocityArrow);
        _GetLinearVelocityMissile = MissileProjectileVtbl.write_vfunc(0x86, GetLinearVelocityMissile);
        logger::info("Hook installed!");
    }

private:
    static void Process(RE::Projectile* a_this, bool is_arrow);
    static void GetLinearVelocityProjectile(RE::Projectile* a_this, RE::NiPoint3& a_outVelocity);
    static void GetLinearVelocityArrow(RE::Projectile* a_this, RE::NiPoint3& a_outVelocity);
    static void GetLinearVelocityMissile(RE::Projectile* a_this, RE::NiPoint3& a_outVelocity);

    static inline REL::Relocation<decltype(GetLinearVelocityProjectile)> _GetLinearVelocityProjectile;
    static inline REL::Relocation<decltype(GetLinearVelocityArrow)>      _GetLinearVelocityArrow;
    static inline REL::Relocation<decltype(GetLinearVelocityMissile)>    _GetLinearVelocityMissile;
};