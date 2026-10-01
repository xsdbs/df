#include "includes.h"
#include <unordered_set>

Aimbot g_aimbot {};

void Aimbot::BuildSeedTable( ) {
	static bool built = false;
	if ( built )
		return;

	for ( int i = 0; i < 255; i++ )
	{
		auto& seed = g_aimbot.m_seeds[ i ];

		// seed randomseed.
		g_csgo.RandomSeed( ( i & 0xff ) + 1 );

		// generate needed floats.
		seed.r1 = g_csgo.RandomFloat( 0.f, 1.f );
		seed.r2 = g_csgo.RandomFloat( 0.f, math::pi_2 );
		seed.r3 = g_csgo.RandomFloat( 0.f, 1.f );
		seed.r4 = g_csgo.RandomFloat( 0.f, math::pi_2 );

		// get needed sine / cosine values.
		seed.c1 = std::cos( seed.r2 );
		seed.c2 = std::cos( seed.r4 );
		seed.s1 = std::sin( seed.r2 );
		seed.s2 = std::sin( seed.r4 );
	}

	built = true;
}

void Aimbot::SetupTargets( ) {
	BuildSeedTable( );

	g_cl.m_sorted_players.clear( );
	if ( g_cl.m_unsorted_players.empty( ) )
		return;

	auto reset_hittable = [ ]( AimPlayer* data ) {
		data->m_hittable = false;
		data->m_hittable_tick = 0;
		};

	const vec3_t local_origin = g_cl.m_local->m_vecOrigin( );

	for ( int id : g_cl.m_unsorted_players ) {
		Player* player = g_csgo.m_entlist->GetClientEntity<Player*>( id );
		AimPlayer* data = &m_players[ id - 1 ];

		data->m_lagcomp = nullptr;
		data->m_target = false;
		data->m_broke_lc = false;

		if ( data->m_entity != player )
			reset_hittable( data );

		data->m_entity = player;
		if ( !player || player->m_fImmuneToGunGameDamageTime( ) || !IsValidTarget( player ) || player->dormant( ) ) {
			reset_hittable( data );
			continue;
		}

		data->m_lagcomp = &g_lagcompensation.m_players[ id - 1 ];
		if ( !data->m_lagcomp || data->m_lagcomp->m_records.empty( ) ) {
			reset_hittable( data );
			continue;
		}

		// calculate fov
		data->m_fov = math::GetFOV( g_cl.m_view_angles, g_cl.m_shoot_pos, player->WorldSpaceCenter( ) );
		data->m_origin = player->m_vecOrigin( );
		data->m_hp = player->m_iHealth( );

		if ( data->m_hittable_tick > 0 && std::abs( data->m_hittable_tick - g_csgo.m_globals->m_tick_count ) > 48 )
			reset_hittable( data );

		data->GetScanRecords( );

		if ( !data->m_candidates.empty( ) )
			g_cl.m_sorted_players.push_front( id );
	}

	if ( g_cl.m_sorted_players.empty( ) )
		return;

	// sort descending
	std::sort(
		g_cl.m_sorted_players.begin( ),
		g_cl.m_sorted_players.end( ),
		[ ]( int a, int b )
		{
			const AimPlayer* p1 = &g_aimbot.m_players[ a - 1 ];
			const AimPlayer* p2 = &g_aimbot.m_players[ b - 1 ];

			return p1->m_fov < p2->m_fov;
		}
	);

	const size_t max_targets = g_menu.main.aimbot.optimizations.get( 0 ) ? 2 : 4;
	if ( g_cl.m_sorted_players.size( ) > max_targets )
		g_cl.m_sorted_players.resize( max_targets );

	for ( int id : g_cl.m_sorted_players ) {
		AimPlayer* data = &m_players[ id - 1 ];
		data->m_hittable = false;

		g_lagcompensation.m_players[ id - 1 ].m_show_backtrack = true;
	}
}

void Aimbot::init( )
{
	m_fire = false;
	m_rapid_fire = false;
	m_alternate_head_points = false;
	m_aim_hitboxes.clear( );
	m_player = nullptr;
	m_required_damage = 0.f;
	m_angle = ang_t {};
	m_point = {};
	m_record = nullptr;
	m_stop = false;
	m_skip_hold_fakelag = false;
}

