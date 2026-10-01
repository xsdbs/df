#include "includes.h"

HVH g_hvh { };;

void HVH::InstantYawUpdate( )
{
	if ( !g_menu.main.antiaim.instant_yaw_update.get( ) || g_cl.m_shot || !g_cl.m_lag )
		return;

	int last_cmd = ( g_cl.m_cmd->m_command_number - g_cl.m_lag ) % 150;

	//if ( m_update_command[ last_cmd ] /*|| g_aimbot.m_shot_command[ last_cmd ]*/ )
	//	return;

	CUserCmd* cmd = &g_csgo.m_input->m_commands[ last_cmd ];
	if ( !cmd )
		return;

	CVerifiedUserCmd* verified = &g_csgo.m_input->m_verified_commands[ last_cmd ];
	if ( !verified )
		return;

	if ( ( cmd->m_buttons & IN_ATTACK ) != 0
		|| g_cl.m_weapon_type == REVOLVER
		|| cmd->m_buttons != verified->m_cmd.m_buttons
		|| cmd->m_view_angles.y == g_cl.m_cmd->m_view_angles.y )
		return;

	m_update_command[ last_cmd ] = true;

	cmd->m_view_angles.y = g_cl.m_cmd->m_view_angles.y;
	verified->m_cmd.m_view_angles.y = g_cl.m_cmd->m_view_angles.y;

	static auto cl_showerror = g_csgo.m_cvar->FindVar( HASH( "cl_showerror" ) );
	if ( cl_showerror->GetInt( ) > 0 ) {
		g_cl.print( tfm::format( "yaw updated %2.f %d\n",
			std::fabs( math::AngleDiff( cmd->m_view_angles.y, g_cl.m_cmd->m_view_angles.y ) ),
			g_cl.m_lag ) );
	}

	g_csgo.m_prediction->servercmds( ) = INT_MAX;
	g_inputpred.update( );
}

void HVH::ManualAntiaim( ) {
	if ( !m_should_listen )
		return;

	static int press_count = 0;

	// map keys direction
	const std::pair<int, int> keymap[ ] = {
		{ g_menu.main.antiaim.manual_left.get( ),   LEFT },
		{ g_menu.main.antiaim.manual_right.get( ),  RIGHT },
		{ g_menu.main.antiaim.manual_back.get( ),   BACK },
		{ g_menu.main.antiaim.manual_front.get( ),  FORWARD }
	};

	// see if a manual key was pressed
	for ( auto [key, dir] : keymap ) {
		if ( GetAsyncKeyState( key ) & 1 ) {
			m_queued_dir = dir;
			m_last_press_time = g_csgo.m_globals->m_realtime;
			++press_count;
			break;
		}
	}

	// process queued direction
	if ( m_queued_dir > NONE &&
		( g_csgo.m_globals->m_realtime - m_last_press_time >= game::TICKS_TO_TIME( 12 )
			|| press_count > 1
			|| !g_menu.main.antiaim.manual_lastmoving.get( ) ) )
	{
		if ( press_count > 1 && g_menu.main.antiaim.manual_lastmoving.get( ) )
		{
			m_manual_lastmove = m_queued_dir;
		}
		else
		{
			if ( m_manual_direction == m_queued_dir || m_manual_lastmove == m_queued_dir )
				m_manual_direction = NONE;
			else
				m_manual_direction = m_queued_dir;

			m_manual_lastmove = NONE;
		}

		press_count = 0;
		m_queued_dir = NONE;
	}
}

float HVH::DynamicFreestand( Player* target, std::vector<AdaptiveAngle> angles, bool get_worst )
{
	if ( !target )
		return angles[ 0 ].m_yaw;

	constexpr float STEP = 4.f;

	vec3_t start = target->GetShootPosition( );

	for ( auto it = angles.begin( ); it != angles.end( ); ++it )
	{
		vec3_t end { g_cl.m_shoot_pos.x + std::cos( math::deg_to_rad( it->m_yaw ) ) * it->m_range,
			g_cl.m_shoot_pos.y + std::sin( math::deg_to_rad( it->m_yaw ) ) * it->m_range,
			g_cl.m_shoot_pos.z + 16.f };

		//g_csgo.m_debug_overlay->AddLineOverlay(start, end, 0, 0, 255, true, 0.1f);

		vec3_t dir = end - start;
		float len = dir.normalize( );
		if ( len <= 0.f )
			continue;

		// step thru the total distance, 4 units per step.
		float i = 0.f;
		for ( ; i < len; i += STEP )
		{
			int contents = g_csgo.m_engine_trace->GetPointContents( start + ( dir * i ), MASK_SHOT_HULL );
			if ( !( contents & MASK_SHOT_HULL ) )
				continue;

			it->m_dist += STEP;
		}

		vec3_t endpos = start + ( dir * i );
		if ( endpos.dist_to( start ) < endpos.dist_to( end ) )
		{
			it->m_dist = std::numeric_limits<float>::max( );
		}
	}

	// put the most distance at the front of the container.
	std::sort( angles.begin( ), angles.end( ), [ ]( const AdaptiveAngle& a, const AdaptiveAngle& b )
		{
			return a.m_dist > b.m_dist;
		} );

	return get_worst ? angles.back( ).m_yaw : angles.front( ).m_yaw;
}

