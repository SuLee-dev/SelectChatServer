#pragma once

#include <vector>
#include <unordered_map>

namespace NServerNetLib
{
	class TcpNetwork;
	class ILog;
}

namespace NLogicLib
{
	struct LobbyManagerConfig
	{
		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};

	struct LobbySmallInfo
	{
		short num;
		short UserCount;
	};

	class Lobby;

	class LobbyManager
	{
		using TcpNet = NServerNetLib::ITcpNetwork;
		using ILog = NServerNetLib::ILog;

	public:
		LobbyManager();
		virtual ~LobbyManager();

		void Init(const LobbyManagerConfig config, TcpNet* pNetwork, ILog* pLogger);

		Lobby* GetLobby(short lobbyId);

	public:
		void SendLobbyListInfo(const int sessionIndex);


	private:
		TcpNet* m_pRefNetwork;
		ILog* m_pRefLogger;

		std::vector<Lobby> m_LobbyList;
	};
}