#include "includes.h"

Resolver g_resolver {};;

void Resolver::OnBodyUpdate( Player* player, float value )
{
	LagCompensation* data = &g_lagcompensation.m_players[ player->index( ) - 1 ];

	// set data.
	data->m_old_body = data->m_body;
	data->m_body = value;
}

float Resolver::SnapToNearest( float angle, std::vector<float> angles )
{
	float ret = 0.f;
	float lowest_diff = std::numeric_limits<float>::max( );

	for ( int i = 0; i < angles.size( ); i++ )
	{
		float delta = abs( math::NormalizedAngle( angle - angles[ i ] ) );

		if ( delta < lowest_diff )
		{
			ret = angles[ i ];
			lowest_diff = delta;
		}
	}

	return ret;
}

void Resolver::SetConfidence( LagCompensation* data, LagRecord& record, bool balance_adjust, int clamp_preference )
{
	float confidence = LagCompensation::ResolverConfidence_t::VERY_LOW;
	float& a = record.m_eye_angles.y;

	confidence += 1.f - ( abs( math::NormalizedAngle( a - record.m_backwards_angle ) ) / 180.f );
	confidence += 1.f - ( abs( math::NormalizedAngle( a - record.m_freestand_angle ) ) / 180.f );

	switch ( clamp_preference )
	{
	case unsure:
		if ( balance_adjust )
		{
			confidence -= 1.f - ( abs( math::NormalizedAngle( a - record.m_body ) ) / 180.f );
		}
		else
		{
			confidence += 1.f - ( abs( math::NormalizedAngle( a - record.m_body ) ) / 180.f );
		}
		break;
	case bias_clamped:
		confidence += 1.f - ( abs( math::NormalizedAngle( a - ( record.m_body + 180.f ) ) ) / 180.f );
		break;
	case bias_unclamped:
		confidence += 1.f - ( abs( math::NormalizedAngle( a - record.m_body ) ) / 180.f );
		break;
		// this shit is unhittable 1337
	case neutral:
		confidence -= 2.f;
		break;
	}

	if ( g_hvh.IsYawSideways( g_cl.m_shoot_pos, record.m_origin, a ) )
		confidence += 1.f;

	// dont want it going < 0 with balance_adjust
	data->m_resolver_confidence = std::max( 0, static_cast< int >( confidence ) );
}

void Resolver::CheckAnimations( LagRecord& record, LagCompensation* data )
{
	int new_count = 0, old_count = 0;

	bool old = false;

	for ( int i = 0; i < data->m_records.size( ); i++ )
	{
		LagRecord* cur = &data->m_records[ i ];
		if ( cur->m_first_out_of_pvs )
			break;

		if ( cur->m_layers[ 3 ].m_weight == 0.f && cur->m_layers[ 3 ].m_cycle == 0.f )
		{
			if ( old )
				++old_count;
			else
				++new_count;

			if ( i == 0 && data->m_player->GetSequenceActivity( cur->m_layers[ 3 ].m_sequence ) == 979 && !data->m_cant_predict_lby )
			{
				data->m_update_offsets.push_front( { data->m_updates_post_lby, record.m_anim_time } );
			}
		}

		if ( cur->m_mode == RESOLVE_UPDATE )
		{
			if ( old )
			{
				break;
			}
			else
			{
				old = true;
			}
		}
	}

	if ( !old )
		return;

	data->m_979_count = new_count;
	data->m_old_979_count = old_count;
}

