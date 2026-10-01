#include "includes.h"
#include <unordered_set>
#include "animations.hpp"

bool AimPlayer::AllowMultiPoint( int hitbox ) {
	switch ( hitbox ) {
	case HITBOX_HEAD:          return g_menu.main.aimbot.multipoint.get( 0 );
	case HITBOX_THORAX:
	case HITBOX_CHEST:
	case HITBOX_UPPER_CHEST:  return g_menu.main.aimbot.multipoint.get( 1 );
	case HITBOX_PELVIS:
	case HITBOX_BODY:          return g_menu.main.aimbot.multipoint.get( 2 );
	case HITBOX_R_UPPER_ARM:
	case HITBOX_L_UPPER_ARM:   return g_menu.main.aimbot.multipoint.get( 3 );
	case HITBOX_R_THIGH:
	case HITBOX_L_THIGH:       return g_menu.main.aimbot.multipoint.get( 4 );
	case HITBOX_R_CALF:
	case HITBOX_L_CALF:        return g_menu.main.aimbot.multipoint.get( 5 );
	case HITBOX_R_FOOT:
	case HITBOX_L_FOOT:        return g_menu.main.aimbot.multipoint.get( 6 );
	default:                   return false;
	}
}

void AimPlayer::GetScanRecords( ) {
	m_candidates.clear( );

	bool broke_lc = false;

	LagRecord* newest_record = nullptr;
	LagRecord* oldest_record = nullptr;
	LagRecord* moving_record = nullptr;
	LagRecord* flick_record = nullptr;
	LagRecord* retrack_record = nullptr;

	// clean up old records.
	for ( LagRecord& record : m_lagcomp->m_records ) {
		if ( record.m_hittable_tick >= 0 &&
			std::abs( record.m_hittable_tick - g_csgo.m_globals->m_tick_count ) > 32 ) {
			record.m_hittable_tick = 0;
			record.m_hittable = false;
		}
	}

	for ( LagRecord& record : m_lagcomp->m_records ) {
		if ( broke_lc )
			break;

		if ( record.m_broke_lc ) {
			if ( !record.can_delay_shot( ) || record.delay_shot( ) )
				break;

			broke_lc = true;
		}

		if ( !record.valid( ) )
			continue;

		if ( !newest_record )
			newest_record = &record;

		if ( !moving_record &&
			record.m_mode == Resolver::Modes::RESOLVE_MOVE )
			moving_record = &record;

		if ( !flick_record &&
			record.m_mode == Resolver::Modes::RESOLVE_UPDATE )
			flick_record = &record;

		if ( record.m_hittable &&
			( !retrack_record || record.m_mode > retrack_record->m_mode ) )
			retrack_record = &record;

		oldest_record = &record;
	}

	m_broke_lc = broke_lc;

	// move -> retrack (make an option for backtrack over resolve later) -> flick -> newest -> oldest

	if ( moving_record )
		m_candidates.push_back( moving_record );

	if ( retrack_record &&
		retrack_record != moving_record )
		m_candidates.push_back( retrack_record );

	if ( flick_record &&
		flick_record != moving_record &&
		flick_record != retrack_record )
		m_candidates.push_back( flick_record );

	if ( newest_record &&
		newest_record != moving_record &&
		newest_record != retrack_record &&
		newest_record != flick_record )
		m_candidates.push_back( newest_record );

	if ( oldest_record &&
		oldest_record != moving_record &&
		oldest_record != retrack_record &&
		oldest_record != flick_record &&
		oldest_record != newest_record )
		m_candidates.push_back( oldest_record );
}