void HVH::SetFreestandInfo( Player* target )
{
	constexpr float STEP = 4.f;

	static std::vector< AdaptiveAngle > angles;
	angles.clear( );

	angles.emplace_back( m_view + 90.f, 48.f );
	angles.emplace_back( m_view - 90.f, 48.f );

	angles.emplace_back( m_view + 90.f, 16.f );
	angles.emplace_back( m_view - 90.f, 16.f );

	vec3_t start = target->GetShootPosition( );

	for ( auto it = angles.begin( ); it != angles.end( ); ++it )
	{
		vec3_t end { g_cl.m_shoot_pos.x + std::cos( math::deg_to_rad( it->m_yaw ) ) * it->m_range,
			g_cl.m_shoot_pos.y + std::sin( math::deg_to_rad( it->m_yaw ) ) * it->m_range,
			g_cl.m_shoot_pos.z + 16.f };

		//g_csgo.m_debug_overlay->AddLineOverlay(start, end, 0, 0, 255, true, 0.1f);

		vec3_t dir = end - start;
		float len = dir.normalize( );
		if ( len <= 0.f )
			continue;

		// step thru the total distance, 4 units per step.
		float i = 0.f;
		for ( ; i < len; i += STEP )
		{
			int contents = g_csgo.m_engine_trace->GetPointContents( start + ( dir * i ), MASK_SHOT_HULL );
			if ( !( contents & MASK_SHOT_HULL ) )
				continue;

			it->m_dist += STEP;
		}

		vec3_t endpos = start + ( dir * i );
		if ( endpos.dist_to( start ) < endpos.dist_to( end ) )
		{
			it->m_dist = std::numeric_limits<float>::max( );
		}
	}

	m_left_dist = angles[ 0 ].m_dist;
	m_right_dist = angles[ 1 ].m_dist;

	m_left_close_dist = angles[ 2 ].m_dist;
	m_right_close_dist = angles[ 3 ].m_dist;
}

Player* HVH::GetFOVTarget( )
{
	Player* best_player = nullptr;

	float best_fov = 180.f;

	for ( int i { 1 }; i <= g_csgo.m_globals->m_max_clients; ++i )
	{
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >( i );
		if ( !g_aimbot.IsValidTarget( player ) )
			continue;

		float fov = math::GetFOV( g_cl.m_view_angles, g_cl.m_shoot_pos, player->WorldSpaceCenter( ) );
		if ( player->dormant( ) && fov > 45.f )
			continue;

		if ( fov < best_fov )
		{
			best_player = player;
			best_fov = fov;
		}
	}

	return best_player;
}

bool HVH::IsYawSideways( vec3_t from, vec3_t to, float yaw )
{
	float base = math::CalcAngle( from, to ).y;
	float front_delta = std::min( abs( math::NormalizedAngle( yaw - ( base + 180.f ) ) ), abs( math::NormalizedAngle( yaw - base ) ) );
	return front_delta > 45.f;
}

void HVH::ApplySupression( )
{
	// dont need to check activiation, only gets called when we want it.

	// could check this to only flick to 1 side but its wtv
	//float diff = math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - m_lastmove_yaw );

	const int ticks_after_upd = game::TIME_TO_TICKS( g_csgo.m_globals->m_curtime - ( g_cl.m_body_pred - 1.1f ) );
	const int ticks_until_upd = game::TIME_TO_TICKS( g_cl.m_body_pred - g_csgo.m_globals->m_curtime );

	if ( m_manual_direction > NONE )
	{
		if ( ticks_after_upd <= 4 || ticks_until_upd <= 4 )
		{
			g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw + 180.f;
		}
	}

	if ( ticks_after_upd <= 2 )
	{
		g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw - 90.f;
		g_cl.m_hide_angle = 1;
		return;
	}

	if ( ticks_until_upd <= 2 )
	{
		g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw + 90.f;
		g_cl.m_hide_angle = 1;
	}
}

