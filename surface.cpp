#include "includes.h"

void Hooks::LockCursor( )
{
	if ( g_gui.m_open )
	{
		g_csgo.m_surface->UnlockCursor( );
	}
	else
	{
		g_hooks.m_surface.GetOldMethod< LockCursor_t >( ISurface::LOCKCURSOR )( this );
		g_csgo.m_input_system->EnableInput( true );
	}
}

void Hooks::OnScreenSizeChanged( int oldwidth, int oldheight )
{
	g_hooks.m_surface.GetOldMethod< OnScreenSizeChanged_t >( ISurface::ONSCREENSIZECHANGED )( this, oldwidth, oldheight );

	render::init( );
}