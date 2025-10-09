#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <cmath>
#include <vector>

// 분리된 센서 타입 포함
#include "core/sensor/SensorManager.h"

// Define ImGui docking flags if not available
#ifndef IMGUI_HAS_DOCK
#define ImGuiDockNodeFlags int
#define ImGuiDockNodeFlags_None 0
#endif

using namespace DachshundEngine::Sensor;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main()
{
    std::cout << "Dachshund Engine Dashboard Initialized!" << std::endl;
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return -1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1440, 720, "Dachshund Engine - Raspberry Pi Sensor Dashboard", nullptr, nullptr);
    if (window == nullptr)
        return -1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Only enable docking if supported
    #ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    #endif

    // Setup Dear ImGui style
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Mode management
    bool monitoring_mode = true;
    
    // 센서 매니저 초기화 (기본값: 목 데이터 모드)
    SensorDataManager sensorManager(SensorMode::MOCK_DATA);
    ConnectionStatus connection;
    bool simulate_connection = false; // Toggle for testing
    
    // 연결 설정
    static char raspberry_pi_ip[64] = "192.168.219.111";
    static int raspberry_pi_port = 8080;
    static bool use_raspberry_pi = false;
    
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    // Data storage for plotting
    std::vector<float> temp_data, humidity_data, pressure_data, light_data;
    std::vector<float> time_data;
    
    // System Status data storage
    std::vector<float> cpu_data, memory_data;
    std::vector<float> system_time_data;
    const int max_system_data_points = 60; // 60개 데이터 포인트 (60초)
    
    const int max_data_points = 100;
    
    // System Status 선택 옵션
    enum class SystemMetric {
        CPU,
        MEMORY
    };
    SystemMetric selected_metric = SystemMetric::CPU;
    
    // 시스템 데이터 수집 타이머 (1초마다)
    float last_system_data_time = 0.0f;
    const float system_data_interval = 1.0f; // 1초마다 수집

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        float current_time = glfwGetTime();
        
        // 연결 상태 업데이트
        connection.updateStatus(sensorManager.isConnected(), current_time);

        // 센서 데이터 가져오기
        SensorData current_data = sensorManager.getCurrentSensorData();
        
        // Store sensor data for plotting only when connected and data is valid
        if (current_data.isValid()) {   // 목 데이터 확인 용 조건 주석처리 connection.is_connected && 
            time_data.push_back(current_time);
            temp_data.push_back(current_data.temperature);
            humidity_data.push_back(current_data.humidity);
            pressure_data.push_back(current_data.pressure);
            light_data.push_back(current_data.light);
            
            // Keep only recent data
            if (time_data.size() > max_data_points) {
                time_data.erase(time_data.begin());
                temp_data.erase(temp_data.begin());
                humidity_data.erase(humidity_data.begin());
                pressure_data.erase(pressure_data.begin());
                light_data.erase(light_data.begin());
            }
        }
        
        // Store system status data (1초마다만 수집)
        if (current_data.data_valid && (current_time - last_system_data_time >= system_data_interval)) {
            system_time_data.push_back(current_time);
            cpu_data.push_back(current_data.cpu_usage);
            memory_data.push_back(current_data.memory_usage);
            
            // 60개 데이터 포인트만 유지
            if (system_time_data.size() > max_system_data_points) {
                system_time_data.erase(system_time_data.begin());
                cpu_data.erase(cpu_data.begin());
                memory_data.erase(memory_data.begin());
            }
            
            last_system_data_time = current_time;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create main menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Monitoring Mode", nullptr, &monitoring_mode);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("Simulate Connection", nullptr, &simulate_connection);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Monitoring Mode
        if (monitoring_mode) {
            // Main monitoring window with header
            ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1420, 680), ImGuiCond_FirstUseEver);
            
            ImGui::Begin("Monitoring Mode", &monitoring_mode, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

            if (ImGui::BeginMenuBar()) {
                ImGui::Text("Raspberry Pi Sensor Monitoring Dashboard");
                ImGui::EndMenuBar();
            }

            // Create grid layout for 6 windows (2x3)
            float window_width = (ImGui::GetContentRegionAvail().x - 20) / 3;
            float window_height = (ImGui::GetContentRegionAvail().y - 10) / 2;

            // First row
            // 1. Connection Status
            ImGui::BeginChild("ConnectionStatus", ImVec2(window_width, window_height), true);             
            ImGui::Text("Connection Status");
            ImGui::Separator();
            
            // 연결 모드 선택
            ImGui::Text("Data Source:");
            if (ImGui::RadioButton("Mock Data", !use_raspberry_pi)) {
                use_raspberry_pi = false;
                sensorManager.setMode(SensorMode::MOCK_DATA);
                sensorManager.disconnect();
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Raspberry Pi", use_raspberry_pi)) {
                use_raspberry_pi = true;
            }
            
            ImGui::Separator();
            
            // Raspberry Pi 연결 설정
            if (use_raspberry_pi) {
                ImGui::Text("Raspberry Pi Settings:");
                ImGui::InputText("IP Address", raspberry_pi_ip, sizeof(raspberry_pi_ip));
                ImGui::InputInt("Port", &raspberry_pi_port);
                
                if (!connection.is_connected) {
                    if (ImGui::Button("Connect to Raspberry Pi", ImVec2(-1, 0))) {
                        std::cout << "Connecting to " << raspberry_pi_ip << ":" << raspberry_pi_port << std::endl;
                        if (sensorManager.connectToRaspberryPi(raspberry_pi_ip, raspberry_pi_port)) {
                            std::cout << "Connected successfully!" << std::endl;
                        } else {
                            std::cout << "Connection failed!" << std::endl;
                        }
                    }
                } else {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "● CONNECTED");
                    ImGui::Text("IP: %s:%d", raspberry_pi_ip, raspberry_pi_port);
                    ImGui::Text("Last Data: %.1f sec ago", current_time - connection.last_data_time);
                    
                    if (ImGui::Button("Disconnect", ImVec2(-1, 0))) {
                        sensorManager.disconnect();
                        std::cout << "Disconnected from Raspberry Pi" << std::endl;
                    }
                }
            } else {
                ImGui::TextColored(ImVec4(0, 1, 1, 1), "● MOCK DATA MODE");
                ImGui::Text("Generating simulated sensor data");
            }
            
            if (connection.is_connected) {
                ImGui::Separator();
                ImGui::Text("Status: %s", connection.status_message.c_str());
            } else if (use_raspberry_pi) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "● DISCONNECTED");
                ImGui::Text("Reconnect Attempts: %d", connection.reconnect_attempts);
            }
            
            ImGui::EndChild();

            ImGui::SameLine();

            // 2. System Status (작업 관리자 스타일)
            ImGui::BeginChild("SystemStatus", ImVec2(window_width, window_height), true);
            ImGui::Text("System Status");
            ImGui::Separator();
            
            // 두 열로 분할
            float left_panel_width = window_width * 0.30f;
            float right_panel_width = window_width * 0.65f;
            
            // 왼쪽 패널: 선택 가능한 메트릭 리스트
            ImGui::BeginChild("MetricList", ImVec2(left_panel_width, -1), false);
            
            // CPU 선택 (모든 내용이 하나의 Selectable 영역)
            bool is_cpu_selected = (selected_metric == SystemMetric::CPU);
            ImVec2 cpu_item_pos = ImGui::GetCursorScreenPos();
            
            // Selectable 높이를 미니 그래프 포함해서 설정
            float item_height = 50.0f;
            if (ImGui::Selectable("##cpu_select", is_cpu_selected, 0, ImVec2(left_panel_width - 10, item_height))) {
                selected_metric = SystemMetric::CPU;
            }
            
            // Selectable 위에 내용 그리기 (미니 그래프 왼쪽, 텍스트 오른쪽)
            ImGui::SetCursorScreenPos(ImVec2(cpu_item_pos.x + 5, cpu_item_pos.y + 5));
            ImGui::BeginGroup();
            
            // 미니 그래프를 먼저 그리기 (왼쪽)
            if (!cpu_data.empty() && cpu_data.size() > 1) {
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
                ImGui::PlotLines("##cpu_mini", cpu_data.data(), cpu_data.size(), 
                    0, nullptr, 0.0f, 100.0f, ImVec2(50, item_height - 10));
                ImGui::PopStyleColor(2);
            } else {
                ImGui::Dummy(ImVec2(50, item_height - 10));
            }
            
            // 텍스트를 오른쪽에 배치
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("CPU");
            ImGui::Text("%.1f%%", current_data.cpu_usage);  // 실제 CPU 속도로 대체 가능
            ImGui::EndGroup();
            
            ImGui::EndGroup();
            
            // 다음 아이템 위치 조정
            ImGui::SetCursorScreenPos(ImVec2(cpu_item_pos.x, cpu_item_pos.y + item_height));
            ImGui::Spacing();
            
            // Memory 선택 (모든 내용이 하나의 Selectable 영역)
            bool is_memory_selected = (selected_metric == SystemMetric::MEMORY);
            ImVec2 memory_item_pos = ImGui::GetCursorScreenPos();
            
            if (ImGui::Selectable("##memory_select", is_memory_selected, 0, ImVec2(left_panel_width - 10, item_height))) {
                selected_metric = SystemMetric::MEMORY;
            }
            
            // Selectable 위에 내용 그리기 (미니 그래프 왼쪽, 텍스트 오른쪽)
            ImGui::SetCursorScreenPos(ImVec2(memory_item_pos.x + 5, memory_item_pos.y + 5));
            ImGui::BeginGroup();
            
            // 미니 그래프를 먼저 그리기 (왼쪽)
            if (!memory_data.empty() && memory_data.size() > 1) {
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.6f, 0.4f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
                ImGui::PlotLines("##memory_mini", memory_data.data(), memory_data.size(), 
                    0, nullptr, 0.0f, 100.0f, ImVec2(50, item_height - 10));
                ImGui::PopStyleColor(2);
            } else {
                ImGui::Dummy(ImVec2(50, item_height - 10));
            }
            
            // 텍스트를 오른쪽에 배치
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("Memory");
            ImGui::Text("%.1f%%", current_data.memory_usage);
            ImGui::EndGroup();
            
            ImGui::EndGroup();
            
            // 다음 아이템 위치 조정
            ImGui::SetCursorScreenPos(ImVec2(memory_item_pos.x, memory_item_pos.y + item_height));
            
            ImGui::EndChild();
            
            ImGui::SameLine();
            
            // 오른쪽 패널: 선택된 메트릭의 상세 그래프
            ImGui::BeginChild("MetricDetail", ImVec2(right_panel_width, -1), false);
            
            if (selected_metric == SystemMetric::CPU) {
                // CPU 상세 정보
                ImGui::Text("CPU Usage");
                ImGui::Separator();
                
                // 큰 사용률 표시
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
                if (current_data.data_valid) {
                    ImGui::SetWindowFontScale(2.0f);
                    ImGui::Text("%.0f%%", current_data.cpu_usage);
                    ImGui::SetWindowFontScale(1.0f);
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "--%%");
                }
                ImGui::PopStyleColor();
                
                // CPU 상세 그래프
                if (!cpu_data.empty() && !system_time_data.empty()) {
                    ImGui::Spacing();
                    
                    // 상대적 시간 계산 (현재 시간 기준으로 과거 몇 초 전인지)
                    std::vector<float> relative_time_data;
                    relative_time_data.reserve(system_time_data.size());
                    for (size_t i = 0; i < system_time_data.size(); ++i) {
                        relative_time_data.push_back(system_time_data[i] - current_time);
                    }
                    
                    if (ImPlot::BeginPlot("##cpu_detail", ImVec2(-1, window_height * 0.5f), 
                        ImPlotFlags_NoTitle | ImPlotFlags_NoLegend)) {
                        ImPlot::SetupAxes("Time", "Usage (%)", 0, 0);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Always);
                        // X축: -60초부터 0초(현재)까지
                        ImPlot::SetupAxisLimits(ImAxis_X1, -60.0, 0.0, ImGuiCond_Always);
                        ImPlot::SetupAxisFormat(ImAxis_X1, "%.0fs");
                        
                        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
                        ImPlot::PlotLine("CPU", relative_time_data.data(), cpu_data.data(), cpu_data.size());
                        ImPlot::PopStyleColor();
                        
                        ImPlot::EndPlot();
                    }
                }
                
                // 추가 정보
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Last 60 seconds");
                if (!cpu_data.empty()) {
                    float avg_cpu = 0.0f;
                    for (float val : cpu_data) avg_cpu += val;
                    avg_cpu /= cpu_data.size();
                    ImGui::Text("Average: %.1f%%", avg_cpu);
                }
                
            } else if (selected_metric == SystemMetric::MEMORY) {
                // Memory 상세 정보
                ImGui::Text("Memory Usage");
                ImGui::Separator();
                
                // 큰 사용률 표시
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.4f, 0.8f, 1.0f));
                if (current_data.data_valid) {
                    ImGui::SetWindowFontScale(2.0f);
                    ImGui::Text("%.0f%%", current_data.memory_usage);
                    ImGui::SetWindowFontScale(1.0f);
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "--%%");
                }
                ImGui::PopStyleColor();
                
                // Memory 상세 그래프
                if (!memory_data.empty() && !system_time_data.empty()) {
                    ImGui::Spacing();
                    
                    // 상대적 시간 계산 (현재 시간 기준으로 과거 몇 초 전인지)
                    std::vector<float> relative_time_data;
                    relative_time_data.reserve(system_time_data.size());
                    for (size_t i = 0; i < system_time_data.size(); ++i) {
                        relative_time_data.push_back(system_time_data[i] - current_time);
                    }
                    
                    if (ImPlot::BeginPlot("##memory_detail", ImVec2(-1, window_height * 0.5f),
                        ImPlotFlags_NoTitle | ImPlotFlags_NoLegend)) {
                        ImPlot::SetupAxes("Time", "Usage (%)", 0, 0);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Always);
                        // X축: -60초부터 0초(현재)까지
                        ImPlot::SetupAxisLimits(ImAxis_X1, -60.0, 0.0, ImGuiCond_Always);
                        ImPlot::SetupAxisFormat(ImAxis_X1, "%.0fs");
                        
                        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.6f, 0.4f, 0.8f, 1.0f));
                        ImPlot::PlotLine("Memory", relative_time_data.data(), memory_data.data(), memory_data.size());
                        ImPlot::PopStyleColor();
                        
                        ImPlot::EndPlot();
                    }
                }
                
                // 추가 정보
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Last 60 seconds");
                if (!memory_data.empty()) {
                    float avg_memory = 0.0f;
                    for (float val : memory_data) avg_memory += val;
                    avg_memory /= memory_data.size();
                    ImGui::Text("Average: %.1f%%", avg_memory);
                }
            }
            
            ImGui::EndChild();
            
            ImGui::EndChild();

            ImGui::SameLine();

            // 3. Temperature Monitor
            ImGui::BeginChild("TemperatureMonitor", ImVec2(window_width, window_height), true);
            ImGui::Text("Temperature Monitor");
            ImGui::Separator();
            
            if (connection.is_connected && current_data.data_valid) {
                ImGui::Text("Temperature: %.2f°C", current_data.temperature);
                
                if (!temp_data.empty() && !time_data.empty()) {
                    // 상대적 시간 계산
                    std::vector<float> relative_time;
                    relative_time.reserve(time_data.size());
                    for (size_t i = 0; i < time_data.size(); ++i) {
                        relative_time.push_back(time_data[i] - current_time);
                    }
                    
                    if (ImPlot::BeginPlot("Temp", ImVec2(-1, window_height * 0.6f))) {
                        ImPlot::SetupAxes("Time", "°C", 0, 0);
                        ImPlot::SetupAxisLimits(ImAxis_X1, -60.0, 0.0, ImGuiCond_Always);
                        ImPlot::SetupAxisFormat(ImAxis_X1, "%.0fs");
                        ImPlot::PlotLine("°C", relative_time.data(), temp_data.data(), temp_data.size());
                        ImPlot::EndPlot();
                    }
                }
                
                if (current_data.temperature > 28.0f) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "⚠ High Temp!");
                } else if (current_data.temperature < 22.0f) {
                    ImGui::TextColored(ImVec4(0, 0, 1, 1), "❄ Low Temp");
                }
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No temperature data");
            }
            ImGui::EndChild();

            // Second row
            // 4. Environmental Sensors
            ImGui::BeginChild("EnvironmentalSensors", ImVec2(window_width, window_height), true);
            ImGui::Text("Environmental Sensors");
            ImGui::Separator();
            
            if (connection.is_connected && current_data.data_valid) {
                ImGui::Text("Humidity: %.1f%%", current_data.humidity);
                ImGui::Text("Pressure: %.1f hPa", current_data.pressure);
                ImGui::Text("Light: %.1f%%", current_data.light);
                
                if (!humidity_data.empty() && !time_data.empty()) {
                    // 상대적 시간 계산
                    std::vector<float> relative_time;
                    relative_time.reserve(time_data.size());
                    for (size_t i = 0; i < time_data.size(); ++i) {
                        relative_time.push_back(time_data[i] - current_time);
                    }
                    
                    if (ImPlot::BeginPlot("Environment", ImVec2(-1, window_height * 0.5f))) {
                        ImPlot::SetupAxes("Time", "%", 0, 0);
                        ImPlot::SetupAxisLimits(ImAxis_X1, -60.0, 0.0, ImGuiCond_Always);
                        ImPlot::SetupAxisFormat(ImAxis_X1, "%.0fs");
                        ImPlot::PlotLine("Humidity", relative_time.data(), humidity_data.data(), humidity_data.size());
                        ImPlot::PlotLine("Light", relative_time.data(), light_data.data(), light_data.size());
                        ImPlot::EndPlot();
                    }
                }
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No environmental data");
                ImGui::Text("Humidity: --%");
                ImGui::Text("Pressure: -- hPa");
                ImGui::Text("Light: --%");
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // 5. Motion/Proximity Sensors
            ImGui::BeginChild("MotionProximity", ImVec2(window_width, window_height), true);
            ImGui::Text("Motion/Proximity");
            ImGui::Separator();
            
            ImGui::Text("Motion Detection:");
            if (connection.is_connected && current_data.data_valid) {
                if (current_data.motion_detected) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "● MOTION DETECTED");
                } else {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "● No Motion");
                }
                ImGui::Text("PIR: %s", current_data.motion_detected ? "Triggered" : "Idle");
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "● No Data");
                ImGui::Text("PIR: No connection");
            }
            ImGui::Text("Distance: %s", (connection.is_connected && current_data.data_valid) ? "-- cm" : "No connection");
            ImGui::EndChild();

            ImGui::SameLine();

            // 6. Data Logging
            ImGui::BeginChild("DataLogging", ImVec2(window_width, window_height), true);
            ImGui::Text("Data Logging");
            ImGui::Separator();
            
            if (connection.is_connected) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "● Collection active");
                ImGui::Text("Data Points: %zu", time_data.size());
                ImGui::Text("Rate: Real-time");
                
                if (ImGui::Button("Export Data")) {
                    std::cout << "Exporting sensor data..." << std::endl;
                }
                if (ImGui::Button("Clear Data")) {
                    time_data.clear();
                    temp_data.clear();
                    humidity_data.clear();
                    pressure_data.clear();
                    light_data.clear();
                    system_time_data.clear();
                    cpu_data.clear();
                    memory_data.clear();
                }
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "● Collection stopped");
                ImGui::Text("Data Points: %zu (cached)", time_data.size());
                ImGui::Text("Rate: Waiting...");
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Connect to resume");
            }
            ImGui::EndChild();

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}