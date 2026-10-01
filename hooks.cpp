#include "includes.h"
#include "minhook/MinHook.h"

Hooks                g_hooks { };;
CustomEntityListener g_custom_entity_listener { };;

void Pitch_proxy( CRecvProxyData* data, Address ptr, Address out )
{
	// normalize this fucker.
	math::NormalizeAngle( data->m_Value.m_Float );

	// clamp to remove retardedness.
	math::clamp( data->m_Value.m_Float, -90.f, 90.f );

	// call original netvar proxy.
	if ( g_hooks.m_Pitch_original )
		g_hooks.m_Pitch_original( data, ptr, out );
}

void Body_proxy( CRecvProxyData* data, Address ptr, Address out )
{
	Address stack { reinterpret_cast< uintptr_t >( _AddressOfReturnAddress( ) ) - sizeof( uintptr_t ) };
	static Address RecvTable_Decode { pattern::find( g_csgo.m_engine_dll, XOR( "EB 0D FF 77 10" ) ) };

	// call from entity going into pvs.
	if ( ( stack.get( 2 ) + sizeof( uintptr_t ) ) != RecvTable_Decode )
	{
		// convert to player.
		Player* player = ptr.as< Player* >( );

		// store data about the update.
		//g_resolver.OnBodyUpdate( player, data->m_Value.m_Float );
	}

	// call original proxy.
	if ( g_hooks.m_Body_original )
		g_hooks.m_Body_original( data, ptr, out );
}

void AbsYawLog( CRecvProxyData* proxydata, Address ptr )
{
	Ragdoll* ragdoll = ptr.as< Ragdoll* >( );
	if ( !ragdoll )
		return;

	Player* player = ragdoll->GetPlayer( );
	if ( !player || player->alive( ) || player->m_bIsLocalPlayer( ) )
		return;

	auto data = &g_lagcompensation.m_players[ player->index( ) - 1 ];
	if ( !data || !data->m_get_death_yaw || data->m_records.empty( ) )
		return;

	auto& front = data->m_records[ 0 ];

	float flAbsYaw = proxydata->m_Value.m_Float;

	if ( front.m_mode == Resolver::Modes::RESOLVE_STAND || front.m_mode == Resolver::Modes::RESOLVE_AIR )
	{
		float flTempYawMax = 58.f, flTempYawMin = -58.f;

		// need this because it can narrow in air.
		CCSGOPlayerAnimState* state = player->m_PlayerAnimState( );
		if ( state )
		{
			// narrow the available aim matrix width as speed increases
			float flAimMatrixWidthRange = math::Lerp( std::clamp( state->m_speed_as_portion_of_run_top_speed, 0.f, 1.f ), 1.0f, math::Lerp( state->m_walk_run_transition, 0.8f, 0.5f ) );

			if ( state->m_anim_duck_amount > 0 )
			{
				flAimMatrixWidthRange = math::Lerp( state->m_anim_duck_amount * std::clamp( state->m_speed_as_portion_of_crouch_top_speed, 0.f, 1.f ), flAimMatrixWidthRange, 0.5f );
			}

			flTempYawMax = state->m_aim_yaw_max * flAimMatrixWidthRange;
			flTempYawMin = state->m_aim_yaw_min * flAimMatrixWidthRange;
		}

		float lby_side = math::NormalizedAngle( flAbsYaw - front.m_body );

		// their abs yaw is getting clamped.
		if ( math::NormalizedAngle( flAbsYaw - data->m_max_feet_yaw ) > 0.f || math::NormalizedAngle( flAbsYaw - data->m_min_feet_yaw ) < 0.f )
		{
			float real_angle = lby_side > 0.f ? flAbsYaw + flTempYawMax : flAbsYaw + flTempYawMin;

			LagCompensation::ResolverLog_t log {};
			log.m_reason = LagCompensation::LogReason_t::DEATH_CLAMPED;
			log.m_on_ground = ( front.m_flags & FL_ONGROUND );

			if ( !data->m_cant_predict_lby )
			{
				// lets add some time decay, based on how close we were to an lby update
				// so we can account for preflick and delayed lby breakers.
				float time = std::min( data->m_lby_timer - front.m_anim_time, front.m_anim_time - ( data->m_lby_timer - 1.1f ) );
				float decay = 1.f - ( 0.55f - time ); // 1.1f / 2

				log.m_confidence = decay;
			}
			else
				log.m_confidence = 0.75f;

			log.m_lby_side = lby_side > 0.f ? LagCompensation::LbySide_t::RIGHT : LagCompensation::LbySide_t::LEFT;

			log.m_deltas[ LagCompensation::LogDelta_t::LBY_EYE ] = abs( math::NormalizedAngle( front.m_body - real_angle ) );
			log.m_deltas[ LagCompensation::LogDelta_t::BACKWARDS_EYE ] = math::NormalizedAngle( front.m_backwards_angle - real_angle );
			log.m_deltas[ LagCompensation::LogDelta_t::FREESTAND_EYE ] = math::NormalizedAngle( front.m_freestand_angle - real_angle );

			data->m_shot_log.emplace_back( log );

			data->m_clamp_log.m_times_clamped += 1.f;
		}
		// their abs yaw was not getting clamped, this means that there is a 120 degree radius of where their head could be (60 deg to each side of footyaw)
		else
		{
			for ( float i = flTempYawMin; i <= flTempYawMax; i += 5.f )
			{
				float real_angle = flAbsYaw + i;

				LagCompensation::ResolverLog_t log {};
				log.m_on_ground = ( front.m_flags & FL_ONGROUND );
				log.m_reason = LagCompensation::LogReason_t::DEATH_UNCLAMPED;

				log.m_confidence = 1.f;

				log.m_lby_side = lby_side > 0.f ? LagCompensation::LbySide_t::RIGHT : LagCompensation::LbySide_t::LEFT;

				log.m_deltas[ LagCompensation::LogDelta_t::LBY_EYE ] = abs( math::NormalizedAngle( front.m_body - real_angle ) );
				log.m_deltas[ LagCompensation::LogDelta_t::BACKWARDS_EYE ] = math::NormalizedAngle( front.m_backwards_angle - real_angle );
				log.m_deltas[ LagCompensation::LogDelta_t::FREESTAND_EYE ] = math::NormalizedAngle( front.m_freestand_angle - real_angle );

				data->m_shot_log.emplace_back( log );
			}

			data->m_clamp_log.m_times_unclamped += 1.f;
		}
	}

	data->m_get_death_yaw = false;
}