void Aimbot::GetTicksToStop( )
{
	m_stop_ticks = 0;
	m_alternate_head_points = false;
	m_allow_auto_stop = false;

	if ( g_cl.m_local->m_vecVelocity( ).length_2d( ) <= 0.1 )
	{
		m_allow_auto_stop = false;
		return;
	}

	if ( g_movement.m_fakewalking ||
		!( g_cl.m_local->m_fFlags( ) & FL_ONGROUND ) ||
		( g_cl.m_buttons & IN_JUMP ) )
	{
		m_allow_auto_stop = false;
		return;
	}

	if ( g_cl.m_local->m_flDuckAmount( ) >= 1.0f )
	{
		m_allow_auto_stop = false;
		return;
	}

	float accurate_speed = g_cl.m_weapon->GetMaxSpeed( ) * 0.34f;
	accurate_speed += 1.0f; // small margin

	constexpr int max_ticks = 14;
	while ( ++m_stop_ticks < max_ticks ) {
		g_movement.QuickStop( g_cl.m_cmd );

		g_inputpred.predict( );

		float speed = g_cl.m_local->m_vecVelocity( ).length_2d( );
		if ( speed < accurate_speed )
			break;
	}

	m_debug_overlay.emplace_back( tfm::format( "stop_ticks=%i", m_stop_ticks ) );

	g_cl.m_weapon->UpdateAccuracyPenalty( );
	m_allow_auto_stop = true;

	g_cl.m_local->GetEyePos( &m_early_stop_shootpos );

	*g_cl.m_cmd = m_prestop_cmd;
	g_inputpred.SetDatamap( g_inputpred.og_predicted );

	m_alternate_head_points = m_stop_ticks > 1;
}

bool Aimbot::AutoStop( )
{
	g_inputpred.SetDatamap( g_inputpred.cm_start );
	g_movement.StopUntilAccurate( );
	g_cl.m_cmd->m_buttons |= IN_SPEED;

	g_inputpred.predict( );

	vec3_t new_pos = g_cl.m_local->GetShootPosition( );

	if ( m_point.Scan( m_player->m_entity, m_player->m_required_damage, new_pos, m_record ) == ScanResult_e::HIT_SUCCESS )
	{
		g_cl.m_weapon->UpdateAccuracyPenalty( );
		g_cl.m_shoot_pos = new_pos;
		m_stop = true;
		return true;
	}

	m_debug_overlay.emplace_back( "!valid_auto_stop" );

	// restore prestop cmd if we don't have fallback option on
	if ( !g_menu.main.aimbot.automatic_stop.get( 3 ) ) {
		m_debug_overlay.emplace_back( "!fallback_autostop" );
		*g_cl.m_cmd = m_prestop_cmd;
		g_inputpred.SetDatamap( g_inputpred.og_predicted );
	}

	return false;
}

// return damage value based on settings, ensures minimum is lethal if needed
float Aimbot::GetDamageValue( int health, bool assessed )
{
	float dmg = m_override_dmg
		? g_menu.main.aimbot.smugs_black_leg.get( )
		: ( assessed ? g_menu.main.aimbot.minimal_assesed_damage.get( ) : g_menu.main.aimbot.minimal_damage.get( ) );

	// zeus is always lethal
	if ( g_cl.m_weapon_id == ZEUS )
		return static_cast< float >( health );

	return ( health < dmg ) ? health + 1.0f : dmg;
}

float Aimbot::GetMinAssessedDamage( int health ) { return GetDamageValue( health, true ); }
float Aimbot::GetMinDamage( int health ) { return GetDamageValue( health, false ); }

// prevent firing if not allowed
void Aimbot::StripAttack( )
{
	if ( g_cl.m_weapon_id == REVOLVER )
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK2;
	else
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK;
}

