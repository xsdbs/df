#include "includes.h"

Animations g_lagcompensation {};

void LagCompensation::MatchShot( LagRecord& record, LagRecord* previous ) {
	if ( !previous )
		return;

	Weapon* weapon = m_player->GetActiveWeapon( );
	if ( !weapon )
		return;

	WeaponInfo* weapon_info = weapon->GetWpnData( );
	if ( !weapon_info )
		return;

	const int type = weapon_info->m_weapon_type;
	if ( type == WEAPONTYPE_C4 || type == WEAPONTYPE_GRENADE || type == WEAPONTYPE_KNIFE )
		return;

	const float shot_time = weapon->m_fLastShotTime( );
	if ( shot_time == m_last_shot_time )
		return;

	if ( shot_time - previous->m_sim_time > g_csgo.m_globals->m_interval ) {
		for ( auto& rec : m_records ) {
			if ( std::abs( math::NormalizedAngle( rec.m_eye_angles.x - record.m_eye_angles.x ) ) > 0.1f ) {
				record.m_eye_angles.x = rec.m_eye_angles.x;
				break;
			}
		}
	}

	m_last_shot_time = shot_time;
}

bool LagCompensation::EstimateVelocity( Player* m_player, LagRecord& record, LagRecord* previous, LagRecord* pre_previous, float max_speed )
{
	// set a base velocity estimate at animtime.
	auto accel = ( ( record.m_velocity - previous->m_velocity ) / game::TICKS_TO_TIME( record.m_lag ) ) * g_csgo.m_globals->m_interval;
	record.m_anim_velocity = previous->m_velocity + accel;

	if ( !( record.m_flags & FL_ONGROUND ) )
		return false;

	// both speeds use 2d velocity.
	const float average_speed = record.m_anim_velocity.length_2d( );
	const auto* l6 = &record.m_layers[ 6 ];
	const auto* l11 = &record.m_layers[ 11 ];

	// layer6 is clamped to WalkToTopSpeed (not run), so we only want to use it < 1.f weight.
	// we also already check if its over 0 in the fakewalk check.
	// also if we dont check > prev then ppl go invisible when they deaccelerate LOL
	if ( l6->m_weight < 1.f && l6->m_weight > previous->m_layers[ 6 ].m_weight )
	{
		auto closest_speed_to_minimal = max_speed * 0.34f;
		auto spd_multiplier = std::fmaxf( 0.f, ( max_speed * 0.52f ) - ( max_speed * 0.34f ) );

		auto v208 = 1.f - record.m_duck;

		auto speed_via_6th_layer = ( ( ( v208 * spd_multiplier ) + closest_speed_to_minimal ) * l6->m_weight ) / average_speed;

		record.m_anim_velocity *= speed_via_6th_layer;
		record.m_anim_velocity *= speed_via_6th_layer;
		return true;
	}

	// this logic should (?) work and make sense and be nice and happy
	// also run this after cuz if l6 weight < 1 then we know for sure they aint static velocity.
	if ( !record.m_first_out_of_pvs
		&& !previous->m_first_out_of_pvs
		&& pre_previous && ( record.m_origin - previous->m_origin ) == ( previous->m_origin - pre_previous->m_origin ) )
		return true;

	if ( l11->m_weight > 0.f
		&& l11->m_weight < 1.f
		&& l11->m_playback_rate == previous->m_layers[ 11 ].m_playback_rate )
	{
		const auto speed_as_portion_of_run_top_speed = 0.55f + ( 1.f - l11->m_weight ) * 0.35f;
		const auto velocity_length_xy = speed_as_portion_of_run_top_speed * max_speed;

		record.m_anim_velocity.x *= ( velocity_length_xy / average_speed );
		record.m_anim_velocity.y *= ( velocity_length_xy / average_speed );
		return true;
	}

	return false;
}