void AbsYaw_proxy( CRecvProxyData* data, Address ptr, Address out )
{
	AbsYawLog( data, ptr );

	// call original netvar proxy.
	if ( g_hooks.m_AbsYaw_original )
		g_hooks.m_AbsYaw_original( data, ptr, out );
}

void Force_proxy( CRecvProxyData* data, Address ptr, Address out )
{
	// convert to ragdoll.
	Ragdoll* ragdoll = ptr.as< Ragdoll* >( );

	// get ragdoll owner.
	Player* player = ragdoll->GetPlayer( );

	// we only want this happening to noobs we kill.
	if ( g_menu.main.misc.ragdoll_force.get( ) && g_cl.m_local && player && player->enemy( g_cl.m_local ) )
	{
		// get m_vecForce.
		vec3_t vel = { data->m_Value.m_Vector[ 0 ], data->m_Value.m_Vector[ 1 ], data->m_Value.m_Vector[ 2 ] };

		// give some speed to all directions.
		vel *= 1000.f;

		// boost z up a bit.
		if ( vel.z <= 1.f )
			vel.z = 2.f;

		vel.z *= 2.f;

		// don't want crazy values for this... probably unlikely though?
		math::clamp( vel.x, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );
		math::clamp( vel.y, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );
		math::clamp( vel.z, std::numeric_limits< float >::lowest( ), std::numeric_limits< float >::max( ) );

		// set new velocity.
		data->m_Value.m_Vector[ 0 ] = vel.x;
		data->m_Value.m_Vector[ 1 ] = vel.y;
		data->m_Value.m_Vector[ 2 ] = vel.z;
	}

	if ( g_hooks.m_Force_original )
		g_hooks.m_Force_original( data, ptr, out );
}

typedef bool( __fastcall* fnSendWeaponAnim )( Weapon*, void*, unsigned int );
fnSendWeaponAnim oSendWeaponAnim;

bool __fastcall SendWeaponAnim( Weapon* weapon, void* edx, unsigned int act )
{
	const auto owner = reinterpret_cast< Player* >( g_csgo.m_entlist->GetClientEntityFromHandle( weapon->m_hOwnerEntity( ) ) );
	if ( owner && owner->IsPlayer( ) && owner->alive( ) && owner->index( ) == g_csgo.m_engine->GetLocalPlayer( ) && act != ACT_VM_IDLE )
	{
		g_server_animations.m_viewmodel_act = act;
	}

	return oSendWeaponAnim( weapon, edx, act );
}

typedef void( __thiscall* fnSetupWeaponAction )( CCSGOPlayerAnimState* );
fnSetupWeaponAction oSetupWeaponAction;
void __fastcall SetupWeaponAction( CCSGOPlayerAnimState* ecx, uint32_t )
{
	if ( ecx->m_player->index( ) != g_csgo.m_engine->GetLocalPlayer( ) )
		return oSetupWeaponAction( ecx );

	g_server_animations.OnSetupWeaponAction( ecx );
}

using fnSetupBones = bool( __fastcall* )( void*, void*, BoneArray*, int, int, float );
fnSetupBones oSetupBones;