int Resolver::GetTimingOffset( LagCompensation* data )
{
	constexpr int max_ticks = 37;

	if ( !data->m_blacklisted_timings.empty( ) )
	{
		for ( int i = 0; i <= max_ticks; i++ )
		{
			bool found_timing = false;

			for ( int j = 0; j < data->m_blacklisted_timings.size( ); j++ )
			{
				if ( data->m_blacklisted_timings[ j ] == i )
				{
					found_timing = true;
					break;
				}
			}

			if ( !found_timing )
			{
				return i;
			}
		}

		// we tried every possibility bruzzz... how we havnt run out of ammo? Genius.
		return -1;
	}

	if ( data->m_logged_timer != -1 )
		return data->m_logged_timer;

	if ( data->m_update_offsets.empty( ) )
		return 0;

	static std::vector<std::pair<int, int>> options;
	options.clear( );

	for ( int i = 0; i < data->m_update_offsets.size( ); i++ )
	{
		auto* cur = &data->m_update_offsets[ i ];

		// we probably dont want to use anim timing if they stopped triggering anim.
		if ( i == 0 && data->m_last_update_time - cur->second > 1.1f )
			return 0;

		for ( int j = i + 1; j < data->m_update_offsets.size( ); j++ )
		{
			auto* old = &data->m_update_offsets[ j ];

			for ( int k = j + 1; k < data->m_update_offsets.size( ); k++ )
			{
				auto* older = &data->m_update_offsets[ k ];

				if ( cur->first == old->first && cur->first == older->first )
					options.push_back( { cur->first, LagCompensation::LBYDelayType_t::CONSTANT } );
				else if ( cur->first - old->first == old->first - older->first )
					options.push_back( { cur->first - old->first, LagCompensation::LBYDelayType_t::REPEATING } );
			}
		}
	}

	if ( options.empty( ) )
		return 0;

	struct frequency_t
	{
		int constant_count = 0;
		int repeating_count = 0;
	};

	static std::unordered_map<int, frequency_t> freq;
	freq.clear( );

	for ( const auto& option : options )
	{
		// this is impossible.
		// 0.58 / 37.12 is max time.
		if ( option.first > max_ticks || option.first < 0 )
			continue;

		if ( option.second == LagCompensation::LBYDelayType_t::CONSTANT )
			freq[ option.first ].constant_count++;
		else if ( option.second == LagCompensation::LBYDelayType_t::REPEATING )
			freq[ option.first ].repeating_count++;
	}

	int best_total = 0;

	int best_offset = 0;
	int best_type = 0;

	for ( const auto& entry : freq )
	{
		int total = entry.second.constant_count + entry.second.repeating_count;

		if ( total > best_total )
		{
			best_total = total;
			best_offset = entry.first;
			best_type = entry.second.repeating_count > entry.second.constant_count ? LagCompensation::LBYDelayType_t::REPEATING : LagCompensation::LBYDelayType_t::CONSTANT;
		}
	}

	// since 979 only triggers to one side
	// we want to see if this is likely to be happening.
	if ( best_offset > 0.f )
	{
		for ( const auto& entry : freq )
		{
			if ( entry.first == best_offset - 1 )
			{
				if ( entry.second.constant_count + entry.second.repeating_count > best_total * 0.5f )
				{
					best_offset -= 1;
					break;
				}
			}
		}
	}

	// clear this down here so the ting above ^ has a larger sample size to detect with.
	while ( data->m_update_offsets.size( ) > 64 )
		data->m_update_offsets.pop_back( );

	// this repeating shit is bungus as fuck bruzz...
	// todo: figure out a way to get this workinnn... idk if anyone uses this anyways tho maybe fam or astral idk wtf Kill me
	if ( best_type == LagCompensation::LBYDelayType_t::REPEATING )
		return -INT_MAX;

	return best_offset;
}

void Resolver::Resolve( LagCompensation* data, LagRecord& record )
{
	if ( !g_cl.m_local->enemy( data->m_player ) )
		return;

	LagRecord* previous = data->m_records.size( ) > 1 ? &data->m_records[ 1 ] : nullptr;

	SetFacingPlayer( data, record, previous );

	if ( data->m_went_dormant && previous )
	{
		// animtime is old simtime + 1 tick
		// and if they just came out of dormancy
		// old simtime will be fucked lol.
		if ( ( record.m_origin - previous->m_origin ).length( ) > 10.f
			|| record.m_sim_time > data->m_lby_timer )
		{
			data->m_cant_predict_lby = true;
		}
	}

	// lol shit but it lets us keep updating data if they go to 0 choke.
	const float old_angles = record.m_eye_angles.y;

	if ( record.m_flags & FL_ONGROUND )
	{
		if ( record.m_velocity.length_2d( ) <= 0.1f )
		{
			ResolveStand( data, record );
		}
		else
		{
			ResolveWalk( data, record, previous );
		}

		data->m_air_angle = record.m_eye_angles.y;
	}
	else
	{
		ResolveAir( data, record );
	}

	if ( data->m_override != 0.f && record.m_mode <= RESOLVE_THRESHOLD )
	{
		data->m_override_agreed = abs( math::NormalizedAngle( record.m_eye_angles.y - data->m_override ) ) < 45.f;
		record.m_eye_angles.y = data->m_override;
	}
}

std::pair<float, bool>  Resolver::FreestandYaw( LagCompensation* data, Player* from_player, std::vector<AdaptiveAngle> angles, bool get_worst )
{
	constexpr float STEP { 4.f };
	constexpr float RANGE { 32.f };

	bool valid { false };

	vec3_t shoot_pos = g_cl.m_shoot_pos;

	if ( from_player && from_player != g_cl.m_local )
	{
		from_player->GetEyePos( &shoot_pos );
	}

	vec3_t enemy_pos; data->m_player->GetEyePos( &enemy_pos );

	vec3_t closest_pos {};
	int lowest_iterator = std::numeric_limits<int>::max( );

	for ( auto it = angles.begin( ); it != angles.end( ); ++it )
	{
		vec3_t trace_start { enemy_pos.x + std::cos( math::deg_to_rad( it->m_yaw ) ) * RANGE,
			enemy_pos.y + std::sin( math::deg_to_rad( it->m_yaw ) ) * RANGE,
			enemy_pos.z };

		vec3_t dir = shoot_pos - trace_start;
		float len = dir.normalize( );

		for ( float i { 0.f }; i < len; i += STEP )
		{
			if ( g_csgo.m_engine_trace->GetPointContents( trace_start + ( dir * i ), MASK_SHOT_HULL ) & MASK_SHOT_HULL )
			{
				it->m_dist += STEP;
				valid = true;

				if ( i < lowest_iterator )
				{
					closest_pos = trace_start + ( dir * i );
					lowest_iterator = i;
				}
			}
		}
	}

	if ( !valid )
	{
		return { get_worst ? angles.back( ).m_yaw : angles[ 0 ].m_yaw, false };
	}

	std::sort( angles.begin( ), angles.end( ), [ &get_worst ]( const AdaptiveAngle& a, const AdaptiveAngle& b )
		{
			return get_worst ? a.m_dist < b.m_dist : a.m_dist > b.m_dist;
		} );

	return { angles.front( ).m_yaw, ( closest_pos - shoot_pos ).length( ) * 0.25f > ( closest_pos - enemy_pos ).length( ) };
}

