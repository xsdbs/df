#include "includes.h"

int Hooks::SendDatagram( void* data )
{
	if ( !g_cl.m_processing || !g_csgo.m_engine->IsConnected( ) || !g_csgo.m_engine->IsInGame( ) )
		return g_hooks.m_net_channel.GetOldMethod< SendDatagram_t >( INetChannel::SENDDATAGRAM )( this, data );

	const int backup1 = g_csgo.m_net->m_in_rel_state;
	const int backup2 = g_csgo.m_net->m_in_seq;

	INetChannel* net = g_csgo.m_engine->GetNetChannelInfo( );

	if ( g_menu.main.misc.fake_latency.get( ) || g_aimbot.m_fake_latency )
	{
		float wish_ping = g_cl.m_fakelatency_amt;
		wish_ping /= 1000.f;
		wish_ping -= g_cl.m_lerp;
		wish_ping = std::clamp( wish_ping, g_csgo.m_globals->m_interval, g_csgo.sv_maxunlag->GetFloat( ) );

		const float latency = net->GetLatency( 0 );
		if ( latency < wish_ping )
		{
			net->m_in_seq -= game::TIME_TO_TICKS( wish_ping - latency );

			for ( auto& s : g_cl.m_sequences )
			{
				if ( s.m_seq != net->m_in_seq )
					continue;

				net->m_in_rel_state = s.m_state;
			}
		}
	}

	const int ret = g_hooks.m_net_channel.GetOldMethod< SendDatagram_t >( INetChannel::SENDDATAGRAM )( this, data );

	g_csgo.m_net->m_in_rel_state = backup1;
	g_csgo.m_net->m_in_seq = backup2;

	return ret;
}

void Hooks::ProcessPacket( void* packet, bool header )
{
	g_hooks.m_net_channel.GetOldMethod< ProcessPacket_t >( INetChannel::PROCESSPACKET )( this, packet, header );

	g_cl.UpdateIncomingSequences( );

	for ( CEventInfo* it = g_csgo.m_cl->m_events;
		it != nullptr;
		it = it->m_next ) {

		// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/engine/cl_parse_event.cpp#L89
		if ( it->m_class_id == 0 )
			continue;

		// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/engine/cl_parse_event.cpp#L96
		it->m_fire_delay = 0.f;
	}

	g_csgo.m_engine->FireEvents( );

	if ( g_csgo.m_prediction->servercmds( ) == 0 )
		g_csgo.m_prediction->servercmds( ) = INT_MAX;

	g_inputpred.update( );

	g_shots.Process( );

	for ( int i : g_cl.m_unsorted_players )
	{
		auto data = &g_lagcompensation.m_players[ i - 1 ];
		if ( data ) {
			Player* player = g_csgo.m_entlist->GetClientEntity<Player*>( i );

			if ( player->m_bIsLocalPlayer( ) ) {
				data->reset( );
				continue;
			}

			data->OnNetUpdate( player );
		}
	}
}

bool Hooks::SendNetMsg( INetMessage& msg, bool bForceReliable, bool bVoice ) {
	if ( msg.GetType( ) == 14 ) // Return and don't send messsage if its FileCRCCheck
		return false;

	if ( msg.GetGroup( ) == 9 ) // fix fakelag voice transmit
		bVoice = true;

	//if (msg.GetGroup() == 11) {
	//	if (g_csgo.m_cl->m_delta_tick > 0)
	//		g_csgo.m_prediction->Update(g_csgo.m_cl->m_delta_tick, true, g_csgo.m_cl->m_last_command_ack, g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands);
	//}

	return g_hooks.m_net_channel.GetOldMethod< SendNetMsg_t >( INetChannel::SENDNETMSG )( this, msg, bForceReliable, bVoice );
}