bool __fastcall SetupBones( void* ecx, void* edx, BoneArray* out, int max, int mask, float curtime ) {
	static auto& g_iModelBoneCounter = *pattern::find(
		g_csgo.m_client_dll,
		"3B 05 ? ? ? ? 0F 84 ? ? ? ? 8B 47"
	).add( 0x2 ).to( ).as<int*>( );

	static auto AttachmentHelper = pattern::find(
		g_csgo.m_client_dll,
		"55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4 56 57 85 DB 0F 84"
	).as<void( __thiscall* )( void*, void* )>( );

	auto* player = reinterpret_cast< Player* >( uintptr_t( ecx ) - 4 );
	if ( !player || player->dormant( ) )
		return oSetupBones( ecx, edx, out, max, mask, curtime );

	const int idx = player->index( );
	if ( !player->IsPlayer( ) || !player->alive( ) || idx < 1 || idx > 64 )
		return oSetupBones( ecx, edx, out, max, mask, curtime );

	CBoneCache* cache = &player->m_BoneCache( );
	if ( !cache )
		return oSetupBones( ecx, edx, out, max, mask, curtime );

	if ( player == g_cl.m_local ) {
		std::memcpy(
			cache->m_pCachedBones,
			g_server_animations.local_render_matrix,
			sizeof( BoneArray ) * cache->m_CachedBoneCount
		);

		const vec3_t delta = g_cl.m_local->GetAbsOrigin( ) - g_server_animations.last_render_setup;
		for ( int i = 0; i < cache->m_CachedBoneCount; ++i )
			cache->m_pCachedBones[ i ].SetOrigin( cache->m_pCachedBones[ i ].GetOrigin( ) + delta );

		if ( mask == BONE_USED_BY_ATTACHMENT )
			AttachmentHelper( player, player->GetModelPtr( ) );

		if ( out ) {
			//AssertMsgOnce( (m_vBoneSetupCachedOrigin == GetRenderOrigin()) && (m_qBoneSetupCachedAngles == GetRenderAngles()), "Renderable moved since cached" );
			if ( max >= cache->m_CachedBoneCount )
			{
				std::memcpy( out, cache->m_pCachedBones, sizeof( matrix3x4_t ) * cache->m_CachedBoneCount );
			}
			else
			{
				//Warning( "SetupBones: invalid bone array size (%d - needs %d)\n", nMaxBones, m_CachedBoneData.Count() );
				//m_BoneSetupLock.Unlock();
				return false;
			}
		}
		return true;
	}

	LagCompensation* data = &g_lagcompensation.m_players[ idx - 1 ];
	if ( !data || !data->m_player || data->m_player != player || data->m_records.empty( ) )
		return oSetupBones( ecx, edx, out, max, mask, curtime );

	if ( player->m_iMostRecentModelBoneCounter( ) != g_iModelBoneCounter ) {
		auto& front = data->m_records.front( );
		if ( front.m_bones ) {
			std::memcpy(
				cache->m_pCachedBones,
				front.m_bones,
				sizeof( matrix3x4_t ) * cache->m_CachedBoneCount
			);

			// align bones with render origin
			for ( int i = 0; i < cache->m_CachedBoneCount; ++i ) {
				cache->m_pCachedBones[ i ].SetOrigin(
					cache->m_pCachedBones[ i ].GetOrigin( ) - front.m_origin + player->GetRenderOrigin( )
				);
			}

			AttachmentHelper( player, player->GetModelPtr( ) );
			player->m_iMostRecentModelBoneCounter( ) = g_iModelBoneCounter;
		}
	}

	if ( out ) {
		//AssertMsgOnce( (m_vBoneSetupCachedOrigin == GetRenderOrigin()) && (m_qBoneSetupCachedAngles == GetRenderAngles()), "Renderable moved since cached" );
		if ( max >= cache->m_CachedBoneCount )
		{
			std::memcpy( out, cache->m_pCachedBones, sizeof( matrix3x4_t ) * cache->m_CachedBoneCount );
		}
		else
		{
			//Warning( "SetupBones: invalid bone array size (%d - needs %d)\n", nMaxBones, m_CachedBoneData.Count() );
			//m_BoneSetupLock.Unlock();
			return false;
		}
	}

	return true;
}

typedef void( __thiscall* fnUpdatePostProcessingEffects )( void* );
fnUpdatePostProcessingEffects oUpdatePostProcessingEffects;

void __fastcall UpdatePostProcessingEffects( void* ecx )
{
	// this array gets overwritten during call to ClientModeCSNormal::UpdatePostProcessingEffects, 
	// for visible results, set data after call.
	if ( !g_csgo.m_engine->IsInGame( ) || !g_cl.m_local )
		return oUpdatePostProcessingEffects( ecx );

	static auto post_process_param = pattern::find( g_csgo.m_client_dll, XOR( "0F 11 05 ? ? ? ? 0F 10 87" ) ).add( 0x3 ).get( ).as <PostProcessParameters_t*>( );
	static auto post_process_param_effects = pattern::find( g_csgo.m_client_dll, XOR( "6B C3 2C 05 ? ? ? ?" ) ).add( 0x4 ).get( ).as <PostProcessParameters_t*>( );
	static auto GetActivePostProcessController { pattern::find( g_csgo.m_client_dll, XOR( "E8 ? ? ? ? F3 0F 10 05 ? ? ? ? 8B 8F ? ? ? ?" ) ).rel32( 0x1 ).as < uintptr_t * ( __thiscall* )( void* ) >( ) };

	auto pPPCtrl = GetActivePostProcessController( g_cl.m_local );
	PostProcessParameters_t* effects = post_process_param_effects;

	// GetActivePostProcessController returns nullptr
	// if the map doesn't have specific PPP settings.
	if ( pPPCtrl )
	{   // c: bean
		// copy map post processing settings into sniper zoom effects
		// so they don't change the look of the game when we scope in xd
		std::memcpy( ( effects + 8 ), pPPCtrl + 0x276, sizeof( PostProcessParameters_t ) );  // POST_EFFECT_ZOOMED_SNIPER
		std::memcpy( ( effects + 9 ), pPPCtrl + 0x276, sizeof( PostProcessParameters_t ) ); // POST_EFFECT_ZOOMED_SNIPER_MOVING
	}

	oUpdatePostProcessingEffects( ecx );

	if ( g_menu.main.visuals.noscope.get( ) )
	{
		post_process_param->m_parameters[ PPPN_VIGNETTE_START ] = 0.f;
		post_process_param->m_parameters[ PPPN_VIGNETTE_BLUR_STRENGTH ] = 0.f;
	}

	// ZAYUMMMMMMMMMMMMMMMMMMMMMMMMMMM
	if ( g_gui.m_open )
	{
		// reset all parameters incase the game has them set
		// this makes our blur consistant across all maps.
		std::memset( post_process_param->m_parameters, 0, POST_PROCESS_PARAMETER_COUNT * sizeof( float ) );

		post_process_param->m_parameters[ PPPN_VIGNETTE_BLUR_STRENGTH ] = post_process_param->m_parameters[ PPPN_FILM_GRAIN_STRENGTH ] = post_process_param->m_parameters[ PPPN_SCREEN_BLUR_STRENGTH ] = 2.15 * 0.5f;
	}
}

