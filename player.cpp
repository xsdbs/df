#include "includes.h"

void Hooks::DoExtraBoneProcessing( int studio_hdr, int a3, int a4, int a5, int a6, int a7 )
{
	return;
}
void Hooks::UpdateClientSideAnimation( )
{
	Player* player = ( Player* )this;
	if ( !player )
		return;

	CCSGOPlayerAnimState* state = player->m_PlayerAnimState( );
	if ( !state )
		return;

	const int old_frame = state->m_last_update_frame;

	state->m_player = nullptr;
	state->m_last_update_frame = g_csgo.m_globals->m_frame;
	g_hooks.m_UpdateClientSideAnimation( this );
	state->m_last_update_frame = old_frame;
	state->m_player = player;
}

void Hooks::WeaponShootPosition( void* eye )
{ 
	Player* player = ( Player* )this;
	if ( !player )
		return;

	CCSGOPlayerAnimState* state = player->m_PlayerAnimState( );
	if ( !state )
		return;

	state->m_smooth_height_valid = false;

	g_hooks.m_WeaponShootPosition( this, eye );
}

void Hooks::PostThink( )
{	
	if ( g_inputpred.m_in_pred )
		return;

	g_hooks.m_PostThink( this );
}

void Hooks::PostDataUpdate( DataUpdateType_t type )
{
	Player* player = ( Player* )( ( uintptr_t )this - 0x8 );
	if ( player )
	{
		if ( player->m_bIsLocalPlayer( ) )
		{
			g_skins.think( );
		}
	}

	g_hooks.m_PostDataUpdate( this, type );
}

void CustomEntityListener::OnEntityCreated( Entity* ent ) {
	if ( !ent )
		return;

	if ( ent->IsPlayer( ) ) {
		auto player = ent->as<Player*>( );
		if ( !player )
			return;

		const int idx = player->index( );
		if ( idx < 1 || idx > 64 )
			return;

		const int arr_idx = idx - 1;
		g_cl.m_unsorted_players.push_front( idx );

		AimPlayer* aim_data = &g_aimbot.m_players[ arr_idx ];
		LagCompensation* anim_data = &g_lagcompensation.m_players[ arr_idx ];
		VMT* player_vmt = &g_hooks.m_player[ arr_idx ];
		VMT* net_vmt = &g_hooks.m_player_networkable[ arr_idx ];

		if ( aim_data )
			aim_data->reset( );

		if ( anim_data ) {
			anim_data->reset( );
			anim_data->m_player = nullptr;
		}

		if ( player_vmt ) {
			player_vmt->reset( );
			player_vmt->init( player );

			g_hooks.m_DoExtraBoneProcessing = player_vmt->add<Hooks::DoExtraBoneProcessing_t>(
				Player::DOEXTRABONEPROCESSING, util::force_cast( &Hooks::DoExtraBoneProcessing ) );

			g_hooks.m_UpdateClientSideAnimation = player_vmt->add<Hooks::UpdateClientSideAnimation_t>(
				Player::UPDATECLIENTSIDEANIMATION, util::force_cast( &Hooks::UpdateClientSideAnimation ) );

			g_hooks.m_WeaponShootPosition = player_vmt->add<Hooks::WeaponShootPosition_t>(
				Player::WEAPONSHOOTPOSITION, util::force_cast( &Hooks::WeaponShootPosition ) );

			if ( idx == g_csgo.m_engine->GetLocalPlayer( ) ) {
				g_hooks.m_PostThink = player_vmt->add<Hooks::PostThink_t>(
					Player::POSTTHINK, util::force_cast( &Hooks::PostThink ) );
			}
		}

		if ( net_vmt ) {
			net_vmt->reset( );
			net_vmt->init( reinterpret_cast< void* >( uintptr_t( player ) + 0x8 ) );
			g_hooks.m_PostDataUpdate = net_vmt->add<Hooks::PostDataUpdate_t>(
				7, util::force_cast( &Hooks::PostDataUpdate ) );
		}
	}
	else if ( g_menu.main.visuals.proj.get( ) && g_cl.m_processing ) {
		auto cl = ent->GetClientClass( );
		if ( !cl )
			return;

		switch ( cl->m_ClassID ) {
		case _CMolotovProjectile:
		case _CBaseCSGrenadeProjectile:
		case _CSmokeGrenadeProjectile:
		case _CDecoyProjectile:
			g_trajectories.PushEntityOntoTrack( ent->GetRefEHandle( ) );
			break;
		default:
			break;
		}
	}
}

void CustomEntityListener::OnEntityDeleted( Entity* ent ) {
	if ( !ent || ent->index( ) < 1 || ent->index( ) > 64 )
		return;

	auto player = ent->as<Player*>( );
	if ( !player )
		return;

	const int arr_idx = player->index( ) - 1;

	AimPlayer* aim_data = &g_aimbot.m_players[ arr_idx ];
	LagCompensation* anim_data = &g_lagcompensation.m_players[ arr_idx ];
	VMT* player_vmt = &g_hooks.m_player[ arr_idx ];
	VMT* net_vmt = &g_hooks.m_player_networkable[ arr_idx ];

	if ( aim_data ) {
		aim_data->reset( );
		aim_data->m_entity = nullptr;
	}

	if ( anim_data ) {
		anim_data->reset( );
		anim_data->m_player = nullptr;
	}

	if ( player_vmt )
		player_vmt->reset( );

	if ( net_vmt )
		net_vmt->reset( );

	// remove from unsorted players
	auto& unsorted = g_cl.m_unsorted_players;
	for ( size_t i = 0; i < unsorted.size( ); ++i ) {
		if ( unsorted[ i ] == player->index( ) ) {
			unsorted.erase( unsorted.begin( ) + i );
			break;
		}
	}
}