void HVH::DoDesyncJitter( ) {
	if ( !g_cl.m_processing )
		return;

	if ( !m_desync )
		return;

	CCSGOPlayerAnimState* state = g_cl.m_local->m_PlayerAnimState( );
	if ( !state )
		return;

	static int ticks_landed = 0;

	// only handle walk mode
	if ( m_mode != WALK ) {
		if ( m_mode == AIR )
			ticks_landed = 0; // reset when airborne

		return;
	}

	// early twist logic during landing
	if ( ticks_landed < 2 && m_manual_direction != LEFT && m_manual_direction != RIGHT ) {
		switch ( ticks_landed ) {
		case 0:
			g_cl.m_cmd->m_view_angles.y = ( ( g_cl.m_abs_yaw + g_server_animations.m_fake_feet_yaw ) * 0.5f ) + 180.f;
			break;

		case 1:
			g_cl.m_cmd->m_view_angles.y = ( g_cl.m_abs_yaw + g_server_animations.m_fake_feet_yaw ) * 0.5f;
			m_twist_lag = 1; // wow lag
			break;
		}

		++ticks_landed;
		return;
	}

	// standing still meme
	if ( !g_cl.m_pressing_move && g_cl.m_local->m_vecVelocity( ).length_2d( ) < 5.0f ) {
		g_cl.m_cmd->m_view_angles.y = m_view + 180.f + ( ( g_cl.m_abs_yaw - g_server_animations.m_fake_feet_yaw > 0.0f ) ? -120.f : 120.f );
		g_cl.m_hide_angle = 1;
		return;
	}

	const float delta = math::NormalizedAngle( g_cl.m_abs_yaw - g_server_animations.m_fake_feet_yaw );
	if ( std::abs( delta ) < 10.f )
		return; // skip small desync deltas

	const float approach = state->m_last_update_increment * ( 30.0f + 20.0f * state->m_walk_run_transition );

	float abs_fake_delta = std::clamp( std::fabs( delta ), 0.f, 116.f * g_cl.m_pred_aim_matrix_width_range );
	abs_fake_delta = std::max( abs_fake_delta - ( approach * 2.f ), 0.f );

	if ( abs_fake_delta <= 58.f ) {
		g_cl.m_cmd->m_view_angles.y = math::ApproachAngle( g_cl.m_abs_yaw, g_server_animations.m_fake_feet_yaw, abs_fake_delta / 1.5f ) + 180.f;
		return;
	}

	float average_yaw = math::ApproachAngle( g_cl.m_abs_yaw, g_server_animations.m_fake_feet_yaw, std::fabs( delta / 2.f ) );

	if ( std::fabs( math::NormalizedAngle( m_view + 180.f - average_yaw ) ) > 90.f )
		average_yaw -= 180.f;

	math::NormalizeAngle( average_yaw );

	g_cl.m_cmd->m_view_angles.y = math::ApproachAngle( m_view + 180.f, average_yaw, 29.f * g_server_animations.m_fake_feet_yaw );
}

