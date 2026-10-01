#include "includes.h"

Trajectories g_trajectories;

bool Trajectories::data_t::PhysicsSimulate( )
{
	if ( m_detonated )
		return true;

	const auto new_velocity_z = m_velocity.z - ( g_csgo.sv_gravity->GetFloat( ) * 0.4f ) * g_csgo.m_globals->m_interval;

	const auto move = vec3_t(
		m_velocity.x * g_csgo.m_globals->m_interval,
		m_velocity.y * g_csgo.m_globals->m_interval,
		( m_velocity.z + new_velocity_z ) / 2.f * g_csgo.m_globals->m_interval
	);

	m_velocity.z = new_velocity_z;

	auto trace = CGameTrace( );

	PhysicsPushEntity( move, trace );

	if ( m_detonated )
		return true;

	if ( trace.m_fraction != 1.f )
	{
		update_path< true >( );

		PerformFlyCollisionResolution( trace );
	}

	return false;
}

void Trajectories::data_t::PhysicsTraceEntity( const vec3_t& src, const vec3_t& dst, std::uint32_t mask, CGameTrace& trace )
{
	g_csgo.m_engine_trace->TraceHull(
		src, dst, { -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f },
		mask, m_owner, m_collision_group, &trace
	);

	if ( trace.m_startsolid
		&& ( trace.m_contents & CONTENTS_CURRENT_90 ) )
	{
		trace.clear( );

		g_csgo.m_engine_trace->TraceHull(
			src, dst, { -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f },
			mask & ~CONTENTS_CURRENT_90, m_owner, m_collision_group, &trace
		);
	}

	if ( !trace.hit( )
		|| !trace.m_entity
		|| !reinterpret_cast< Entity* >( trace.m_entity )->IsPlayer( ) )
		return;

	trace.clear( );

	g_csgo.m_engine_trace->TraceLine( src, dst, mask, m_owner, m_collision_group, &trace );
}

void Trajectories::data_t::PhysicsPushEntity( const vec3_t& push, CGameTrace& trace )
{
	PhysicsTraceEntity( m_origin, m_origin + push,
		m_collision_group == COLLISION_GROUP_DEBRIS
		? ( MASK_SOLID | CONTENTS_CURRENT_90 ) & ~CONTENTS_MONSTER
		: MASK_SOLID | CONTENTS_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_CURRENT_90 | CONTENTS_HITBOX,
		trace
	);

	if ( trace.m_startsolid )
	{
		m_collision_group = COLLISION_GROUP_INTERACTIVE_DEBRIS;

		g_csgo.m_engine_trace->TraceLine(
			m_origin - push, m_origin + push,
			( MASK_SOLID | CONTENTS_CURRENT_90 ) & ~CONTENTS_MONSTER,
			m_owner, m_collision_group, &trace
		);
	}

	if ( trace.m_fraction )
	{
		m_origin = trace.m_endpos;
	}

	if ( !trace.m_entity )
		return;

	if ( reinterpret_cast< Entity* >( trace.m_entity )->IsPlayer( )
		|| m_index != 68 && m_index != MOLOTOV && m_index != FIREBOMB ) // TAGRENADE = 68
		return;


	if ( m_index != 68
		&& trace.m_plane.m_normal.z < std::cos( math::deg_to_rad( g_csgo.weapon_molotov_maxdetonateslope->GetFloat( ) ) ) ) // TAGRENADE = 68
		return;

	detonate< true >( );
}

void Trajectories::data_t::PerformFlyCollisionResolution( CGameTrace& trace )
{
	auto surface_elasticity = 1.f;

	if ( trace.m_entity )
	{
		if ( game::IsBreakable( trace.m_entity ) )
		{
			m_velocity *= 0.4f;
			return;
		}

		const auto is_player = reinterpret_cast< Entity* >( trace.m_entity )->IsPlayer( );
		if ( is_player )
		{
			surface_elasticity = 0.3f;
		}

		if ( trace.m_entity->index( ) )
		{
			if ( is_player
				&& m_last_hit_entity == trace.m_entity )
			{
				m_collision_group = COLLISION_GROUP_DEBRIS;

				return;
			}

			m_last_hit_entity = trace.m_entity;
		}
	}

	auto velocity = vec3_t( );

	const auto back_off = m_velocity.dot( trace.m_plane.m_normal ) * 2.f;

	for ( auto i = 0u; i < 3u; i++ )
	{
		const auto change = trace.m_plane.m_normal[ i ] * back_off;

		velocity[ i ] = m_velocity[ i ] - change;

		if ( std::fabs( velocity[ i ] ) >= 1.f )
			continue;

		velocity[ i ] = 0.f;
	}

	velocity *= std::clamp< float >( surface_elasticity * 0.45f, 0.f, 0.9f );

	if ( trace.m_plane.m_normal.z > 0.7f )
	{
		const auto speed_sqr = velocity.length_sqr( );
		if ( speed_sqr > 96000.f )
		{
			const auto l = velocity.normalized( ).dot( trace.m_plane.m_normal );
			if ( l > 0.5f )
			{
				velocity *= 1.f - l + 0.5f;
			}
		}

		if ( speed_sqr < 400.f )
		{
			m_velocity = vec3_t( 0, 0, 0 );
		}
		else
		{
			m_velocity = velocity;

			PhysicsPushEntity( velocity * ( ( 1.f - trace.m_fraction ) * g_csgo.m_globals->m_interval ), trace );
		}
	}
	else
	{
		m_velocity = velocity;

		PhysicsPushEntity( velocity * ( ( 1.f - trace.m_fraction ) * g_csgo.m_globals->m_interval ), trace );
	}

	if ( m_bounces_count > 20 )
		return detonate< false >( );

	++m_bounces_count;
}

