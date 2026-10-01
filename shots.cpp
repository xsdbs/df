#include "includes.h"

Shots g_shots { };

void Shots::Process( ) {
	if ( m_impacts.empty( ) )
		return;

	for ( auto it = m_impacts.begin( ); it != m_impacts.end( ); it = m_impacts.erase( it ) ) {
		ImpactRecord* cur = &( *it );
		if ( !cur || !cur->m_shot || !cur->m_shot->m_target )
			continue;

		Player* target = cur->m_shot->m_target;
		LagCompensation* aimplayer = &g_lagcompensation.m_players[ target->index( ) - 1 ];
		if ( aimplayer->m_player != target )
			continue;

		// shot confirmed hit.
		if ( cur->m_hit ) {
			if ( aimplayer )
				aimplayer->OnImpact( cur );
			continue;
		}

		// discard invalid targets or records.
		if ( !target->alive( ) || !cur->m_shot->m_record || target->dormant( ) )
			continue;

		BackupRecord backup;
		backup.store( target );

		cur->m_shot->m_record->apply( target );

		const vec3_t start = cur->m_shot->m_pos;
		const vec3_t dir = ( cur->m_pos - start ).normalized( );
		const vec3_t end = start + dir * cur->m_shot->m_weapon_dist;

		CGameTrace trace;
		g_csgo.m_engine_trace->ClipRayToEntity( Ray( start, end ), MASK_SHOT, target, &trace );

		backup.restore( target );

		if ( trace.m_entity != target ) {
			// spread miss.
			Color log_color = ( g_cl.m_last_prediction_error_time >= cur->m_shot->m_time )
				? Color( 150, 150, 150 )
				: Color( 255, 255, 255 );

			g_notify.add( XOR( "missed shot due to spread\n" ), log_color );
			continue;
		}

		// trace did hit target -> resolver evaluation.
		const LagRecord* record = cur->m_shot->m_record;
		switch ( record->m_mode ) {
		case Resolver::Modes::RESOLVE_MOVE:
			g_notify.add( XOR( "missed shot due to animations\n" ) );
			break;

		case Resolver::Modes::RESOLVE_NONE:
			g_notify.add( XOR( "missed shot due to unknown\n" ) );
			break;

		default:
			g_notify.add( XOR( "missed shot due to fake angles\n" ) );

			if ( aimplayer )
				aimplayer->OnImpact( cur );
			break;
		}

		backup.restore( target );
	}
}

void Shots::OnShotFire( Player* target, float damage, int bullets, LagRecord* record, int hitbox )
{
	for ( int i { }; i < bullets; ++i )
	{
		// setup new shot data.
		ShotRecord shot;
		shot.m_target = target;
		shot.m_record = record;
		shot.m_time = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );
		shot.m_lat = g_cl.m_latency;
		shot.m_damage = damage;
		shot.m_hitbox = hitbox;
		shot.m_pos = g_cl.m_shoot_pos;
		shot.m_command_number = g_cl.m_cmd->m_command_number;

		if ( g_cl.m_weapon_info )
		{
			shot.m_weapon_dist = g_cl.m_weapon_info->m_range;
		}

		m_unapproved_shots.push_front( shot );
	}
}

void Shots::OnImpact( IGameEvent* evt ) {
	if ( !evt || !g_cl.m_local )
		return;

	const vec3_t pos {
		evt->m_keys->FindKey( HASH( "x" ) )->GetFloat( ),
		evt->m_keys->FindKey( HASH( "y" ) )->GetFloat( ),
		evt->m_keys->FindKey( HASH( "z" ) )->GetFloat( )
	};

	const int attacker = g_csgo.m_engine->GetPlayerForUserID(
		evt->m_keys->FindKey( HASH( "userid" ) )->GetInt( )
	);
	if ( !attacker )
		return;

	Player* ent = g_csgo.m_entlist->GetClientEntity<Player*>( attacker );
	if ( ent ) {
		// bullet visuals (laser beams)
		if ( g_menu.main.visuals.impact_beams.get( ) ) {
			Color beam_color;
			//if ( ent == g_cl.m_local && g_menu.main.visuals.bullets.get( 2 ) )
			beam_color = g_menu.main.visuals.impact_beams_color.get( );
			//else if ( ent->enemy( g_cl.m_local ) && g_menu.main.visuals.bullets.get( 3 ) )
			//	beam_color = g_menu.main.visuals.enemy_lasers.get( );
			//else if ( g_menu.main.visuals.bullets.get( 4 ) )
			//	beam_color = g_menu.main.visuals.local_lasers.get( );

			vec3_t eye_pos;
			ent->GetEyePos( &eye_pos );

			// reuse cached shootpos for localplayer same-tick impacts
			if ( ent == g_cl.m_local/* && g_menu.main.visuals.bullets.get( 2 ) */ ) {
				if ( !m_vis_impacts.empty( ) &&
					m_vis_impacts.back( ).m_tickbase == int( g_csgo.m_globals->m_curtime ) &&
					m_vis_impacts.back( ).m_ent_index == attacker ) {
					eye_pos = m_vis_impacts.back( ).m_shoot_pos;

					m_vis_impacts.push_back( { pos, eye_pos, int( g_csgo.m_globals->m_curtime ), beam_color, attacker } );
				}
			}
		}
	}

	// we only care about our own shots from here
	if ( attacker != g_csgo.m_engine->GetLocalPlayer( ) )
		return;

	// server impacts box
	if ( g_menu.main.visuals.bullet_impacts.get( ) ) {
		Color color = Color( 0, 0, 255, 150 );
		float size = 2.f;
		g_csgo.m_debug_overlay->AddBoxOverlay(
			pos,
			vec3_t( -size, -size, -size ),
			vec3_t( size, size, size ),
			ang_t( 0, 0, 0 ),
			color.r( ), color.g( ), color.b( ), color.a( ),
			2.f
		);
	}

	const float time = game::TICKS_TO_TIME( g_cl.m_local->m_nTickBase( ) );

	// track impacts for hurt-event matching
	m_pre_hitmarker_impacts.push_back( { pos, g_csgo.m_globals->m_curtime, 0 } );

	if ( m_real_shots.empty( ) )
		return;

	m_real_shots.erase(
		std::remove_if( m_real_shots.begin( ), m_real_shots.end( ),
			[ & ]( const ShotRecord& s ) {
				if ( s.m_matched )
					return true;

				float delta = std::abs( time - ( s.m_time + s.m_lat ) );
				return delta > 1.f;
			} ),
		m_real_shots.end( )
	);

	auto best_it = std::min_element(
		m_real_shots.begin( ), m_real_shots.end( ),
		[ & ]( const ShotRecord& a, const ShotRecord& b ) {
			float da = std::abs( time - ( a.m_time + a.m_lat ) );
			float db = std::abs( time - ( b.m_time + b.m_lat ) );
			return da < db;
		}
	);

	if ( best_it == m_real_shots.end( ) )
		return;

	best_it->m_matched = true;

	ImpactRecord impact;
	impact.m_shot = &*best_it;
	impact.m_tick = g_csgo.m_cl->m_client_tick;
	impact.m_pos = pos;

	m_impacts.push_front( impact );

	// correct shootpos for visuals
	if ( g_menu.main.visuals.impact_beams.get( ) && !m_vis_impacts.empty( ) ) {
		if ( m_vis_impacts.back( ).m_ent_index == attacker )
			m_vis_impacts.back( ).m_shoot_pos = g_netdata.GetShootPos( best_it->m_command_number );
	}

	while ( m_impacts.size( ) > 128 )
		m_impacts.pop_back( );
}

