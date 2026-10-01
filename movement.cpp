#include "includes.h"

Movement g_movement { };;

void Movement::EdgeJump( )
{
	if ( !( g_cl.m_local->m_fFlags( ) & FL_ONGROUND ) )
		return;

	g_inputpred.predict( );

	if ( g_cl.m_local->m_fFlags( ) & FL_ONGROUND )
	{
		g_inputpred.SetDatamap( g_inputpred.og_predicted );
		return;
	}

	g_cl.m_cmd->m_buttons |= IN_JUMP;

	g_inputpred.SetDatamap( g_inputpred.cm_start );
	g_inputpred.predict( );
	g_inputpred.GetDatamap( g_inputpred.og_predicted );
}

void Movement::JumpRelated( )
{
	int bad_mts[ ] = { MOVETYPE_LADDER, MOVETYPE_NOCLIP, MOVETYPE_OBSERVER };
	bool bad_movement = std::find( std::begin( bad_mts ), std::end( bad_mts ), g_cl.m_local->m_MoveType( ) ) != std::end( bad_mts );

	if ( bad_movement )
		return;

	if ( ( g_cl.m_cmd->m_buttons & IN_JUMP ) && !( g_cl.m_flags & FL_ONGROUND ) )
	{
		// bhop.
		if ( g_menu.main.movement.bhop.get( ) )
			g_cl.m_cmd->m_buttons &= ~IN_JUMP;

		// duck jump ( crate jump ).
		if ( g_menu.main.movement.airduck.get( ) )
			g_cl.m_cmd->m_buttons |= IN_DUCK;
	}
}

void Movement::AutoStrafe( ) {
	if ( !g_menu.main.movement.autostrafe.get( ) )
		return;

	if ( g_cl.m_flags & FL_ONGROUND )
		return;

	if ( g_cl.m_local->m_MoveType( ) == MOVETYPE_NOCLIP || g_cl.m_buttons & IN_SPEED )
		return;

	vec3_t velocity = g_cl.m_local->m_vecVelocity( );
	float speed_2d = velocity.length_2d( );

	if ( speed_2d < 2.f )
		return;

	float yaw = math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y );

	// adjust yaw based on input
	{
		int buttons = g_cl.m_cmd->m_buttons;
		float offset = 0.f;

		if ( buttons & IN_MOVELEFT )
			offset += 90.f;
		if ( buttons & IN_MOVERIGHT )
			offset -= 90.f;
		if ( buttons & IN_FORWARD )
			offset *= 0.5f;
		else if ( buttons & IN_BACK )
			offset = ( -offset * 0.5f ) + 180.f;

		yaw += offset;

		// stop movement for manual adjustment
		g_cl.m_cmd->m_forward_move = 0.f;
		g_cl.m_cmd->m_side_move = 0.f;
	}

	auto simulate_yaw = [ & ]( float target_yaw ) {
		CUserCmd cmd_copy;
		std::memcpy( &cmd_copy, g_cl.m_cmd, sizeof( CUserCmd ) );

		cmd_copy.m_view_angles.y = target_yaw;
		cmd_copy.m_forward_move = 450.f;
		cmd_copy.m_side_move = 0.f;

		CMoveData move_data;
		RebuiltMovement::SetupMove( g_cl.m_local, &cmd_copy, &move_data );
		RebuiltMovement::CheckParameters( &move_data, g_cl.m_local );
		RebuiltMovement::AirMove( &move_data, g_cl.m_local );

		return move_data.m_vecVelocity.length_2d( );
		};

	auto consider_accel = [ & ]( float test_yaw, float& best_speed, float& best_yaw ) {
		float cmp = simulate_yaw( math::NormalizedAngle( test_yaw ) );
		if ( cmp >= best_speed ) {
			best_speed = cmp;
			best_yaw = math::NormalizedAngle( test_yaw );
		}
		};

	// find velocity angle
	const vec3_t& vel = g_cl.m_local->m_vecVelocity( );
	float velocity_angle = math::rad_to_deg( std::atan2f( vel.y, vel.x ) );

	if ( velocity_angle < 0.f )
		velocity_angle += 360.f;

	velocity_angle -= std::floor( velocity_angle / 360.f + 0.5f ) * 360.f;

	float best_yaw = 0.f;
	float best_speed = -1.f;

	for ( float step = 0.f; step <= 130.f; step += 0.1f )
		consider_accel( velocity_angle + step, best_speed, best_yaw );

	float ideal = math::AngleDiff( best_yaw, velocity_angle );
	float correction = ideal;

	float velocity_delta = math::AngleDiff( yaw, velocity_angle );
	correction += std::fabs( velocity_delta ) *
		( 0.2f * ( 1.f - ( g_menu.main.movement.autostrafe_smoothing.get( ) * 0.01f ) ) );

	if ( ( std::fabs( velocity_delta ) > 170.f && speed_2d > 80.f ) || velocity_delta > 0.f )
		yaw = velocity_angle + correction;
	else if ( velocity_delta <= 0.f )
		yaw = velocity_angle - correction;

	g_cl.m_strafe_angles.y = yaw;
	g_cl.m_cmd->m_side_move = 0.f;
	g_cl.m_cmd->m_forward_move = 450.f; // full speed
}