ScanResult_e AimPoint_t::Scan( Player* target,
	float required_damage,
	const vec3_t& shootpos,
	LagRecord* record ) {
	penetration::PenetrationInput_t in {};
	in.m_damage = required_damage;
	in.m_can_pen = true;
	in.m_target = target;
	in.m_from = g_cl.m_local;
	in.m_pos = m_pos;

	penetration::PenetrationOutput_t out {};
	if ( !penetration::run( &in, &out ) ) {
		m_damage = out.m_damage;
		m_result = ScanResult_e::BAD_DAMAGE;
		return m_result;
	}

	m_damage = out.m_damage;

	ang_t angle;
	math::VectorAngles( m_pos - shootpos, angle );
	GetPointHitchance( target, angle, shootpos, record );

	if ( m_max_hitchance < g_aimbot.m_hit_chance_requirement ) {
		m_result = ScanResult_e::HITCHANCE_FAIL;
		return m_result;
	}

	m_result = ScanResult_e::HIT_SUCCESS;
	return m_result;
}

void AimPoint_t::GetPointHitchance(
	Player* target,
	const ang_t& angle,
	const vec3_t& shootpos,
	LagRecord* record
) {
	const float spread = g_cl.m_weapon->GetSpread( );

	const float required_hitchance = g_menu.main.aimbot.hitchance.get( ) * 0.01f;
	const int seed_count = g_aimbot.m_mini_hitchance_seeds_strength;

	vec3_t forward, right, up;
	math::AngleVectors( angle, &forward, &right, &up );

	int hits = 0;
	int last_failed_seed = 0;

	constexpr float player_half_width = 32.f;
	constexpr float max_distance_from_bounds = ( player_half_width * 2.f ) + 10.f;

	const vec3_t player_top =
		vec3_t(
			record->m_origin.x,
			record->m_origin.y,
			record->m_origin.z + record->m_maxs.z
		);

	const vec3_t bounds_left = player_top - right * player_half_width;
	const vec3_t bounds_right = player_top + right * player_half_width;

	bool entered_bounds = false;

	for ( int seed_index = 0; seed_index < seed_count; ++seed_index ) {
		const int remaining_seeds = seed_count - seed_index;

		// impossible to reach required hitchance anymore.
		if ( static_cast< float >( hits + remaining_seeds ) / seed_count < required_hitchance )
			break;

		auto& seed = g_aimbot.m_seeds[ seed_index ];

		// calculate spread vector.
		vec3_t wep_spread = {
			( seed.c1 * ( seed.r1 * g_aimbot.m_lowest_inaccuracy ) ) + ( seed.c2 * ( seed.r3 * spread ) ),
			( seed.s1 * ( seed.r1 * g_aimbot.m_lowest_inaccuracy ) ) + ( seed.s2 * ( seed.r3 * spread ) ),
			0.f
		};

		// get spread direction.
		vec3_t dir = ( forward + ( right * wep_spread.x ) + ( up * wep_spread.y ) ).normalized( );

		const vec3_t end =
			shootpos + dir * g_cl.m_weapon_info->m_range;

		// once a ray intersects the player bounds, all inner rays will too.
		if ( !entered_bounds ) {
			const float distance_to_bounds =
				math::SegmentToSegment(
					g_cl.m_shoot_pos,
					end,
					bounds_left,
					bounds_right
				);

			if ( distance_to_bounds >= max_distance_from_bounds )
				continue;

			entered_bounds = true;
		}

		penetration::PenetrationInput_t pen_in {};
		pen_in.m_damage = m_damage;
		pen_in.m_can_pen = true;
		pen_in.m_target = target;
		pen_in.m_from = g_cl.m_local;
		pen_in.m_pos = end;

		penetration::PenetrationOutput_t pen_out {};

		if ( penetration::run( &pen_in, &pen_out ) ) {
			// consecutive hits after several successful seeds allow
			// us to assume the remaining inner seeds will also hit.
			if ( seed_index - last_failed_seed >= 4 ) {
				hits += remaining_seeds;
				break;
			}

			++hits;
		}
		else {
			last_failed_seed = seed_index;
		}
	}

	m_max_hitchance =
		static_cast< float >( hits ) /
		static_cast< float >( seed_count );
}

