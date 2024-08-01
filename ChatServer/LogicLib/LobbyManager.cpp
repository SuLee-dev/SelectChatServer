#include <algorithm>

#include "../ServerNetLib/TcpNetwork.h"
#include "../ServerNetLib/ILog.h"
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"
#include "Lobby.h"
#include "LobbyManager.h"

using ERROR_CODE = NCommon::ERROR_CODE;
using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	LobbyManager::LobbyManager()
	{
	}

	LobbyManager::~LobbyManager()
	{
	}

	void LobbyManager::Init(const LobbyManagerConfig config, TcpNet* pNetwork, ILog* pLogger)
	{
		m_pRefNetwork = pNetwork;
		m_pRefLogger = pLogger;

		for (int i = 0; i < config.MaxLobbyCount; ++i)
		{
			Lobby lobby;
			lobby.Init(short(i), (short)config.MaxLobbyUserCount, (short)config.MaxRoomCountByLobby, (short)config.MaxRoomUserCount);
			lobby.SetNetwork(m_pRefNetwork, m_pRefLogger);

			m_LobbyList.push_back(lobby);
		}
	}

	Lobby* LobbyManager::GetLobby(short lobbyId)
	{
		if (lobbyId < 0 || lobbyId >= (short)m_LobbyList.size())
			return nullptr;

		return &m_LobbyList[lobbyId];
	}

	void LobbyManager::SendLobbyListInfo(const int sessionIndex)
	{
		NCommon::PktLobbyListRes resPkt;
		resPkt.ErrorCode = (short)ERROR_CODE::NONE;
		resPkt.LobbyCount = static_cast<short>(m_LobbyList.size());

		int index = 0;
		for (Lobby& lobby : m_LobbyList)
		{
			resPkt.LobbyList[index].LobbyId = lobby.GetIndex();
			resPkt.LobbyList[index].LobbyUserCount = lobby.GetUserCount();
			resPkt.LobbyList[index].LobbyMaxUserCount = lobby.MaxUserCount();

			++index;
		}

		m_pRefNetwork->SendData(sessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(resPkt), (char*)&resPkt);
	}
}