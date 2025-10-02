#pragma once
#include <string>
#include <memory>
namespace DachshundEngine {
    namespace Sensor {

        /// @brief 센서 연결 상태를 나타내는 구조체
        struct ConnectionStatus {
            bool is_connected = false;
            float last_data_time = 0.0f;
            int reconnect_attempts = 0;
            std::string status_message = "Not Connected";
            
            // 연결 상태 업데이트 메서드
            void updateStatus(bool connected, float current_time);
            void incrementReconnectAttempts();
            void resetConnectionStatus();
        };

        /// @brief 센서 데이터 구조체
        struct SensorData {
            float temperature = 0.0f;
            float humidity = 0.0f;
            float pressure = 0.0f;
            float light = 0.0f;
            bool motion_detected = false;
            float cpu_usage = 0.0f;
            float memory_usage = 0.0f;
            bool data_valid = false;
            
            // 데이터 검증 메서드
            bool isValid() const;
            void copyFrom(const SensorData& other);
            void resetSensorData();
        };

        /// @brief 센서 데이터 관리 모드
        enum class SensorMode {
            MOCK_DATA,      // 목 데이터 생성
            RASPBERRY_PI,   // 라즈베리파이 실제 센서 데이터
            FILE_REPLAY    // 파일에서 데이터 재생 (추후)
        };

        /// @brief 센서 데이터 매니저 클래스
        class SensorDataManager {
            public:
                SensorDataManager(SensorMode mode = SensorMode::MOCK_DATA);
                ~SensorDataManager();
                
                // 복사 방지 (리소스 관리 클래스이므로)
                SensorDataManager(const SensorDataManager&) = delete;
                SensorDataManager& operator=(const SensorDataManager&) = delete;

                // 이동 허용(효율성을 위해)
                SensorDataManager(SensorDataManager&&) noexcept;
                SensorDataManager& operator=(SensorDataManager&&) noexcept;

                // 연결 관리
                bool connectToRaspberryPi(const std::string& ip_address, int port);
                void disconnect();
                bool isConnected() const;

                // 데이터 수신
                SensorData getCurrentSensorData();

                // 모드 변경
                void setMode(SensorMode mode);
                SensorMode getMode() const;

                // 설정
                void setUpdateInterval(float milliseconds);
            private:
                class Impl;
                std::unique_ptr<Impl> pImpl; // Pimpl 패턴으로 구현 숨기기
        };
    }
}