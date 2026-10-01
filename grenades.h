#pragma once

class Grenades
{
private:
	enum GrenadeFlags : size_t
	{
		NONE = 0,
		DETONATE,
		BOUNCE,
	};

	struct bounce_t
	{
		vec3_t point;
		Color  color;
	};

	using path_t = std::vector< vec3_t >;
	using bounces_t = std::vector< bounce_t >;

private:
	int       m_id;
	path_t    m_path;
	bounces_t m_bounces;
	float     m_vel, m_power;
	vec3_t    m_start, m_velocity, m_move;

public:
	void   reset();
	void   paint();
	void   think();
	void   simulate();
	void   setup();
	size_t advance( size_t tick );
	bool   detonate( size_t tick, CGameTrace& trace );
	void   ResolveFlyCollisionBounce( CGameTrace& trace );
	void   PhysicsPushEntity( vec3_t& start, const vec3_t& move, CGameTrace& trace, Entity* ent );
	void   TraceHull( const vec3_t& start, const vec3_t& end, CGameTrace& trace, Entity* ent );
	void   PhysicsAddGravityMove( vec3_t& move );
	void   PhysicsClipVelocity( const vec3_t& in, const vec3_t& normal, vec3_t& out, float overbounce );
};

extern Grenades g_grenades;

class Trajectories
{
public:
	struct data_t
	{
		data_t() = default;

		data_t(Player* owner, int index, const vec3_t& origin, const vec3_t& velocity, float throw_time, int offset) : data_t()
		{
			m_owner = owner;
			m_index = index;

			predict(origin, velocity, throw_time, offset);
		}

		bool PhysicsSimulate();

		void PhysicsTraceEntity(const vec3_t& src, const vec3_t& dst, std::uint32_t mask, CGameTrace& trace);

		void PhysicsPushEntity(const vec3_t& push, CGameTrace& trace);

		void PerformFlyCollisionResolution(CGameTrace& trace);

		void think();

		template < bool _bounced >
		__forceinline void detonate()
		{
			m_detonated = true;

			update_path< _bounced >();
		}

		template < bool _bounced >
		void update_path()
		{
			m_last_update_tick = m_tick;

			m_path.emplace_back(m_origin, _bounced);
		}

		void predict(const vec3_t& origin, const vec3_t& velocity, float throw_time, int offset);

		bool draw() const;

		bool m_detonated, m_drawn;
		Player* m_owner;
		vec3_t	m_origin, m_velocity;
		Entity* m_last_hit_entity;
		Collision_Group_t m_collision_group;
		float m_detonate_time, m_expire_time;
		int	m_index, m_tick, m_next_think_tick, m_last_update_tick, m_bounces_count;
		std::vector< std::pair< vec3_t, bool > >		m_path{};
	} m_data;

	std::unordered_map< unsigned long, data_t > m_list{};
public:
	void OnTick(CUserCmd* cmd);

	void PushEntityOntoTrack(unsigned long handle);

	__forceinline const data_t& GetLocalData() const { return m_data; }

	__forceinline std::unordered_map< unsigned long, data_t >& GetList() { return m_list; }

	std::vector<unsigned long> unsorted;
};

extern Trajectories g_trajectories;