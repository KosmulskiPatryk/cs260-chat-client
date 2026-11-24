#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <optional>


//struct SOCKET;
//struct sockaddr_in;
struct ConnectionManager {
	static std::thread m_recvThread;
	static std::mutex m_mutex;

	static std::queue<std::string> m_sendQueue;
	static std::queue<std::string> m_recvQueue;
	inline static std::atomic<bool> m_running = false;

	static std::string m_recvBuffer;
	//static std::string secret;
	//static SOCKET sock;
	//static sockaddr_in addr;


	static void Connect(const std::string& ip, int port);
	static void Connect();
	static void Disconnect();
	static void Send(const std::string& message);

	static bool Init();

	static bool ValidatePort(int port);
	static bool ValidateIP(const std::string ip);
	static void SetSecret(const std::string secret);

	//<------- Getters and Setters

	static std::string GetSecret();
	static std::optional<std::string> Poll();
	static void SetUserName(const std::string user);
	static std::string GetUser();

	static void RecvLoop();



};