// todo: probably add the shit back that checks local player stuff lol
// just removed for ease while making new extrapolation.
void Movement::FixMove( CUserCmd* cmd, const ang_t& wish_angles, bool local )
{
	vec3_t  move, dir;
	float   delta, len;
	ang_t   move_angle;

	// roll nospread fix.
	//if ( !( g_cl.m_flags & FL_ONGROUND ) && cmd->m_view_angles.z != 0.f )
	//	cmd->m_side_move = 0.f;

	// convert movement to vector.
	move = { cmd->m_forward_move, cmd->m_side_move, 0.f };

	// get move length and ensure we're using a unit vector ( vector with length of 1 ).
	len = move.normalize( );
	if ( !len )
		return;

	// convert move to an angle.
	math::VectorAngles( move, move_angle );

	// calculate yaw delta.
	delta = ( cmd->m_view_angles.y - wish_angles.y );

	// accumulate yaw delta.
	move_angle.y += delta;

	// calculate our new move direction.
	// dir = move_angle_forward * move_length
	math::AngleVectors( move_angle, &dir );

	// scale to og movement.
	dir *= len;

	// strip old flags.
	cmd->m_buttons &= ~( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT );

	float max_fwd_speed = g_csgo.cl_forwardspeed->GetFloat( );
	float max_back_speed = g_csgo.cl_backspeed->GetFloat( );
	float max_side_speed = g_csgo.cl_sidespeed->GetFloat( );

	cmd->m_forward_move = std::clamp( cmd->m_forward_move, -max_back_speed, max_fwd_speed );
	cmd->m_side_move = std::clamp( cmd->m_forward_move, -max_side_speed, max_side_speed );

	// fix ladder and noclip.
	if ( local && g_cl.m_local->m_MoveType( ) == MOVETYPE_LADDER )
	{
		// invert directon for up and down.
		if ( cmd->m_view_angles.x >= 45.f && wish_angles.x < 45.f && std::abs( delta ) <= 65.f )
			dir.x = -dir.x;

		// write to movement.
		cmd->m_forward_move = dir.x;
		cmd->m_side_move = dir.y;

		// set new button flags.
		if ( cmd->m_forward_move > 200.f )
			cmd->m_buttons |= IN_FORWARD;

		else if ( cmd->m_forward_move < -200.f )
			cmd->m_buttons |= IN_BACK;

		if ( cmd->m_side_move > 200.f )
			cmd->m_buttons |= IN_MOVERIGHT;

		else if ( cmd->m_side_move < -200.f )
			cmd->m_buttons |= IN_MOVELEFT;
	}

	// we are moving normally.
	else
	{
		// we must do this for pitch angles that are out of bounds.
		if ( cmd->m_view_angles.x < -90.f || cmd->m_view_angles.x > 90.f )
			dir.x = -dir.x;

		// set move.
		cmd->m_forward_move = dir.x;
		cmd->m_side_move = dir.y;

		// set new button flags.
		// clear movement keys
		int new_buttons = cmd->m_buttons & ~( IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD );

		int leg_mode = g_menu.main.antiaim.leg_movement.get( );

		// normal mode
		if ( leg_mode == 0 ) {
			// forward / backward
			if ( cmd->m_forward_move > 0.f )
				new_buttons |= IN_FORWARD;
			else if ( cmd->m_forward_move < 0.f )
				new_buttons |= IN_BACK;

			// side
			if ( cmd->m_side_move > 0.f )
				new_buttons |= IN_MOVERIGHT;
			else if ( cmd->m_side_move < 0.f )
				new_buttons |= IN_MOVELEFT;
		}

		// moonwalk inverted mode
		else if ( leg_mode == 1 ) {
			// forward / backward inverted
			if ( cmd->m_forward_move > 0.f )
				new_buttons |= IN_BACK;
			else if ( cmd->m_forward_move < 0.f )
				new_buttons |= IN_FORWARD;

			// side inverted
			if ( cmd->m_side_move > 0.f )
				new_buttons |= IN_MOVELEFT;
			else if ( cmd->m_side_move < 0.f )
				new_buttons |= IN_MOVERIGHT;
		}

		// leg_mode > 1 do nothing (keep buttons cleared)

		cmd->m_buttons = new_buttons;
	}
}