void Shots::OnHurt( IGameEvent* evt )
{
	if ( !evt || !g_cl.m_local )
		return;

	const int attacker_userid = evt->m_keys->FindKey( HASH( "attacker" ) )->GetInt( );
	const int victim_userid = evt->m_keys->FindKey( HASH( "userid" ) )->GetInt( );

	const int attacker = g_csgo.m_engine->GetPlayerForUserID( attacker_userid );
	const int victim = g_csgo.m_engine->GetPlayerForUserID( victim_userid );

	// skip invalid indexes
	if ( attacker < 1 || attacker > 64 || victim < 1 || victim > 64 )
		return;

	// ignore self or if attacker isn't local
	if ( attacker != g_csgo.m_engine->GetLocalPlayer( ) || victim == g_csgo.m_engine->GetLocalPlayer( ) )
		return;

	const int group = evt->m_keys->FindKey( HASH( "hitgroup" ) )->GetInt( );
	if ( group == HITGROUP_GEAR )
		return;

	Player* target = g_csgo.m_entlist->GetClientEntity<Player*>( victim );
	if ( !target )
		return;

	player_info_t info {};
	if ( !g_csgo.m_engine->GetPlayerInfo( victim, &info ) )
		return;

	const std::string name = std::string( info.m_name ).substr( 0, 24 );
	const float damage = static_cast< float >( evt->m_keys->FindKey( HASH( "dmg_health" ) )->GetInt( ) );
	const int hp = evt->m_keys->FindKey( HASH( "health" ) )->GetInt( );

	g_visuals.m_hit_start = g_csgo.m_globals->m_curtime;

	if ( g_menu.main.misc.hitsound.get( ) )
		g_csgo.m_sound->EmitAmbientSound( XOR( "buttons/arena_switch_press_02.wav" ), 1.f );

	if ( g_menu.main.misc.notifications.get( 0 ) )
	{
		g_notify.add( tfm::format(
			XOR( "hit %s in the %s for %i damage (%i health remaining)\n" ),
			name, m_groups[ group ], ( int )damage, hp
		) );
	}

	if ( group == HITGROUP_GENERIC || m_impacts.empty( ) )
		return;

	ImpactRecord* impact = nullptr;
	for ( auto& i : m_impacts )
	{
		if ( i.m_tick != g_csgo.m_cl->m_client_tick )
			continue;

		if ( i.m_shot->m_target != target )
			continue;

		impact = &i;
		break;
	}

	if ( !impact )
		return;

	impact->m_hitgroup = group;

	// find nearest pre-hitmarker impact
	float best_dist = std::numeric_limits<float>::max( );
	vec3_t best_pos {};
	LagRecord* shot_rec = impact->m_shot->m_record;

	if ( shot_rec )
	{
		for ( auto& i : m_pre_hitmarker_impacts )
		{
			const float dist = ( i.m_pos - shot_rec->m_origin ).length_sqr( );
			if ( dist < best_dist )
			{
				best_dist = dist;
				best_pos = i.m_pos;
			}
		}
	}

	m_pre_hitmarker_impacts.clear( );
	m_post_hitmarker_impacts.push_back( { best_pos, g_csgo.m_globals->m_curtime, int( damage ), target->m_iHealth( ) < 1, group == HITGROUP_HEAD } );

	impact->m_hit = true;

	// store hit record

	HitRecord hit;
	hit.m_impact = impact;
	hit.m_group = group;
	hit.m_damage = damage;

	m_hits.push_front( hit );

	while ( m_hits.size( ) > 128 )
		m_hits.pop_back( );
}