void Aimbot::GenerateHitboxList( )
{
	// only head
	if ( g_menu.main.aimbot.hitbox.get( 0 ) )
	{
		if ( g_menu.main.aimbot.miscellaneous.get( 3 ) ) {
			m_aim_hitboxes.push_back( HITBOX_HEAD );
			return;
		}
	}

	const bool baim_only = m_force_baim || g_cl.m_weapon_id == ZEUS;

	if ( baim_only )
	{
		// stomach / pelvis
		if ( g_menu.main.aimbot.hitbox.get( 2 ) ) {
			m_aim_hitboxes.push_back( HITBOX_BODY );
		}

		// chest group
		if ( g_menu.main.aimbot.hitbox.get( 1 ) ) {
			m_aim_hitboxes.push_back( HITBOX_CHEST );
			m_aim_hitboxes.push_back( HITBOX_UPPER_CHEST );
		}

		return;
	}

	// head
	if ( g_menu.main.aimbot.hitbox.get( 0 ) ) {
		m_aim_hitboxes.push_back( HITBOX_HEAD );
	}

	// stomach / pelvis
	if ( g_menu.main.aimbot.hitbox.get( 2 ) ) {
		m_aim_hitboxes.push_back( HITBOX_BODY );
		m_aim_hitboxes.push_back( HITBOX_PELVIS );
	}

	// chest group
	if ( g_menu.main.aimbot.hitbox.get( 1 ) ) {
		m_aim_hitboxes.push_back( HITBOX_THORAX );
		m_aim_hitboxes.push_back( HITBOX_CHEST );
		m_aim_hitboxes.push_back( HITBOX_UPPER_CHEST );
	}

	// lower scan intensity.
	if ( g_cl.m_throttled && g_menu.main.aimbot.optimizations.get( 1 ) )
	{
		// just scan thighs out of all of them.
		// thighs
		if ( g_menu.main.aimbot.hitbox.get( 4 ) ) {
			m_aim_hitboxes.push_back( HITBOX_R_THIGH );
			m_aim_hitboxes.push_back( HITBOX_L_THIGH );
		}

		return;
	}

	// arms
	if ( g_menu.main.aimbot.hitbox.get( 3 ) ) {
		m_aim_hitboxes.push_back( HITBOX_R_UPPER_ARM );
		m_aim_hitboxes.push_back( HITBOX_L_UPPER_ARM );
		m_aim_hitboxes.push_back( HITBOX_R_HAND );
		m_aim_hitboxes.push_back( HITBOX_L_HAND );
	}

	// thighs
	if ( g_menu.main.aimbot.hitbox.get( 4 ) ) {
		m_aim_hitboxes.push_back( HITBOX_R_THIGH );
		m_aim_hitboxes.push_back( HITBOX_L_THIGH );
	}

	// calves
	if ( g_menu.main.aimbot.hitbox.get( 5 ) ) {
		m_aim_hitboxes.push_back( HITBOX_R_CALF );
		m_aim_hitboxes.push_back( HITBOX_L_CALF );
	}

	// feet
	if ( g_menu.main.aimbot.hitbox.get( 6 ) ) {
		m_aim_hitboxes.push_back( HITBOX_R_FOOT );
		m_aim_hitboxes.push_back( HITBOX_L_FOOT );
	}
}

bool Aimbot::AutoScope( )
{
	const bool can_scope = g_cl.m_weapon_id == AUG || g_cl.m_weapon_id == SG553 || g_cl.m_weapon_type == WEAPONTYPE_SNIPER_RIFLE;
	if ( !can_scope )
		return false;

	if ( g_cl.m_local->m_bIsScoped( )
		|| !g_menu.main.aimbot.automatic_stop.get( 5 )
		|| g_cl.m_weapon->m_zoomLevel( ) != 0 )
		return false;

	bool result = g_cl.InstantActions( false, IN_ATTACK2 );
	if ( result )
		m_debug_overlay.emplace_back( "zoom" );

	return result;
}