bool AimPlayer::GetHitchance(
	const ang_t& angle,
	const vec3_t& shootpos,
	LagRecord* record
) {
	vec3_t forward, right, up;
	math::AngleVectors( angle, &forward, &right, &up );

	constexpr int max_seeds = 128;

	const float weapon_range = g_cl.m_weapon_info->m_range;
	const float inaccuracy = g_cl.m_weapon->GetInaccuracy( );
	const float spread = g_cl.m_weapon->GetSpread( );

	const int required_hits = static_cast< int >(
		std::ceil( g_aimbot.m_hit_chance_requirement * max_seeds )
		);

	const int penetration_check_step =
		4 - std::clamp<int>(
			g_menu.main.aimbot.hitchance_strength.get( ),
			0,
			3
		);

	const float minimum_damage =
		g_aimbot.GetMinAssessedDamage( m_hp );

	int valid_hits = 0;

	for ( int seed_index = 0; seed_index < max_seeds; ++seed_index ) {
		const int remaining_seeds = max_seeds - seed_index;

		// impossible to reach the required hitchance.
		if ( valid_hits + remaining_seeds < required_hits )
			break;

		auto& seed = g_aimbot.m_seeds[ seed_index ];

		// get spread.
		vec3_t wep_spread = {
			( seed.c1 * ( seed.r1 * inaccuracy ) ) + ( seed.c2 * ( seed.r3 * spread ) ),
			( seed.s1 * ( seed.r1 * inaccuracy ) ) + ( seed.s2 * ( seed.r3 * spread ) ),
			0.f
		};

		// get spread direction.
		vec3_t dir = ( forward + ( right * wep_spread.x ) + ( up * wep_spread.y ) ).normalized( );

		const vec3_t end =
			shootpos + dir * weapon_range;

		CGameTrace trace;
		g_csgo.m_engine_trace->ClipRayToEntity(
			Ray( shootpos, end ),
			MASK_SHOT,
			m_entity,
			&trace
		);

		if ( trace.m_entity != m_entity )
			continue;

		// only run expensive penetration checks on selected seeds.
		if ( seed_index % penetration_check_step == 0 ) {
			penetration::PenetrationInput_t pen_in {};
			pen_in.m_damage = minimum_damage;
			pen_in.m_can_pen = true;
			pen_in.m_target = m_entity;
			pen_in.m_from = g_cl.m_local;
			pen_in.m_pos = end;

			penetration::PenetrationOutput_t pen_out {};

			if ( penetration::run( &pen_in, &pen_out ) )
				++valid_hits;
		}
		else {
			++valid_hits;
		}

		if ( valid_hits >= required_hits )
			return true;
	}

	return false;
}

