#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <cmath>
#include <vector>
#include <random>

// Define ImGui docking flags if not available
#ifndef IMGUI_HAS_DOCK
#define ImGuiDockNodeFlags int
#define ImGuiDockNodeFlags_None 0
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Connection and sensor data structures
struct ConnectionStatus {
    bool is_connected = false;
    float last_data_time = 0.0f;
    int reconnect_attempts = 0;
    std::string status_message = "Not Connected";
};

struct SensorData {
    float temperature = 0.0f;
    float humidity = 0.0f;
    float pressure = 0.0f;
    float light = 0.0f;
    bool motion_detected = false;
    float cpu_usage = 0.0f;
    float memory_usage = 0.0f;
    bool data_valid = false;
};

// Generate mock sensor data only when connected
SensorData generateMockData(bool is_connected) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> temp_dist(20.0, 30.0);
    static std::uniform_real_distribution<> humidity_dist(40.0, 80.0);
    static std::uniform_real_distribution<> pressure_dist(1000.0, 1020.0);
    static std::uniform_real_distribution<> light_dist(0.0, 100.0);
    static std::uniform_real_distribution<> cpu_dist(10.0, 90.0);
    static std::uniform_real_distribution<> mem_dist(30.0, 70.0);
    static std::uniform_int_distribution<> motion_dist(0, 1);
    
    SensorData data;
    
    if (is_connected) {
        data.temperature = temp_dist(gen);
        data.humidity = humidity_dist(gen);
        data.pressure = pressure_dist(gen);
        data.light = light_dist(gen);
        data.motion_detected = motion_dist(gen);
        data.cpu_usage = cpu_dist(gen);
        data.memory_usage = mem_dist(gen);
        data.data_valid = true;
    } else {
        // No data when not connected
        data.data_valid = false;
    }
    
    return data;
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
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Mode management
    bool monitoring_mode = true;
    
    // Connection management
    ConnectionStatus connection;
    bool simulate_connection = false; // Toggle for testing
    
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    // Data storage for plotting
    std::vector<float> temp_data, humidity_data, pressure_data, light_data;
    std::vector<float> time_data;
    const int max_data_points = 100;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        float current_time = glfwGetTime();
        
        // Update connection status
        connection.is_connected = simulate_connection;
        if (connection.is_connected) {
            connection.status_message = "Connected to Raspberry Pi";
            connection.last_data_time = current_time;
            connection.reconnect_attempts = 0;
        } else {
            connection.status_message = "Not Connected - Waiting for Raspberry Pi";
        }

        // Generate sensor data only when connected
        SensorData current_data = generateMockData(connection.is_connected);
        
        // Store data for plotting only when connected and data is valid
        if (connection.is_connected && current_data.data_valid) {
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
            
            if (connection.is_connected) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "● CONNECTED");
                ImGui::Text("Status: %s", connection.status_message.c_str());
                ImGui::Text("Last Data: %.1f sec ago", current_time - connection.last_data_time);
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "● DISCONNECTED");
                ImGui::Text("Status: %s", connection.status_message.c_str());
                ImGui::Text("Reconnect Attempts: %d", connection.reconnect_attempts);
            }
            
            if (!connection.is_connected) {
                if (ImGui::Button("Retry Connection")) {
                    connection.reconnect_attempts++;
                    std::cout << "Attempting to reconnect... (Attempt " << connection.reconnect_attempts << ")" << std::endl;
                }
            }
            ImGui::Checkbox("Simulate Connection", &simulate_connection);
            ImGui::EndChild();

            ImGui::SameLine();

            // 2. System Status
            ImGui::BeginChild("SystemStatus", ImVec2(window_width, window_height), true);
            ImGui::Text("System Status");
            ImGui::Separator();
            
            if (connection.is_connected && current_data.data_valid) {
                ImGui::Text("CPU: %.1f%%", current_data.cpu_usage);
                ImGui::ProgressBar(current_data.cpu_usage / 100.0f, ImVec2(-1, 0));
                ImGui::Text("Memory: %.1f%%", current_data.memory_usage);
                ImGui::ProgressBar(current_data.memory_usage / 100.0f, ImVec2(-1, 0));
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No system data");
                ImGui::Text("CPU: --%");
                ImGui::ProgressBar(0.0f, ImVec2(-1, 0));
                ImGui::Text("Memory: --%");
                ImGui::ProgressBar(0.0f, ImVec2(-1, 0));
            }
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::EndChild();

            ImGui::SameLine();

            // 3. Temperature Monitor
            ImGui::BeginChild("TemperatureMonitor", ImVec2(window_width, window_height), true);
            ImGui::Text("Temperature Monitor");
            ImGui::Separator();
            
            if (connection.is_connected && current_data.data_valid) {
                ImGui::Text("Temperature: %.2f°C", current_data.temperature);
                
                if (!temp_data.empty() && ImPlot::BeginPlot("Temp", ImVec2(-1, window_height * 0.6f))) {
                    ImPlot::PlotLine("°C", time_data.data(), temp_data.data(), temp_data.size());
                    ImPlot::EndPlot();
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
                
                if (!humidity_data.empty() && ImPlot::BeginPlot("Environment", ImVec2(-1, window_height * 0.5f))) {
                    ImPlot::PlotLine("Humidity", time_data.data(), humidity_data.data(), humidity_data.size());
                    ImPlot::PlotLine("Light", time_data.data(), light_data.data(), light_data.size());
                    ImPlot::EndPlot();
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