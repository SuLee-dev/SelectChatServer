#pragma comment(lib, "ws2_32.lib")

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
		return false;
	}

	std::tuple<int, char*> ClientTcp::Receive()
	{
		return std::tuple<int, char*>();
	}

	void ClientTcp::Send(const char* sendData)
	{
	}

	void ClientTcp::Close()
	{
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