LagRecord m_resolver_record;

void LagCompensation::OnImpact( ImpactRecord* impact )
{
	int target_hitbox = impact->m_shot->m_hitbox;
	int actual_hitgroup = impact->m_hitgroup;

	if ( target_hitbox != HITBOX_HEAD )
		return;

	LagRecord* shot_record = impact->m_shot->m_record;

	if ( !shot_record
		|| !( shot_record->m_flags & FL_ONGROUND )
		|| !( shot_record->m_mode == Resolver::Modes::RESOLVE_STAND || shot_record->m_mode == Resolver::Modes::RESOLVE_UPDATE ) )
		return;

	CCSGOPlayerAnimState* animstate = m_player->m_PlayerAnimState( );
	if ( !animstate )
		return;

	CCSGOPlayerAnimState backup_state = *animstate;

	float old_poses[ 24 ];
	m_player->GetPoseParameters( old_poses );

	C_AnimationLayer old_layers[ 13 ];
	m_player->GetAnimLayers( old_layers );

	LagRecord& record = m_resolver_record;

	{
		record.m_lag = shot_record->m_lag;
		record.m_sim_time = shot_record->m_sim_time;
		record.m_flags = shot_record->m_flags;
		record.m_origin = shot_record->m_origin;
		record.m_velocity = shot_record->m_velocity;
		record.m_mins = shot_record->m_mins;
		record.m_maxs = shot_record->m_maxs;
		record.m_eye_angles = shot_record->m_eye_angles;
		record.m_body = shot_record->m_body;
		record.m_duck = shot_record->m_duck;

		for ( int i = 0; i < 13; ++i )
		{
			record.m_layers[ i ] = shot_record->m_layers[ i ];
		}

		for ( int i = 0; i < 24; ++i )
		{
			record.m_poses[ i ] = shot_record->m_poses[ i ];
		}

		record.m_old_state = shot_record->m_old_state;

		record.m_anim_velocity = shot_record->m_anim_velocity;
		record.m_max_feet_yaw = shot_record->m_max_feet_yaw;
		record.m_min_feet_yaw = shot_record->m_min_feet_yaw;

		if ( !record.m_bones )
		{
			record.m_bones = ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 );
		}

		record.m_abs_ang = shot_record->m_abs_ang;
		record.m_broke_lc = shot_record->m_broke_lc;
		record.m_lby_offset = shot_record->m_lby_offset;
		record.m_mode = shot_record->m_mode;
		record.m_fake_walk = shot_record->m_fake_walk;
		record.m_anim_time = shot_record->m_anim_time;
		record.m_interp_time = shot_record->m_interp_time;
	}

	auto trace_start = impact->m_shot->m_pos;
	auto shot_dir = ( impact->m_pos - trace_start ).normalized( );
	auto trace_end = trace_start + ( shot_dir * impact->m_shot->m_weapon_dist );

	if ( !impact->m_hit )
	{
		if ( record.m_mode == Resolver::Modes::RESOLVE_UPDATE )
		{
			if ( m_logged_timer == record.m_lby_offset )
			{
				m_logged_timer = -1;
			}

			m_blacklisted_timings.emplace_back( record.m_lby_offset );
		}
		else
			++m_missed_shots;

		BackupRecord backup_record;

		for ( int i = 0; i < 360; i += 10 )
		{
			if ( i != 0 )
			{
				record.m_eye_angles.y += i;

				*animstate = record.m_old_state;

				animstate->m_last_update_frame = g_csgo.m_globals->m_frame - 1;
				animstate->update_( record.m_eye_angles );

				m_player->GetPoseParameters( record.m_poses );
				record.m_abs_ang = { 0.f, animstate->m_abs_yaw, 0.f };

				if ( g_bones.BuildBones( m_player, BONE_USED_BY_HITBOX, record.m_bones, record ) )
				{
					CGameTrace tr;

					// realllyyyy shit way of doing this
					// but as far as i can tell it fixes the bone setup issues
					// so idgaf LOL XD LOLOL XD ROFL LMAO
					backup_record.store( m_player );
					record.apply( m_player );
					g_csgo.m_engine_trace->ClipRayToEntity( Ray( trace_start, trace_end ), MASK_SHOT, m_player, &tr );
					backup_record.restore( m_player );

					if ( tr.m_entity != m_player )
					{
						continue;
					}
				}
			}

			LagCompensation::ResolverLog_t log {};
			log.m_reason = LagCompensation::LogReason_t::MISS;
			log.m_on_ground = ( record.m_flags & FL_ONGROUND );

			log.m_confidence = 1.f;

			float body_delta = math::NormalizedAngle( record.m_eye_angles.y - record.m_body );
			log.m_lby_side = abs( body_delta ) <= 35.f ? LagCompensation::LbySide_t::EQUAL : ( body_delta > 0.f ? LagCompensation::LbySide_t::RIGHT : LagCompensation::LbySide_t::LEFT );

			log.m_deltas[ LBY_EYE ] = abs( math::NormalizedAngle( record.m_body - record.m_eye_angles.y ) );
			log.m_deltas[ BACKWARDS_EYE ] = math::NormalizedAngle( record.m_backwards_angle - record.m_eye_angles.y );
			log.m_deltas[ FREESTAND_EYE ] = math::NormalizedAngle( record.m_freestand_angle - record.m_eye_angles.y );

			m_shot_log.emplace_back( log );

			m_blacklisted_angles.emplace_back( record.m_eye_angles.y );
		}
	}
	else
	{
		if ( record.m_mode == Resolver::Modes::RESOLVE_UPDATE )
		{
			m_logged_timer = record.m_lby_offset;
		}

		BackupRecord backup_record;

		for ( int i = 10; i < 360; i += 10 )
		{
			record.m_eye_angles.y += i;

			*animstate = record.m_old_state;

			animstate->m_last_update_frame = g_csgo.m_globals->m_frame - 1;
			animstate->update( record.m_eye_angles );

			m_player->GetPoseParameters( record.m_poses );
			record.m_abs_ang = { 0.f, animstate->m_abs_yaw, 0.f };

			if ( g_bones.BuildBones( m_player, BONE_USED_BY_HITBOX, record.m_bones, record ) )
			{
				CGameTrace tr;

				// realllyyyy shit way of doing this
				// but as far as i can tell it fixes the bone setup issues
				// so idgaf LOL XD LOLOL XD ROFL LMAO
				backup_record.store( m_player );
				record.apply( m_player );
				g_csgo.m_engine_trace->ClipRayToEntity( Ray( trace_start, trace_end ), MASK_SHOT, m_player, &tr );
				backup_record.restore( m_player );

				if ( tr.m_entity == m_player && tr.m_hitgroup == actual_hitgroup )
				{
					LagCompensation::ResolverLog_t log {};
					log.m_reason = actual_hitgroup == HITGROUP_HEAD ? LagCompensation::LogReason_t::HIT : LagCompensation::LogReason_t::OVERLAP;
					log.m_on_ground = ( record.m_flags & FL_ONGROUND );

					log.m_confidence = 1.f;

					float body_delta = math::NormalizedAngle( record.m_eye_angles.y - record.m_body );
					log.m_lby_side = abs( body_delta ) <= 35.f ? LagCompensation::LbySide_t::EQUAL : ( body_delta > 0.f ? LagCompensation::LbySide_t::RIGHT : LagCompensation::LbySide_t::LEFT );

					log.m_deltas[ LBY_EYE ] = abs( math::NormalizedAngle( record.m_body - record.m_eye_angles.y ) );
					log.m_deltas[ BACKWARDS_EYE ] = math::NormalizedAngle( record.m_backwards_angle - record.m_eye_angles.y );
					log.m_deltas[ FREESTAND_EYE ] = math::NormalizedAngle( record.m_freestand_angle - record.m_eye_angles.y );

					m_shot_log.emplace_back( log );
				}
			}
		}
	}

	*animstate = backup_state;
	m_player->SetPoseParameters( old_poses );
	m_player->SetAnimLayers( old_layers );
}