void Trajectories::data_t::think( )
{
	switch ( m_index )
	{
	case SMOKE:
		if ( m_velocity.length_sqr( ) <= 0.01f )
		{
			detonate< false >( );
		}

		break;
	case DECOY:
		if ( m_velocity.length_sqr( ) <= 0.04f )
		{
			detonate< false >( );
		}

		break;
	case FLASHBANG:
	case HEGRENADE:
	case MOLOTOV:
	case FIREBOMB:
		if ( game::TICKS_TO_TIME( m_tick ) > m_detonate_time )
		{
			detonate< false >( );
		}

		break;
	}

	m_next_think_tick = m_tick + game::TIME_TO_TICKS( 0.2f );
}

void Trajectories::data_t::predict( const vec3_t& origin, const vec3_t& velocity, float throw_time, int offset )
{
	m_origin = origin;
	m_velocity = velocity;
	m_collision_group = COLLISION_GROUP_PROJECTILE;

	const auto tick = game::TIME_TO_TICKS( 1.f / 30.f );

	m_last_update_tick = -tick;

	switch ( m_index )
	{
	case SMOKE: m_next_think_tick = game::TIME_TO_TICKS( 1.5f ); break;
	case DECOY: m_next_think_tick = game::TIME_TO_TICKS( 2.f ); break;
	case FLASHBANG:
	case HEGRENADE:
		m_detonate_time = 1.5f;
		m_next_think_tick = game::TIME_TO_TICKS( 0.02f );

		break;
	case MOLOTOV:
	case FIREBOMB:
		m_detonate_time = g_csgo.molotov_throw_detonate_time->GetFloat( );
		m_next_think_tick = game::TIME_TO_TICKS( 0.02f );

		break;
	}

	for ( ; m_tick < game::TIME_TO_TICKS( 60.f ); ++m_tick )
	{
		if ( m_next_think_tick <= m_tick )
		{
			think( );
		}

		if ( m_tick < offset )
			continue;

		if ( PhysicsSimulate( ) )
			break;

		if ( m_last_update_tick + tick > m_tick )
			continue;

		update_path< false >( );
	}

	if ( m_last_update_tick + tick <= m_tick )
	{
		update_path< false >( );
	}

	m_expire_time = throw_time + game::TICKS_TO_TIME( m_tick );
}

const unsigned short INVALID_STRING_INDEX = ( unsigned short )-1;
bool PrecacheModel( const char* szModelName )
{
	INetworkStringTable* m_pModelPrecacheTable = g_csgo.m_networkstringtable->FindTable( "modelprecache" );

	if ( m_pModelPrecacheTable )
	{
		g_csgo.m_model_info->FindOrLoadModel( szModelName );
		int idx = m_pModelPrecacheTable->AddString( false, szModelName );
		if ( idx == INVALID_STRING_INDEX )
			return false;
	}
	return true;
}

void DrawBeamPaw( vec3_t src, vec3_t end, Color color, float life )
{
	BeamInfo_t beamInfo;
	beamInfo.m_vecStart = src;
	beamInfo.m_vecEnd = end;
	beamInfo.m_flLife = life;
	beamInfo.m_flWidth = 8.f;
	beamInfo.m_flEndWidth = 8.f;
	beamInfo.m_pszModelName = "sprites/purplelaser1.vmt";
	beamInfo.m_nModelIndex = g_csgo.m_model_info->GetModelIndex( "sprites/purplelaser1.vmt" );
	beamInfo.m_flAmplitude = 0.f;
	beamInfo.m_flSpeed = 0.f;
	beamInfo.m_flFadeLength = 2.f;
	beamInfo.m_nStartFrame = 0.f;
	beamInfo.m_flFrameRate = 60.f;
	beamInfo.m_flRed = color.r( );
	beamInfo.m_flGreen = color.g( );
	beamInfo.m_flBlue = color.b( );
	beamInfo.m_flBrightness = color.a( );
	beamInfo.m_nFlags = FBEAM_FADEOUT;
	beamInfo.m_nHaloIndex = -1;
	beamInfo.m_flHaloScale = 3;

	Beam_t* myBeam = g_csgo.m_beams->CreateBeamPoints( beamInfo );
	if ( myBeam )
		g_csgo.m_beams->DrawBeam( myBeam );
}

