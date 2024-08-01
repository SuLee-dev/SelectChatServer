#include <algorithm>

#include "../ServerNetLib/TcpNetwork.h"
#include "../ServerNetLib/ILog.h"
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"
#include "User.h"
#include "Room.h"
#include "Lobby.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	Lobby::Lobby() {}

	Lobby::~Lobby() {}

	void Lobby::Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount)
	{
		m_LobbyIndex = lobbyIndex;
		m_MaxUserCount = maxLobbyUserCount;

		for (int i = 0; i < maxLobbyUserCount; ++i)
		{
			LobbyUser lobbyUser;
			lobbyUser.Index = (short)i;
			lobbyUser.pUser = nullptr;

			m_UserList.push_back(lobbyUser);
		}

		for (int i = 0; i < maxRoomCountByLobby; ++i)
		{
			m_RoomList.emplace_back(new Room());
			m_RoomList[i]->Init((short)i, maxRoomUserCount);
		}
	}

	void Lobby::Release()
	{
		for (int i = 0; i < (int)m_RoomList.size(); ++i)
		{
			delete m_RoomList[i];
		}

		m_RoomList.clear();
	}

	void Lobby::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
	{
		m_pRefNetwork = pNetwork;
		m_pRefLogger = pLogger;

		for (auto pRoom : m_RoomList)
		{
			pRoom->SetNetwork(m_pRefNetwork, m_pRefLogger);
		}
	}

	ERROR_CODE Lobby::EnterUser(User* pUser)
	{
		if (m_UserIndexDic.size() >= m_MaxUserCount)
			return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;

		if (FindUser(pUser->GetIndex()) != nullptr)
			return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;

		auto addRet = AddUser(pUser);
		if (addRet != ERROR_CODE::NONE)
			return addRet;

		pUser->EnterLobby(m_LobbyIndex);

		m_UserIndexDic.insert({ pUser->GetIndex(), pUser });
		m_UserIDDic.insert({ pUser->GetID().c_str(), pUser });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Lobby::LeaveUser(const int userIndex)
	{
		RemoveUser(userIndex);

		auto pUser = FindUser(userIndex);
		if (pUser == nullptr)
			return ERROR_CODE::LOBBY_LEAVE_USER_INVALID_UNIQUEINDEX;

		pUser->LeaveLobby();

		m_UserIndexDic.erase(pUser->GetIndex());
		m_UserIDDic.erase(pUser->GetID().c_str());

		return ERROR_CODE::NONE;
	}

	User* Lobby::FindUser(const int userIndex)
	{
		auto iter = m_UserIndexDic.find(userIndex);

		if (iter == m_UserIndexDic.end())
			return nullptr;

		return (User*)iter->second;
	}

	ERROR_CODE Lobby::AddUser(User* pUser)
	{
		auto findIter = std::find_if(m_UserList.begin(), m_UserList.end()
			, [](LobbyUser& lobbyUser)
			{
				return lobbyUser.pUser == nullptr;
			});

		findIter->pUser = pUser;
		return ERROR_CODE::NONE;
	}

	void Lobby::RemoveUser(const int userIndex)
	{
		auto findIter = std::find_if(m_UserList.begin(), m_UserList.end()
			, [userIndex](LobbyUser& lobbyUser)
			{
				return lobbyUser.pUser != nullptr && lobbyUser.Index == userIndex;
			});

		if (findIter == m_UserList.end())
			return;

		findIter->pUser = nullptr;
	}

	short Lobby::GetUserCount()
	{
		return static_cast<short>(m_UserList.size());
	}

	void Lobby::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserIndex)
	{
		for (auto& pUser : m_UserIndexDic)
		{
			if ((pUser.second->GetIndex() != passUserIndex) && pUser.second->IsCurDomainInLobby())
				m_pRefNetwork->SendData(pUser.second->GetSessionIndex(), packetId, dataSize, pData);
		}
	}

	Room* Lobby::CreateRoom()
	{
		for (int i = 0; i < (int)m_RoomList.size(); ++i)
		{
			if (m_RoomList[i]->IsUsed() == false)
				return m_RoomList[i];
		}

		return nullptr;
	}

	Room* Lobby::GetRoom(const short roomIndex)
	{
		if (roomIndex < 0 || roomIndex >= m_RoomList.size())
			return nullptr;

		return m_RoomList[roomIndex];
	}
}