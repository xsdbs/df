#include "includes.h"

void Hooks::PacketStart( int incoming, int outgoing ) {
	// erase commands that are out of range by a huge margin.
	g_cl.m_sent_cmds.erase(
		std::remove_if(
			g_cl.m_sent_cmds.begin( ),
			g_cl.m_sent_cmds.end( ),
			[ outgoing ]( const int& cmd ) {
				return abs( static_cast< int >( outgoing - cmd ) ) >= 150;
			}
		),
		g_cl.m_sent_cmds.end( )
	);

	// rollback the ack count to what we aimed for.
	int target_acknowledged = outgoing;
	for ( const auto cmd : g_cl.m_sent_cmds )
		if ( outgoing >= cmd )
			target_acknowledged = cmd;

	g_hooks.m_client_state.GetOldMethod< PacketStart_t >( CClientState::PACKETSTART )( this, incoming, target_acknowledged );
}