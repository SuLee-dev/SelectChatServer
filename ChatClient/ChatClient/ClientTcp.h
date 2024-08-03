#pragma once

#include <string>
#include <tuple>
#include <WinSock2.h>

namespace NClient
{
	class ClientTcp
	{
	public:
		ClientTcp();
		~ClientTcp();

		void Init();

		bool Connect(const char* pIP, int port);

		std::tuple<int, char*> Receive();

		void Send(const char* sendData);

		void Close();

		bool IsConnected();

		bool CheckSocketConnection();

	private:
		SOCKET m_ClientSock;
		std::string m_LatestErrorMsg;
	};
}