void Resolver::SetFacingPlayer( LagCompensation* data, LagRecord& record, LagRecord* previous )
{
	if ( previous ) {
		// want it to work when ppl are fakewalking, so manually calc it with origins.
		vec3_t velocity = record.m_origin - previous->m_origin;

		if ( velocity.length_2d( ) <= 0.1f )
		{
			if ( data->m_facing_player && data->m_facing_player->alive( ) )
				return;
		}
		else
		{
			data->m_velocity_dir = math::rad_to_deg( std::atan2( velocity.y, velocity.x ) );
		}
	}
	else {
		data->m_velocity_dir = math::rad_to_deg( std::atan2( record.m_velocity.y, record.m_velocity.x ) );
	}

	// adding some weighting, since it wasnt perfect before.
	constexpr float distance_exponent = 0.5f;
	constexpr float angle_exponent = 2.f;

	float best_score = FLT_MAX;
	Player* best_player = nullptr;

	for ( int i = 1; i <= g_csgo.m_globals->m_max_clients; i++ )
	{
		Player* player = g_csgo.m_entlist->GetClientEntity<Player*>( i );
		if ( !player || !player->alive( ) || !player->enemy( data->m_player ) )
			continue;

		float raw_dist = ( record.m_origin - player->m_vecOrigin( ) ).length( );
		float angle_diff = fabsf(
			math::NormalizedAngle(
				math::CalcAngle( record.m_origin, player->m_vecOrigin( ) ).y
				- data->m_velocity_dir
			)
		) / 180.f;

		float d = std::pow( raw_dist, distance_exponent );
		float a = std::pow( 1.f - angle_diff, angle_exponent );
		float score = d * a;

		if ( score < best_score )
		{
			best_score = score;
			best_player = player;
		}
	}

	data->m_facing_player = best_player;
}