void DrawBeamCircle( vec3_t src, Color color, float radius, float life )
{
	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMDISK;
	beamInfo.m_pszModelName = "sprites/purplelaser1.vmt";
	beamInfo.m_nModelIndex = g_csgo.m_model_info->GetModelIndex( "sprites/purplelaser1.vmt" );
	beamInfo.m_flLife = life; //
	beamInfo.m_flWidth = 3.f;
	beamInfo.m_flFadeLength = 2.f; // sets the duration of fade
	beamInfo.m_flAmplitude = 0.f;
	beamInfo.m_flRed = color.r( );
	beamInfo.m_flGreen = color.g( );
	beamInfo.m_flBlue = color.b( );
	beamInfo.m_flBrightness = color.a( );
	beamInfo.m_nStartFrame = 0.f;
	beamInfo.m_vecCenter = src;
	beamInfo.m_flStartRadius = 1.f;
	beamInfo.m_flEndRadius = radius;
	beamInfo.m_flSpeed = 0.f;
	beamInfo.m_flFrameRate = 60.f;
	beamInfo.m_nFlags = FBEAM_FADEOUT;
	beamInfo.m_flHaloScale = 3;

	Beam_t* myBeam = g_csgo.m_beams->CreateBeamRingPoint( beamInfo );
	if ( myBeam )
		g_csgo.m_beams->DrawBeam( myBeam );
}

bool Trajectories::data_t::draw( ) const
{
	if ( m_path.size( ) <= 1 || m_drawn ) // if this has been drawn, we dont need to do it again. honestly, it should never run more than once anyways, but just to make sure.
		return false;

	Color col = { g_menu.main.visuals.proj_color.get( ) };
	const int steps = 4; // Number of interpolated beam segments

	for ( std::size_t i { 1 }; i < m_path.size( ); i++ )
	{		
		float life = m_expire_time - g_csgo.m_globals->m_curtime;

		vec3_t prev = std::get< vec3_t >( m_path.at( i - 1 ) );
		vec3_t cur = std::get< vec3_t >( m_path.at( i ) );

		for ( int j = 0; j < steps; j++ )
		{
			float t0 = static_cast< float >( j ) / steps;
			float t1 = static_cast< float >( j + 1 ) / steps;

			vec3_t p0 = prev + ( cur - prev ) * t0;
			vec3_t p1 = prev + ( cur - prev ) * t1;

			DrawBeamPaw( p0, p1, col, life );
		}

		if ( i == m_path.size( ) - 1 )
			DrawBeamCircle( cur, col, 100.f, life );
	}

	return true;
}

void Trajectories::PushEntityOntoTrack( unsigned long handle )
{
	unsorted.emplace_back( handle );
}
void Trajectories::OnTick( CUserCmd* cmd )
{
	auto& list = GetList( );

	// walk backwards so erases are safe
	for ( int i = static_cast< int >( unsorted.size( ) ) - 1; i >= 0; --i ) {
		unsigned long handle = unsorted[ i ];
		auto ent = g_csgo.m_entlist->GetClientEntityFromHandle<Weapon*>( handle );

		if ( !ent || ent->dormant( ) )
		{
			unsorted.erase( unsorted.begin( ) + i );
			continue;
		}

		const auto studio_model = ent->GetModel( );
		if ( !studio_model )
		{
			unsorted.erase( unsorted.begin( ) + i );
			continue;
		}

		if ( ent->m_fEffects( ) & EF_NODRAW )
		{
			unsorted.erase( unsorted.begin( ) + i );
			continue;
		}

		auto t = g_csgo.m_entlist->GetClientEntityFromHandle<Player*>( ent->m_hOwnerEntity( ) );
		if ( !t || ( !t->enemy( g_cl.m_local ) && t != g_cl.m_local ) )
		{
			unsorted.erase( unsorted.begin( ) + i );
			continue;
		}

		if ( list.find( handle ) == list.end( ) )
		{
			Weapons_t type = FLASHBANG;

			if ( ent->is( HASH( "CMolotovProjectile" ) ) )
				type = MOLOTOV;
			else if ( ent->is( HASH( "CBaseCSGrenadeProjectile" ) )
				&& std::string_view( studio_model->m_name ).find( "fraggrenade" ) != std::string::npos )
				type = HEGRENADE;
			else if ( ent->is( HASH( "CSmokeGrenadeProjectile" ) ) )
				type = SMOKE;
			else if ( ent->is( HASH( "CDecoyProjectile" ) ) )
				type = DECOY;

			const float sim_time = reinterpret_cast< Player* >( ent )->m_flSimulationTime( );
			const float spawn_time = ent->m_flSpawnTime_Grenade( );

			list.emplace(
				std::piecewise_construct,
				std::forward_as_tuple( handle ),
				std::forward_as_tuple(
					t,
					type,
					ent->m_vecOrigin( ),
					ent->m_vecVelocity( ),
					spawn_time,
					game::TIME_TO_TICKS( sim_time - spawn_time )
				)
			);
		}

		// processed this handle, remove from unsorted
		unsorted.erase( unsorted.begin( ) + i );
	}

	if ( list.empty( ) )
		return;

	for ( auto it = list.begin( ); it != list.end( ); ) {
		if ( it->second.draw( ) ) {
			it->second.m_drawn = true;
			++it;
		}
		else {
			it = list.erase( it );
		}
	}
}