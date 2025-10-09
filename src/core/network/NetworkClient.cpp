#include "core/network/NetworkClient.h"
#include <sstream>
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketType;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VALUE SOCKET_ERROR
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    typedef int SocketType;
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VALUE -1
    #define closesocket close
#endif

namespace DachshundEngine {
    namespace Network {

        /// @brief NetworkClient 구현 클래스 (Pimpl 패턴)
        class NetworkClient::Impl {
        public:
            ConnectionState state;
            SocketType socket;
            std::string ip_address;
            int port;
            std::string last_error;

            // 콜백 함수들
            std::function<void(const Sensor::SensorData&)> onSensorDataReceived;
            std::function<void(ConnectionState)> onConnectionStateChanged;

            // 윈도우 소켓 초기화 플래그
            #ifdef _WIN32
            bool wsa_initialized;
            #endif

            Impl() : state(ConnectionState::DISCONNECTED), 
                     socket(INVALID_SOCKET_VALUE),
                     port(0) {
                #ifdef _WIN32
                WSADATA wsaData;
                wsa_initialized = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
                if (!wsa_initialized) {
                    last_error = "WSAStartup failed";
                }
                #endif
            }

            ~Impl() {
                if (socket != INVALID_SOCKET_VALUE) {
                    closesocket(socket);
                }
                #ifdef _WIN32
                if (wsa_initialized) {
                    WSACleanup();
                }
                #endif
            }

            void setState(ConnectionState new_state) {
                if (state != new_state) {
                    state = new_state;
                    if (onConnectionStateChanged) {
                        onConnectionStateChanged(new_state);
                    }
                }
            }

            bool setNonBlocking() {
                #ifdef _WIN32
                u_long mode = 1;
                return ioctlsocket(socket, FIONBIO, &mode) == 0;
                #else
                int flags = fcntl(socket, F_GETFL, 0);
                return fcntl(socket, F_SETFL, flags | O_NONBLOCK) != -1;
                #endif
            }
        };

        /// @brief NetworkClient 메서드 구현
        NetworkClient::NetworkClient() : pImpl(std::make_unique<Impl>()) {}
        NetworkClient::~NetworkClient() = default;
        NetworkClient::NetworkClient(NetworkClient&&) noexcept = default;
        NetworkClient& NetworkClient::operator=(NetworkClient&&) noexcept = default;

        bool NetworkClient::connect(const std::string& ip_address, int port) {
            if (pImpl->state == ConnectionState::CONNECTED) {
                disconnect();
            }

            pImpl->setState(ConnectionState::CONNECTING);
            pImpl->ip_address = ip_address;
            pImpl->port = port;

            // 소켓 생성
            pImpl->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (pImpl->socket == INVALID_SOCKET_VALUE) {
                pImpl->last_error = "Failed to create socket";
                pImpl->setState(ConnectionState::CONNECTION_ERROR);
                return false;
            }

            // 서버 주소 설정
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            
            #ifdef _WIN32
            inet_pton(AF_INET, ip_address.c_str(), &server_addr.sin_addr);
            #else
            inet_pton(AF_INET, ip_address.c_str(), &server_addr.sin_addr);
            #endif

            // 연결 시도
            if (::connect(pImpl->socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR_VALUE) {
                #ifdef _WIN32
                pImpl->last_error = "Connection failed: " + std::to_string(WSAGetLastError());
                #else
                pImpl->last_error = "Connection failed";
                #endif
                closesocket(pImpl->socket);
                pImpl->socket = INVALID_SOCKET_VALUE;
                pImpl->setState(ConnectionState::CONNECTION_ERROR);
                return false;
            }

            // 논블로킹 모드로 설정
            if (!pImpl->setNonBlocking()) {
                pImpl->last_error = "Failed to set non-blocking mode";
            }

            pImpl->setState(ConnectionState::CONNECTED);
            std::cout << "Connected to " << ip_address << ":" << port << std::endl;
            return true;
        }

        void NetworkClient::disconnect() {
            if (pImpl->socket != INVALID_SOCKET_VALUE) {
                closesocket(pImpl->socket);
                pImpl->socket = INVALID_SOCKET_VALUE;
            }
            pImpl->setState(ConnectionState::DISCONNECTED);
        }

        ConnectionState NetworkClient::getConnectionState() const {
            return pImpl->state;
        }

        bool NetworkClient::sendMessage(const NetworkMessage& message) {
            if (pImpl->state != ConnectionState::CONNECTED) {
                pImpl->last_error = "Not connected";
                return false;
            }

            // 메시지 포맷: [길이(4바이트)][JSON 페이로드]
            uint32_t payload_length = static_cast<uint32_t>(message.payload.size());
            uint32_t network_length = htonl(payload_length);

            // 길이 전송
            if (send(pImpl->socket, (const char*)&network_length, 4, 0) != 4) {
                pImpl->last_error = "Failed to send message length";
                return false;
            }

            // 페이로드 전송
            int total_sent = 0;
            while (total_sent < payload_length) {
                int sent = send(pImpl->socket, 
                               message.payload.c_str() + total_sent, 
                               payload_length - total_sent, 
                               0);
                if (sent == SOCKET_ERROR_VALUE) {
                    pImpl->last_error = "Failed to send payload";
                    return false;
                }
                total_sent += sent;
            }

            return true;
        }

        bool NetworkClient::requestSensorData() {
            std::string cmd_json = JsonUtil::createCommandMessage("get_sensor_data");
            NetworkMessage msg{MessageType::COMMAND, cmd_json, 0};
            return sendMessage(msg);
        }

        bool NetworkClient::setSamplingRate(int rate_ms) {
            std::ostringstream params;
            params << "{\"rate_ms\":" << rate_ms << "}";
            std::string cmd_json = JsonUtil::createCommandMessage("set_sampling_rate", params.str());
            NetworkMessage msg{MessageType::COMMAND, cmd_json, 0};
            return sendMessage(msg);
        }

        int NetworkClient::processIncomingMessages() {
            if (pImpl->state != ConnectionState::CONNECTED) {
                return 0;
            }

            int messages_processed = 0;

            // 논블로킹 수신
            while (true) {
                // 메시지 길이 읽기 (4바이트)
                uint32_t network_length;
                int received = recv(pImpl->socket, (char*)&network_length, 4, 0);
                
                if (received == 0) {
                    // 연결 종료
                    pImpl->last_error = "Connection closed by server";
                    disconnect();
                    break;
                } else if (received == SOCKET_ERROR_VALUE) {
                    #ifdef _WIN32
                    int error = WSAGetLastError();
                    if (error == WSAEWOULDBLOCK) {
                        // 수신할 데이터 없음 (정상)
                        break;
                    }
                    #else
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        break;
                    }
                    #endif
                    pImpl->last_error = "Receive error";
                    break;
                } else if (received != 4) {
                    pImpl->last_error = "Invalid message length header";
                    break;
                }

                uint32_t payload_length = ntohl(network_length);
                
                // 페이로드 읽기
                std::string payload;
                payload.resize(payload_length);
                
                int total_received = 0;
                while (total_received < payload_length) {
                    received = recv(pImpl->socket, 
                                   &payload[total_received], 
                                   payload_length - total_received, 
                                   0);
                    if (received <= 0) {
                        pImpl->last_error = "Failed to receive payload";
                        return messages_processed;
                    }
                    total_received += received;
                }

                // JSON 파싱 및 센서 데이터 처리
                Sensor::SensorData sensor_data;
                if (JsonUtil::parseSensorData(payload, sensor_data)) {
                    if (pImpl->onSensorDataReceived) {
                        pImpl->onSensorDataReceived(sensor_data);
                    }
                    messages_processed++;
                }
            }

            return messages_processed;
        }