constexpr int group_seperation = 10;
constexpr int group_size = 180 / group_seperation;
constexpr int num_buckets = 360 / group_seperation;

int get_bucket( float angle, float range = 360.f )
{
	float n = math::NormalizedAngle( angle );
	float shifted = n + 180.f;

	shifted = std::fmod( shifted, range );
	if ( shifted < 0.f )
		shifted += range;

	return static_cast< int >( shifted / group_seperation );
}

std::array<float, num_buckets> GetTrack( LagCompensation* data, LagRecord& record, const int& clamp_preference )
{
	auto GetAngle = [ & ]( int i ) -> float
		{
			switch ( i )
			{
			case 0: return record.m_body;
			case 1: return record.m_backwards_angle;
			case 2: return record.m_freestand_angle;
			default:
				g_notify.add( XOR( "unexpected iterator in GetTrack!!\n" ) );
				return 0.f;
			}
		};

	std::array<float, num_buckets> track = {};

	float fs_side = math::NormalizedAngle( record.m_freestand_angle - record.m_body );

	for ( int i = 0; i < data->m_shot_log.size( ); i++ )
	{
		auto* cur = &data->m_shot_log[ i ];
		if ( !cur->m_on_ground && ( record.m_flags & FL_ONGROUND ) )
			continue;

		float confidence = cur->m_confidence;

		if ( record.m_flags & FL_ONGROUND )
		{
			bool likely_side = ( abs( fs_side ) <= 35.f && cur->m_lby_side == LagCompensation::LbySide_t::EQUAL )
				|| ( fs_side > 0.f && cur->m_lby_side == LagCompensation::LbySide_t::RIGHT )
				|| ( fs_side < 0.f && cur->m_lby_side == LagCompensation::LbySide_t::LEFT );

			if ( likely_side )
			{
				confidence *= 1.25f;
			}
		}

		for ( int j = 0; j < 3; j++ )
		{
			auto IncrementTrack = [ & ]( float amt ) -> void
				{
					if ( j == LagCompensation::LogDelta_t::LBY_EYE )
					{
						track[ get_bucket( GetAngle( j ) + cur->m_deltas[ j ] ) ] += amt;
					}

					track[ get_bucket( GetAngle( j ) - cur->m_deltas[ j ] ) ] += amt;
				};

			switch ( cur->m_reason )
			{
			case LagCompensation::LogReason_t::DEATH_CLAMPED:
			case LagCompensation::LogReason_t::HIT:
			case LagCompensation::LogReason_t::OVERLAP:
				IncrementTrack( confidence );
				break;
			case LagCompensation::LogReason_t::DEATH_UNCLAMPED:
				switch ( clamp_preference )
				{
				case Resolver::unsure:
				case Resolver::neutral:
					IncrementTrack( confidence * 0.5f );
					break;
				case Resolver::ClampPreference_t::bias_unclamped:
					IncrementTrack( confidence );
					break;
				}
				break;
			case LagCompensation::LogReason_t::MISS:
				IncrementTrack( -confidence );
				break;
			}
		}
	}

	return track;
}

std::pair<float, float> SelectBest( std::array<float, num_buckets> track, LagCompensation* data, LagRecord& record, std::vector<float>& angles, const int& clamp_preference )
{
	float best_chance = -FLT_MAX;
	float best_angle = -1.f;

	for ( int i = 0; i < angles.size( ); i++ )
	{
		const float& ang = angles[ i ];

		if ( !data->m_blacklisted_angles.empty( ) )
		{
			bool bad_angle = false;

			for ( int j = 0; j < data->m_blacklisted_angles.size( ); j++ )
			{
				if ( abs( math::NormalizedAngle( ang - data->m_blacklisted_angles[ j ] ) ) <= group_seperation )
				{
					bad_angle = true;
					break;
				}
			}

			if ( bad_angle )
			{
				continue;
			}
		}

		auto& cur = track[ get_bucket( ang ) ];

		if ( cur > best_chance )
		{
			best_chance = cur;
			best_angle = ang;
		}
	}

	// if this happened, we tried all the bruteforce angles and couldnt find a valid one.
	// in that case, restard the bruteforce.
	if ( best_chance == -FLT_MAX )
	{
		data->m_blacklisted_angles.clear( );
		return SelectBest( track, data, record, angles, clamp_preference );
	}

	return { best_chance, best_angle };
}