void Movement::AutoPeek( ) {
	m_autopeek_retreating = false;

	// holding autopeek key
	if ( g_input_system.GetKeyState( g_menu.main.movement.autopeek.get( ) ) ) {
		// if we've reached our starting position, stop retreating
		if ( ( g_cl.m_local->m_vecOrigin( ) - m_autopeek_pos ).length_2d( ) < 10.f )
			m_autopeek_retreat = false;

		if ( m_autopeek_retreat ) {
			m_autopeek_retreating = true;

			// quickswitch logic
			if ( m_should_quickswitch && g_menu.main.movement.quickswitch_on_autopeek.get( ) ) {
				auto weapons = g_cl.m_local->m_hMyWeapons( );

				for ( int i = 0; weapons[ i ].IsValid( ); i++ ) {
					Weapon* weapon = ( Weapon* )g_csgo.m_entlist->GetClientEntityFromHandle( weapons[ i ] );
					if ( !weapon )
						continue;

					WeaponInfo* data = weapon->GetWpnData( );
					if ( !data )
						continue;

					if ( data->m_weapon_type == WEAPONTYPE_KNIFE ) {
						//// prefer ZEUS if damage_effects allow it
						//if ( g_menu.main.misc.damage_effects.get( 5 ) &&
						//	weapon->m_iItemDefinitionIndex( ) == ZEUS ) {
						//	g_cl.m_cmd->m_weapon_select = weapon->index( );
						//	break;
						//}

						// fallback to normal knife if not ZEUS
						if ( weapon->m_iItemDefinitionIndex( ) != ZEUS ) {
							g_cl.m_cmd->m_weapon_select = weapon->index( );
							break;
						}
					}
				}

				m_should_quickswitch = false;
			}

			// strafe back to autopeek origin
			vec3_t ang = math::CalcAngle( g_cl.m_local->m_vecOrigin( ), m_autopeek_pos );

			g_cl.m_cmd->m_forward_move = 450.f;
			g_cl.m_cmd->m_side_move = 0.f;
			g_cl.m_strafe_angles.y = ang.y;
		}
	}
	else {
		// store starting point when autopeek key released
		m_autopeek_pos = g_cl.m_local->m_vecOrigin( );
		m_autopeek_retreat = true;
	}
}

void Movement::ProcessAutoStop( CUserCmd* cmd ) {
	if ( ( !g_menu.main.aimbot.automatic_stop.get( 0 ) && !g_menu.main.aimbot.automatic_stop.get( 1 ) )
		|| m_autopeek_retreating
		|| m_fakewalking ) // were currently in autopeek/fakewalk
		return;

	if ( !g_cl.m_pressing_move ) // fast stop
		QuickStop( cmd );
	else if ( g_aimbot.m_stop )
		StopUntilAccurate( );
}

