#pragma once

// Predefine man
class AimPlayer;
class LagCompensation;

enum ScanResult_e : uint8_t {
	NOT_SCANNED = 0,
	HIT_SUCCESS = 1,
	BAD_DAMAGE,
	HITCHANCE_FAIL,
};

struct AimPoint_t
{
	mstudiobbox_t* m_bbox { nullptr };
	vec3_t m_pos { 0.0f, 0.0f, 0.0f };
	float m_score { 0.0f };
	bool m_first_pass { false }; // this point is reserved for second pass, it's either an edge point, or something else.
	float m_fake_amt { 0.0f };
	float m_damage { 0.0f };
	float m_max_hitchance { 0.0f };
	int m_hitbox { -1 };
	ScanResult_e m_result { NOT_SCANNED };

	void GetPointHitchance( Player* target, const ang_t& angle, const vec3_t& shootpos, LagRecord* record );

	ScanResult_e Scan( Player* target, float required_damage, const vec3_t& shootpos, LagRecord* record );
};

struct BestTarget_t
{
	AimPoint_t m_point = AimPoint_t { };
	AimPlayer* player { nullptr };
	LagRecord* record { nullptr };

	__forceinline void reset( )
	{
		player = nullptr;
		record = nullptr;
		m_point = AimPoint_t { }; // reinitialize AimPoint_t safely
	}

	__forceinline BestTarget_t( )
	{
		reset( );
	}
};

// this class just stores player aimbot data
class AimPlayer
{
public:
	// essential data.
	Player* m_entity;
	LagCompensation* m_lagcomp;
	std::vector<LagRecord*> m_candidates;

	bool  m_broke_lc;
	float m_required_damage = 0.f;
	int m_threat_score = 0;

	bool  m_target = false;
	bool  m_hittable = false;
	int   m_hittable_tick = 0;
	float m_fov;
	vec3_t m_origin;
	float m_hp;
	float m_held_damage;
public:
	bool SetupHitboxPoints( LagRecord* record, std::vector< AimPoint_t >& points, bool generate_debug = false );
	void GetScanRecords( );

	bool AllowMultiPoint( int hitbox );

	bool GetHitchance( const ang_t& angle, const vec3_t& shootpos, LagRecord* record );
	bool CanEarlyStop( BestTarget_t& best_target );

	bool ScanRecord( LagRecord* record, BestTarget_t& best_target );
public:
	__forceinline void ComputeThreatScore( const vec3_t& local_origin )
	{
		const float dist = m_origin.dist_to( local_origin );

		const float dist_norm =
			1.f - std::clamp( dist / 2000.f, 0.f, 1.f );

		const float close_norm =
			1.f - std::clamp( dist / 300.f, 0.f, 1.f );

		const float fov_norm =
			1.f - std::clamp( m_fov / 180.f, 0.f, 1.f );

		const float hp_norm =
			std::clamp( m_hp / 100.f, 0.f, 1.f );

		const float health_bias = 1.f - hp_norm;
		const float health_curve = health_bias * health_bias;

		float score =
			( dist_norm * 0.25f ) +
			( fov_norm * 0.20f ) +
			( health_curve * 0.30f );

		if ( g_cl.m_weapon_info )
		{
			const float weapon_damage =
				penetration::scale(
					m_entity,
					g_cl.m_weapon_info->m_damage,
					g_cl.m_weapon_info->m_armor_ratio,
					HITGROUP_STOMACH );

			float lethal_factor = lethal_factor =
				std::clamp( weapon_damage / m_hp, 0.f, 2.f ) * 0.5f;

			score += ( lethal_factor * 0.35f );
		}

		score *= ( 1.f + close_norm * 0.5f );

		if ( m_hittable )
			score *= 1.35f;

		m_threat_score = score;
	}

	__forceinline void reset( )
	{
		m_entity = nullptr;
		m_hittable = false;
		m_hittable_tick = 0;
		m_fov = m_held_damage = 0.f;
	}
};

class Aimbot
{
public:
	struct seed_t {
		float r1, r2, r3, r4, c1, c2, s1, s2;
	};

	seed_t m_seeds[ 256 ];
public:
	bool m_shot_command[ 150 ];

	float m_hit_chance_requirement = 0.f;
	CUserCmd m_prestop_cmd;

	std::vector< int > m_aim_hitboxes;

	std::array< AimPlayer, 64 > m_players;

	BackupRecord m_backup[ 64 ];

	std::vector<AimPoint_t> m_early_aim_points;

	// found target stuff.
	AimPoint_t m_point;
	AimPlayer* m_player;
	ang_t      m_angle;
	float      m_required_damage;
	LagRecord* m_record;
	player_info_t m_player_info;

	bool m_early_autostop = false;

	void GetTicksToStop( );

	float GetDamageValue( int health, bool assessed );

	vec3_t m_early_stop_shootpos;

	bool m_allow_auto_stop;

	int m_mini_hitchance_seeds_strength = 32;

	bool m_skip_hold_fakelag;
	bool m_rapid_fire;
	bool m_alternate_head_points;

	bool m_stop;
	bool m_fire;

	int m_stop_ticks;
	float m_lowest_inaccuracy;

	std::vector<std::string> m_debug_overlay;

	bool m_fake_latency;
	bool m_force_baim;
	bool m_override_dmg;

public:
	__forceinline void reset( )
	{
		// reset aimbot data.
		init( );

		// reset all players data.
		for ( auto& p : m_players )
			p.reset( );
	}

	__forceinline bool IsValidTarget( Player* player, bool check_local = true )
	{
		if ( !player )
			return false;

		if ( !player->IsPlayer( ) )
			return false;

		if ( !player->alive( ) )
			return false;

		if ( player->m_bIsLocalPlayer( ) && check_local )
			return false;

		if ( !player->enemy( g_cl.m_local ) )
			return false;

		return true;
	}

public:
	// aimbot.
	void init( );
	float GetMinAssessedDamage( int health );
	float GetMinDamage( int health = 100 );
	void StripAttack( );
	void GenerateHitboxList( );
	void find( );
	void SetupTargets( );

	void think( );
	void apply( );
	bool ShouldIncreaseFireRate( );
	bool AutoStop( );
	bool AutoScope( );
	bool RecalcShootpos( ang_t& aim_angle );
	void BuildSeedTable( );
};

extern Aimbot g_aimbot;