#include "includes.h"

void RebuiltMovement::SetupMove(Player* player, CUserCmd* cmd, CMoveData* data) {
	data->m_bFirstRunOfFunctions = false;
	data->m_bGameCodeMovedPlayer = false;
	data->m_nPlayerHandle = ((IHandleEntity*)player)->GetRefEHandle();

	data->m_vecVelocity = player->m_vecVelocity();
	data->m_vecAbsOrigin = player->GetAbsOrigin();
	data->m_flClientMaxSpeed = player->m_flMaxspeed();

	data->m_vecAbsViewAngles = data->m_vecAngles = data->m_vecViewAngles = cmd->m_view_angles;
	data->m_nImpulseCommand = cmd->m_impulse;
	data->m_nButtons = cmd->m_buttons;

	data->m_flForwardMove = cmd->m_forward_move;
	data->m_flSideMove = cmd->m_side_move;
	data->m_flUpMove = cmd->m_up_move;

	data->m_vecConstraintCenter = {};
	data->m_flConstraintRadius = 0.f;
	data->m_flConstraintSpeedFactor = 0.f;

	//g_csgo.m_game_movement->SetupMovementBounds(data);
}

CMoveData RebuiltMovement::SetupMove(Player* player, CUserCmd* cmd) {
	CMoveData data{};

	data.m_bFirstRunOfFunctions = false;
	data.m_bGameCodeMovedPlayer = false;
	data.m_nPlayerHandle = ((IHandleEntity*)player)->GetRefEHandle();

	data.m_vecVelocity = player->m_vecVelocity();
	data.m_vecAbsOrigin = player->GetAbsOrigin();
	data.m_flClientMaxSpeed = player->m_flMaxspeed();

	data.m_vecAbsViewAngles = data.m_vecAngles = data.m_vecViewAngles = cmd->m_view_angles;
	data.m_nImpulseCommand = cmd->m_impulse;
	data.m_nButtons = cmd->m_buttons;

	data.m_flForwardMove = cmd->m_forward_move;
	data.m_flSideMove = cmd->m_side_move;
	data.m_flUpMove = cmd->m_up_move;

	data.m_vecConstraintCenter = {};
	data.m_flConstraintRadius = 0.f;
	data.m_flConstraintSpeedFactor = 0.f;

	g_csgo.m_game_movement->SetupMovementBounds(&data);

	return data;
}

void RebuiltMovement::AirAccelerate(vec3_t const& wishdir, float wishspeed, CMoveData* mv, float friction) {

	const auto wishspd = std::min(wishspeed, 30.f);
	auto const currentspeed = mv->m_vecVelocity.dot(wishdir);
	auto const addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;

	auto accelspeed = g_csgo.sv_airaccelerate->GetFloat() * wishspeed * g_csgo.m_globals->m_interval * friction;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	mv->m_vecVelocity += wishdir * accelspeed;
}

void RebuiltMovement::Accelerate(const vec3_t& wishdir, float wishspeed, float accel, CMoveData* mv, Player* ent) {

	Weapon* weapon = ent->GetActiveWeapon( );

	auto stored_accel = accel;
	auto currentspeed = mv->m_vecVelocity.dot(wishdir);
	const float addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;

	if (currentspeed < 0)
		currentspeed = 0;

	const auto is_ducking = mv->m_nButtons & IN_DUCK || ent->m_bDucking() || ent->m_fFlags() & FL_DUCKING;
	const auto is_walking = (mv->m_nButtons & IN_SPEED) != 0 && !is_ducking;

	constexpr auto max_speed = 250.0f;
	auto acceleration_scale = std::max(max_speed, wishspeed);
	auto goal_speed = acceleration_scale;

	auto is_slow_sniper_scoped = false;

	if (g_csgo.sv_accelerate_use_weapon_speed->GetInt() && weapon) {

		auto weapon_info = weapon->GetWpnData( );

		is_slow_sniper_scoped = (weapon->GetMaxSpeed() > 0 && weapon_info && weapon_info->m_zoom_levels > 1 && weapon->GetMaxSpeed() * 0.52f) < 110.0;

		goal_speed *= std::min(1.0f, weapon->GetMaxSpeed() / max_speed);

		if ((!is_ducking && !is_walking) || ((is_walking || is_ducking) && is_slow_sniper_scoped))
			acceleration_scale *= std::min(1.0f, weapon->GetMaxSpeed() / max_speed);
	}

	if (is_ducking) {

		if (!is_slow_sniper_scoped)
			acceleration_scale *= 0.34f;

		goal_speed *= 0.34f;
	}

	if (is_walking) {

		if (!is_slow_sniper_scoped)
			acceleration_scale *= 0.52f;

		goal_speed *= 0.52f;
	}

	if (is_walking && currentspeed > (goal_speed - 5))
		stored_accel *= std::clamp(1.0f - (std::max(0.0f, currentspeed - (goal_speed - 5)) / std::max(0.0f, goal_speed - (goal_speed - 5))), 0.0f, 1.0f);

	auto accelspeed = stored_accel * g_csgo.m_globals->m_interval * acceleration_scale * ent->m_surfaceFriction();
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	mv->m_vecVelocity += (wishdir * accelspeed);
}