bool AimPlayer::SetupHitboxPoints(
	LagRecord* record,
	std::vector<AimPoint_t>& points,
	bool debug
) {
	points.clear( );

	const model_t* model = m_entity->GetModel( );
	if ( !model )
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel( model );
	if ( !hdr )
		return false;

	mstudiohitboxset_t* set =
		hdr->GetHitboxSet( m_entity->m_nHitboxSet( ) );

	if ( !set )
		return false;

	const vec3_t head_target =
		record->m_origin + vec3_t { 0.f, 0.f, 64.f };

	const vec3_t body_target =
		record->m_origin + vec3_t { 0.f, 0.f, 32.f };

	const float player_centre =
		( ( record->m_origin + record->m_maxs ) -
			( record->m_origin + record->m_mins ) ).length( ) * 0.5f;

	// aids
	const float body_scale =
		g_menu.main.aimbot.body_scale.get( ) * 0.01f;

	const float head_scale =
		g_menu.main.aimbot.head_scale.get( ) * 0.01f;

	const float limbs_scale =
		g_menu.main.aimbot.limbs_scale.get( ) * 0.01f;

	const float aim_height =
		g_menu.main.aimbot.aim_height.get( ) * 0.01f;

	const float air_scale_limit =
		g_menu.main.aimbot.point_scale_air_limiter.get( ) * 0.01f;

	const int multipoint_strength =
		g_menu.main.aimbot.multipoint_strength.get( );

	const bool separate_scale =
		g_menu.main.aimbot.separate_pointscale.get( );

	const bool dynamic_scale =
		g_menu.main.aimbot.dynamic_scale.get( );

	auto calculate_fake_amount = [ & ]( const vec3_t& pos, int hitbox ) {
		if ( hitbox > HITBOX_UPPER_CHEST )
			return 0.f;

		vec3_t delta =
			pos - ( hitbox == HITBOX_HEAD ? head_target : body_target );

		if ( hitbox == HITBOX_HEAD )
			delta.z *= 4.f;

		return delta.length( ) / player_centre;
		};

	auto push_point = [ & ]( const vec3_t& pos,
		int hitbox,
		bool first_pass,
		mstudiobbox_t* bbox ) {
			AimPoint_t& point = points.emplace_back( );

			point.m_pos = pos;
			point.m_hitbox = hitbox;
			point.m_first_pass = first_pass;
			point.m_bbox = bbox;
			point.m_fake_amt =
				calculate_fake_amount( pos, hitbox );
		};

	for ( int hitbox : g_aimbot.m_aim_hitboxes ) {
		mstudiobbox_t* bbox = set->GetHitbox( hitbox );
		if ( !bbox )
			continue;

		if ( !debug ) {
			const float scaled_damage =
				penetration::scale(
					m_entity,
					g_cl.m_weapon_info->m_damage,
					g_cl.m_weapon_info->m_armor_ratio,
					game::HitboxToHitgroup( hitbox ) );

			if ( scaled_damage < m_required_damage )
				continue;
		}

		vec3_t mins, maxs;

		math::VectorTransform(
			bbox->m_mins,
			record->m_bones[ bbox->m_bone ],
			mins );

		math::VectorTransform(
			bbox->m_maxs,
			record->m_bones[ bbox->m_bone ],
			maxs );

		const vec3_t center =
			( mins + maxs ) * 0.5f;

		if ( !debug )
			push_point( center, hitbox, false, bbox );

		if ( g_cl.m_weapon_id == ZEUS ||
			!AllowMultiPoint( hitbox ) )
			continue;

		// precache cuz we need em l2r
		const bool is_head = hitbox == HITBOX_HEAD;
		const bool is_limb = hitbox > HITBOX_UPPER_CHEST;

		float point_scale = body_scale;

		if ( separate_scale ) {
			if ( is_head )
				point_scale = head_scale;
			else if ( is_limb )
				point_scale = limbs_scale;
		}

		if ( !( record->m_flags & FL_ONGROUND ) &&
			air_scale_limit > 0.f ) {
			point_scale *= ( 1.f - air_scale_limit );
		}

		if ( dynamic_scale &&
			!( record->m_flags & FL_DUCKING ) &&
			bbox->m_radius > 0.f ) {

			const float adjustment =
				center.dist_to( g_cl.m_shoot_pos ) *
				std::tanf(
					g_aimbot.m_lowest_inaccuracy *
					g_aimbot.m_lowest_inaccuracy
				) * 0.1f;

			point_scale =
				std::clamp(
					point_scale - adjustment,
					0.f,
					point_scale
				);
		}

		vec3_t forward =
			( center - g_cl.m_shoot_pos ).normalized( );

		vec3_t right =
			forward.cross( { 0.f, 0.f, 1.f } ).normalized( );

		right *= bbox->m_radius * point_scale;

		// no cap
		if ( is_limb ) {
			push_point( center + right, hitbox, true, bbox );
			push_point( center - right, hitbox, true, bbox );
			continue;
		}

		vec3_t up =
			right.cross( forward ).normalized( );

		up *= bbox->m_radius * aim_height;

		if ( is_head ) {
			if ( g_aimbot.m_alternate_head_points ) {
				push_point( center + up * 0.25f, hitbox, true, bbox );
				push_point( center + up * 0.50f, hitbox, true, bbox );
				push_point( center + up * 0.75f, hitbox, true, bbox );
				push_point( center + up, hitbox, true, bbox );
			}
			else {
				const vec3_t top = maxs + up;

				push_point( math::Lerp( 0.25f, top, top - right ), hitbox, true, bbox );
				push_point( math::Lerp( 0.25f, top, top + right ), hitbox, true, bbox );

				const vec3_t upper_mid = maxs + up * 0.5f;

				push_point( math::Lerp( 0.75f, upper_mid, upper_mid - right ), hitbox, true, bbox );
				push_point( math::Lerp( 0.75f, upper_mid, upper_mid + right ), hitbox, true, bbox );

				const vec3_t lower_mid = maxs + up * 0.75f;

				push_point( math::Lerp( 0.5f, lower_mid, lower_mid - right ), hitbox, true, bbox );
				push_point( math::Lerp( 0.5f, lower_mid, lower_mid + right ), hitbox, true, bbox );

				push_point( center + right, hitbox, true, bbox );
				push_point( center - right, hitbox, true, bbox );
			}

			continue;
		}

		const bool allow_points =
			hitbox == HITBOX_UPPER_CHEST ||
			hitbox == HITBOX_BODY ||
			( ( hitbox == HITBOX_PELVIS ||
				hitbox == HITBOX_THORAX ) &&
				multipoint_strength > 0 ) ||
			( hitbox == HITBOX_CHEST &&
				multipoint_strength > 1 );

		if ( !allow_points )
			continue;

		// prescan lol
		const bool first_pass =
			hitbox == HITBOX_UPPER_CHEST ||
			hitbox == HITBOX_BODY;

		push_point( center + right, hitbox, first_pass, bbox );
		push_point( center - right, hitbox, first_pass, bbox );
	}

	return !points.empty( );
}