void Aimbot::find( ) {
	m_hit_chance_requirement = g_menu.main.aimbot.hitchance.get( ) * 0.01f;

	m_early_autostop = g_menu.main.aimbot.automatic_stop.get( 0 );

	const bool on_ground = ( g_cl.m_local->m_fFlags( ) & FL_ONGROUND ) &&
		( g_cl.m_flags & FL_ONGROUND || !( g_cl.m_buttons & IN_JUMP ) );

	bool hit_early = false;
	BestTarget_t final_target {};

	for ( int id : g_cl.m_sorted_players ) {
		bool found_point = false;

		AimPlayer& data = m_players[ id - 1 ];
		if ( !data.m_entity )
			continue;

		data.m_required_damage = g_aimbot.GetMinDamage( data.m_hp );
		if ( *g_cl.m_packet )
			data.m_held_damage = 0.0f;

		BestTarget_t best {};

		for ( LagRecord* record : data.m_candidates ) {
			if ( data.ScanRecord( record, best ) )
			{
				found_point = true;
				break;
			}
		}

		if ( found_point ) {
			final_target = best;
			break;
		}

		if ( !found_point && m_stop_ticks > 0 && m_early_autostop && !m_stop && on_ground ) {
			if ( data.CanEarlyStop( best ) ) {
				hit_early = true;
				m_stop = true;
			}
		}
	}

	if ( !final_target.player || !final_target.record )
		return;

	if ( !hit_early )
		m_debug_overlay.emplace_back( "!hit_early" );

	final_target.player->m_target = true;

	m_player = final_target.player;
	m_point = final_target.m_point;
	m_record = final_target.record;
	m_required_damage = final_target.player->m_required_damage;

	math::VectorAngles( m_point.m_pos - g_cl.m_shoot_pos, m_angle );
	m_record->apply( m_player->m_entity );

	m_player->m_hittable_tick = g_csgo.m_globals->m_tick_count;
	m_player->m_hittable = true;

	m_record->m_hittable = true;
	m_record->m_hittable_tick = g_csgo.m_globals->m_tick_count;

	if ( m_allow_auto_stop && m_early_autostop ) {
		float pred_time = g_csgo.m_globals->m_curtime + game::TICKS_TO_TIME( m_stop_ticks + 1 );
		if ( pred_time < g_cl.m_local->m_flNextAttack( )
			|| pred_time < g_cl.m_weapon->m_flNextPrimaryAttack( ) ) {
			m_debug_overlay.emplace_back( tfm::format( "time_until_fire:", game::TIME_TO_TICKS( pred_time - g_csgo.m_globals->m_curtime ) ) );
			return;
		}
	}

	bool can_stop_requirements = g_cl.m_weapon_fire || g_menu.main.aimbot.between_shots.get( ) || m_early_autostop;
	bool can_stop = can_stop_requirements && g_menu.main.aimbot.automatic_stop.get( 1 ) && m_allow_auto_stop && !m_stop;

	if ( can_stop ) {
		if ( !AutoStop( ) )
			return;
	}

	bool revolver = g_cl.m_weapon_id == REVOLVER && g_cl.m_revolver_cock != 0;
	if ( revolver && g_cl.m_revolver_cock == g_cl.m_revolver_query ) {
		m_debug_overlay.emplace_back( "!revolver" );
		return;
	}

	if ( !RecalcShootpos( m_angle ) ) {
		m_debug_overlay.emplace_back( "!shootpos" );
		return;
	}

	if ( g_cl.m_max_lag > 1 && g_cl.m_weapon_fire && !g_cl.m_lag ) {
		AutoScope( );
		m_debug_overlay.emplace_back( "!psilent" );
		*g_cl.m_packet = false;
		return;
	}

	if ( m_player->m_broke_lc && m_record->can_delay_shot( ) && m_record->delay_shot( ) ) {
		m_debug_overlay.emplace_back( "!can_shoot" );
		AutoScope( );
		return;
	}

	if ( !g_menu.main.aimbot.silent_aim.get( ) )
		g_csgo.m_engine->SetViewAngles( m_angle );

	m_angle -= g_cl.m_local->m_aimPunchAngle( ) * g_csgo.weapon_recoil_scale->GetFloat( );
	g_cl.m_cmd->m_view_angles = m_angle;

	const float inaccuracy = g_cl.m_weapon->GetInaccuracy( );

	const bool too_inaccurate = static_cast< int >( inaccuracy * 1000.f ) > static_cast< int >( m_lowest_inaccuracy * 1000.f );

	bool can_skip_hc = g_input_system.GetKeyState( g_menu.main.aimbot.force_shot.get( ) ) && !too_inaccurate;

	if ( too_inaccurate && g_menu.main.aimbot.automatic_stop.get( 4 ) && !m_rapid_fire ) {
		m_debug_overlay.emplace_back( "!accuracy_passed" );
		AutoScope( );
		return;
	}

	if ( !g_cl.m_weapon_fire ) {
		m_debug_overlay.emplace_back( "!can_fire" );
		AutoScope( );
		StripAttack( );
		return;
	}

	if ( can_skip_hc )
		m_debug_overlay.emplace_back( "force_shot" );
	else {
		if ( !m_player->GetHitchance( m_angle, g_cl.m_shoot_pos, m_record ) ) {
			m_debug_overlay.emplace_back( "!validated" );
			AutoScope( );
			return;
		}
	}

	if ( ShouldIncreaseFireRate( ) ) {
		return;
	}

	if ( m_rapid_fire )
		m_debug_overlay.emplace_back( "rapid_fire" );

	if ( g_cl.m_weapon_fire && g_menu.main.aimbot.auto_fire.get( ) )
		m_fire = true;
}