void HVH::ApplyTwist( )
{
	if ( !g_menu.main.antiaim.twist_move.get( 0 ) )
		return; // wow such twist off

	if ( m_desync )
		return;

	static int ticks_landed;

	if ( m_mode != WALK )
	{
		if ( m_mode == AIR )
			ticks_landed = 0; // reset air, brohio

		return;
	}

	// early twist during landing
	if ( ticks_landed < 2 && m_manual_direction != LEFT && m_manual_direction != RIGHT )
	{
		switch ( ticks_landed )
		{
		case 0:
			g_cl.m_cmd->m_view_angles.y = ( ( g_cl.m_abs_yaw + g_server_animations.m_fake_feet_yaw ) * 0.5f ) + 180.f; // yeet first tick
			break;
		case 1:
			g_cl.m_cmd->m_view_angles.y = ( g_cl.m_abs_yaw + g_server_animations.m_fake_feet_yaw ) * 0.5f; // brohio second tick
			m_twist_lag = 1; // wow lag
			break;
		}
		++ticks_landed;
		return;
	}

	// calculate delta between eye and feet
	const float delta = math::NormalizedAngle( g_cl.m_abs_yaw - g_server_animations.m_fake_feet_yaw );
	if ( abs( delta ) < 10.f )
		return; // no need to twist, bruh

	bool in_use = ( g_cl.m_cmd->m_buttons & IN_USE );

	static float target_angle;
	float angle_1337 = in_use ? m_view : m_view + 180.f; // brohio default angle

	// respect manual direction
	if ( m_manual_direction > NONE )
		target_angle = angle_1337 = g_cl.m_cmd->m_view_angles.y; // yeet manual dir
	else if ( g_menu.main.antiaim.twist_move.get( 3 ) )
	{
		if ( m_left_dist != m_right_dist )
		{
			if ( m_left_dist > m_right_dist )
				angle_1337 -= abs( delta * 0.5f ); // tilt left, wow
			else
				angle_1337 += abs( delta * 0.5f ); // tilt right, bruh
		}
	}

	// lock target angle if threshold exceeded
	if ( abs( math::NormalizedAngle( target_angle - angle_1337 ) ) > g_menu.main.antiaim.lock_threshold.get( ) )
		target_angle = angle_1337; // brohio lock

	// eye-foot delta
	float flEyeFootDelta = math::NormalizedAngle( target_angle - g_cl.m_abs_yaw );
	if ( abs( flEyeFootDelta ) > 105.f )
		return; // too much delta, yeet

	float width = g_cl.m_pred_aim_matrix_width_range - 1.f;

	float enemy_min = g_server_animations.m_fake_feet_yaw - width;
	float enemy_max = g_server_animations.m_fake_feet_yaw + width;
	float local_min = g_cl.m_abs_yaw - width;
	float local_max = g_cl.m_abs_yaw + width;

	// twist logic
	if ( delta > 0.f )
		g_cl.m_cmd->m_view_angles.y = flEyeFootDelta > 0.f ? enemy_max : local_min; // brohio twist right
	else
		g_cl.m_cmd->m_view_angles.y = flEyeFootDelta > 0.f ? local_max : enemy_min; // brohio twist left

	// set twist lag
	if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) > 0.1f && abs( delta ) > g_menu.main.antiaim.minimum_desync.get( ) )
		m_twist_lag = g_menu.main.antiaim.twist_move.get( 2 ) ? 1 : 0; // yeet lag
	else
		m_twist_lag = 0; // no lag, bruh
}

void HVH::ApplyFlick( )
{
	static float flick_angle;

	if ( g_movement.m_transition_flick )
	{
		// first part is to flick and update lby so the enemy sees this.
		if ( !g_movement.m_transition_stage.first )
		{
			float freestand = DynamicFreestand( m_best_target, { g_cl.m_abs_yaw + g_cl.m_pred_aim_matrix_width_range, g_cl.m_abs_yaw - g_cl.m_pred_aim_matrix_width_range } ); // meme?

			*g_cl.m_packet = true;

			g_cl.m_cmd->m_view_angles.y = flick_angle = freestand;
			g_movement.m_transition_stage.first = true;

			g_cl.m_hide_angle = 2;
		}
		else if ( !g_movement.m_transition_stage.second )
		{
			*g_cl.m_packet = false;

			g_cl.m_cmd->m_view_angles.y = flick_angle + 180.f;
			g_movement.m_transition_stage.second = true;

			g_cl.m_hide_angle = 1;
		}
		else
		{
			g_movement.m_transition_reset = false;
		}
	}

	if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) <= 0.1f && !g_cl.m_pressing_move )
	{
		g_movement.m_transition_reset = true;
	}
}

