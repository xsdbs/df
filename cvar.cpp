#include "includes.h"

int Hooks::DebugSpreadGetInt( )
{
	if ( !g_menu.main.visuals.force_xhair.get( ) )
		return g_hooks.m_debug_spread.GetOldMethod< GetInt_t >( ConVar::GETINT )( this );

	static auto address = pattern::find( g_csgo.m_client_dll, "85 C0 0F 84 ? ? ? ? 8B 07 8B CF 8B 80 04 05 00 00" );

	if ( ( uintptr_t )_ReturnAddress( ) == address && g_cl.m_processing && !g_cl.m_local->m_bIsScoped( ) )
		return 3;

	return g_hooks.m_debug_spread.GetOldMethod< GetInt_t >( ConVar::GETINT )( this );
}