bool AimPlayer::ScanRecord( LagRecord* record, BestTarget_t& best_target ) {
	record->apply( m_entity );

	std::vector<AimPoint_t> points;
	if ( !SetupHitboxPoints( record, points ) )
		return false;

	bool found_prescan_hit = false;

	// first pass scan.
	for ( auto it = points.begin( ); it != points.end( );) {
		AimPoint_t& point = *it;

		if ( !point.m_first_pass ) {
			++it;
			continue;
		}

		if ( point.Scan(
			m_entity,
			m_required_damage,
			g_cl.m_shoot_pos,
			record
		) == ScanResult_e::HIT_SUCCESS ) {
			found_prescan_hit = true;
			break;
		}

		it = points.erase( it );
	}

	if ( !found_prescan_hit )
		return false;

	const float hitchance_bias =
		std::clamp(
			g_menu.main.aimbot.prefer_hitchance.get( ) * 0.01f,
			-1.f,
			1.f
		);

	const float body_bias =
		std::clamp(
			g_menu.main.aimbot.prefer_body_over_damage.get( ) * 0.01f,
			0.f,
			1.f
		);

	const float body_overlap_bias =
		std::clamp(
			g_menu.main.aimbot.prefer_fake_body_overlap.get( ) * 0.01f,
			0.f,
			1.f
		);

	const float head_overlap_bias =
		std::clamp(
			g_menu.main.aimbot.prefer_fake_head_overlap.get( ) * 0.01f,
			0.f,
			1.f
		);

	bool has_head_hitbox = false;
	bool has_body_hitbox = false;
	bool has_limb_hitbox = false;

	// scan remaining points.
	for ( AimPoint_t& point : points ) {
		if ( point.m_result == NOT_SCANNED )
			point.Scan(
				m_entity,
				m_required_damage,
				g_cl.m_shoot_pos,
				record
			);

		if ( point.m_result != ScanResult_e::HIT_SUCCESS )
			continue;

		if ( point.m_hitbox == HITBOX_HEAD )
			has_head_hitbox = true;

		if ( point.m_hitbox > HITBOX_UPPER_CHEST )
			has_limb_hitbox = true;

		if ( point.m_hitbox > HITBOX_LOWER_NECK &&
			point.m_hitbox <= HITBOX_UPPER_CHEST )
			has_body_hitbox = true;
	}

	points.erase(
		std::remove_if(
			points.begin( ),
			points.end( ),
			[ ]( const AimPoint_t& point ) {
				return point.m_result != ScanResult_e::HIT_SUCCESS;
			}
		),
		points.end( )
	);

	if ( points.empty( ) )
		return false;

	AimPoint_t* best_point = nullptr;

	const float score_blend =
		( hitchance_bias + 1.f ) * 0.5f;

	for ( AimPoint_t& point : points ) {
		const float damage_score =
			std::clamp(
				point.m_damage / m_hp,
				0.f,
				1.f
			);

		const float hitchance_score =
			std::clamp(
				point.m_max_hitchance,
				0.f,
				1.f
			);

		float score =
			damage_score * ( 1.f - score_blend ) +
			hitchance_score * score_blend;

		const bool is_body =
			point.m_hitbox > HITBOX_LOWER_NECK &&
			point.m_hitbox <= HITBOX_UPPER_CHEST;

		if ( ( has_head_hitbox || has_limb_hitbox ) &&
			has_body_hitbox &&
			is_body ) {
			score *= ( 1.f + body_bias );
		}

		if ( point.m_hitbox <= HITBOX_UPPER_CHEST ) {
			const float overlap =
				1.f - std::clamp(
					point.m_fake_amt,
					0.f,
					1.f
				);

			const float overlap_bias =
				point.m_hitbox == HITBOX_HEAD
				? head_overlap_bias
				: body_overlap_bias;

			score *= ( 1.f + overlap * overlap_bias );
		}

		point.m_score = score;

		if ( !best_point || score > best_point->m_score )
			best_point = &point;
	}

	if ( !best_point )
		return false;

	best_target.player = this;
	best_target.record = record;
	best_target.m_point = *best_point;

	return true;
}

