#pragma once

namespace RebuiltMovement {

	inline bool computing = false;

	extern void SetupMove(Player* player, CUserCmd* cmd, CMoveData* mv);
	CMoveData SetupMove(Player* player, CUserCmd* cmd);
	extern void AirAccelerate(vec3_t const& wishdir, float wishspeed, CMoveData* mv, float friction);
	extern void Accelerate(const vec3_t& wishdir, float wishspeed, float accel, CMoveData* mv, Player* ent);
	extern void Friction(CMoveData* mv, Player* player);
	extern void CheckParameters(CMoveData* mv, Player* player);
	extern void AirMove(CMoveData* mv, Player* ent);
	extern void WalkMove(CMoveData* mv, Player* ent);
	extern void Extrapolate(Player* player, vec3_t origin, vec3_t velocity, int flags, bool on_ground);
}