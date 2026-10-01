#pragma once

// pre-declare.
class LagRecord;

class BackupRecord
{
public:
	BoneArray* m_bones;
	int        m_bone_count;
	vec3_t     m_origin, m_abs_origin;
	vec3_t     m_mins;
	vec3_t     m_maxs;
	ang_t      m_abs_ang;

public:
	__forceinline void store( Player* player )
	{
		// get bone cache ptr.
		CBoneCache* cache = &player->m_BoneCache( );

		// store bone data.
		m_bones = cache->m_pCachedBones;
		m_bone_count = cache->m_CachedBoneCount;

		m_origin = player->m_vecOrigin( );
		m_mins = player->m_vecMins( );
		m_maxs = player->m_vecMaxs( );
		m_abs_origin = player->GetAbsOrigin( );
		m_abs_ang = player->GetAbsAngles( );
	}

	__forceinline void restore( Player* player )
	{
		// get bone cache ptr.
		CBoneCache* cache = &player->m_BoneCache( );

		cache->m_pCachedBones = m_bones;
		cache->m_CachedBoneCount = m_bone_count;

		player->m_vecOrigin( ) = m_origin;
		player->SetCollisionBounds( m_mins, m_maxs );
		player->SetAbsAngles( m_abs_ang );
		player->SetAbsOrigin( m_origin );
	}
};