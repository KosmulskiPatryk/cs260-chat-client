#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <optional>

#include <WinSock2.h>
#include <Ws2tcpip.h>

struct ConnectionManager {
	static std::thread m_recvThread;
	static std::mutex m_mutex;

	static std::queue<std::string> m_sendQueue;
	static std::queue<std::string> m_recvQueue;
	inline static std::atomic<bool> m_running = false;

	static std::string m_recvBuffer;
	static SOCKET sock;
	static sockaddr_in addr;

	static void Connect(const std::string& ip, int port);
	static void Disconnect();
	static void Send(const std::string& message);
	static bool ValidatePort(int port);
	static bool ValidateIP(const std::string ip);

	static std::optional<std::string> Poll();

	static void RecvLoop();
};