enum PaintModes_t {
	PAINT_UIPANELS = ( 1 << 0 ),
	PAINT_INGAMEPANELS = ( 1 << 1 ),
};

using FnPaint = void( __fastcall* )( void*, void*, PaintModes_t );
FnPaint oPaint;

void __fastcall hkPaint( IEngineVGui* ecx, void* edx, PaintModes_t mode )
{
	static auto start = pattern::find( g_csgo.m_vguimatsurface_dll, XOR( "55 8B EC 83 E4 C0 83 EC 38 80 3D ? ? ? ? ? 56 57 8B F9 75 53" ) ).as< void( __thiscall* ) ( void* ) >( );
	static auto end = pattern::find( g_csgo.m_vguimatsurface_dll, XOR( "8B 0D ? ? ? ? 56 C6 05 ? ? ? ? ? 8B 01 FF 90" ) ).as< void( __thiscall* ) ( void* ) >( );

	using GetCurrentTonemappingSystem = void* ( __cdecl* )( );
	static auto GetCurrentTonemappingSystemFn = pattern::find( g_csgo.m_client_dll,
		XOR( "A1 ? ? ? ? A8 ? 75 ? 83 C8 ? B9 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? B8" ) ).as<GetCurrentTonemappingSystem>( );

	auto current_tmap = GetCurrentTonemappingSystemFn( );
	float backup = *( float* )( ( uintptr_t )current_tmap + 0x560 );

	const vec3_t def( 1.f, 1.f, 1.f );

	if ( g_cl.m_processing && g_cl.m_local )
	{
		g_csgo.m_prediction->SetLocalViewAngles( g_cl.m_sent_angles );
	}

	g_csgo.m_material_system->GetRenderContext( )->SetToneMappingScaleLinear( def ); // some weird drawing shit was bugging so lets just force it here too.. no problem, most likely the render context tonemap doesnt get reset for vgui rendering because it runs beforehand, but because we modify the tonemap we need to set it back here as well.

	oPaint( ecx, edx, mode );

	if ( ecx->m_static_transition_panel && ( mode & PAINT_UIPANELS ) )
	{
		start( g_csgo.m_surface );
		g_cl.OnPaint( );
		end( g_csgo.m_surface );
	}

	g_csgo.m_material_system->GetRenderContext( )->SetToneMappingScaleLinear( vec3_t( backup, backup, backup ) );
}

using ModifyEyePosition_t = void( __fastcall* )( void* ecx, void* edx, vec3_t& pos );
ModifyEyePosition_t oModifyEyePosition;

void __fastcall ModifyEyePosition( CCSGOPlayerAnimState* ecx, void* edx, vec3_t& pos )
{	
	if ( ecx->m_player != g_cl.m_local )
		return oModifyEyePosition( ecx, edx, pos );

	if ( g_hooks.m_calc_view )
		return;

	pos.z = g_cl.m_shoot_pos.z;
}

using FnHudScopePaint = void( __fastcall* )( void*, void* );
FnHudScopePaint oHudScopePaint;

void __fastcall hkHudScopePaint( void* ecx, void* edx ) {
	if ( g_menu.main.visuals.noscope.get( ) ) {
		static IMaterial* scope = g_csgo.m_material_system->FindMaterial( "dev/scope_bluroverlay", "Other textures" ); // sg and aug overlay
		scope->SetFlag( MATERIAL_VAR_NO_DRAW, true );
		return;
	}

	oHudScopePaint( ecx, edx );
}

using FnProcessInterpolatedList = void( __stdcall* )( );
FnProcessInterpolatedList oProcessInterpolatedList;

void __stdcall hkProcessInterpolatedList( ) {
	static auto s_bAllowExtrapolation = pattern::find( g_csgo.m_client_dll, XOR( "A2 ? ? ? ? 8B 45 E8" ) ) + 1;
	**( bool** )( s_bAllowExtrapolation ) = false;

	oProcessInterpolatedList( );
}

using fnCSBloodSprayCallback = void( __cdecl* )( void*, void* );
fnCSBloodSprayCallback o_CSBloodSprayCallback;

void __cdecl hkCSBloodSprayCallback( void* thisptr, void* edx ) {
	if ( g_menu.main.misc.increase_responsiveness.get( ) ) {
		auto effect_data = reinterpret_cast< C_TEEffectDispatch* >( thisptr );

		if ( !effect_data/*
			|| ( effect_data->m_EffectData.m_iEffectName != 550065032
				&& effect_data->m_EffectData.m_iEffectName != 555570056
				&& effect_data->m_EffectData.m_iEffectName != 553800584 )*/ )
			return o_CSBloodSprayCallback( thisptr, edx );

		if ( effect_data->m_EffectData.m_flMagnitude < 0.99 )
			return o_CSBloodSprayCallback( thisptr, edx ); // headshot sparks are at 0 i believe?

		effect_data->m_EffectData.m_flMagnitude = 100; // force >40, this will make fx_cs_bloodspray huge, doesnt matter what num as long as its over 40
	}

	return o_CSBloodSprayCallback( thisptr, edx );
}

using fnSetChoked = void( __fastcall* )( void*, void* );
fnSetChoked oSetChoked;