void Aimbot::think( ) {
	init( );

	if ( !g_cl.m_weapon || g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_C4 )
		return;

	if ( !g_cl.m_weapon_fire )
		StripAttack( );

	if ( !g_menu.main.aimbot.enable.get( ) )
		return;

	if ( g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS )
		return;

	GenerateHitboxList( );
	if ( m_aim_hitboxes.empty( ) )
		return;

	m_prestop_cmd = *g_cl.m_cmd;

	GetTicksToStop( );

	m_lowest_inaccuracy = g_inputpred.get_lowest_inaccuracy( g_cl.m_weapon );

	find( );

	apply( );

	m_rapid_fire = false;
}

void Aimbot::apply( )
{
	if ( m_fire ) {
		g_cl.m_cmd->m_buttons |= IN_ATTACK;
		m_shot_command[ g_cl.m_cmd->m_command_number % 150 ] = true;
	}

	const bool want_attack = ( g_cl.m_cmd->m_buttons & IN_ATTACK )
		|| ( g_cl.m_weapon_id == REVOLVER && ( g_cl.m_cmd->m_buttons & IN_ATTACK2 ) );
	if ( !want_attack )
		return;

	// there's an candidate we're attacking.
	if ( m_record )
	{
		if ( g_cl.m_old_shot )
			m_debug_overlay.emplace_back( "old_shot" );

		*g_cl.m_packet = false;
		g_cl.m_cmd->m_tick = game::TIME_TO_TICKS( m_record->m_sim_time ) + game::TIME_TO_TICKS( g_cl.m_lerp );

		g_shots.OnShotFire( m_player->m_entity, m_point.m_damage, g_cl.m_weapon_info->m_bullets, m_record, m_point.m_hitbox );
		g_csgo.m_engine->GetPlayerInfo( m_player->m_entity->index( ), &m_player_info );

		static std::vector<std::pair<Color, std::string>> log_entries;

		log_entries.clear( );

		auto add = [ & ]( const Color& c, std::string text ) {
			log_entries.emplace_back( c, text );
			};

		const auto accent = g_menu.main.config.menu_color.get( );
		const Color white( 220, 220, 220 );

		// build message
		add( accent, "[supremacy] " );
		add( white, "firing at " );
		add( accent, m_player_info.m_name );
		add( white, "'s " );
		add( accent, g_shots.m_groups[ game::HitboxToHitgroup( m_point.m_hitbox ) ] );
		add( white, " for " );
		add( accent, std::to_string( ( int )m_point.m_damage ) );
		add( white, " damage and " );
		add( accent, std::to_string( ( int )( m_point.m_max_hitchance * 100.f ) ) );
		add( white, " hitchance" );

		int bt = game::TIME_TO_TICKS( m_player->m_entity->m_flSimulationTime( ) - m_record->m_sim_time );
		if ( bt != 0 ) {
			add( white, " with " );
			add( accent, std::to_string( std::abs( bt ) ) );
			add( white, std::abs( bt ) > 1 ? " ticks of backtrack" : " tick of backtrack" );
		}

		if ( m_rapid_fire ) {
			add( accent, " rf" );
		}

		add( accent, "\n" );

		for ( const auto& [color, text] : log_entries )
			g_csgo.m_cvar->ConsoleColorPrintf( color, text.c_str( ) );

		m_player->m_held_damage += m_point.m_damage;

		auto player_weapon = m_player->m_entity->GetActiveWeapon( );
		float max_speed = player_weapon ? player_weapon->GetMaxSpeed( ) : 250.0f;
		const float curr_speed = m_record->m_velocity.length( );

		m_skip_hold_fakelag = ( m_player->m_held_damage >= m_player->m_hp ) || m_player->m_broke_lc || ( curr_speed > max_speed * 0.34f );

		g_movement.m_stop_the_fakewalk = true;
		g_movement.m_should_quickswitch = true;
		m_stop = false;

		if ( g_menu.main.players.skeleton.get( 5 ) )
			g_visuals.DrawHitboxMatrix( m_player->m_entity, m_record->m_bones, g_menu.main.players.aimbot_matrix.get( ), 4.f );
	}
	else {
		g_cl.m_cmd->m_view_angles -= g_cl.m_local->m_aimPunchAngle( ) * g_csgo.weapon_recoil_scale->GetFloat( );
	}

	g_cl.m_shot = true;
	g_movement.m_autopeek_retreat = true;
}

