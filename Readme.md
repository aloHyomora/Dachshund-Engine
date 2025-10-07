# Dachshund Engine: 실시간 IoT 센서 모니터링 시스템

1️⃣ 실시간 센서 데이터 수집 및 시각화
라즈베리파이에 연결된 센서(온도, 습도, 기압, 조도, 모션 등)의 데이터를 실시간으로 수집
ImGui/ImPlot 기반의 직관적인 그래픽 대시보드로 시각화
시계열 그래프로 센서 변화 추이 분석

2️⃣ 원격 IoT 디바이스 모니터링
TCP/IP 네트워크를 통한 라즈베리파이 원격 연결
Windows/Linux PC에서 센서 데이터 실시간 모니터링
양방향 통신으로 디바이스 제어 가능

3️⃣ 크로스 플랫폼 개발 학습
Windows/Ubuntu 모두에서 빌드/실행 가능
CMake + vcpkg를 활용한 C++ 구조
Pimpl 패턴, 콜백 기반 아키텍처 등 설계 패턴 학습

## Prerequisites
```bash
sudo apt update
sudo apt install -y git build-essential pkg-config cmake ninja-build \
    libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev

# vcpkg
git clone https://github.com/microsoft/vcpkg ~/vcpkg
cd ~/vcpkg

# 부트스트랩 (vcpkg 빌드)
./bootstrap-vcpkg.sh          # (Windows는 .\bootstrap-vcpkg.bat)

# CMake 전역 통합
./vcpkg integrate install

cd ~/github/Dachshund-Engine

# 의존성 설치 (vcpkg.json이 자동 인식됨)
~/vcpkg/vcpkg install
```

### Ubuntu

```bash
# CMake 설정 (Ubuntu)
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_BUILD_TYPE=Release

# 빌드
cmake --build build -j

cd build
./dashboard

```

### Windows
```bash
# CMake 설정 (Windows)
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE="$env:USERPROFILE\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_BUILD_TYPE=Release

# 빌드
cmake --build build -j

cd build\Release
.\dashboard.exe
```