void Movement::QuickStop( CUserCmd* cmd ) {
	vec3_t velocity = g_cl.m_local->m_vecVelocity( );
	if ( velocity.length( ) <= 25.f ) {
		cmd->m_forward_move = 0.f;
		cmd->m_side_move = 0.f;
		return;
	}

	// get direction opposite to current velocity
	ang_t dir;
	math::VectorAngles( -velocity, dir );

	// align to view yaw
	ang_t view_angles;
	g_csgo.m_engine->GetViewAngles( view_angles );
	dir.y = view_angles.y - dir.y;

	// convert to movement vector
	vec3_t forward;
	math::AngleVectors( dir, &forward );

	// scale against max move speeds
	float forward_speed = g_csgo.cl_forwardspeed->GetFloat( );
	float side_speed = g_csgo.cl_sidespeed->GetFloat( );

	cmd->m_forward_move = forward.x * forward_speed;
	cmd->m_side_move = forward.y * side_speed;
}

void Movement::StopToSpeed( float speed, CMoveData* mv, Player* player ) {
	// apply friction and movement parameter checks.
	RebuiltMovement::Friction( mv, player );
	RebuiltMovement::CheckParameters( mv, player );

	// copy move data and simulate a walk move.
	auto mv_cp = *mv;
	RebuiltMovement::WalkMove( &mv_cp, player );

	// already below or equal to target speed.
	if ( mv_cp.m_vecVelocity.length_2d( ) <= speed )
		return;

	// if current velocity is above desired speed, stop harder.
	if ( mv->m_vecVelocity.length_2d( ) > speed ) {
		ang_t ang {};
		math::VectorAngles( -mv->m_vecVelocity, ang );
		ang.y = math::normalize_float( mv->m_vecViewAngles.y - ang.y );

		vec3_t forward {};
		math::AngleVectors( ang, &forward );

		forward.z = 0.f;
		math::fast_vec_normalize( forward );

		const float target_len = std::min( mv->m_vecVelocity.length_2d( ) - speed, mv->m_flMaxSpeed );
		mv->m_flForwardMove = forward.x * target_len;
		mv->m_flSideMove = forward.y * target_len;
		mv->m_flUpMove = 0.f;
		return;
	}

	// otherwise, apply gradual stop.
	vec3_t forward, right;
	math::AngleVectors( mv->m_vecViewAngles, &forward, &right );
	forward.z = right.z = 0.f;

	math::fast_vec_normalize( forward );
	math::fast_vec_normalize( right );

	// calculate desired direction.
	vec3_t wishdir(
		forward.x * mv->m_flForwardMove + right.x * mv->m_flSideMove,
		forward.y * mv->m_flForwardMove + right.y * mv->m_flSideMove,
		0.f
	);
	math::fast_vec_normalize( wishdir );

	const float current_speed = mv->m_vecVelocity.dot( wishdir );
	const float target_accelspeed = speed - mv->m_vecVelocity.length_2d( );
	const float target_len = std::min( current_speed + target_accelspeed, mv->m_flMaxSpeed );

	// scale movement down if needed.
	const float move_sq = mv->m_flForwardMove * mv->m_flForwardMove
		+ mv->m_flSideMove * mv->m_flSideMove
		+ mv->m_flUpMove * mv->m_flUpMove;

	const float ratio = target_len / std::sqrt( move_sq );
	if ( ratio < 1.f ) {
		mv->m_flForwardMove *= ratio;
		mv->m_flSideMove *= ratio;
		mv->m_flUpMove *= ratio;
	}
}