void LagCompensation::UpdateAnimations( LagRecord& record )
{
	CCSGOPlayerAnimState* state = m_player->m_PlayerAnimState( );
	if ( !state )
		return;

	if ( m_player->m_flSpawnTime( ) != m_spawn )
	{
		game::ResetAnimationState( state );
		m_spawn = m_player->m_flSpawnTime( );
	}

	const float curtime = g_csgo.m_globals->m_curtime;
	const float frametime = g_csgo.m_globals->m_frametime;

	LagRecord* previous = m_records.size( ) > 1 ? &m_records[ 1 ] : nullptr;
	LagRecord* pre_previous = m_records.size( ) > 2 ? &m_records[ 2 ] : nullptr;

	g_csgo.m_globals->m_curtime = record.m_anim_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	m_backup.store( m_player );

	record.m_broke_lc = false;
	record.m_fake_walk = false;
	record.m_mode = Resolver::Modes::RESOLVE_NONE;

	// https://github.com/VSES/SourceEngine2007/blob/master/se2007/game/client/c_baseplayer.cpp#L659
	if ( record.m_lag > 0 || m_went_dormant )
	{
		if ( previous && !m_went_dormant )
		{
			record.m_velocity = ( record.m_origin - previous->m_origin ) * ( 1.f / game::TICKS_TO_TIME( record.m_lag ) );

			// restore previous animation data, makes sure our new anim update is accurate.
			m_player->SetAnimLayers( previous->m_layers );

			state->m_primary_cycle = previous->m_layers[ 6 ].m_cycle;
			state->m_move_weight = previous->m_layers[ 6 ].m_weight;
			state->m_strafe_sequence = previous->m_layers[ 7 ].m_sequence;
			state->m_strafe_change_weight = previous->m_layers[ 7 ].m_weight;
			state->m_strafe_change_cycle = previous->m_layers[ 7 ].m_cycle;
			state->m_acceleration_weight = previous->m_layers[ 12 ].m_weight;

			record.m_broke_lc = ( record.m_origin - previous->m_origin ).length_sqr( ) > 4096.0f;
		}
		// bro was dormant he coulda desync walked.
		else
		{
			m_detected_desync = false;

			// could be lower than 58 here,
			// depending on velocity etc.
			m_max_feet_yaw = record.m_body + 58.f;
			m_min_feet_yaw = record.m_body - 58.f;
		}
	}

	record.m_anim_velocity = record.m_velocity;

	if ( record.m_lag > 1 )
	{
		if ( record.m_flags & FL_ONGROUND && record.m_layers[ 6 ].m_weight == 0.f )
		{
			record.m_fake_walk = true;
			record.m_anim_velocity = record.m_velocity = { 0.f, 0.f, 0.f };
		}

		if ( previous && !m_went_dormant )
		{
			m_player->m_fFlags( ) = previous->m_flags;
			m_player->m_fFlags( ) &= ~FL_ONGROUND;

			if ( record.m_flags & FL_ONGROUND && previous->m_flags & FL_ONGROUND )
			{
				m_player->m_fFlags( ) |= FL_ONGROUND;
			}
			else
			{
				if ( record.m_layers[ 4 ].m_weight != 1.f && previous->m_layers[ 4 ].m_weight == 1.f && record.m_layers[ 5 ].m_weight != 0.f )
				{
					m_player->m_fFlags( ) |= FL_ONGROUND;
				}
				if ( record.m_flags & FL_ONGROUND && !( previous->m_flags & FL_ONGROUND ) )
				{
					m_player->m_fFlags( ) &= ~FL_ONGROUND;
				}
			}

			float duck = record.m_duck - previous->m_duck;
			float time = record.m_sim_time - previous->m_sim_time;

			float change = ( duck / time ) * g_csgo.m_globals->m_interval;

			m_player->m_flDuckAmount( ) = previous->m_duck + change;

			if ( !record.m_fake_walk )
			{
				float max_spd = 260.f;

				auto weapon = m_player->GetActiveWeapon( );
				if ( weapon && !weapon->IsKnife( ) )
				{
					auto wpndata = weapon->GetWpnData( );
					if ( wpndata )
					{
						max_spd = weapon->m_zoomLevel( ) == 0 ? wpndata->m_max_player_speed_alt : wpndata->m_max_player_speed;
					}
				}

				EstimateVelocity( m_player, record, previous, pre_previous, max_spd );

				// this happens very occasionally and makes ppl go yabadabadoo
				// also this is better than just setting it to 0 like the old 1 did (obviously). 
				if ( !record.m_anim_velocity.valid( ) )
				{
					record.m_anim_velocity = record.m_velocity;
				}
			}
		}
	}

	m_player->SetAbsOrigin( record.m_origin );

	m_player->m_vecVelocity( ) = m_player->m_vecAbsVelocity( ) = record.m_anim_velocity;
	m_player->m_iEFlags( ) &= ~0x1000;

	g_resolver.Resolve( this, record );
	MatchShot( record, previous );

	m_player->m_angEyeAngles( ) = record.m_eye_angles;

	record.m_old_state = *state;

	// allow reanimating in the same frame
	if ( state->m_last_update_frame == g_csgo.m_globals->m_frame )
		state->m_last_update_frame = g_csgo.m_globals->m_frame - 1;

	g_hooks.m_animation_setup = true;

	m_player->m_bClientSideAnimation( ) = true;
	g_hooks.m_UpdateClientSideAnimation( m_player );
	m_player->m_bClientSideAnimation( ) = false;

	g_hooks.m_animation_setup = false;

	g_server_animations.SimulateFeetYaw( state, record.m_body, record.m_body/*record.m_eye_angles.y*/, m_max_feet_yaw );
	g_server_animations.SimulateFeetYaw( state, record.m_body, record.m_body/*record.m_eye_angles.y*/, m_min_feet_yaw );

	record.m_max_feet_yaw = m_max_feet_yaw;
	record.m_min_feet_yaw = m_min_feet_yaw;

	if ( !m_went_dormant && m_player->enemy( g_cl.m_local ) && record.m_mode == Resolver::Modes::RESOLVE_MOVE && previous )
	{
		if ( previous->m_mode <= Resolver::Modes::RESOLVE_THRESHOLD )
		{
			record.m_max_feet_yaw = m_max_feet_yaw = record.m_body + 58.f;
			record.m_min_feet_yaw = m_min_feet_yaw = record.m_body - 58.f;
		}
	}
	else
		m_detected_desync = false;

	m_player->GetPoseParameters( record.m_poses );
	record.m_abs_ang = m_player->GetAbsAngles( );

	m_player->SetAnimLayers( record.m_layers ); // bone setup does NOT do this
	g_bones.BuildBones( m_player, BONE_USED_BY_ANYTHING & ~BONE_USED_BY_BONE_MERGE, record.m_bones, record );

	m_backup.restore( m_player );

	g_csgo.m_globals->m_curtime = curtime;
	g_csgo.m_globals->m_frametime = frametime;
}