void RebuiltMovement::Friction(CMoveData* mv, Player* player) {

	// Calculate speed
	const auto speed = mv->m_vecVelocity.length();

	// If too slow, return
	if (speed < 0.1f)
		return;

	auto drop = 0.f;

	// apply ground friction
	if (player->m_hGroundEntity() != 0) { // On an entity that is the ground

		const auto friction = g_csgo.sv_friction->GetFloat() * player->m_surfaceFriction();

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount.
		const auto control = (speed < g_csgo.sv_stopspeed->GetFloat()) ? g_csgo.sv_stopspeed->GetFloat() : speed;

		// Add the amount to the drop amount.
		drop += control * friction * g_csgo.m_globals->m_interval;
	}

	// scale the velocity
	auto newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	if (newspeed != speed) {

		// Determine proportion of old speed we are using.
		newspeed /= speed;

		// Adjust velocity according to proportion.
		mv->m_vecVelocity *= newspeed;
	}

	mv->m_outWishVel -= mv->m_vecVelocity * (1.f - newspeed);
}

void RebuiltMovement::CheckParameters(CMoveData* mv, Player* player) {

	const auto speed_squared = (mv->m_flForwardMove * mv->m_flForwardMove) +
		(mv->m_flSideMove * mv->m_flSideMove) +
		(mv->m_flUpMove * mv->m_flUpMove);

	// Slow down by the speed factor
	auto speed_factor = 1.0f;
	if (const auto surface_data = reinterpret_cast<surfacedata_t*>(player->m_SurfaceData()); surface_data != nullptr)
		speed_factor = surface_data->m_game.m_max_speed_factor;

	//if (player->GetSurfaceData())
	//	speed_factor = player->GetSurfaceData()->m_game.m_max_speed_factor;

	mv->m_flMaxSpeed = player->GetActiveWeapon() ? player->GetActiveWeapon()->GetMaxSpeed() : mv->m_flClientMaxSpeed;
	mv->m_flMaxSpeed *= speed_factor;

	// stamina slowing factor
	if (player->m_flStamina() > 0) {

		auto speed_scale = std::clamp(1.0f - player->m_flStamina() / 100.f, 0.f, 1.f);
		speed_scale *= speed_scale;	// square the scale factor so that it correlates more closely with the jump penalty (i.e. a 50% stamina jumps .25 * normal height)
		mv->m_flMaxSpeed *= speed_scale;
	}

	// Same thing but only do the sqrt if we have to.
	if ((speed_squared != 0.0) && (speed_squared > mv->m_flMaxSpeed * mv->m_flMaxSpeed)) {

		const auto ratio = mv->m_flMaxSpeed / sqrt(speed_squared);
		mv->m_flForwardMove *= ratio;
		mv->m_flSideMove *= ratio;
		mv->m_flUpMove *= ratio;
	}
}

void RebuiltMovement::AirMove(CMoveData* mv, Player* ent) {

	vec3_t forward, right, up;
	math::AngleVectors(mv->m_vecViewAngles, &forward, &right, &up); // Determine movement angles

	// Copy movement amounts
	auto const fmove = mv->m_flForwardMove;
	auto const smove = mv->m_flSideMove;

	forward.z = 0;
	math::fast_vec_normalize(forward);

	right.z = 0;
	math::fast_vec_normalize(right);

	vec3_t wishvel(forward.x * fmove + right.x * smove, forward.y * fmove + right.y * smove, 0.f);

	auto wishdir = wishvel;
	auto wishspeed = wishdir.normalize();

	// clamp to server defined max speed
	if (wishspeed != 0 && (wishspeed > mv->m_flMaxSpeed)) {

		wishvel = wishvel * (mv->m_flMaxSpeed / wishspeed);
		wishspeed = mv->m_flMaxSpeed;
	}

	AirAccelerate(wishdir, wishspeed, mv, ent->m_surfaceFriction());
}

