#include <tuple>

#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "User.h"
#include "UserManager.h"
#include "Lobby.h"
#include "LobbyManager.h"
#include "Room.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::RoomEnter(PacketInfo packetInfo)
	{
		auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.pRefData;
		NCommon::PktRoomEnterRes resPkt;

		auto errorCode = std::get<0>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
		auto pUser = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));

		if (errorCode != ERROR_CODE::NONE)
		{
			resPkt.SetError(errorCode);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
			return errorCode;
		}

		if (pUser->IsCurDomainInLobby() == false)
		{
			resPkt.SetError(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
			return ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN;
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);

		if (pLobby == nullptr)
		{
			resPkt.SetError(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
			return ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX;
		}

		Room* pRoom = nullptr;

		if (reqPkt->IsCreate)
		{
			pRoom = pLobby->AllocRoom();
			if (pRoom == nullptr)
			{
				resPkt.SetError(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
				m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
				return ERROR_CODE::ROOM_ENTER_EMPTY_ROOM;
			}

			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
			if (ret != ERROR_CODE::NONE)
			{
				resPkt.SetError(ret);
				m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
				return ret;
			}
		}
		else
		{
			pRoom = pLobby->GetRoom(reqPkt->RoomIndex);

			if (pRoom == nullptr)
			{
				resPkt.SetError(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
				m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
				return ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX;
			}
		}

		auto enterRet = pRoom->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE)
		{
			resPkt.SetError(enterRet);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
			return enterRet;
		}

		// 유저 정보를 룸에 들어왔다고 변경
		pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

		// 룸에 새로운 유저가 들어왔다고 알림
		pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());
	
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::RoomLeave(PacketInfo packetInfo)
	{
		NCommon::PktRoomLeaveRes resPkt;
		auto errorCode = std::get<0>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
		auto pUser = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));

		if (errorCode != ERROR_CODE::NONE)
		{
			resPkt.SetError(errorCode);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomLeaveRes), (char*)&resPkt);
			return errorCode;
		}

		if (pUser->IsCurDomainInRoom() == false)
		{
			resPkt.SetError(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomLeaveRes), (char*)&resPkt);
			return ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN;
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);

		if (pLobby == nullptr)
		{
			resPkt.SetError(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomLeaveRes), (char*)&resPkt);
			return ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX;
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr)
		{
			resPkt.SetError(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomLeaveRes), (char*)&resPkt);
			return ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX;
		}

		auto leaveRet = pRoom->LeaveUser(pUser->GetIndex());
		if (leaveRet != ERROR_CODE::NONE)
		{
			resPkt.SetError(leaveRet);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomLeaveRes), (char*)&resPkt);
			return leaveRet;
		}

		// 유저 정보를 로비로 변경
		pUser->EnterLobby(lobbyIndex);

		// 룸 전체에 유저가 나갔음을 알림
		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::RoomChat(PacketInfo packetInfo)
	{
		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
		NCommon::PktRoomChatRes resPkt;

		auto errorCode = std::get<0>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
		auto pUser = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));

		if (errorCode != ERROR_CODE::NONE)
		{
			resPkt.SetError(errorCode);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(NCommon::PktRoomChatRes), (char*)&resPkt);
			return errorCode;
		}

		if (pUser->IsCurDomainInRoom() == false)
		{
			resPkt.SetError(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(NCommon::PktRoomChatRes), (char*)&resPkt);
			return ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN;
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr)
		{
			resPkt.SetError(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(NCommon::PktRoomChatRes), (char*)&resPkt);
			return ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX;
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr)
		{
			resPkt.SetError(ERROR_CODE::ROOM_CHAT_INVALID_ROOM_INDEX);
			m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(NCommon::PktRoomChatRes), (char*)&resPkt);
			return ERROR_CODE::ROOM_CHAT_INVALID_ROOM_INDEX;
		}

		pRoom->NotifyChat(pUser->GetSessionIndex(), pUser->GetID().c_str(), reqPkt->Msg);
	
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(NCommon::PktRoomEnterRes), (char*)&resPkt);
		return ERROR_CODE::NONE;
	}
}