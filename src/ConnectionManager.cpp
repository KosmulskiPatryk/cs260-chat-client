#include "ConnectionManager.h"
#include "json.hpp"
#include "WinSock2.h"
#include "WS2tcpip.h"
#include <sstream>
#include <iostream>

std::thread ConnectionManager::m_recvThread;
std::mutex ConnectionManager::m_mutex;

std::queue<std::string> ConnectionManager::m_sendQueue;
std::queue<std::string> ConnectionManager::m_recvQueue;

std::string ConnectionManager::m_recvBuffer;

SOCKET sock;
sockaddr_in addr;
std::string m_secret;
std::string m_user;
std::string m_ip;
int m_port;


#define BUF_LEN 1400

int GetError(std::string functionName, int clientNum = -1) {
	int err = WSAGetLastError();
	if (err == WSAEWOULDBLOCK)return 1;
	if (err == WSAEISCONN)return 2;
	if (clientNum > -1)
		std::cout << "Client #" << clientNum << " Proxy server error: " << functionName << " code: " << err << std::endl;
	else std::cout << functionName << ": " << err << std::endl;
	return 0;
}

bool ConnectionManager::Init() {
	WSADATA wsaData;
	int result = WSAStartup(
		MAKEWORD(2, 2), /*The version of WinSock, always 2.2*/
		&wsaData		/*Structure to receive details of WinSock*/);
	if (result != 0) {
		std::cout << "WSAStartup: " << result << std::endl;
		return false;
	}
	return true;

}

void ConnectionManager::Exit()
{
	int result = WSACleanup();
	if (result != 0) {
		GetError("WSACleanup");
		return;
	}
	return;
}

void ConnectionManager::Connect(const std::string& ip, int port) {

	m_ip = ip;m_port = port;

    // TODO: create a TCP socket
	sock = socket(AF_INET,	SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		GetError("socket");
		Disconnect();
		return;
	}

	// TODO: set the socket address to ip, port
		//Create the sockaddr
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
	addr.sin_family = AF_INET;	/*Address family*/
	addr.sin_port = htons(port);	/*The Host->Network port in the type: short*/

	//Converts IPv4 to its binary form
	int result = inet_pton(addr.sin_family,ip.c_str(),&addr.sin_addr);
	//Checking is its an IP or DNS name
	if (result != 1) {// 0 For non-IP strings, -1 for other errors
		ADDRINFOA hint;
		PADDRINFOA pAddr;
		memset(&hint, 0, sizeof(ADDRINFOA));
		hint.ai_family = AF_INET;
		result = getaddrinfo(ip.c_str(), nullptr, &hint, &pAddr);
		if (result != 0) {
			Disconnect();
			return;
		}
		else {
			addr.sin_addr = ((SOCKADDR_IN*)pAddr->ai_addr)->sin_addr;
			freeaddrinfo(pAddr);
		}
	}
	
	//TODO: make the socket non-blocking
	u_long mode = 1;
	int err = ioctlsocket(sock,FIONBIO,&mode);
	if (err != 0) {
		GetError("ioctlsocket");
		Disconnect();
		return;
	}

	// TODO: connect to the server, don't forget to verify all return values for success!
	//while (true) {
	int res = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if (res == SOCKET_ERROR) {
		int err = GetError("connect");
		if (err == 1) {
			//continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		//else if (err == 2) {
		//	return;
		//}
		else Disconnect();
	}
	//else {
	//	//break;
	//}

	//}
	m_running = true;
	// create a thread that will run ConnectionManager::RecvLoop and detach it
	m_recvThread = std::thread(&ConnectionManager::RecvLoop);
	if (m_recvThread.joinable()) m_recvThread.detach();
}

void ConnectionManager::Connect()
{
	Connect(m_ip,m_port);
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
		nlohmann::json message;
		message["type"] = "DISCONNECT";

		m_sendQueue.push(nlohmann::to_string(message));
		
		while (!m_sendQueue.empty()) {
			std::string msg = m_sendQueue.front();
			m_sendQueue.pop();
			while (true) {
				int byteSent = send(
					sock,								/*The Socket to send from*/
					msg.c_str(),					/*The message to send*/
					static_cast<int>(msg.size()),	/*The length of out message*/
					0									/*Any flags, currently 0*/
				);
				if (byteSent == SOCKET_ERROR) {
					GetError("send");
					return;
				}
				else if (byteSent <= static_cast<int>(msg.size()))
					break;
			}

		}
		while (!m_recvQueue.empty()) {
			m_recvQueue.pop();
		}
		
		
		int result = closesocket(sock);
		if (result != 0) {
			GetError("closesocket");
			return;
		}
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
	m_port = port;
	return true;
}

bool ConnectionManager::ValidateIP(const std::string ip)
{
	ADDRINFOA hint;
	PADDRINFOA pAddr;
	memset(&hint, 0, sizeof(ADDRINFOA));
	hint.ai_family = AF_INET;
	int result = getaddrinfo(ip.c_str(), nullptr, &hint, &pAddr);
	if (result != 0) {
		GetError("getaddrinfo");
		Disconnect();
		return false;
	}
	
	m_ip = ip;
	return true;
}

void ConnectionManager::SetSecret(const std::string secret)
{
	m_secret = secret;
}

std::string ConnectionManager::GetSecret()
{
	return m_secret;
}

std::optional<std::string> ConnectionManager::Poll() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_recvQueue.empty()) return std::nullopt;
    // TODO: return the next message in the queue
	std::string msg = m_recvQueue.front();
	m_recvQueue.pop();
	return msg;
}

void ConnectionManager::SetUserName(const std::string user)
{
	m_user = user;
}

std::string ConnectionManager::GetUser()
{
	return m_user;
}

void ConnectionManager::RecvLoop() {
 	while (m_running) {
		// Send queued messages
		{
			std::lock_guard<std::mutex> lock(m_mutex);
            // TODO: send all messages in the sendQueue 
			while (!m_sendQueue.empty()) {
				std::string msg = m_sendQueue.front();
				m_sendQueue.pop();
				while (true) {
					int byteSent = send(
						sock,								/*The Socket to send from*/
						msg.c_str(),					/*The message to send*/
						static_cast<int>(msg.size()),	/*The length of out message*/
						0									/*Any flags, currently 0*/
					);
					if (byteSent == SOCKET_ERROR) {
						GetError("send");
						Disconnect();
						//return;
					}
					else if (byteSent <= static_cast<int>(msg.size()))
						break;
				}

			}
		}

		// TODO: Receive from the server, push to the recvQueue
		// Remember to handle Server Disconnects, WSAEWOULDBLOCK, and other failure cases
		char buf[BUF_LEN]; /*We don't need the +1 anymore since we dont need the null terminator*/
		int bytes = recv(
			sock,		/*The sock we are receiving on*/
			buf,		/*The response buffer*/
			BUF_LEN,	/*The length of the buffer*/
			0			/*Flags, none*/
		);

		if (bytes == SOCKET_ERROR) {
			int err = GetError("receive");
			if (err != 1) {
				Disconnect();
				//return;
			}
		}
		else if (bytes == 0)
			break;
		else {
			m_recvBuffer += std::string(buf, bytes);
			std::stringstream ss(m_recvBuffer);
			std::string msg;
			
			while (std::getline(ss, msg) && !ss.eof()) {
				m_recvQueue.push(msg);
				size_t it = m_recvBuffer.find(msg);
				if(it != m_recvBuffer.npos)
					m_recvBuffer.erase(it,msg.size()+1);
				
			}

		}


        // sleep briefly to avoid problems with loops 
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}