void Movement::StopToSpeed( float speed, CUserCmd* cmd, Player* player ) {
	// setup move data and run stop logic.
	CMoveData data = RebuiltMovement::SetupMove( player, cmd );
	StopToSpeed( speed, &data );

	// write back to usercmd.
	cmd->m_forward_move = data.m_flForwardMove;
	cmd->m_side_move = data.m_flSideMove;

	// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/game/shared/cstrike15/cs_gamemovement.cpp#L829
	// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/game/shared/cstrike15/cs_gamemovement.cpp#L107
	float factor = 0.34f * player->m_flDuckAmount( ) + 1.0f - player->m_flDuckAmount( );
	cmd->m_forward_move /= factor;
	cmd->m_side_move /= factor;
}

bool Movement::StopUntilAccurate( ) {
	// we're already accurate dawg
	if ( g_cl.m_local->m_flDuckAmount( ) > 0.99f )
		return true;

	if ( !g_cl.m_weapon_info )
		return false;

	const float max_speed = g_cl.m_weapon->GetMaxSpeed( );
	const float vel_len = g_cl.m_local->m_vecVelocity( ).length_2d( );

	if ( vel_len < 0.01f )
		return true;

	// too fast, need full quick stop first.
	if ( vel_len > max_speed ) {
		QuickStop( g_cl.m_cmd );
		return false;
	}

	// slow down further to ~1/3rd weapon speed.
	StopToSpeed( max_speed * 0.33f, g_cl.m_cmd );
	return true;
}


void Movement::FakeWalk( )
{
	vec3_t velocity { g_cl.m_local->m_vecVelocity( ) };

	m_fake_walk_ticks_left = -1;

	const bool fakewalking = g_input_system.GetKeyState( g_menu.main.movement.fakewalk.get( ) );

	if ( !( fakewalking || !g_cl.m_pressing_move ) )
	{
		m_fakewalking = false;
		return;
	}

	if ( !g_cl.m_local->GetGroundEntity( ) )
	{
		if ( fakewalking && g_menu.main.aimbot.automatic_stop.get( 2 ) )
		{
			QuickStop( g_cl.m_cmd );
		}

		m_fakewalking = false;
		return;
	}

	// this shit makes u slow as fuck lol.
	if ( fakewalking )
	{
		g_cl.m_cmd->m_buttons &= ~IN_SPEED;
	}
	else
	{
		m_stop_the_fakewalk = false;

		if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) <= 0.1f )
			return;
	}

	CUserCmd bak_usrcmd = *g_cl.m_cmd;

	// not updated in CreateMove yet.
	g_inputpred.store( );

	int ticks = 0;
	while ( ticks <= g_cl.m_max_lag )
	{
		if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) <= 0.1f )
			break;

		QuickStop( g_cl.m_cmd );

		// keep predicting until we stop moving.
		g_inputpred.predict( );
		++ticks;
	}

	if ( ticks == 0 )
		m_fakewalking = true;

	m_fake_walk_ticks_left = ticks;

	g_inputpred.SetDatamap( g_inputpred.cm_start );
	g_inputpred.restore( );
	*g_cl.m_cmd = bak_usrcmd;

	// we just want the stop ticks here.
	if ( !fakewalking )
		return;

	const float curtime = g_cl.m_local->m_nTickBase( ) * g_csgo.m_globals->m_interval;

	const int ticks_until_upd = game::TIME_TO_TICKS( g_cl.m_body_pred - curtime );
	const int ticks_after_upd = game::TIME_TO_TICKS( curtime - ( g_cl.m_body_pred - 1.1f ) );

	// -1 kinda weird break sometimeszzzzzzzzzz.
	int choke_remaining = ( g_cl.m_max_lag - g_cl.m_lag ) - 2;

	const int max_delay = g_menu.main.movement.max_shot_delay.get( );
	bool limit_speed = max_delay > 0 && ticks > max_delay;

	if ( ticks >= choke_remaining
		|| ( ( ticks_until_upd <= choke_remaining || ticks_after_upd <= 1 ) && m_fakewalk_lby_stop )
		|| !g_cl.m_lag
		|| g_movement.m_stop_the_fakewalk
		|| !m_fakewalking
		|| limit_speed )
	{
		QuickStop( g_cl.m_cmd );
	}
}