void __fastcall hkSetChoked( void* ecx, void* edx )
{
	if ( g_cl.m_local
		&& g_cl.m_local->alive( )
		&& g_csgo.m_engine->IsInGame( )
		&& !g_csgo.m_gamerules->m_bFreezePeriod( )
		&& !( g_cl.m_local->m_fFlags( ) & FL_FROZEN ) ) {
		const int out_sequence = g_csgo.m_cl->m_net_channel->m_out_seq;
		const int choked_packets = g_csgo.m_cl->m_net_channel->m_choked_packets;

		g_csgo.m_cl->m_net_channel->m_choked_packets = 0;

		// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/engine/net_chan.cpp#L2244
		g_csgo.m_cl->m_net_channel->SendDatagram( nullptr );

		g_csgo.m_cl->m_net_channel->m_out_seq = out_sequence;
		g_csgo.m_cl->m_net_channel->m_choked_packets = choked_packets;
	}

	g_csgo.m_cl->m_net_channel->m_out_seq++; // sends to be done since move command use sequence number
	g_csgo.m_cl->m_net_channel->m_choked_packets++;
}

void __fastcall hkDrawModelExecute( void* ECX, void* EDX, IMatRenderContext* MatRenderContext, DrawModelState_t& DrawModelState, ModelRenderInfo_t& RenderInfo, matrix3x4_t* pCustomBoneToWorld )
{
	Player* entity = ( Player* )( RenderInfo.m_renderable->GetIClientUnknown( )->GetBaseEntity( ) );
	if ( !entity )
		return oDrawModelExecute( ECX, MatRenderContext, DrawModelState, RenderInfo, pCustomBoneToWorld );

	if ( !g_chams.OnDrawModelExecute( entity, ECX, MatRenderContext, DrawModelState, RenderInfo, pCustomBoneToWorld ) )
		oDrawModelExecute( ECX, MatRenderContext, DrawModelState, RenderInfo, pCustomBoneToWorld );
}

using AddRenderableFn = void( __thiscall* )( void* ecx, void* renderable, bool render_with_viewmodels, int type, int model_type, int split_screen_enables );
AddRenderableFn oAddRenderable;

void __fastcall hkAddRenderable( void* ecx, void* edx, void* renderable, bool render_with_viewmodels, int type, int model_type, int split_screen_enables )
{
	auto renderable_addr = ( std::uintptr_t )renderable;
	if ( !renderable_addr || renderable_addr == 0x4 )
		return oAddRenderable( ecx, renderable, render_with_viewmodels, type, model_type, split_screen_enables );

	auto entity = ( Entity* )( renderable_addr - 0x4 );
	if ( !entity )
		return oAddRenderable( ecx, renderable, render_with_viewmodels, type, model_type, split_screen_enables );

	int index = *( int* )( ( std::uintptr_t )entity + 0x64 );

	if ( index < 1 || index > 64 )
		return oAddRenderable( ecx, renderable, render_with_viewmodels, type, model_type, split_screen_enables );

	if ( index == g_csgo.m_engine->GetLocalPlayer( ) )
		type = 1; // make chams render on top of props and in odd lighting areas

	oAddRenderable( ecx, renderable, render_with_viewmodels, type, model_type, split_screen_enables );
}

using FnListInLeavesBox = int( __thiscall* )( void*, vec3_t&, vec3_t&, unsigned short*, int );
FnListInLeavesBox oListLeavesInBox;

int __fastcall hkListLeavesInBox( void* ecx, void* edx, vec3_t& mins, vec3_t& maxs, unsigned short* pList, int listMax )
{
	if ( *( uint32_t* )_ReturnAddress( ) != 0x8B087D8B )
		return oListLeavesInBox( ecx, mins, maxs, pList, listMax );

	// get current renderable info from stack ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L1470 )
	auto info = *( RenderableInfo_t** )( ( uintptr_t )_AddressOfReturnAddress( ) + 0x14 );
	if ( !info || !info->m_pRenderable )
		return oListLeavesInBox( ecx, mins, maxs, pList, listMax );

	// check if disabling occulusion for players ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L1491 )
	auto base_entity = info->m_pRenderable->GetIClientUnknown( )->GetBaseEntity( );
	if ( !base_entity )
		return oListLeavesInBox( ecx, mins, maxs, pList, listMax );

	auto id = base_entity->GetClientClass( )->m_ClassID;

	if ( !base_entity->IsPlayer( ) && id != _CEconWearable && id != _CCSRagdoll )
		return oListLeavesInBox( ecx, mins, maxs, pList, listMax );

	// fix render order, force translucent group ( https://www.unknowncheats.me/forum/2429206-post15.html )
   // AddRenderablesToRenderLists: https://i.imgur.com/hcg0NB5.png ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L2473 )
	info->m_Flags &= ~0x100;
	info->m_Flags2 |= 0xC0; // we do this in addrenderable...

	// extend world space bounds to maximum ( https://github.com/pmrowla/hl2sdk-csgo/blob/master/game/client/clientleafsystem.cpp#L707 )
	static vec3_t map_min = vec3_t( -16384.0f, -16384.0f, -16384.0f );
	static vec3_t map_max = vec3_t( 16384.0f, 16384.0f, 16384.0f );
	return oListLeavesInBox( ecx, map_min, map_max, pList, listMax );
}

// 55 8B EC 83 E4 C0 A1 ?? ?? ?? ?? 81 EC 74

typedef int( __thiscall* StartSound_Immediatefn )( StartSoundParams_t* );
StartSound_Immediatefn oStartSound_Immediate;