void PushUnclampedAngles( std::vector<float>& test_angles, std::vector<float>& angles, float body_yaw )
{
	for ( int i = 0; i < test_angles.size( ); i++ )
	{
		if ( abs( math::NormalizedAngle( test_angles[ i ] - body_yaw ) ) > 58.f )
			continue;

		angles.emplace_back( test_angles[ i ] );
	}
}

void PushClampedAngles( LagCompensation* data, LagRecord& record, std::vector<float>& angles )
{
	// future-proofing incase i forget, i intent on adding to unclamped/clamp counter using twist resolver.
	if ( data->m_shot_log.empty( ) )
		return;

	float fs_side = math::NormalizedAngle( record.m_freestand_angle - record.m_body );

	std::array<float, group_size> lby_groups {};

	for ( int i = 0; i < data->m_shot_log.size( ); i++ )
	{
		auto* log = &data->m_shot_log[ i ];
		if ( log->m_reason != LagCompensation::LogReason_t::DEATH_CLAMPED && log->m_reason != LagCompensation::LogReason_t::HIT && log->m_reason != LagCompensation::LogReason_t::MISS )
			continue;

		if ( !log->m_on_ground && ( record.m_flags & FL_ONGROUND ) )
			continue;

		float confidence = log->m_confidence;

		if ( record.m_flags & FL_ONGROUND )
		{
			bool likely_side = ( abs( fs_side ) <= 35.f && log->m_lby_side == LagCompensation::LbySide_t::EQUAL )
				|| ( fs_side > 0.f && log->m_lby_side == LagCompensation::LbySide_t::RIGHT )
				|| ( fs_side < 0.f && log->m_lby_side == LagCompensation::LbySide_t::LEFT );

			if ( likely_side )
			{
				confidence *= 1.25f;
			}
		}

		if ( log->m_reason == LagCompensation::LogReason_t::MISS )
		{
			lby_groups[ get_bucket( log->m_deltas[ LagCompensation::LogDelta_t::LBY_EYE ], 180.f ) ] -= confidence;
		}
		else
			lby_groups[ get_bucket( log->m_deltas[ LagCompensation::LogDelta_t::LBY_EYE ], 180.f ) ] += confidence;
	}

	std::pair<float, float> best_lby = { 0.f, -FLT_MAX };

	for ( int i = 0; i < lby_groups.size( ); i++ )
	{
		if ( lby_groups[ i ] > best_lby.second )
		{
			best_lby.first = i * group_seperation;
			best_lby.second = lby_groups[ i ];
		}
	}

	if ( best_lby.second == -FLT_MAX )
		return;

	// the logging with backwards & freestand considered should handle the side-selection.
	angles.emplace_back( record.m_body - best_lby.first );
	angles.emplace_back( record.m_body + best_lby.first );
}

