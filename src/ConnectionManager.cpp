#include "ConnectionManager.h"
#include "json.hpp"


std::thread ConnectionManager::m_recvThread;
std::mutex ConnectionManager::m_mutex;

std::queue<std::string> ConnectionManager::m_sendQueue;
std::queue<std::string> ConnectionManager::m_recvQueue;

std::string ConnectionManager::m_recvBuffer;

void ConnectionManager::Connect(const std::string& ip, int port) {
    // TODO: create a TCP socket
	sock = socket(AF_INET,	SOCK_STREAM,IPPROTO_TCP);

	// TODO: set the socket address to ip, port
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
	addr.sin_family = AF_INET;	/*Address family*/
	addr.sin_port = htons(port);	/*The Host->Network port in the type: short*/
	//Converts IPv4 to its binary form
	int result = inet_pton(
		addr.sin_family,ip.c_str(),&addr.sin_addr
	);
	//Checking is its an IP or DNS name
	if (result != 1) {// 0 For non-IP strings, -1 for other errors
		ADDRINFOA hint;
		PADDRINFOA pAddr;
		memset(&hint, 0, sizeof(ADDRINFOA));
		hint.ai_family = AF_INET;
		result = getaddrinfo(ip.c_str(), nullptr, &hint, &pAddr);
		if (result != 0) {
			return;
		}
		else {
			addr.sin_addr = ((SOCKADDR_IN*)pAddr->ai_addr)->sin_addr;
			freeaddrinfo(pAddr);
		}
	}

	// TODO: make the socket non-blocking
	u_long mode = 1;
	ioctlsocket(sock, FIONBIO, &mode);
	// TODO: connect to the server, don't forget to verify all return values for success!
	int res = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    // create a thread that will run ConnectionManager::RecvLoop and detach it
	m_recvThread = std::thread(&ConnectionManager::RecvLoop);
	if (m_recvThread.joinable()) m_recvThread.detach();
}

void ConnectionManager::Disconnect() {
	if (!m_running) return;

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// TODO: flush anything still queued, then send a graceful DISCONNECT so the
		// server doesn't log us as a protocol violator for closing the socket
		// without notice. Sent synchronously (under the lock, so it can't race
		// the RecvLoop's own draining) to guarantee it goes out before we stop.

		// TODO: create a DISCONNECT message and send it

	}

	// stop the receive loop
	m_running = false;
}

void ConnectionManager::Send(const std::string& message) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_sendQueue.push(message);
}

bool ConnectionManager::ValidatePort(int port)
{
	std::string str_port = std::to_string(port);
	for (int i = 0; i < static_cast<int>(str_port.size()); ++i) {
		if (isalpha(str_port[i])) {
			return false;
		}
	}
	if (port < 1 || port > 65535) {
		return false;

	}
	return true;
}

bool ConnectionManager::ValidateIP(const std::string ip)
{
	return true;
}

std::optional<std::string> ConnectionManager::Poll() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_recvQueue.empty()) return std::nullopt;;
    // TODO: return the next message in the queue
	return std::nullopt;
}

void ConnectionManager::RecvLoop() {
	while (m_running) {
		// Send queued messages
		{
			std::lock_guard<std::mutex> lock(m_mutex);
            // TODO: send all messages in the sendQueue 
		}

		// TODO: Receive from the server, push to the recvQueue
		// Remember to handle Server Disconnects, WSAEWOULDBLOCK, and other failure cases
		
        // sleep briefly to avoid problems with loops 
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}