int __fastcall hkStartSound_Immediate( StartSoundParams_t* params, int edx )
{
	if ( g_inputpred.m_in_pred )
		return 0;

	if ( !params || !params->pSfx )
		return 0;

	char name[ MAX_PATH ];
	params->pSfx->getname( name, MAX_PATH );

	Player* p = g_csgo.m_entlist->GetClientEntity<Player*>( params->soundsource );

	if ( p && !p->IsPlayer( ) )
	{
		const auto parent = p->GetShadowParent( );
		const auto unknown = parent != nullptr ? parent->GetIClientUnknown( ) : nullptr;
		const auto owner = unknown != nullptr ? reinterpret_cast< Player* const >( unknown->GetIClientEntity( ) ) : nullptr;

		if ( owner && owner->IsPlayer( ) )
			p = owner;
	}

	if ( !p || p->index( ) < 1 || p->index( ) > 64 || !g_cl.m_local )
		return oStartSound_Immediate( params );

	const char* auto_semiauto_switch = XOR( "auto_semiauto_switch" ); //_( auto_semiauto_switch, "auto_semiauto_switch" );
	const char* player_footsteps = XOR( "player\\footsteps" ); //_( auto_semiauto_switch, "auto_semiauto_switch" );
	const char* player_land = XOR( "\\land" ); //_( auto_semiauto_switch, "auto_semiauto_switch" );

	const auto wpn = strstr( name, "weapons" );

	auto origin = params->origin;

	const auto land = strstr( name, player_land );
	if ( land )
		origin.z -= 32.f;

	if ( wpn || strstr( name, player_footsteps ) || land )
	{

		if ( p->dormant( ) )
		{
			CGameTrace tr;
			Ray ray( origin + vec3_t( 0.f, 0.f, 10.f ), origin - vec3_t( 0.f, 0.f, 74.04f ) );
			CTraceFilterWorldOnly filter {};
			g_csgo.m_engine_trace->TraceRay( ray, MASK_PLAYERSOLID, &filter, &tr );
			if ( tr.m_fraction < 1.f )
				origin = tr.m_endpos;

			g_visuals.origin_cache[ p->index( ) - 1 ] = origin;
			g_visuals.m_time_cache[ p->index( ) - 1 ] = g_csgo.m_globals->m_realtime;
			g_visuals.m_opacities[ p->index( ) - 1 ] = 1.f;
			g_visuals.m_draw[ p->index( ) - 1 ] = true;
			p->SetAbsOrigin( origin );
			p->m_bSpotted( ) = true;
		}
	}

	return oStartSound_Immediate( params );
}

typedef int( __fastcall* CL_DispatchSoundfn )( SoundInfo_t* );
CL_DispatchSoundfn oCL_DispatchSound;

int __fastcall hkCL_DispatchSound( SoundInfo_t* sound, void* edx ) // if somehow the other one didn't get their info this one should
{
	if ( sound->m_origin.is_zero( ) )
		return oCL_DispatchSound( sound );

	if ( sound->m_entity_index < 1 || sound->m_entity_index > g_csgo.m_globals->m_max_clients || sound->m_entity_index == g_csgo.m_engine->GetLocalPlayer( ) )
		return oCL_DispatchSound( sound );

	Player* plr = g_csgo.m_entlist->GetClientEntity<Player*>( sound->m_entity_index );

	if ( !plr )
		return oCL_DispatchSound( sound );

	if ( plr->dormant( ) && g_visuals.m_time_cache[ plr->index( ) - 1 ] < g_csgo.m_globals->m_realtime ) // only do this if we couldn't find it from other stuff
	{
		vec3_t pos = sound->m_origin;

		plr->SetAbsOrigin( pos );
		g_visuals.origin_cache[ plr->index( ) - 1 ] = pos;
		g_visuals.m_time_cache[ plr->index( ) - 1 ] = g_csgo.m_globals->m_realtime;
		g_visuals.m_opacities[ plr->index( ) - 1 ] = 1.f;
		g_visuals.m_draw[ plr->index( ) - 1 ] = true;
		plr->m_bSpotted( ) = true;
	}

	return oCL_DispatchSound( sound );
}

using ShouldSkipAnimationFramefn = bool( __fastcall* )( void*, void* );
inline ShouldSkipAnimationFramefn oShouldSkipAnimationFrame;
bool __fastcall ShouldSkipAnimationFrame( void* ecx, uint32_t* edx )
{
	return false;
}

using StartSoundfn = void( __fastcall* )( void*, void*, const vec3_t&, const char* );
inline StartSoundfn oStartSound;
void __fastcall StartSound( void* ecx, void* edx, const vec3_t& origin, const char* soundname )
{
	if ( g_inputpred.m_in_pred )
		return;

	if ( !g_movement.m_computing )
		oStartSound( ecx, edx, origin, soundname );
}

typedef void( __fastcall* fnCalcView ) ( void*, void*, vec3_t&, vec3_t&, float&, float&, float& );
inline fnCalcView oCalcView;

