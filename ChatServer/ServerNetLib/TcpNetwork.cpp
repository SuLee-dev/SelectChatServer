#include <cstring>
#include <vector>
#include <deque>

#include "ILog.h"
#include "TcpNetwork.h"


namespace NServerNetLib
{
	TcpNetwork::TcpNetwork()
	{
	}

	TcpNetwork::~TcpNetwork() // �� Ŭ���̾�Ʈ ������ Recv/Send ���� ����
	{
		for (auto& client : m_ClientSessionPool)
		{
			if (client.pRecvBuffer)
				delete[] client.pRecvBuffer;
			if (client.pSendBuffer)
				delete[] client.pSendBuffer;
		}
	}

	// ���� ����, ����� ���� and ���� ���� �ʱ�ȭ
	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* pConfig, ILog* pLogger)
	{
		memcpy(&m_Config, pConfig, sizeof(ServerConfig));
		m_pRefLogger = pLogger;

		auto initRet = InitServerSocket();
		if (initRet != NET_ERROR_CODE::NONE)
			return initRet;

		auto bindListenRet = BindListen(m_Config.Port, m_Config.BackLogCount);
		if (bindListenRet != NET_ERROR_CODE::NONE)
			return bindListenRet;

		FD_ZERO(&m_Readfds);
		FD_SET(m_ServerSockFD, &m_Readfds);

		auto sessionPoolSize = CreateSessionPool(m_Config.MaxClientCount);

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Session Pool Size: %d", __FUNCTION__, sessionPoolSize);

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::Release()
	{
		WSACleanup();
	}

	// ��Ŷ ť���� ��Ŷ ��������
	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;

		if (!m_PacketQueue.empty())
		{
			packetInfo = m_PacketQueue.front();
			m_PacketQueue.pop_front();
		}

		return packetInfo;
	}

	// Ŭ���̾�Ʈ ���� ���� ����
	void TcpNetwork::ForcingClose(const int sessionIndex)
	{
		if (!m_ClientSessionPool[sessionIndex].IsConnected()) return;
		
		CloseSession(SOCKET_CLOSE_CASE::FORCING_CLOSE, m_ClientSessionPool[sessionIndex].SocketFD, sessionIndex);
	}

	// ���� ���� �Լ�
	void TcpNetwork::Run()
	{
		auto read_set = m_Readfds;
		auto write_set = m_Readfds;

		TIMEVAL timeout{ 0, 1000 };

		int selectResult = select(0, &read_set, &write_set, 0, &timeout);
		bool isFDSetChanged = RunCheckSelectResult(selectResult);

		if (!isFDSetChanged) return;

		// ���� ���� ��ȭ �� Ŭ���̾�Ʈ ���� ���� ����
		if (FD_ISSET(m_ServerSockFD, &read_set))
			NewSession();

		RunCheckSelectClients(read_set, write_set);
	}

	// FD_SET ��ȭ üũ
	bool TcpNetwork::RunCheckSelectResult(const int result)
	{
		return result == 0 ? false : true;
	}

	// Ŭ���̾�Ʈ FD ��ȭ üũ
	void TcpNetwork::RunCheckSelectClients(fd_set& read_set, fd_set& write_set)
	{
		for (int i = 0; i < m_ClientSessionPool.size(); ++i)
		{
			auto& session = m_ClientSessionPool[i];

			if (!session.IsConnected()) continue;

			SOCKET fd = session.SocketFD;
			auto sessionIndex = session.Index;

			// �б� ��Ʈ üũ
			bool retReceive = RunProcessReceive(sessionIndex, fd, read_set);
			if (!retReceive) continue;

			// ���� ��Ʈ üũ
			RunProcessWrite(sessionIndex, fd, write_set);
		}
	}

	// �б� ��Ʈ ��ȭ �� ���� ����
	bool TcpNetwork::RunProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set)
	{
		if (!FD_ISSET(fd, &read_set)) return true;

		// Recv ���ۿ� ���� ������ �ֱ�
		auto ret = RecvSocket(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, fd, sessionIndex);
			return false;
		}

		// Recv ���� ���� ������ ó��
		ret = RecvBufferProcess(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, fd, sessionIndex);
			return false;
		}

		return true;
	}

	// Send ���� �ȿ� ������ �ֱ�
	NET_ERROR_CODE TcpNetwork::SendData(const int sessionIndex, const short packetId, const short bodySize, const char* pMsg)
	{
		auto& session = m_ClientSessionPool[sessionIndex];

		auto pos = session.SendSize;
		auto totalSize = (int16_t)(bodySize + PACKET_HEADER_SIZE);
		if ((pos + totalSize) > m_Config.MaxClientSendBufferSize)
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;

		PacketHeader pktHeader{ totalSize, packetId, (uint8_t)0 };
		memcpy(&session.pSendBuffer[pos], (char*)&pktHeader, PACKET_HEADER_SIZE);

		if (bodySize > 0)
			memcpy(&session.pSendBuffer[pos + PACKET_HEADER_SIZE], pMsg, bodySize);

		session.SendSize += totalSize;

		return NET_ERROR_CODE::NONE;
	}

	// Ŭ���̾�Ʈ ���� ������ ������ŭ �̸� ����
	int TcpNetwork::CreateSessionPool(const int maxClientCount)
	{
		for (int i = 0; i < maxClientCount; ++i)
		{
			ClientSession session;
			session.Clear();
			session.Index = i;
			session.pRecvBuffer = new char[m_Config.MaxClientRecvBufferSize];
			session.pSendBuffer = new char[m_Config.MaxClientSendBufferSize];

			m_ClientSessionPool.push_back(session);
			m_ClientSessionPoolIndex.push_back(i);
		}

		return maxClientCount;
	}
	 
	// Ŭ���̾�Ʈ ���ǿ� index �Ҵ�
	int TcpNetwork::AllocClientSessionIndex()
	{
		if (m_ClientSessionPoolIndex.empty())
			return -1;

		int index = m_ClientSessionPoolIndex.front();
		m_ClientSessionPoolIndex.pop_front();
		return index;
	}

	// �ش� index�� ���� ����
	void TcpNetwork::ReleaseSessionIndex(const int index)
	{
		m_ClientSessionPool[index].Clear();
		m_ClientSessionPoolIndex.push_back(index);
	}

	// ���� ���� ����
	NET_ERROR_CODE TcpNetwork::InitServerSocket()
	{
		WSADATA wsaData;
		WORD wVersionRequeted = MAKEWORD(2, 2);
		WSAStartup(wVersionRequeted, &wsaData);

		m_ServerSockFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_ServerSockFD == INVALID_SOCKET)
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;

		int option = 1;
		if (setsockopt(m_ServerSockFD, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option)) == SOCKET_ERROR)
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;

		return NET_ERROR_CODE::NONE;
	}

	// ���� ���� ���ε� �� ����
	NET_ERROR_CODE TcpNetwork::BindListen(short port, int backlogCount)
	{
		SOCKADDR_IN server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(port);

		if (bind(m_ServerSockFD, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;

		NET_ERROR_CODE netError = SetNonBlockSocket(m_ServerSockFD);
		if (netError != NET_ERROR_CODE::NONE)
			return netError;

		if (listen(m_ServerSockFD, backlogCount) == SOCKET_ERROR)
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockFD", __FUNCTION__, m_ServerSockFD);
		return NET_ERROR_CODE::NONE;
	}

	// Ŭ���̾�Ʈ connect ��û �� accept �� ���� �ο�
	NET_ERROR_CODE TcpNetwork::NewSession()
	{
		int tryCount = 0;

		do
		{
			++tryCount;

			SOCKADDR_IN client_addr;
			int client_len = sizeof(client_addr);
			SOCKET client_sockFD = accept(m_ServerSockFD, (SOCKADDR*)&client_addr, &client_len);

			if (client_sockFD == INVALID_SOCKET)
			{
				if (WSAGetLastError() == WSAEWOULDBLOCK)
					return NET_ERROR_CODE::ACCEPT_API_WSAEWOULDBLOCK;

				m_pRefLogger->Write(LOG_TYPE::L_ERROR, "%s | Wrong socket cannot accept", __FUNCTION__);
				return NET_ERROR_CODE::ACCEPT_API_ERROR;
			}

			int newSessionIndex = AllocClientSessionIndex();
			if (newSessionIndex < 0)
			{
				m_pRefLogger->Write(LOG_TYPE::L_WARN, "%s | client_sockFD(%I64u) >= MAX_SESSION", __FUNCTION__, client_sockFD);

				CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, client_sockFD, -1);
				return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
			}

			char clientIP[MAX_IP_LEN] = { 0, };
			inet_ntop(PF_INET, &client_addr.sin_addr.s_addr, clientIP, MAX_IP_LEN - 1);
			SetSockOption(client_sockFD);
			SetNonBlockSocket(client_sockFD);

			FD_SET(client_sockFD, &m_Readfds);
			ConnectedSession(newSessionIndex, client_sockFD, clientIP);
				
		} while (tryCount < FD_SETSIZE);

		return NET_ERROR_CODE::NONE;
	}

	// Ŭ���̾�Ʈ ���� ���� ����
	void TcpNetwork::ConnectedSession(const int sessionIndex, const SOCKET fd, const char* pIP)
	{
		++m_ConnectSeq;

		auto& session = m_ClientSessionPool[sessionIndex];
		session.Seq = m_ConnectSeq;
		session.SocketFD = fd;
		memcpy(session.IP, pIP, MAX_IP_LEN - 1);

		++m_ConnectedSessionCount;

		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CONNECT_SESSION, 0, nullptr);

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | New Session. FD(%I64u)", m_ConnectSeq);
	}

	// ���� �ɼ� ����
	void TcpNetwork::SetSockOption(const SOCKET fd)
	{
		linger ling;
		ling.l_linger = 0;
		ling.l_onoff = 0;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		int size1 = m_Config.MaxClientSockOptRecvBufferSize;
		int size2 = m_Config.MaxClientSockOptSendBufferSize;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)size1, sizeof(size1));
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)size2, sizeof(size2));
	}

	// Ŭ���̾�Ʈ ���� ����
	void TcpNetwork::CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex)
	{
		if (m_ClientSessionPool[sessionIndex].IsConnected() == false)
			return;

		closesocket(sockFD);
		FD_CLR(sockFD, &m_Readfds);

		if (closeCase == SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
			return;

		m_ClientSessionPool[sessionIndex].Clear();
		ReleaseSessionIndex(sessionIndex);
		--m_ConnectedSessionCount;

		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	// Recv ���ۿ� ���� ������ �ֱ�
	NET_ERROR_CODE TcpNetwork::RecvSocket(const int sessionIndex)
	{
		auto& session = m_ClientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		if (session.IsConnected() == false)
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;

		int recvPos = 0;

		if (session.RemainingDataSize > 0)
		{
			memcpy(session.pRecvBuffer, &session.pRecvBuffer[session.PrevReadPosInRecvBuffer], session.RemainingDataSize);
			recvPos += session.RemainingDataSize;
		}

		auto recvSize = recv(fd, &session.pRecvBuffer[recvPos], (MAX_PACKET_BODY_SIZE * 2), 0);

		if (recvSize == 0)
			return NET_ERROR_CODE::RECV_REMOTE_CLOSE;

		if (recvSize == SOCKET_ERROR)
		{
			auto netError = WSAGetLastError();

			if (netError != WSAEWOULDBLOCK)
				return NET_ERROR_CODE::RECV_API_ERROR;
			else
				return NET_ERROR_CODE::NONE;
		}

		session.RemainingDataSize += recvSize;
		return NET_ERROR_CODE::NONE;
	}

	// Recv ���� ���� ������ ó��
	NET_ERROR_CODE TcpNetwork::RecvBufferProcess(const int sessionIndex)
	{
		auto& session = m_ClientSessionPool[sessionIndex];

		int readPos = 0;
		const int dataSize = session.RemainingDataSize;
		PacketHeader* pPktHeader;

		while ((dataSize - readPos) >= PACKET_HEADER_SIZE)
		{
			pPktHeader = (PacketHeader*)&session.pRecvBuffer[readPos];
			readPos += PACKET_HEADER_SIZE;
			auto bodySize = (int16_t)(pPktHeader->TotalSize - PACKET_HEADER_SIZE);

			if (bodySize > 0)
			{
				// ��Ŷ�� �ٵ� ���� ������ ���۵��� �ʾ��� ��
				if (bodySize > (dataSize - readPos))
				{
					readPos -= PACKET_HEADER_SIZE;
					break;
				}

				if (bodySize > MAX_PACKET_BODY_SIZE)
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
			}

			AddPacketQueue(sessionIndex, pPktHeader->Id, bodySize, &session.pRecvBuffer[readPos]);
			readPos += bodySize;
		}

		session.RemainingDataSize -= readPos;
		session.PrevReadPosInRecvBuffer = readPos;

		return NET_ERROR_CODE::NONE;
	}

	// ��Ŷ ť�� ��Ŷ �߰�
	void TcpNetwork::AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos)
	{
		RecvPacketInfo packetInfo;
		packetInfo.SessionIndex = sessionIndex;
		packetInfo.PacketId = pktId;
		packetInfo.PacketBodySize = bodySize;
		packetInfo.pRefData = pDataPos;

		m_PacketQueue.push_back(packetInfo);
	}

	void TcpNetwork::RunProcessWrite(const int sessionIndex, const SOCKET fd, fd_set& write_set)
	{
		if (!FD_ISSET(fd, &write_set))
			return;

		auto retSend = FlushSendBuff(sessionIndex);
		if (retSend.Error != NET_ERROR_CODE::NONE)
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, fd, sessionIndex);
	}

	// Send ���� �� ������ ó��
	NetError TcpNetwork::FlushSendBuff(const int sessionIndex)
	{
		auto& session = m_ClientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);
		if (session.IsConnected() == false)
			return NetError(NET_ERROR_CODE::CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE);

		auto result = SendSocket(fd, session.pSendBuffer, session.SendSize);

		if (result.Error != NET_ERROR_CODE::NONE)
			return result;

		auto sendSize = result.Value;
		if (sendSize < session.SendSize) // �����Ͱ� �κ������θ� Send �Ǿ��� ��
		{
			memmove(&session.pSendBuffer[0], &session.pSendBuffer[sendSize], session.SendSize - sendSize);
			session.SendSize -= sendSize;
		}
		else // �����Ͱ� ���� Send �Ǿ��� ��
			session.SendSize = 0;
			
		return result;
	}

	// Send
	NetError TcpNetwork::SendSocket(const SOCKET fd, const char* pMsg, const int size)
	{
		NetError result(NET_ERROR_CODE::NONE);

		if (size <= 0)
			return result;
		
		result.Value = send(fd, pMsg, size, 0);
		if (result.Value <= 0)
			result.Error = NET_ERROR_CODE::SEND_SIZE_ZERO;

		return result;
	}

	NET_ERROR_CODE TcpNetwork::SetNonBlockSocket(const SOCKET sock)
	{
		unsigned long mode = 1;

		if (ioctlsocket(sock, FIONBIO, &mode) == SOCKET_ERROR)
			return NET_ERROR_CODE::SERVER_SOCKET_FIONBIO_FAIL;

		return NET_ERROR_CODE::NONE;
	}
}