void Resolver::ResolveStand( LagCompensation* data, LagRecord& record )
{
	// its fine to only set this inside of ResolveStand since we dont log any other states.
	float backwards = math::CalcAngle( data->m_facing_player ? data->m_facing_player->m_vecOrigin( ) : g_cl.m_shoot_pos, record.m_origin ).y;
	auto fs_data = FreestandYaw( data, data->m_facing_player, { backwards, backwards - 90.f, backwards + 90.f, backwards + 180.f } );
	data->m_freestand_angle = fs_data.first;

	record.m_backwards_angle = backwards;
	record.m_freestand_angle = data->m_freestand_angle;

	if ( data->m_cant_predict_lby && data->m_body == data->m_old_body )
	{
		data->m_updates_post_lby = -1;
	}
	else
	{
		data->m_cant_predict_lby = false;

		if ( record.m_anim_time > data->m_lby_timer || data->m_body != data->m_old_body )
		{
			data->m_updates_post_lby = 0;
			data->m_lby_timer = record.m_anim_time + 1.1f;
		}
	}

	CheckAnimations( record, data );

	record.m_lby_offset = GetTimingOffset( data );

	data->m_scary_lby = record.m_lby_offset != 0 || data->m_blacklisted_timings.size( ) > 0;

	if ( data->m_updates_post_lby == record.m_lby_offset || data->m_body != data->m_old_body )
	{
		record.m_eye_angles.y = record.m_body;
		record.m_mode = RESOLVE_UPDATE;
	}

	++data->m_updates_post_lby;

	if ( record.m_mode == RESOLVE_UPDATE )
		return;

	record.m_mode = RESOLVE_STAND;

	int clamp_preference = unsure;

	float total = data->m_clamp_log.m_times_clamped + data->m_clamp_log.m_times_unclamped;

	// we can be sure that one is happening more often than the other.
	if ( total >= 4.f )
	{
		float clamp_percent = data->m_clamp_log.m_times_clamped / total;

		if ( clamp_percent >= 0.66f )
		{
			clamp_preference = bias_clamped;
		}
		else if ( clamp_percent <= 0.33f )
		{
			clamp_preference = bias_unclamped;
		}
		else
		{
			clamp_preference = neutral;
		}
	}

	auto* layer3 = &record.m_layers[ 3 ];

	bool anim_adjust = data->m_player->GetSequenceActivity( layer3->m_sequence ) == 979
		// should check < 0.92 i think, not <=, but the actual value here is dependant on how much they lag, not sure how to calculate it (0.92 seems to be the lowest it will go).
		&& ( layer3->m_weight > 0.f || ( layer3->m_cycle == 0.f && layer3->m_prev_cycle < 0.92f ) );

	float body_side = math::NormalizedAngle( record.m_backwards_angle - record.m_body );
	float fs_side = math::NormalizedAngle( record.m_freestand_angle - backwards );

	static std::vector<float> angles;
	angles.clear( );

	// DISTORTION.
	if ( std::max( data->m_old_979_count, data->m_979_count ) >= 3 )
	{
		angles.emplace_back( fs_data.first );
	}

	if ( data->m_old_979_count > 0 )
	{
		// this happens when ppl failbreak a lot, so try their lby angle.
		if ( !anim_adjust )
			angles.emplace_back( record.m_body );

		if ( clamp_preference == bias_unclamped )
		{
			std::vector<float> test_angles;

			test_angles.emplace_back( body_side > 0.f ? backwards + 45.f : backwards - 45.f );
			test_angles.emplace_back( body_side > 0.f ? backwards + 20.f : backwards - 20.f );
			test_angles.emplace_back( backwards );
			test_angles.emplace_back( body_side > 0.f ? backwards + 60.f : backwards - 60.f );

			test_angles.emplace_back( body_side > 0.f ? backwards - 45.f : backwards + 45.f );
			test_angles.emplace_back( body_side > 0.f ? backwards - 20.f : backwards + 20.f );
			test_angles.emplace_back( body_side > 0.f ? backwards - 60.f : backwards + 60.f );

			PushUnclampedAngles( test_angles, angles, record.m_body );
		}
		else
		{
			if ( clamp_preference == bias_clamped )
				PushClampedAngles( data, record, angles );

			if ( abs( body_side ) >= 17.5f )
			{
				angles.emplace_back( body_side > 0.f ? backwards + 45.f : backwards - 45.f );
				angles.emplace_back( body_side > 0.f ? backwards + 20.f : backwards - 20.f );
				angles.emplace_back( backwards );
				angles.emplace_back( body_side > 0.f ? backwards + 60.f : backwards - 60.f );
			}
			else
			{
				angles.emplace_back( FreestandYaw( data, data->m_facing_player, { backwards + 45.f, backwards - 45.f } ).first );
				angles.emplace_back( FreestandYaw( data, data->m_facing_player, { backwards + 20.f, backwards - 20.f } ).first );
				angles.emplace_back( backwards );
				angles.emplace_back( FreestandYaw( data, data->m_facing_player, { backwards + 60.f, backwards - 60.f } ).first );
			}
		}
	}
	else
	{
		if ( clamp_preference == bias_clamped )
			PushClampedAngles( data, record, angles );

		if ( anim_adjust )
		{
			if ( clamp_preference == bias_unclamped )
			{
				std::vector<float> test_angles;

				if ( fs_data.second )
					test_angles.emplace_back( fs_data.first );

				test_angles.emplace_back( backwards );

				if ( abs( body_side ) >= 17.5f )
				{
					test_angles.emplace_back( body_side > 0.f ? backwards + 45.f : backwards - 45.f );
					test_angles.emplace_back( body_side > 0.f ? backwards + 20.f : backwards - 20.f );
					test_angles.emplace_back( body_side > 0.f ? backwards + 60.f : backwards - 60.f );
				}
				else
				{
					test_angles.emplace_back( FreestandYaw( data, data->m_facing_player, { backwards + 45.f, backwards - 45.f } ).first );
					test_angles.emplace_back( FreestandYaw( data, data->m_facing_player, { backwards + 20.f, backwards - 20.f } ).first );
					test_angles.emplace_back( FreestandYaw( data, data->m_facing_player, { backwards + 60.f, backwards - 60.f } ).first );
				}

				PushUnclampedAngles( test_angles, angles, record.m_body );
			}
			else
			{
				if ( fs_data.second )
					angles.emplace_back( fs_data.first );

				angles.emplace_back( record.m_backwards_angle );
			}
		}
		else
		{
			angles.emplace_back( record.m_body );
			angles.emplace_back( record.m_body + 35.f );
			angles.emplace_back( record.m_body - 35.f );

			angles.emplace_back( backwards );

			if ( abs( body_side ) >= 17.5f )
			{
				angles.emplace_back( body_side > 0.f ? backwards + 45.f : backwards - 45.f );
				angles.emplace_back( body_side > 0.f ? backwards + 60.f : backwards - 60.f );
			}
			else
			{
				angles.emplace_back( FreestandYaw( data, data->m_facing_player, { backwards + 45.f, backwards - 45.f } ).first );
				angles.emplace_back( FreestandYaw( data, data->m_facing_player, { backwards + 60.f, backwards - 60.f } ).first );
			}
		}
	}

	for ( int i = 0; i < 360; i += group_seperation )
	{
		angles.emplace_back( record.m_freestand_angle + fs_side > 0 ? -i : i );
	}

	record.m_eye_angles.y = SelectBest( GetTrack( data, record, clamp_preference ), data, record, angles, clamp_preference ).second;

	SetConfidence( data, record, anim_adjust, clamp_preference );
}