void RebuiltMovement::WalkMove(CMoveData* mv, Player* ent) {

	vec3_t forward, right, up;
	math::AngleVectors(mv->m_vecViewAngles, &forward, &right, &up); // Determine movement angles

	// Copy movement amounts
	auto const fmove = mv->m_flForwardMove;
	auto const smove = mv->m_flSideMove;

	forward.z = 0;
	math::fast_vec_normalize(forward);

	right.z = 0;
	math::fast_vec_normalize(right);

	vec3_t wishvel(forward.x * fmove + right.x * smove, forward.y * fmove + right.y * smove, 0.f);

	auto wishdir = wishvel;
	auto wishspeed = wishdir.normalize();

	// clamp to server defined max speed
	if (wishspeed != 0 && (wishspeed > mv->m_flMaxSpeed)) {
		wishvel = wishvel * (mv->m_flMaxSpeed / wishspeed);
		wishspeed = mv->m_flMaxSpeed;
	}

	// Set pmove velocity
	mv->m_vecVelocity[2] = 0;
	Accelerate(wishdir, wishspeed, g_csgo.sv_accelerate->GetFloat(), mv, ent);
	mv->m_vecVelocity[2] = 0;

	// Additional max speed clamp to keep us from going faster than allowed while turning
	if (mv->m_vecVelocity.length_sqr() > mv->m_flMaxSpeed * mv->m_flMaxSpeed) {
		float fRatio = mv->m_flMaxSpeed / mv->m_vecVelocity.length();
		mv->m_vecVelocity *= fRatio;
	}

	// If we made it all the way, then copy trace end as new player position.
	mv->m_outWishVel += wishdir * wishspeed;
}

void RebuiltMovement::Extrapolate(Player* player, vec3_t origin, vec3_t velocity, int flags, bool on_ground) {

	static auto sv_gravity = g_csgo.sv_gravity;
	static auto sv_jump_impulse = g_csgo.sv_jump_impulse;

	if (!(flags & FL_ONGROUND))
		velocity.z -= game::TICKS_TO_TIME(sv_gravity->GetFloat());
	else if ((player->m_fFlags() & FL_ONGROUND) && !on_ground)
		velocity.z = sv_jump_impulse->GetFloat();

	const auto src = origin;
	auto end = src + velocity * g_csgo.m_globals->m_interval;

	CGameTrace t;
	CTraceFilterWorldOnly filter;
	g_csgo.m_engine_trace->TraceRay(Ray(src, end, player->m_vecMins(), player->m_vecMaxs()), MASK_PLAYERSOLID, &filter, &t);

	if (t.m_fraction != 1.f) {

		for (auto i = 0; i < 2; i++) {

			velocity -= t.m_plane.m_normal * velocity.dot(t.m_plane.m_normal);

			const auto dot = velocity.dot(t.m_plane.m_normal);
			if (dot < 0.f)
				velocity -= vec3_t{ dot * t.m_plane.m_normal.x, dot * t.m_plane.m_normal.y, dot * t.m_plane.m_normal.z };

			end = t.m_endpos + velocity * game::TICKS_TO_TIME(1.f - t.m_fraction);

			g_csgo.m_engine_trace->TraceRay(Ray(t.m_endpos, end, player->m_vecMins(), player->m_vecMaxs()), MASK_PLAYERSOLID, &filter, &t);

			if (t.m_fraction == 1.f)
				break;
		}
	}

	origin = end = t.m_endpos;
	end.z -= 2.f;

	g_csgo.m_engine_trace->TraceRay(Ray(origin, end, player->m_vecMins(), player->m_vecMaxs()), MASK_PLAYERSOLID, &filter, &t);

	flags &= ~FL_ONGROUND;

	if (t.hit() && t.m_plane.m_normal.z > .7f)
		flags |= FL_ONGROUND;
}