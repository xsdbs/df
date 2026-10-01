#include "includes.h"

// im not sure we need to do the shit with setting ShouldSkipAnimFrame
// since it seems like thats only called by SetupBones itself, and we are rebuilding it.
// but ill leave it, just incase.

Bones g_bones {};;

bool Bones::BuildBones( Player* target, int mask, BoneArray* out, LagRecord& record )
{
	vec3_t		     pos[ 128 ];
	quaternion_t     q[ 128 ];
	vec3_t           backup_origin;
	ang_t            backup_angles;
	float            backup_poses[ 24 ];
	C_AnimationLayer backup_layers[ 13 ];

	// get hdr.
	CStudioHdr* hdr = target->GetModelPtr( );
	if ( !hdr )
		return false;

	// get ptr to bone accessor.
	CBoneAccessor* accessor = &target->m_BoneAccessor( );
	if ( !accessor )
		return false;

	// store origial output matrix.
	// likely cachedbonedata.
	BoneArray* backup_matrix = accessor->m_pBones;
	if ( !backup_matrix )
		return false;

	g_csgo.m_model_cache->BeginLock( );

	int m_nComputedLODframe = target->m_nComputedLODframe( );
	int m_nAnimLODflags = target->m_nAnimLODflags( );
	int m_fEffects = target->m_fEffects( );

	// fix pvs boi
	target->m_nComputedLODframe( ) = -1;
	target->m_nAnimLODflags( ) = 0;

	// backup original.
	backup_origin = target->GetAbsOrigin( );
	backup_angles = target->GetAbsAngles( );
	target->GetPoseParameters( backup_poses );
	target->GetAnimLayers( backup_layers );

	// compute transform from raw data.
	matrix3x4_t transform;
	math::AngleMatrix( record.m_abs_ang, record.m_origin, transform );

	// set non interpolated data.
	target->AddEffect( EF_NOINTERP );
	target->SetAbsOrigin( record.m_origin );
	target->SetAbsAngles( record.m_abs_ang );
	target->SetPoseParameters( record.m_poses );
	target->SetAnimLayers( record.m_layers );

	// null lean.
	target->m_AnimOverlay( )[ 12 ].m_weight = 0.f;

	// set bone array for write.
	accessor->m_pBones = out;

	// compute and build bones.
	target->StandardBlendingRules( hdr, pos, q, record.m_sim_time, mask );

	uint8_t computed[ 0x100 ];
	std::memset( computed, 0, 0x100 );
	target->BuildTransformations( hdr, pos, q, transform, mask, computed );

	// restore old matrix.
	accessor->m_pBones = backup_matrix;

	// restore original interpolated entity data.
	target->SetAbsOrigin( backup_origin );
	target->SetAbsAngles( backup_angles );
	target->SetPoseParameters( backup_poses );
	target->SetAnimLayers( backup_layers );
	target->m_fEffects( ) = m_fEffects;

	target->m_nComputedLODframe( ) = m_nComputedLODframe;
	target->m_nAnimLODflags( ) = m_nAnimLODflags;

	g_csgo.m_model_cache->EndLock( );

	return record.m_bones != NULL;
}

bool Bones::SetupBonesSimple( Player* target, int mask, BoneArray* out )
{
	vec3_t		     pos[ 128 ];
	quaternion_t     q[ 128 ];

	CStudioHdr* hdr = target->GetModelPtr( );
	if ( !hdr )
		return false;

	CBoneAccessor* accessor = &target->m_BoneAccessor( );
	if ( !accessor )
		return false;

	BoneArray* backup_matrix = accessor->m_pBones;
	if ( !backup_matrix )
		return false;

	g_csgo.m_model_cache->BeginLock( );

	int m_fEffects = target->m_fEffects( );

	target->AddEffect( EF_NOINTERP );

	matrix3x4_t transform;
	math::AngleMatrix( target->GetAbsAngles( ), target->GetAbsOrigin( ), transform );

	accessor->m_pBones = out;

	target->StandardBlendingRules( hdr, pos, q, g_csgo.m_globals->m_curtime, mask );

	uint8_t computed[ 0x100 ];
	std::memset( computed, 0, 0x100 );
	target->BuildTransformations( hdr, pos, q, transform, mask, computed );
	target->m_fEffects( ) = m_fEffects;

	accessor->m_pBones = backup_matrix;

	g_csgo.m_model_cache->EndLock( );

	return true;
}