void __fastcall CalcView( void* ecx, void* edx, vec3_t& origin, vec3_t& eye_angles, float& znear, float& zfar, float& flfov ) {
	if ( ecx != g_cl.m_local
		|| !g_cl.m_local
		|| !g_cl.m_local->alive( ) ) {
		g_hooks.m_calc_view = true;
		oCalcView( ecx, edx, origin, eye_angles, znear, zfar, flfov );
		g_hooks.m_calc_view = false;
		return;
	}

	ang_t aim_punch = g_cl.m_local->m_aimPunchAngle( );
	ang_t view_punch = g_cl.m_local->m_viewPunchAngle( );

	if ( g_menu.main.visuals.novisrecoil.get( ) ) {
		g_cl.m_local->m_aimPunchAngle( ) = ang_t( 0, 0, 0 );
		g_cl.m_local->m_viewPunchAngle( ) = ang_t( 0, 0, 0 );
	}

	g_hooks.m_calc_view = true;
	oCalcView( ecx, edx, origin, eye_angles, znear, zfar, flfov );
	g_hooks.m_calc_view = false;

	if ( g_menu.main.visuals.novisrecoil.get( ) ) {
		g_cl.m_local->m_aimPunchAngle( ) = aim_punch;
		g_cl.m_local->m_viewPunchAngle( ) = view_punch;
	}
}

using SetupLeanFn = void( __thiscall* )( void* _this );
inline SetupLeanFn oSetupLean;

// just rape my butt ok
void __fastcall hkSetupLean( void* _this )
{
	// we dont want lean for enemy playesr
	if ( g_hooks.m_animation_setup )
		return;

	oSetupLean( _this );
}

