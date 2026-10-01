#include "includes.h"

// familyhook hooks this and does something inside idk yet
void Hooks::FinishMove( Entity* player, CUserCmd* cmd, CMoveData* data )
{
	return g_hooks.m_prediction.GetOldMethod< FinishMove_t >( CPrediction::FINISHMOVE )( this, player, cmd, data );
}

void Hooks::PostEntityPacketReceived( )
{
	return;
}

void Hooks::ProcessImpacts( )
{
	// this is too early to call it
	if ( g_inputpred.m_in_pred )
		return;

	g_hooks.m_move_helper.GetOldMethod< ProcessImpacts_t >( IMoveHelper::PROCESSIMPACTS )( this );
}

void Hooks::RunCommand( Entity* ent, CUserCmd* cmd, IMoveHelper* movehelper )
{
	g_inputpred.run( ent, cmd, movehelper );
}

void Hooks::ProcessMovement( Entity* player, CMoveData* data )
{
	data->m_bGameCodeMovedPlayer = false;
	return g_hooks.m_game_movement.GetOldMethod< ProcessMovement_t >( CGameMovement::PROCESSMOVEMENT )( this, player, data );
}

void Hooks::PostNetworkDataReceived( int commands_acknowledged )
{
	if ( g_cl.m_processing && commands_acknowledged > 0 )
	{
		g_netdata.FixCompression( commands_acknowledged - 1 );
		g_netdata.StoreAnims( commands_acknowledged - 1, false );

		// this is the point where our info gets overrided with bungus shit from server
		// so restore our clients pred info from this cmd.
		// also, idk if we need to check tolerance or anytinnnnggg like that lool.
		//g_netdata.SetViewmodel( commands_acknowledged ); THIS DOES NOT WORK
	}

	return g_hooks.m_prediction.GetOldMethod< PostNetworkDataReceived_t >( CPrediction::POSTNETWORKDATARECEIVED )( this, commands_acknowledged );
}