void Resolver::ResolveWalk( LagCompensation* data, LagRecord& record, LagRecord* previous )
{
	record.m_eye_angles.y = record.m_body;
	record.m_mode = RESOLVE_MOVE;

	++data->m_ticks_moved;
	data->m_missed_shots = 0;

	data->m_lby_timer = record.m_anim_time + 0.22f;
	data->m_cant_predict_lby = false;
	data->m_updates_post_lby = 0;

	data->m_blacklisted_angles.clear( );
	data->m_blacklisted_timings.clear( );

	data->m_resolver_confidence = LagCompensation::ResolverConfidence_t::MAX;

	if ( data->m_ticks_moved < 3 )
	{
		data->m_resolver_confidence = LagCompensation::ResolverConfidence_t::HIGH;
	}
	else if ( record.m_velocity.length_2d( ) > 60.f )
		data->m_resolver_confidence = LagCompensation::ResolverConfidence_t::MAX;
	else
		data->m_resolver_confidence = LagCompensation::ResolverConfidence_t::VERY_HIGH;
}

void Resolver::ResolveAir( LagCompensation* data, LagRecord& record )
{
	record.m_mode = RESOLVE_AIR;

	if ( data->m_body != data->m_old_body )
	{
		data->m_updates_post_lby = 0;
		data->m_lby_timer = record.m_anim_time + 0.22f;

		record.m_mode = RESOLVE_UPDATE;
		record.m_eye_angles.y = data->m_air_angle = data->m_body;
	}
	else
	{
		record.m_eye_angles.y = data->m_air_angle;
		data->m_resolver_confidence = LagCompensation::ResolverConfidence_t::LOW;
	}
}

// the ~45 degree accuracy from the cycle resolving is too inaccurate to push an exact angle from.
void Resolver::CheckUnclamped( LagCompensation* data, float angle )
{
	constexpr float bro = 58.f - 45.f;
	if ( abs( math::NormalizedAngle( data->m_body - angle ) ) >= bro )
		return;

	data->m_clamp_log.m_times_unclamped += 0.33f;
}

void Resolver::process_override( )
{
	bool held = GetAsyncKeyState( g_menu.main.aimbot.resolver_override.get( ) );

	static bool old_held;
	static float unheld_yaw;

	static LagCompensation* overriding;

	constexpr float min_change = 0.5f;

	if ( held )
	{
		if ( !old_held || !overriding )
		{
			float closest = FLT_MAX;

			for ( int i = 1; i <= g_csgo.m_globals->m_max_clients; i++ )
			{
				LagCompensation* data = &g_lagcompensation.m_players[ i - 1 ];
				if ( !data || !data->m_player )
					continue;

				constexpr int head_bone = 6;

				vec3_t bone = data->m_player->m_vecOrigin( );
				if ( data->m_player->m_BoneCache( ).m_pCachedBones )
					data->m_player->m_BoneCache( ).m_pCachedBones->get_bone( bone, head_bone );

				ang_t ang;
				math::VectorAngles( bone - g_cl.m_shoot_pos, ang );

				auto dist = abs( math::NormalizedAngle( g_cl.m_cmd->m_view_angles.x - ang.x ) + math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - ang.y ) );

				if ( dist < closest )
				{
					overriding = data;
					closest = dist;
				}
			}
		}
		else if ( abs( math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - unheld_yaw ) ) >= min_change )
		{
			vec3_t base = overriding->m_player->GetAbsOrigin( ) + vec3_t { 0.f, 0.f, 48.f };

			ang_t Bro;
			math::VectorAngles( base - g_cl.m_shoot_pos, Bro );

			overriding->m_override = Bro.y;

			if ( math::NormalizedAngle( Bro.x - g_cl.m_cmd->m_view_angles.x ) < 0.f )
			{
				overriding->m_override += 180.f;
				overriding->m_override += std::clamp( ( Bro.y - g_cl.m_cmd->m_view_angles.y ) * 30.f, -90.f, 90.f );
			}
			else
				overriding->m_override -= std::clamp( ( Bro.y - g_cl.m_cmd->m_view_angles.y ) * 30.f, -90.f, 90.f );
		}
	}
	else
	{
		if ( old_held )
		{
			if ( abs( math::NormalizedAngle( g_cl.m_cmd->m_view_angles.y - unheld_yaw ) ) < min_change )
			{
				for ( int i = 1; i <= g_csgo.m_globals->m_max_clients; i++ )
				{
					LagCompensation* data = &g_lagcompensation.m_players[ i - 1 ];
					if ( !data )
						continue;

					data->m_override = 0.f;
				}
			}
		}

		unheld_yaw = g_cl.m_cmd->m_view_angles.y;
	}

	old_held = held;
}