void HVH::AntiAim( )
{
	if ( !g_menu.main.antiaim.enable.get( ) )
		return;

	ManualAntiaim( );

	if ( g_menu.main.antiaim.misc.get( 0 ) && m_best_target )
	{
		m_view = math::CalcAngle( g_cl.m_local->m_vecOrigin( ), m_best_target->m_vecOrigin( ) ).y;
	}
	else
		m_view = g_cl.m_cmd->m_view_angles.y;

	m_fresh_jump = false;

	if ( ( ( g_cl.m_buttons & IN_JUMP ) || !( g_cl.m_flags & FL_ONGROUND ) ) && m_mode != AntiAimMode::AIR ) // on the first tick we should send to get rid of our lastmove being the angle we update in-air with
		m_fresh_jump = true;

	m_mode = AntiAimMode::STAND;

	if ( ( g_cl.m_buttons & IN_JUMP ) || !( g_cl.m_flags & FL_ONGROUND ) ) {
		m_mode = AntiAimMode::AIR;
		m_lby_timing_broken = false;
	}
	else if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) > 0.1f && !g_movement.m_fakewalking )
	{
		m_mode = AntiAimMode::WALK;
		m_lby_timing_broken = false;
	}

	// disable conditions.
	if ( g_cl.m_local->m_MoveType( ) == MOVETYPE_LADDER || g_cl.m_local->m_MoveType( ) == MOVETYPE_NOCLIP )
		return;

	// grenade throwing
	// CBaseCSGrenade::ItemPostFrame()
	// https://github.com/VSES/SourceEngine2007/blob/master/src_main/game/shared/cstrike/weapon_basecsgrenade.cpp#L209
	if ( g_cl.m_weapon_type == WEAPONTYPE_GRENADE && !g_cl.m_weapon->m_bPinPulled( )
		&& g_csgo.m_globals->m_curtime - g_cl.m_weapon->m_fThrowTime( ) <= ( g_cl.m_latency * 0.5f ) + g_csgo.m_globals->m_interval )
	{
		*g_cl.m_packet = true;
		return;
	}

	if ( ( g_cl.m_round_end && !g_menu.main.antiaim.misc.get( 3 ) )
		|| ( g_csgo.m_gamerules->m_bFreezePeriod( ) && !g_menu.main.antiaim.misc.get( 4 ) ) )
	{
		g_movement.m_fakewalk_lby_stop = false;
		g_cl.m_hide_angle = 0;
		return;
	}

	if ( g_cl.m_weapon && g_cl.m_weapon_fire )
	{
		bool knife = g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS;

		const bool attack = g_cl.m_cmd->m_buttons & IN_ATTACK;
		const bool attack2 = g_cl.m_cmd->m_buttons & IN_ATTACK2;

		// Hi we are not going to be manual firing with the R8 Revolver
		if ( g_cl.m_weapon_id != REVOLVER && ( attack || ( attack2 && knife ) ) )
			return;
	}

	bool in_use = g_cl.m_cmd->m_buttons & IN_USE;

	if ( in_use && !g_menu.main.antiaim.misc.get( 5 ) )
	{
		g_movement.m_fakewalk_lby_stop = false;
		g_cl.m_hide_angle = 0;
		return;
	}

	if ( !in_use )
	{
		switch ( g_menu.main.antiaim.pitch.get( ) )
		{
		case 0:
			g_cl.m_cmd->m_view_angles.x = 89.f;
			break;
		case 1:
			g_cl.m_cmd->m_view_angles.x = -89.f;
			break;
		case 2:
			g_cl.m_cmd->m_view_angles.x = 0.f;
			break;
		}
	}

	if ( *g_cl.m_packet )
	{
		if ( in_use )
		{
			g_cl.m_cmd->m_view_angles.y = m_view;
		}
		else
		{
			if ( m_mode == STAND && g_menu.main.antiaim.fake_body_flags.get( 0 ) )
			{
				if ( g_cl.m_enemy_body_pred > g_cl.m_body_pred )
				{
					g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw;
				}
				else
				{
					g_cl.m_cmd->m_view_angles.y = DynamicFreestand( m_best_target, { m_view + 180.f, m_view - 90.f, m_view + 90.f }, g_menu.main.antiaim.lby_orientation.get( ) == 0 );

					if ( abs( math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - m_real_angles.y ) ) <= 35.f )
					{
						g_cl.m_cmd->m_view_angles.y += g_csgo.RandomFloat( -90.f, 90.f );
					}
				}
			}
			else
			{
				g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw;
			}
		}

		return;
	}

	if ( g_cl.m_lag > 0 ) {
		return;
	}

	m_twist_lag = 0;

	g_movement.m_fakewalk_lby_stop = false;

	bool skip_normal_aa = false;
	bool skip_twist = false;
	bool disable_manual = false;
	bool skip_desync = false;
	bool supress_979 = false;

	if ( g_menu.main.antiaim.fake_body_flags.get( 0 ) )
	{
		if ( ( g_menu.main.antiaim.fake_body_flags.get( 1 ) || m_manual_lastmove > NONE ) && m_mode == WALK && ( !g_cl.m_pressing_move || g_input_system.GetKeyState( g_menu.main.movement.fakewalk.get( ) ) ) )
		{
			switch ( m_manual_lastmove )
			{
			case NONE:
				if ( g_menu.main.antiaim.lby_side.get( ) )
				{
					g_cl.m_cmd->m_view_angles.y = DynamicFreestand( m_best_target,
						{ m_view - 90.f, m_view + 90.f },
						g_menu.main.antiaim.lby_orientation.get( ) == 0 );
				}
				else
				{
					g_cl.m_cmd->m_view_angles.y = DynamicFreestand( m_best_target,
						{ g_cl.m_abs_yaw + g_cl.m_pred_aim_matrix_width_range, g_cl.m_abs_yaw - g_cl.m_pred_aim_matrix_width_range },
						g_menu.main.antiaim.lby_orientation.get( ) == 0 );
				}
				break;
			case BACK:
				g_cl.m_cmd->m_view_angles.y = m_view + 180.f;
				break;
			case RIGHT:
				g_cl.m_cmd->m_view_angles.y = m_view - 90.f;
				break;
			case LEFT:
				g_cl.m_cmd->m_view_angles.y = m_view + 90.f;
				break;
			case FORWARD:
				g_cl.m_cmd->m_view_angles.y = m_view;
				break;
			}

			g_cl.m_hide_angle = 1;

			// test, make head stick out less.
			//g_cl.m_cmd->m_view_angles.x = -89.f;

			skip_normal_aa = true;
			skip_desync = true;
			skip_twist = true;
			disable_manual = true;
		}
		else if ( m_mode == STAND )
		{
			/*
				m_flFootYaw = ApproachAngle( m_pPlayer->m_flLowerBodyYawTarget.Get(), m_flFootYaw, m_flLastUpdateIncrement * CSGO_ANIM_LOWER_CATCHUP_IDLE );

				What i want:
				Have footyaw and eyeyaw within 35 degrees on update tick so we can delay it.
				Then have lby flick the next tick, without changing m_flLowerBodyYawTarget value.

				How to do it:
				m_flFootYaw would need to be far away enough from our goal lby angle on the two important ticks (update tick and the one after)
				so that we can set our eyeangles to footyaw on the update tick, and then flick the tick after.
			*/

			//m_lby_timing_broken = abs( g_cl.m_enemy_body_pred - g_cl.m_body_pred ) >= 4 * g_csgo.m_globals->m_interval;

			g_movement.m_fakewalk_lby_stop = true;

			if ( !m_lby_timing_broken )
			{
				if ( g_csgo.m_globals->m_curtime > g_cl.m_body_pred + ( g_csgo.m_globals->m_interval * 4 ) )
				{
					g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw;
					m_lby_timing_broken = true;
					g_cl.m_hide_angle = 2;
				}
				else if ( g_csgo.m_globals->m_curtime > g_cl.m_body_pred )
				{
					g_cl.m_cmd->m_view_angles.y = g_cl.m_abs_yaw;
					g_cl.m_hide_angle = 1;
				}
				else
				{
					g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw + 180.f;
					g_cl.m_hide_angle = 1;
				}

				disable_manual = true;
			}
			else if ( g_csgo.m_globals->m_curtime > g_cl.m_body_pred )
			{
				g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw;
				g_cl.m_hide_angle = 2;
				disable_manual = true;
			}
			else if ( !g_menu.main.antiaim.lby_breaker_type.get( ) )
			{
				if ( g_csgo.m_globals->m_curtime >= g_cl.m_body_pred - ( g_csgo.m_globals->m_interval * 2 ) )
				{
					g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw + 180.f;
					g_cl.m_hide_angle = 1;
					disable_manual = true;
				}
				else
				{
					float back = in_use ? m_view : m_view + 180.f;
					float lby_side = math::NormalizedAngle( back - m_lastmove_yaw );

					float add = ( g_menu.main.antiaim.angle_preference.get( ) && !in_use ) ? 30.f : 45.f;

					g_cl.m_cmd->m_view_angles.y = lby_side > 0.f ? back + add : back - add;
				}
			}
			else
			{
				g_cl.m_cmd->m_view_angles.y = m_lastmove_yaw + 180.f;
				supress_979 = true;
			}

			skip_normal_aa = true;
		}
	}

	if ( !skip_normal_aa )
	{
		if ( m_best_target )
		{
			SetFreestandInfo( m_best_target );
		}
		else
			m_left_dist = m_left_close_dist = m_right_dist = m_right_close_dist = 0.f;

		const float min_dist = ( m_left_dist + m_right_dist ) * 0.75f;

		if ( g_menu.main.antiaim.misc.get( 2 ) && !( g_cl.m_flags & FL_ONGROUND ) )
		{
			g_cl.m_cmd->m_view_angles.y = m_view + 180.f;
			vec3_t vel = g_cl.m_local->m_vecVelocity( );
			float velyaw = math::rad_to_deg( std::atan2( vel.y, vel.x ) );

			if ( abs( math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - ( m_view + 180 ) ) ) <= 45.f
				|| abs( math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - g_cl.m_local->m_flLowerBodyYawTarget( ) ) ) <= 45.f
				|| abs( math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - m_lastmove_yaw ) ) <= 45.f
				|| abs( math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - velyaw ) ) <= 45.f )
			{
				// using outgoing instead of g_cl.m_cmd number to fix jitter with anim smoothing.
				g_cl.m_cmd->m_view_angles.y += g_csgo.m_cl->m_last_outgoing_command % 2 == 0 ? 10.f : -10.f;
			}

			disable_manual = true;
		}
		else if ( in_use )
		{
			g_cl.m_cmd->m_view_angles.y = m_view;
		}
		else if ( g_menu.main.antiaim.misc.get( 1 ) && g_cl.m_cmd->m_buttons & IN_DUCK )
		{
			g_cl.m_cmd->m_view_angles.y = m_view + 180.f;
		}
		else
		{
			switch ( g_menu.main.antiaim.base_yaw.get( ) )
			{
			case 0:
				if ( m_left_dist > min_dist && m_right_close_dist < min_dist )
				{
					g_cl.m_cmd->m_view_angles.y = m_view + 90.f;
				}
				else if ( m_right_dist > min_dist && m_left_close_dist < min_dist )
				{
					g_cl.m_cmd->m_view_angles.y = m_view - 90.f;
				}
				else
				{
					g_cl.m_cmd->m_view_angles.y = m_view + 180.f;
				}

				break;
			case 1:
				g_cl.m_cmd->m_view_angles.y += 180.f;
				break;
			}
		}
	}

	if ( m_manual_direction > NONE && !disable_manual && !in_use )
	{
		switch ( m_manual_direction )
		{
		case FORWARD:
			g_cl.m_cmd->m_view_angles.y = m_view;
			break;
		case LEFT:
			g_cl.m_cmd->m_view_angles.y = m_view + 90.f;
			break;
		case RIGHT:
			g_cl.m_cmd->m_view_angles.y = m_view - 90.f;
			break;
		case BACK:
			g_cl.m_cmd->m_view_angles.y = m_view + 180.f;
			break;
		}
	}

	if ( supress_979 )
		ApplySupression( );

	if ( !skip_desync )
		DoDesyncJitter( );

	if ( !skip_twist )
		ApplyTwist( );

	ApplyFlick( );
}

