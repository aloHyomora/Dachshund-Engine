# Dachshund Engine: Interactive Robotic System

## 프로젝트 개요

Dachshund Engine은 Jetson Orin 기반의 상호작용 로봇 시스템(Interactive Robotic System)으로,
사용자의 입력(명령·제스처·음성 등)에 반응하여 상호작용하고
센서 데이터를 실시간으로 수집·시각화·제어할 수 있는 로컬 지능형 인터랙티브 플랫폼입니다.

본 엔진은 PC 대시보드와 Jetson 기반 제어 시스템이
Event Bus 기반 양방향 통신 구조로 연결되어,
센서·AI·UI 모듈 간의 독립적 확장성과 비동기 통신을 제공합니다.

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