void Hooks::init( )
{
	MH_Initialize( );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 53 8B 5D ? 56 57 FF 75 ? 8B F1" ) ), &CalcView, reinterpret_cast< void** >( &oCalcView ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 53 56 8B 35 ?? ?? ?? ?? 57 8B F9 8B" ) ), &SendWeaponAnim, reinterpret_cast< void** >( &oSendWeaponAnim ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 51 53 56 8B F1 0F" ) ), &SetupWeaponAction, reinterpret_cast< void** >( &oSetupWeaponAction ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 8B 0D" ) ), &SetupBones, reinterpret_cast< void** >( &oSetupBones ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 51 53 56 8B 35 ? ? ? ? 57 8B F9 85 F6 74 10 8B 06 8B CE 8B 80 ? ? ? ? FF D0 84" ) ), &UpdatePostProcessingEffects, reinterpret_cast< void** >( &oUpdatePostProcessingEffects ) );
	MH_CreateHook( pattern::find( g_csgo.m_engine_dll, XOR( "55 8B EC 83 EC 40 53 8B D9 8B 0D ? ? ? ? 89" ) ), &hkPaint, reinterpret_cast< void** >( &oPaint ) );
	//MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 53 8B D9 56 57 8B 53 5C" ) ), &hkSetVisualsData, reinterpret_cast< void** >( &oSetVisualsData ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F8 83 EC 58 56 57 8B F9 83 7F 60 00 " ) ), &ModifyEyePosition, reinterpret_cast< void** >( &oModifyEyePosition ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F8 83 EC 78 56 57 8B 3D ? ? ? ? 89 4C 24 18" ) ), &hkHudScopePaint, reinterpret_cast< void** >( &oHudScopePaint ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "0F B7 05 ? ? ? ? 3D ? ? ? ? 74 3F" ) ), &hkProcessInterpolatedList, reinterpret_cast< void** >( &oProcessInterpolatedList ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 8B 4D ? F3 0F 10 51 ? 8D 51" ) ), &hkCSBloodSprayCallback, reinterpret_cast< void** >( &o_CSBloodSprayCallback ) );
	MH_CreateHook( pattern::find( g_csgo.m_engine_dll, XOR( "FF 41 ? FF 41 ? C3" ) ), &hkSetChoked, reinterpret_cast< void** >( &oSetChoked ) );
	MH_CreateHook( util::get_method<DrawModelExecuteFn>( g_csgo.m_model_render, 21 ), &hkDrawModelExecute, reinterpret_cast< void** >( &oDrawModelExecute ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 56 8B 75 08 57 FF 75 18" ) ), &hkAddRenderable, reinterpret_cast< void** >( &oAddRenderable ) );
	MH_CreateHook( util::get_method< void*>( g_csgo.m_engine->GetBSPTreeQuery( ), 6 ), &hkListLeavesInBox, reinterpret_cast< void** >( &oListLeavesInBox ) );
	MH_CreateHook( pattern::find( g_csgo.m_engine_dll, XOR( "55 8B EC 83 E4 C0 A1 ?? ?? ?? ?? 81 EC 74" ) ), &hkStartSound_Immediate, reinterpret_cast< void** >( &oStartSound_Immediate ) );
	MH_CreateHook( pattern::find( g_csgo.m_engine_dll, XOR( "55 8B EC 81 EC ?? ?? ?? ?? 56 8B F1 8D 4D 98 E8" ) ), &hkCL_DispatchSound, reinterpret_cast< void** >( &oCL_DispatchSound ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F8 83 EC 24 83 7D" ) ), &StartSound, reinterpret_cast< void** >( &oStartSound ) );

	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "55 8B EC 83 E4 F8 A1 ?? ?? ?? ?? 83 EC 20 F3" ) ), &hkSetupLean, reinterpret_cast< void** >( &oSetupLean ) );
	MH_CreateHook( pattern::find( g_csgo.m_client_dll, XOR( "57 8B F9 8B 07 8B 80 ? ? ? ? FF D0 84 C0 75 02" ) ), ShouldSkipAnimationFrame, reinterpret_cast< void** >( &oShouldSkipAnimationFrame ) );
	MH_EnableHook( MH_ALL_HOOKS );

	// hook wndproc.
	auto m_hWindow = FindWindowA( XOR( "Valve001" ), NULL );
	m_old_wndproc = ( WNDPROC )g_winapi.SetWindowLongA( m_hWindow, GWL_WNDPROC, util::force_cast< LONG >( Hooks::WndProc ) );

	// setup normal VMT hooks.
	m_client.init( g_csgo.m_client );
	m_client.add( CHLClient::SHUTDOWN, util::force_cast( &Hooks::Shutdown ) );
	m_client.add( CHLClient::LEVELINITPREENTITY, util::force_cast( &Hooks::LevelInitPreEntity ) );
	m_client.add( CHLClient::LEVELINITPOSTENTITY, util::force_cast( &Hooks::LevelInitPostEntity ) );
	m_client.add( CHLClient::LEVELSHUTDOWN, util::force_cast( &Hooks::LevelShutdown ) );
	m_client.add( CHLClient::FRAMESTAGENOTIFY, util::force_cast( &Hooks::FrameStageNotify ) );

	m_engine.init( g_csgo.m_engine );
	m_engine.add( IVEngineClient::ISCONNECTED, util::force_cast( &Hooks::IsConnected ) );
	m_engine.add( IVEngineClient::ISHLTV, util::force_cast( &Hooks::IsHLTV ) );
	m_engine.add( IVEngineClient::ISGETSCREENASPECTRATIO, util::force_cast( &Hooks::GetScreenApsectRatio ) );

	m_prediction.init( g_csgo.m_prediction );
	m_prediction.add( CPrediction::POSTENTITYPACKETRECEIVED, util::force_cast( &Hooks::PostEntityPacketReceived ) );
	m_prediction.add( CPrediction::POSTNETWORKDATARECEIVED, util::force_cast( &Hooks::PostNetworkDataReceived ) );
	m_prediction.add( CPrediction::RUNCOMMAND, util::force_cast( &Hooks::RunCommand ) );
	m_prediction.add( CPrediction::FINISHMOVE, util::force_cast( &Hooks::FinishMove ) );

	m_game_movement.init( g_csgo.m_game_movement );
	m_game_movement.add( CGameMovement::PROCESSMOVEMENT, util::force_cast( &Hooks::ProcessMovement ) );

	m_client_mode.init( g_csgo.m_client_mode );
	m_client_mode.add( IClientMode::SHOULDDRAWPARTICLES, util::force_cast( &Hooks::ShouldDrawParticles ) );
	m_client_mode.add( IClientMode::SHOULDDRAWFOG, util::force_cast( &Hooks::ShouldDrawFog ) );
	m_client_mode.add( IClientMode::OVERRIDEVIEW, util::force_cast( &Hooks::OverrideView ) );
	m_client_mode.add( IClientMode::CREATEMOVE, util::force_cast( &Hooks::CreateMove ) );
	m_client_mode.add( IClientMode::DOPOSTSPACESCREENEFFECTS, util::force_cast( &Hooks::DoPostScreenSpaceEffects ) );
	m_client_mode.add( IClientMode::GETVIEWMODELFOV, util::force_cast( &Hooks::GetViewmodelFOV ) );

	m_surface.init( g_csgo.m_surface );
	m_surface.add( ISurface::LOCKCURSOR, util::force_cast( &Hooks::LockCursor ) );
	m_surface.add( ISurface::ONSCREENSIZECHANGED, util::force_cast( &Hooks::OnScreenSizeChanged ) );

	m_shadow_mgr.init( g_csgo.m_shadow_mgr );
	m_shadow_mgr.add( IClientShadowMgr::COMPUTESHADOWDEPTHTEXTURES, util::force_cast( &Hooks::ComputeShadowDepthTextures ) );

	m_view_render.init( g_csgo.m_view_render );
	m_view_render.add( CViewRender::RENDER2DEFFECTSPOSTHUD, util::force_cast( &Hooks::Render2DEffectsPostHUD ) );
	m_view_render.add( CViewRender::RENDERSMOKEOVERLAY, util::force_cast( &Hooks::RenderSmokeOverlay ) );

	m_material_system.init( g_csgo.m_material_system );
	m_material_system.add( IMaterialSystem::OVERRIDECONFIG, util::force_cast( &Hooks::OverrideConfig ) );

	// this is what crashes default supremacy.
	m_client_state.init( g_csgo.m_hookable_cl );
	m_client_state.add( CClientState::PACKETSTART, util::force_cast( &Hooks::PacketStart ) );

	m_move_helper.init( g_csgo.m_move_helper );
	m_move_helper.add( IMoveHelper::PROCESSIMPACTS, util::force_cast( &Hooks::ProcessImpacts ) );

	g_custom_entity_listener.init( );

	m_device.init( g_csgo.m_device );
	/*m_device.add( 16, util::force_cast( &Hooks::Reset ) );
	m_device.add( 17, util::force_cast( &Hooks::Present ) );*/

	// cvar hooks.
	m_debug_spread.init( g_csgo.weapon_debug_spread_show );
	m_debug_spread.add( ConVar::GETINT, util::force_cast( &Hooks::DebugSpreadGetInt ) );

	// set netvar proxies.
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_angEyeAngles[0]" ), Pitch_proxy, m_Pitch_original );
	g_netvars.SetProxy( HASH( "DT_CSPlayer" ), HASH( "m_flLowerBodyYawTarget" ), Body_proxy, m_Body_original );
	g_netvars.SetProxy( HASH( "DT_CSRagdoll" ), HASH( "m_vecForce" ), Force_proxy, m_Force_original );
	g_netvars.SetProxy( HASH( "DT_CSRagdoll" ), HASH( "m_flAbsYaw" ), AbsYaw_proxy, m_AbsYaw_original );
}