void HVH::SendPacket( )
{
	if ( !*g_cl.m_final_packet )
		*g_cl.m_packet = false; // bro ohio, no packet yet

	// yepppp... aa runs after, so we get the fov target now
	m_best_target = GetFOVTarget( );

	if ( g_menu.main.antiaim.lag_enable.get( ) )
	{
		int limit = 1; // default tick bro

		// check if we in the air, jump or fallin'
		bool in_air = ( g_cl.m_buttons & IN_JUMP ) || !( g_cl.m_local->m_fFlags( ) & FL_ONGROUND );

		// bro ohio, movement fakelag
		if ( g_menu.main.antiaim.lag_active.get( 1 ) && g_cl.m_local->m_vecVelocity( ).length_2d( ) > 0.1f && !in_air )
		{
			limit = g_movement.m_transition_flick || m_desync ? 1 : g_menu.main.antiaim.lag_limit.get( );
		}
		// bro ohio, air fakelag
		else if ( g_menu.main.antiaim.lag_active.get( 2 ) && in_air )
		{
			limit = g_movement.m_transition_flick ? 1 : g_menu.main.antiaim.lag_limit.get( );
		}
		// duck fakelag, hehe
		else if ( g_menu.main.antiaim.lag_active.get( 3 ) && g_cl.m_local->m_bDucking( ) )
		{
			limit = g_movement.m_transition_flick ? 1 : g_menu.main.antiaim.lag_limit.get( );
		}

		// twisty lag memery
		if ( !g_movement.m_transition_flick )
		{
			switch ( m_twist_lag )
			{
			case 1:
			case 2:
				limit = 1; // single tick gang
				break;
			case 3:
				limit = 14 - int( g_cl.m_view_yaw_change ); // oof, adjust by view yaw
				if ( limit < 1 )
					limit = 1; // no negatives bro
				break;
			}
		}

		*g_cl.m_packet = false; // reset packet

		// adaptive fakelag gang
		if ( g_menu.main.antiaim.lag_type.get( ) != 0 )
		{
			vec3_t cur = g_cl.m_local->m_vecOrigin( );
			vec3_t prev = g_cl.m_net_pos.empty( ) ? g_cl.m_local->m_vecOrigin( ) : g_cl.m_net_pos.front( ).m_pos;

			if ( m_mode == AIR )
			{
				if ( m_updates_cycle > 3 )
					m_updates_cycle = 0;

				switch ( m_updates_cycle )
				{
				case 0:
					limit = 4096.f / g_cl.m_local->m_vecVelocity( ).length_sqr( ); // bro air physics
					break;
				case 1:
					limit = 13; break; // lol fixed tick
				case 2:
					limit = 2; break; // tiny tick, hehe
				}
			}
		}
		else
		{
			vec3_t cur = g_cl.m_local->m_vecOrigin( );
			vec3_t prev = g_cl.m_net_pos.empty( ) ? g_cl.m_local->m_vecOrigin( ) : g_cl.m_net_pos.front( ).m_pos;

			if ( ( cur - prev ).length_sqr( ) > 4096.0f )
				*g_cl.m_packet = true; // bro ohio, big move
		}

		// peek / fakewalk checks
		if ( m_mode == WALK && ( !g_cl.m_pressing_move || g_input_system.GetKeyState( g_menu.main.movement.fakewalk.get( ) ) )
			&& g_menu.main.antiaim.fake_body_flags.get( 0 ) && !g_cl.m_old_packet )
		{
			limit = std::min( g_cl.m_max_lag - 1, g_movement.m_fake_walk_ticks_left - 1 ); // fakewalk memery
		}

		// stand lby logic
		if ( g_menu.main.antiaim.fake_body_flags.get( 0 ) && g_menu.main.antiaim.lag_active.get( 0 ) && m_mode == STAND && !g_cl.m_pressing_move )
		{
			int lby_limit = g_menu.main.antiaim.lag_limit.get( );

			if ( lby_limit > 1 && m_lby_timing_broken )
			{
				if ( g_movement.m_fakewalk_lby_stop )
				{
					const int ticks_until_upd = game::TIME_TO_TICKS( g_cl.m_body_pred - g_csgo.m_globals->m_curtime );
					const int ticks_after_upd = game::TIME_TO_TICKS( g_csgo.m_globals->m_curtime - ( g_cl.m_body_pred - 1.1f ) );

					if ( ticks_until_upd > 1 && ticks_after_upd > 1 )
					{
						*g_cl.m_packet = false; // broke lby memery
						limit = lby_limit;
					}
				}
				else
				{
					*g_cl.m_packet = false; // no packet gang
					limit = lby_limit;
				}
			}
		}

		if ( g_cl.m_lag >= limit )
			*g_cl.m_packet = true; // finally send it bro
	}

	// if we just shot, send packet unless forced peek
	if ( g_cl.m_old_shot )
	{
		if ( g_aimbot.m_skip_hold_fakelag || !g_menu.main.antiaim.lag_active.get( 5 ) )
			*g_cl.m_packet = true; // shot gang
	}

	// fakewalk continuation
	if ( g_movement.m_fakewalking )
	{
		if ( g_movement.m_fake_walk_ticks_left > 0 )
			*g_cl.m_packet = false; // keep fakewalking bro
		else if ( g_movement.m_stop_the_fakewalk && !g_cl.m_old_packet )
		{
			*g_cl.m_packet = true; // stop fakewalk now
			g_movement.m_stop_the_fakewalk = false;
		}
	}

	// max lag or fresh jump = send packet
	if ( g_cl.m_lag >= g_cl.m_max_lag || m_fresh_jump )
	{
		*g_cl.m_packet = true;
		g_cl.m_weapon_fire = false; // no fire memery
	}

	if ( *g_cl.m_packet )
		m_updates_cycle++; // bro ohio, increment cycle
}

void HVH::CanHit( ) {
	return;
	if ( g_cl.m_sorted_players.empty( ) )
		return;

	for ( auto id : g_cl.m_sorted_players ) {
		auto player = &g_aimbot.m_players[ id - 1 ];
		if ( !player->m_entity )
			continue;
	}
}