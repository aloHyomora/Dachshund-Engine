# Dachshund Engine

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

