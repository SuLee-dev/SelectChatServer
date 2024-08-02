#pragma once

#include <vector>
#include <string>
#include <memory>

#include "User.h"

namespace NServerNetLib
{
	class ITcpNetwork;
	class ILog;
}

namespace NCommon
{
	enum class ERROR_CODE : short;
}

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{
	using TcpNet = NServerNetLib::ITcpNetwork;
	using ILog = NServerNetLib::ILog;

	class Room
	{
	public:
		Room();
		virtual ~Room();

		void Init(const short index, const short maxUserCount);

		void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

		void Clear();

		short GetIndex() { return m_Index; }

		bool IsUsed() { return m_IsUsed; }

		const wchar_t* GetTitle() { return m_Title.c_str(); }

		short MaxUserCount() { return m_MaxUserCount; }

		short GetUserCount() { return (short)m_UserList.size(); }

		ERROR_CODE CreateRoom(const wchar_t* pRoomTitle);

		ERROR_CODE EnterUser(User* pUser);

		ERROR_CODE LeaveUser(const short userIndex);

		bool IsMaster(const short userIndex);

		void Update();

		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserIndex = -1);

		void NotifyEnterUserInfo(const short userIndex, const char* pszUserID);

		void NotifyLeaveUserInfo(const char* pszUserID);

		void NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg);

	private:
		TcpNet* m_pRefNetwork;
		ILog* m_pRefLogger;

		short m_Index = -1;
		short m_MaxUserCount;

		bool m_IsUsed = false;
		std::wstring m_Title;
		std::vector<User*> m_UserList;
	};

}