bool Aimbot::ShouldIncreaseFireRate( )
{
	if ( !g_menu.main.aimbot.increase_fire_rate.get( ) || m_player->m_broke_lc )
		return false;

	if ( !g_cl.m_weapon ||
		g_cl.m_weapon_id == SSG08 ||
		g_cl.m_weapon_id == AWP ||
		g_cl.m_weapon_id == REVOLVER )
		return false;

	int oldest_seq = g_csgo.m_cl->m_last_outgoing_command + 1;
	do {
		if ( ++oldest_seq >= g_cl.m_cmd->m_command_number )
			return false;
	} while ( !m_record->valid( m_record->m_sim_time, g_netdata.GetTimebase( oldest_seq ) ) );

	CUserCmd* cmd = g_csgo.m_input->GetUserCmd( oldest_seq );
	if ( !cmd )
		return false;

	int inaccuracy = g_netdata.GetInaccuracy( cmd->m_command_number );

	if ( static_cast< int >( inaccuracy * 1000.f ) > static_cast< int >( m_lowest_inaccuracy * 1000.f ) ) {
		m_debug_overlay.push_back( "fire rate: inaccuracy too high" );
		return false;
	}

	vec3_t shootpos = g_netdata.GetShootPos( cmd->m_command_number );
	vec3_t backup_shootpos = g_cl.m_shoot_pos;
	g_cl.m_shoot_pos = shootpos;

	penetration::PenetrationInput_t in;

	in.m_damage = m_required_damage;
	in.m_can_pen = true;
	in.m_target = m_player->m_entity;
	in.m_from = g_cl.m_local;
	in.m_pos = m_point.m_pos;

	penetration::PenetrationOutput_t out;

	if ( penetration::run( &in, &out ) ) {
		cmd->m_view_angles = m_angle;
		cmd->m_buttons |= IN_ATTACK;
		cmd->m_tick = game::TIME_TO_TICKS( m_record->m_sim_time + g_cl.m_lerp );

		cmd->m_predicted = false;

		CVerifiedUserCmd* verified = g_csgo.m_input->GetVerifiedCmd( oldest_seq );
		verified->m_cmd = *cmd;
		verified->m_crc = cmd->get_checksum( );

		g_csgo.m_prediction->servercmds( ) = INT_MAX;
		g_inputpred.update( );

		m_rapid_fire = true;
		g_cl.m_shoot_pos = backup_shootpos;

		return !g_cl.CanFireWeapon( );
	}

	g_cl.m_shoot_pos = backup_shootpos;
	return false;
}

bool Aimbot::RecalcShootpos( ang_t& aim_angle )
{
	constexpr int max_iterations = 16; // usually stabilizes in 2–3

	float original_pitch = g_cl.m_local->m_flPoseParameter( )[ 12 ];

	bool hit = false;

	for ( int i = 0; i < max_iterations; ++i ) {
		g_cl.m_local->m_flPoseParameter( )[ 12 ] = ( aim_angle.x + 90.f ) / 180.f;

		vec3_t prev = g_cl.m_shoot_pos;

		g_bones.SetupBonesSimple( g_cl.m_local, BONE_USED_BY_ANYTHING & ~BONE_USED_BY_BONE_MERGE, g_server_animations.local_matrix );
		g_cl.m_shoot_pos = g_cl.m_local->GetShootPosition( );

		hit = ( g_cl.m_shoot_pos - prev ).length( ) < 0.01f;
		if ( hit ) {
			break;
		}
	}

	g_cl.m_local->m_flPoseParameter( )[ 12 ] = original_pitch;

	math::VectorAngles( m_point.m_pos - g_cl.m_shoot_pos, aim_angle );

	static auto cl_showerror = g_csgo.m_cvar->FindVar( HASH( "cl_showerror" ) );

	if ( !hit )
	{
		if ( cl_showerror->GetInt( ) > 2 )
			g_cl.print( "shootpos failed\n" );
	}

	hit = m_point.Scan( m_player->m_entity, m_player->m_required_damage, g_cl.m_shoot_pos, m_record ) == ScanResult_e::HIT_SUCCESS;
	if ( !hit )
	{
		if ( cl_showerror->GetInt( ) > 2 )
			g_cl.print( "shootpos failed(2)\n" );
	}

	return hit;
}