        void NetworkClient::setOnSensorDataReceived(std::function<void(const Sensor::SensorData&)> callback) {
            pImpl->onSensorDataReceived = callback;
        }

        void NetworkClient::setOnConnectionStateChanged(std::function<void(ConnectionState)> callback) {
            pImpl->onConnectionStateChanged = callback;
        }

        std::string NetworkClient::getLastError() const {
            return pImpl->last_error;
        }

        /// @brief JSON 유틸리티 함수 구현
        namespace JsonUtil {
            
            std::string sensorDataToJson(const Sensor::SensorData& data) {
                std::ostringstream oss;
                oss << "{"
                    << "\"type\":\"sensor_data\","
                    << "\"timestamp\":" << 0 << ","
                    << "\"data\":{"
                    << "\"temperature\":" << data.temperature << ","
                    << "\"humidity\":" << data.humidity << ","
                    << "\"pressure\":" << data.pressure << ","
                    << "\"light\":" << data.light << ","
                    << "\"motion_detected\":" << (data.motion_detected ? "true" : "false") << ","
                    << "\"cpu_usage\":" << data.cpu_usage << ","
                    << "\"memory_usage\":" << data.memory_usage
                    << "}"
                    << "}";
                return oss.str();
            }

            bool parseSensorData(const std::string& json, Sensor::SensorData& data) {
                // 간단한 JSON 파싱 (실제로는 라이브러리 사용 권장)
                // 여기서는 기본적인 파싱만 구현
                
                try {
                    // "temperature": 값 추출
                    size_t pos = json.find("\"temperature\":");
                    if (pos != std::string::npos) {
                        data.temperature = std::stof(json.substr(pos + 15));
                    }

                    pos = json.find("\"humidity\":");
                    if (pos != std::string::npos) {
                        data.humidity = std::stof(json.substr(pos + 11));
                    }

                    pos = json.find("\"pressure\":");
                    if (pos != std::string::npos) {
                        data.pressure = std::stof(json.substr(pos + 11));
                    }

                    pos = json.find("\"light\":");
                    if (pos != std::string::npos) {
                        data.light = std::stof(json.substr(pos + 8));
                    }

                    pos = json.find("\"motion_detected\":");
                    if (pos != std::string::npos) {
                        std::string value = json.substr(pos + 18, 4);
                        data.motion_detected = (value.find("true") != std::string::npos);
                    }

                    pos = json.find("\"cpu_usage\":");
                    if (pos != std::string::npos) {
                        data.cpu_usage = std::stof(json.substr(pos + 12));
                    }

                    pos = json.find("\"memory_usage\":");
                    if (pos != std::string::npos) {
                        data.memory_usage = std::stof(json.substr(pos + 15));
                    }

                    data.data_valid = true;
                    return true;

                } catch (...) {
                    data.data_valid = false;
                    return false;
                }
            }

            std::string createCommandMessage(const std::string& command, const std::string& params) {
                std::ostringstream oss;
                oss << "{"
                    << "\"type\":\"command\","
                    << "\"cmd\":\"" << command << "\"";
                
                if (!params.empty()) {
                    oss << ",\"params\":" << params;
                }
                
                oss << "}";
                return oss.str();
            }
        }

    } // namespace Network
} // namespace DachshundEngine