void LagCompensation::OnNetUpdate( Player* player )
{
	if ( !player->alive( ) )
	{
		player->m_bClientSideAnimation( ) = true;
		reset( );
		return;
	}

	m_get_death_yaw = true;

	if ( !m_player || m_player != player )
	{
		reset( );

		m_shot_log.clear( );
		m_logged_timer = -1;
	}

	m_player = player;

	if ( player->dormant( ) )
	{
		m_went_dormant = true;
		return;
	}

	LagRecord* previous = m_records.empty( ) ? nullptr : &m_records.front( );

	bool updated_on_server = previous == nullptr;
	if ( !updated_on_server ) {
		auto server_layers = player->m_AnimOverlay( );

		// did server animate?
		for ( int i = 0; i < 13; ++i ) {
			const auto& current = server_layers[ i ];
			const auto& old = previous->m_layers[ i ];

			if ( current.m_sequence != old.m_sequence
				|| std::abs( current.m_cycle - old.m_cycle ) > 0.0001f
				|| std::abs( current.m_weight - old.m_weight ) > 0.0001f
				|| std::abs( current.m_playback_rate - old.m_playback_rate ) > 0.0001f ) {
				updated_on_server = true;
				break;
			}
		}
	}

	if ( !updated_on_server ) {
		m_went_dormant = false;
		return;
	}

	LagRecord& current = m_records.emplace_front( );
	current.store( m_player );
	current.m_first_out_of_pvs = m_went_dormant;

	UpdateAnimations( current );

	m_highest_sim_time = std::max( m_highest_sim_time, current.m_sim_time );
	m_went_dormant = false;

	while ( m_records.size( ) > g_cl.m_tickrate )
		m_records.pop_back( );
}