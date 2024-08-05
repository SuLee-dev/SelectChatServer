#pragma comment(lib, "ws2_32.lib")

#include "WS2tcpip.h"
#include "ClientTcp.h"

namespace NClient
{
	ClientTcp::ClientTcp()
	{
	}

	ClientTcp::~ClientTcp()
	{
		Close();
		WSACleanup();
	}

	void ClientTcp::Init()
	{
		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD(2, 2);
		if (WSAStartup(wVersionRequested, &wsaData) != 0)
			m_LatestErrorMsg = "WSAStartup Failed";
	}

	bool ClientTcp::Connect(const char* pIP, int port)
	{
		try
		{
			SOCKADDR_IN server_addr;
			memset(&server_addr, 0, sizeof(server_addr));
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = htons(port);

			if (inet_pton(PF_INET, pIP, &server_addr.sin_addr.s_addr) <= 0)
			{
				m_LatestErrorMsg = "Invalid IP address \n";
				return false;
			}

			m_ClientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (m_ClientSock == INVALID_SOCKET)
			{
				m_LatestErrorMsg = "Socket creation failed \n";
				return false;
			}
			
			if (connect(m_ClientSock, (SOCKADDR*)&server_addr, sizeof(server_addr) == SOCKET_ERROR))
			{
				m_LatestErrorMsg = "Connection failed \n";
				closesocket(m_ClientSock);
				m_ClientSock = INVALID_SOCKET;
				return false;
			}

			return true;
		}
		catch (const std::exception& ex)
		{
			m_LatestErrorMsg = ex.what();
			return false;
		}
	}

	std::tuple<int, char*> ClientTcp::Receive()
	{
		char ReadBuffer[2048];
		int nRecv = recv(m_ClientSock, ReadBuffer, sizeof(ReadBuffer), 0);

		if (nRecv <= 0)
		{
			m_LatestErrorMsg = "Receive failed or connection closed \n";
			return { -1, nullptr };
		}

		return { nRecv, ReadBuffer };
	}

	void ClientTcp::Send(const char* sendData)
	{
		try
		{
			if (m_ClientSock != INVALID_SOCKET && IsConnected())
			{
				int sendResult = send(m_ClientSock, sendData, sizeof(sendData), 0);
				if (sendResult == SOCKET_ERROR)
				{
					m_LatestErrorMsg = "Send failed \n";
				}
				else
				{
					m_LatestErrorMsg = "Connect to the server first \n";
				}
			}
		}
		catch (std::exception& ex)
		{
			m_LatestErrorMsg = ex.what();
		}
	}

	void ClientTcp::Close()
	{
		if (m_ClientSock != INVALID_SOCKET && IsConnected())
		{
			closesocket(m_ClientSock);
			m_ClientSock = INVALID_SOCKET;
		}
	}

	bool ClientTcp::IsConnected()
	{
		return (m_ClientSock != INVALID_SOCKET && CheckSocketConnection());
	}

	bool ClientTcp::CheckSocketConnection()
	{
		char buffer;
		int result = recv(m_ClientSock, &buffer, 1, MSG_PEEK);

		if (result == 0)
			return false;

		if (result == SOCKET_ERROR)
			return (WSAGetLastError() == WSAEWOULDBLOCK);

		return true;
	}
}