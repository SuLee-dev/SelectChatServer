#pragma once


namespace NServerNetLib
{
	struct ServerConfig
	{
		unsigned short Port;
		int BackLogCount;

		int MaxClientCount;
		int ExtraClientCount; // 로그인에서 짜르기 위한 여분

		short MaxClientSockOptRecvBufferSize;
		short MaxClientSockOptSendBufferSize;
		short MaxClientRecvBufferSize;
		short MaxClientSendBufferSize;

		bool IsLoginCheck; // 연결 후 특정 시간 이내 로그인 완료 여부 조사

		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};

	const int MAX_IP_LEN = 32; // IP 문자열 최대 길이
	const int MAX_PACKET_BODY_SIZE = 1024; // 최대 패킷 바디 크기

	struct ClientSession
	{
		bool IsConnected() { return SocketFD != 0 ? true : false; }

		void Clear()
		{
			Seq = 0;
			SocketFD = 0;
			IP[0] = '\0';
			RemainingDataSize = 0;
			PrevReadPosInRecvBuffer = 0;
			SendSize = 0;
		}

		int Index = 0;
		long long Seq = 0;
		unsigned long long SocketFD = 0;
		char IP[MAX_IP_LEN] = { 0, };

		char*	pRecvBuffer = nullptr;
		int		RemainingDataSize = 0; // Recv 버퍼 안의 데이터 크기
		int		PrevReadPosInRecvBuffer = 0; // Recv 버퍼 내에서 지금까지 읽은 데이터 위치

		char*	pSendBuffer = nullptr;
		int		SendSize = 0; // Send 데이터 크기
	};

	struct RecvPacketInfo
	{
		int		SessionIndex = 0;
		short	PacketId = 0;
		short	PacketBodySize = 0;
		char*	pRefData = 0;
	};

	enum class SOCKET_CLOSE_CASE : short
	{
		SESSION_POOL_EMPTY = 1,
		SELECT_ERROR = 2,
		SOCKET_RECV_ERROR = 3,
		SOCKET_RECV_BUFFER_PROCESS_ERROR = 4,
		SOCKET_SEND_ERROR = 5,
		FORCING_CLOSE = 6,
	};

	enum class PACKET_ID : short
	{
		NTF_SYS_CONNECT_SESSION = 2,
		NTF_SYS_CLOSE_SESSION = 3,
	};

#pragma pack(push, 1)
	struct PacketHeader
	{
		short TotalSize;
		short Id;
		unsigned char Reserve;
	};

	const int PACKET_HEADER_SIZE = sizeof(PacketHeader);

	struct PktNtfSysCloseSession : PacketHeader
	{
		int SockFD;
	};
#pragma pack(pop)
}