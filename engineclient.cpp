#include "includes.h"

void CClientState::NET_ProcessSocket() {

	if (!m_net_channel || IsPaused())
		return;

	g_csgo.NET_ProcessSocket.as<void(__fastcall*)(int, uintptr_t)>()(0, reinterpret_cast<uintptr_t>(this) + 12);
}

int& CClientState::GetCommandAcknowledged() {
	return *reinterpret_cast<int*>(uintptr_t(this) + 0x4CBC);
}

bool& CClientState::IsPaused() {
	return *reinterpret_cast<bool*> (uintptr_t(this) + 0x0178);
}

int& CClientState::GetSignOnState() {
	return *reinterpret_cast<int*>(uintptr_t(this) + 0x0108);
}
