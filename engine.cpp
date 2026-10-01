#include "includes.h"

bool Hooks::IsConnected( )
{
	static Address IsLoadoutAllowed { pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 04 B0 01 5F" ) ) };

	if ( _ReturnAddress( ) == IsLoadoutAllowed )
		return false;

	return g_hooks.m_engine.GetOldMethod< IsConnected_t >( IVEngineClient::ISCONNECTED )( this );
}

bool Hooks::IsHLTV( )
{
	// https://gitlab.com/KittenPopo/csgo-2018-source/-/blob/main/game/client/cstrike15/c_cs_player.cpp#L6289
	// familyhook only does setup vel
	static Address return_to_setup_velocity = pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80" ) );

	if ( _ReturnAddress( ) == return_to_setup_velocity )
		return true;

	return g_hooks.m_engine.GetOldMethod< IsHLTV_t >( IVEngineClient::ISHLTV )( this );
}

float Hooks::GetScreenApsectRatio( int width, int height )
{
	if ( !g_menu.main.misc.override_aspect_ratio.get( ) )
		return g_hooks.m_engine.GetOldMethod< IsGetScreenApsectRatio_t >( IVEngineClient::ISGETSCREENASPECTRATIO )( this, width, height );

	return g_menu.main.misc.aspect_ratio.get( ) > 0 ? g_menu.main.misc.aspect_ratio.get( ) / 100.f : g_hooks.m_engine.GetOldMethod< IsGetScreenApsectRatio_t >( IVEngineClient::ISGETSCREENASPECTRATIO )( this, width, height );
}

void Hooks::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char* pSample, float flVolume, float flAttenuation, int nSeed, int iFlags, int iPitch, const vec3_t* pOrigin, const vec3_t* pDirection, void* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity )
{
	if ( strstr( pSample, "null" ) )
	{
		iFlags = ( 1 << 2 ) | ( 1 << 5 );
	}

	g_hooks.m_engine_sound.GetOldMethod<EmitSound_t>( 5 )( this, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, flAttenuation, nSeed, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity );
}