bool AimPlayer::CanEarlyStop( BestTarget_t& best_target ) {
	LagRecord* chosen_record = nullptr;

	struct Extreme {
		AimPoint_t* pt = nullptr;
		float       val = 0.f;
	};

	for ( auto& rec : m_lagcomp->m_records ) {
		if ( !rec.valid( rec.m_sim_time + game::TICKS_TO_TIME( g_aimbot.m_stop_ticks ) ) )
			continue;

		chosen_record = &rec;

		const float base_dir =
			( chosen_record->m_origin - g_aimbot.m_early_stop_shootpos ).y;

		if ( !SetupHitboxPoints( chosen_record, g_aimbot.m_early_aim_points ) )
			continue;

		Extreme highest, rightmost, leftmost;

		for ( AimPoint_t& p : g_aimbot.m_early_aim_points ) {
			if ( !p.m_first_pass )
				continue;

			const float rel_dir = math::NormalizedAngle(
				( p.m_pos - g_aimbot.m_early_stop_shootpos ).y - base_dir
			);

			if ( !highest.pt || p.m_pos.z > highest.pt->m_pos.z )
				highest.pt = &p;

			if ( !rightmost.pt || rel_dir > rightmost.val ) {
				rightmost.pt = &p;
				rightmost.val = rel_dir;
			}

			if ( !leftmost.pt || rel_dir < leftmost.val ) {
				leftmost.pt = &p;
				leftmost.val = rel_dir;
			}
		}

		for ( AimPoint_t* pt : { highest.pt, rightmost.pt, leftmost.pt } ) {
			if ( !pt )
				continue;

			if ( pt->Scan(
				m_entity,
				m_required_damage,
				g_aimbot.m_early_stop_shootpos,
				chosen_record
			) != ScanResult_e::HIT_SUCCESS )
				continue;

			best_target.record = chosen_record;
			best_target.m_point = *pt;
			return true;
		}
	}

	return false;
}