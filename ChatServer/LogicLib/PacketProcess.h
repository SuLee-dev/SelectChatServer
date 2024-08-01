#pragma once

#include <memory>
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"
#include "../ServerNetLib/Define.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NServerNetLib
{
	class ITcpNetwork;
	class ILog;
}

namespace NLogicLib
{
	class ConnectedUserManager;
	class UserManager;
	class LobbyManager;

	using ServerConfog = NServerNetLib::ServerConfig;

	class PacketProcess
	{
		using PacketInfo = NServerNetLib::RecvPacketInfo;
		typedef ERROR_CODE(PacketProcess::*PacketFunc)(PacketInfo);
		PacketFunc PacketFuncArray[(int)NCommon::PACKET_ID::MAX];

		using TcpNet = NServerNetLib::ITcpNetwork;
		using ILog = NServerNetLib::ILog;

	public:
		PacketProcess();
		~PacketProcess();

		void Init(TcpNet* pNetwork, UserManager* pUserMgr, LobbyManager* pLobbyMgr, ServerConfog* pConfig, ILog* pLogger);

		void Process(PacketInfo packetInfo);

		void StateCheck();

	private:
		TcpNet* m_pRefNetwork;
		ILog* m_pRefLogger;

		UserManager* m_pRefUserMgr;
		LobbyManager* m_pRefLobbyMgr;

		std::unique_ptr<ConnectedUserManager> m_pConnectedUserManager;

	private:
		ERROR_CODE NtfSysConnectSession(PacketInfo packetInfo);
		ERROR_CODE NtfSysCloseSession(PacketInfo packetInfo);

		ERROR_CODE Login(PacketInfo packetInfo);

		ERROR_CODE LobbyList(PacketInfo packetInfo);

		ERROR_CODE LobbyEnter(PacketInfo packetInfo);

		ERROR_CODE LobbyLeave(PacketInfo packetInfo);

		ERROR_CODE RoomEnter(PacketInfo packetInfo);

		ERROR_CODE RoomLeave(PacketInfo packetInfo);

		ERROR_CODE RoomChat(PacketInfo packetInfo);

		ERROR_CODE DevEcho(PacketInfo packetInfo);
	};
}