# Raspberry Pi Sensor Server

라즈베리파이에서 센서 데이터를 수집하고 Windows PC로 전송하는 서버 프로그램

## 📋 요구사항

- Raspberry Pi 3/4/5 (Raspberry Pi OS)
- Python 3.7 이상

## 🚀 설치 및 실행

### 1. 파일 전송 (Windows → Raspberry Pi)

Windows PowerShell에서:

```powershell
# rpi 폴더 전체 전송
scp -r rpi/ aloho@<ip_address>:~/dachshund-engine/
```

또는 단일 파일:

```powershell
scp rpi/src/sensor_server.py aloho@192.168.219.111:~/sensor_server.py
```

### 2. 라즈베리파이에서 실행

SSH로 접속:

```bash
ssh aloho@192.168.219.111
```

서버 실행:

```bash
cd ~/dachshund-engine/rpi/src
chmod +x sensor_server.py
python3 sensor_server.py
```

서버가 `0.0.0.0:8080`에서 대기합니다.

### 3. 자동 시작 설정 (선택사항)

시스템 부팅 시 자동 실행:

```bash
# systemd 서비스 파일 생성
sudo nano /etc/systemd/system/sensor-server.service
```

다음 내용 입력:

```ini
[Unit]
Description=Dachshund Engine Sensor Server
After=network.target

[Service]
Type=simple
User=aloho
WorkingDirectory=/home/aloho/dachshund-engine/rpi/src
ExecStart=/usr/bin/python3 /home/aloho/dachshund-engine/rpi/src/sensor_server.py
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

서비스 활성화:

```bash
sudo systemctl daemon-reload
sudo systemctl enable sensor-server.service
sudo systemctl start sensor-server.service
sudo systemctl status sensor-server.service
```

## 🔌 네트워크 설정

라즈베리파이의 IP 주소 확인:

```bash
hostname -I
```

방화벽 설정 (필요시):

```bash
sudo ufw allow 8080/tcp
```

## 📊 프로토콜

### 메시지 포맷

모든 메시지는 **4바이트 길이 헤더 + JSON 페이로드** 형식

```
[Length(4 bytes, network byte order)][JSON Payload]
```

### PC → Raspberry Pi (명령)

센서 데이터 요청:
```json
{
  "type": "command",
  "cmd": "get_sensor_data"
}
```

샘플링 레이트 변경:
```json
{
  "type": "command",
  "cmd": "set_sampling_rate",
  "params": {
    "rate_ms": 1000
  }
}
```

### Raspberry Pi → PC (센서 데이터)

```json
{
  "type": "sensor_data",
  "timestamp": 1696680000,
  "data": {
    "temperature": 25.3,
    "humidity": 65.2,
    "pressure": 1013.5,
    "light": 45.0,
    "motion_detected": false,
    "cpu_usage": 35.2,
    "memory_usage": 52.1
  }
}
```

## 🛠️ 실제 센서 연결

현재는 목(mock) 데이터를 전송합니다. 실제 센서를 연결하려면:

1. `sensor_server.py`에서 `SensorReader` 클래스 수정(예정)
2. 센서별 라이브러리 설치 (예: `adafruit-circuitpython-dht`)
3. `mock_mode = False`로 변경
