#pragma once

#include <string>
#include <functional>
#include <memory>
#include "core/sensor/SensorManager.h"

namespace DachshundEngine {
    namespace Network {

        /// @brief 네트워크 연결 상태
        enum class ConnectionState {
            DISCONNECTED,
            CONNECTING,
            CONNECTED,
            CONNECTION_ERROR
        };

        /// @brief 메시지 타입
        enum class MessageType {
            SENSOR_DATA,
            COMMAND,
            RESPONSE,
            HEARTBEAT,
            MESSAGE_ERROR
        };

        /// @brief 네트워크 메시지 구조체
        struct NetworkMessage {
            MessageType type;
            std::string payload;  // JSON 문자열
            uint64_t timestamp;
        };

        /// @brief TCP 소켓 기반 네트워크 클라이언트
        class NetworkClient {
        public:
            NetworkClient();
            ~NetworkClient();

            // 복사 및 이동 생성자/대입 연산자 (Pimpl 패턴)
            NetworkClient(const NetworkClient&) = delete;
            NetworkClient& operator=(const NetworkClient&) = delete;
            NetworkClient(NetworkClient&&) noexcept;
            NetworkClient& operator=(NetworkClient&&) noexcept;

            /// @brief 라즈베리파이 서버에 연결
            /// @param ip_address 라즈베리파이 IP 주소
            /// @param port 포트 번호 (기본 8080)
            /// @return 연결 성공 여부
            bool connect(const std::string& ip_address, int port = 8080);

            /// @brief 연결 해제
            void disconnect();

            /// @brief 연결 상태 확인
            /// @return 연결 상태
            ConnectionState getConnectionState() const;

            /// @brief 메시지 전송
            /// @param message 전송할 메시지
            /// @return 전송 성공 여부
            bool sendMessage(const NetworkMessage& message);

            /// @brief 센서 데이터 요청
            /// @return 요청 성공 여부
            bool requestSensorData();

            /// @brief 샘플링 레이트 설정 명령 전송
            /// @param rate_ms 샘플링 주기 (밀리초)
            /// @return 전송 성공 여부
            bool setSamplingRate(int rate_ms);

            /// @brief 수신된 메시지 처리 (논블로킹)
            /// @return 수신된 메시지 개수
            int processIncomingMessages();

            /// @brief 센서 데이터 수신 콜백 설정
            /// @param callback 센서 데이터 수신 시 호출될 함수
            void setOnSensorDataReceived(std::function<void(const Sensor::SensorData&)> callback);

            /// @brief 연결 상태 변경 콜백 설정
            /// @param callback 연결 상태 변경 시 호출될 함수
            void setOnConnectionStateChanged(std::function<void(ConnectionState)> callback);

            /// @brief 마지막 에러 메시지 반환
            /// @return 에러 메시지
            std::string getLastError() const;

        private:
            class Impl;
            std::unique_ptr<Impl> pImpl;
        };

        /// @brief JSON 유틸리티 함수들
        namespace JsonUtil {
            /// @brief SensorData를 JSON 문자열로 변환
            std::string sensorDataToJson(const Sensor::SensorData& data);

            /// @brief JSON 문자열을 SensorData로 파싱
            bool parseSensorData(const std::string& json, Sensor::SensorData& data);

            /// @brief 명령 메시지 생성
            std::string createCommandMessage(const std::string& command, const std::string& params = "");
        }

    } // namespace Network
} // namespace DachshundEngine
