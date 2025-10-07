#!/usr/bin/env python3
"""
Dachshund Engine - Raspberry Pi Sensor Server
센서 데이터를 수집하고 TCP 소켓을 통해 PC로 전송하는 서버
"""

import socket
import json
import time
import struct
import threading
from typing import Dict, Any, Optional
from dataclasses import dataclass, asdict


@dataclass
class SensorData:
    """센서 데이터 구조체"""
    temperature: float = 0.0
    humidity: float = 0.0
    pressure: float = 0.0
    light: float = 0.0
    motion_detected: bool = False
    cpu_usage: float = 0.0
    memory_usage: float = 0.0


class SensorReader:
    """센서 데이터 수집 클래스"""
    
    def __init__(self):
        # TODO: 실제 센서 초기화
        self.mock_mode = True  # 일단 목 데이터 모드
        
    def read_sensors(self) -> SensorData:
        """센서 데이터 읽기"""
        
        if self.mock_mode:
            # 목 데이터 생성
            import random
            return SensorData(
                temperature=random.uniform(20.0, 30.0),
                humidity=random.uniform(40.0, 80.0),
                pressure=random.uniform(1000.0, 1020.0),
                light=random.uniform(0.0, 100.0),
                motion_detected=random.choice([True, False]),
                cpu_usage=self._get_cpu_usage(),
                memory_usage=self._get_memory_usage()
            )
        else:
            # TODO: 실제 센서에서 데이터 읽기
            return SensorData()
    
    def _get_cpu_usage(self) -> float:
        """CPU 사용률 측정"""
        try:
            with open('/proc/stat', 'r') as f:
                cpu_line = f.readline()
                values = [float(x) for x in cpu_line.split()[1:]]
                total = sum(values)
                idle = values[3]
                return 100.0 * (1 - idle / total) if total > 0 else 0.0
        except:
            return 0.0
    
    def _get_memory_usage(self) -> float:
        """메모리 사용률 측정"""
        try:
            with open('/proc/meminfo', 'r') as f:
                lines = f.readlines()
                mem_total = int(lines[0].split()[1])
                mem_free = int(lines[1].split()[1])
                return 100.0 * (1 - mem_free / mem_total) if mem_total > 0 else 0.0
        except:
            return 0.0


class SensorServer:
    """TCP 소켓 기반 센서 데이터 서버"""
    
    def __init__(self, host: str = '0.0.0.0', port: int = 8080):
        self.host = host
        self.port = port
        self.server_socket: Optional[socket.socket] = None
        self.client_socket: Optional[socket.socket] = None
        self.running = False
        self.sensor_reader = SensorReader()
        self.sampling_rate_ms = 1000  # 기본 1초
        
    def start(self):
        """서버 시작"""
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(1)
        
        self.running = True
        print(f"[Server] Listening on {self.host}:{self.port}")
        
        try:
            while self.running:
                print("[Server] Waiting for client connection...")
                client_socket, client_address = self.server_socket.accept()
                print(f"[Server] Client connected from {client_address}")
                
                self.client_socket = client_socket
                self.handle_client()
                
        except KeyboardInterrupt:
            print("\n[Server] Shutting down...")
        finally:
            self.stop()
    
    def handle_client(self):
        """클라이언트 처리"""
        # 센서 데이터 전송 스레드 시작
        sender_thread = threading.Thread(target=self.send_sensor_data_loop)
        sender_thread.daemon = True
        sender_thread.start()
        
        # 명령 수신 루프
        try:
            while self.running and self.client_socket:
                # 메시지 길이 수신 (4바이트)
                length_data = self._recv_exact(4)
                if not length_data:
                    break
                
                message_length = struct.unpack('!I', length_data)[0]
                
                # 페이로드 수신
                payload = self._recv_exact(message_length)
                if not payload:
                    break
                
                # JSON 파싱 및 명령 처리
                try:
                    message = json.loads(payload.decode('utf-8'))
                    self.process_command(message)
                except json.JSONDecodeError as e:
                    print(f"[Server] JSON decode error: {e}")
                    
        except Exception as e:
            print(f"[Server] Client handler error: {e}")
        finally:
            if self.client_socket:
                self.client_socket.close()
                self.client_socket = None
            print("[Server] Client disconnected")
    
    def send_sensor_data_loop(self):
        """주기적으로 센서 데이터 전송"""
        while self.running and self.client_socket:
            try:
                # 센서 데이터 읽기
                sensor_data = self.sensor_reader.read_sensors()
                
                # JSON 메시지 생성
                message = {
                    "type": "sensor_data",
                    "timestamp": int(time.time() * 1000),
                    "data": asdict(sensor_data)
                }
                
                # 전송
                self.send_message(message)
                
                # 샘플링 레이트만큼 대기
                time.sleep(self.sampling_rate_ms / 1000.0)
                
            except Exception as e:
                print(f"[Server] Sensor data send error: {e}")
                break
    
    def send_message(self, message: Dict[str, Any]):
        """메시지 전송"""
        if not self.client_socket:
            return
        
        try:
            payload = json.dumps(message).encode('utf-8')
            length = struct.pack('!I', len(payload))
            
            self.client_socket.sendall(length + payload)
            
        except Exception as e:
            print(f"[Server] Send error: {e}")
            if self.client_socket:
                self.client_socket.close()
                self.client_socket = None
    
    def process_command(self, message: Dict[str, Any]):
        """명령 처리"""
        msg_type = message.get('type')
        
        if msg_type == 'command':
            cmd = message.get('cmd')
            params = message.get('params', {})
            
            print(f"[Server] Received command: {cmd}")
            
            if cmd == 'get_sensor_data':
                # 즉시 센서 데이터 전송
                sensor_data = self.sensor_reader.read_sensors()
                response = {
                    "type": "sensor_data",
                    "timestamp": int(time.time() * 1000),
                    "data": asdict(sensor_data)
                }
                self.send_message(response)
                
            elif cmd == 'set_sampling_rate':
                rate_ms = params.get('rate_ms', 1000)
                self.sampling_rate_ms = max(100, min(rate_ms, 10000))  # 100ms ~ 10s
                print(f"[Server] Sampling rate changed to {self.sampling_rate_ms}ms")
                
                # 응답 전송
                response = {
                    "type": "response",
                    "cmd": cmd,
                    "success": True,
                    "message": f"Sampling rate set to {self.sampling_rate_ms}ms"
                }
                self.send_message(response)
    
    def _recv_exact(self, n: int) -> Optional[bytes]:
        """정확히 n바이트 수신"""
        data = b''
        while len(data) < n:
            packet = self.client_socket.recv(n - len(data))
            if not packet:
                return None
            data += packet
        return data
    
    def stop(self):
        """서버 종료"""
        self.running = False
        if self.client_socket:
            self.client_socket.close()
        if self.server_socket:
            self.server_socket.close()


if __name__ == '__main__':
    server = SensorServer(host='0.0.0.0', port=8080)
    server.start()
