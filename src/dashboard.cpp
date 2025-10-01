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
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Dachshund Engine - Raspberry Pi Sensor Dashboard", nullptr, nullptr);
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

    // Window visibility flags
    bool show_connection_status = true;
    bool show_system_status = true;
    bool show_temperature_monitor = true;
    bool show_environmental_sensors = true;
    bool show_motion_proximity = true;
    bool show_data_logging = true;
    
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
                ImGui::MenuItem("Connection Status", nullptr, &show_connection_status);
                ImGui::MenuItem("System Status", nullptr, &show_system_status);
                ImGui::MenuItem("Temperature Monitor", nullptr, &show_temperature_monitor);
                ImGui::MenuItem("Environmental Sensors", nullptr, &show_environmental_sensors);
                ImGui::MenuItem("Motion/Proximity", nullptr, &show_motion_proximity);
                ImGui::MenuItem("Data Logging", nullptr, &show_data_logging);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("Simulate Connection", nullptr, &simulate_connection);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // 1. Connection Status Window
        if (show_connection_status) {
            ImGui::Begin("Connection Status", &show_connection_status);
            
            ImGui::Text("Raspberry Pi Connection");
            ImGui::Separator();
            
            // Connection status indicator
            if (connection.is_connected) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "CONNECTED");
                ImGui::Text("Status: %s", connection.status_message.c_str());
                ImGui::Text("Last Data: %.1f seconds ago", current_time - connection.last_data_time);
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), " DISCONNECTED");
                ImGui::Text("Status: %s", connection.status_message.c_str());
                ImGui::Text("Reconnect Attempts: %d", connection.reconnect_attempts);
            }
            
            ImGui::Separator();
            
            if (!connection.is_connected) {
                if (ImGui::Button("Retry Connection")) {
                    connection.reconnect_attempts++;
                    std::cout << "Attempting to reconnect... (Attempt " << connection.reconnect_attempts << ")" << std::endl;
                }
            }
            
            // Debug toggle
            ImGui::Checkbox("Simulate Connection (Debug)", &simulate_connection);
            
            ImGui::End();
        }

        // 2. System Status Window
        if (show_system_status) {
            ImGui::Begin("System Status", &show_system_status);
            
            ImGui::Text("Raspberry Pi System Monitor");
            ImGui::Separator();
            
            if (connection.is_connected && current_data.data_valid) {
                ImGui::Text("CPU Usage: %.1f%%", current_data.cpu_usage);
                ImGui::ProgressBar(current_data.cpu_usage / 100.0f);
                
                ImGui::Text("Memory Usage: %.1f%%", current_data.memory_usage);
                ImGui::ProgressBar(current_data.memory_usage / 100.0f);
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No system data - Device not connected");
                ImGui::Text("CPU Usage: --%%");
                ImGui::ProgressBar(0.0f);
                ImGui::Text("Memory Usage: --%%");
                ImGui::ProgressBar(0.0f);
            }
            
            ImGui::Separator();
            ImGui::Text("Dashboard FPS: %.1f", io.Framerate);
            
            ImGui::End();
        }

        // 3. Temperature Monitor Window
        if (show_temperature_monitor) {
            ImGui::Begin("Temperature Monitor", &show_temperature_monitor);
            
            if (connection.is_connected && current_data.data_valid) {
                ImGui::Text("Current Temperature: %.2f°C", current_data.temperature);
                
                if (!temp_data.empty() && ImPlot::BeginPlot("Temperature Over Time")) {
                    ImPlot::PlotLine("Temperature (°C)", time_data.data(), temp_data.data(), temp_data.size());
                    ImPlot::EndPlot();
                }
                
                // Temperature alerts
                if (current_data.temperature > 28.0f) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "⚠ High Temperature Warning!");
                } else if (current_data.temperature < 22.0f) {
                    ImGui::TextColored(ImVec4(0, 0, 1, 1), "❄ Low Temperature");
                }
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No temperature data available");
                ImGui::Text("Connect to Raspberry Pi to view temperature data");
            }
            
            ImGui::End();
        }

        // 4. Environmental Sensors Window
        if (show_environmental_sensors) {
            ImGui::Begin("Environmental Sensors", &show_environmental_sensors);
            
            if (connection.is_connected && current_data.data_valid) {
                ImGui::Text("Humidity: %.1f%%", current_data.humidity);
                ImGui::Text("Pressure: %.1f hPa", current_data.pressure);
                ImGui::Text("Light Level: %.1f%%", current_data.light);
                
                if (!humidity_data.empty() && ImPlot::BeginPlot("Environmental Data")) {
                    ImPlot::PlotLine("Humidity (%)", time_data.data(), humidity_data.data(), humidity_data.size());
                    ImPlot::PlotLine("Light Level (%)", time_data.data(), light_data.data(), light_data.size());
                    ImPlot::EndPlot();
                }
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No environmental data available");
                ImGui::Text("Humidity: -- %%");
                ImGui::Text("Pressure: -- hPa");
                ImGui::Text("Light Level: -- %%");
            }
            
            ImGui::End();
        }

        // 5. Motion/Proximity Window
        if (show_motion_proximity) {
            ImGui::Begin("Motion/Proximity Sensors", &show_motion_proximity);
            
            ImGui::Text("Motion Detection:");
            if (connection.is_connected && current_data.data_valid) {
                if (current_data.motion_detected) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), " MOTION DETECTED");
                } else {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), " No Motion");
                }
                
                ImGui::Separator();
                ImGui::Text("PIR Sensor: %s", current_data.motion_detected ? "Triggered" : "Idle");
            } else {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), " No Data");
                
                ImGui::Separator();
                ImGui::Text("PIR Sensor: No connection");
            }
            
            ImGui::Text("Ultrasonic Distance: %s", (connection.is_connected && current_data.data_valid) ? "-- cm" : "No connection");
            
            ImGui::End();
        }

        // 6. Data Logging Window
        if (show_data_logging) {
            ImGui::Begin("Data Logging", &show_data_logging);
            
            ImGui::Text("Data Collection Status");
            ImGui::Separator();
            
            if (connection.is_connected) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), " Data collection active");
                ImGui::Text("Total Data Points: %zu", time_data.size());
                ImGui::Text("Collection Rate: Real-time");
                
                if (ImGui::Button("Export Data")) {
                    std::cout << "Exporting sensor data..." << std::endl;
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear Data")) {
                    time_data.clear();
                    temp_data.clear();
                    humidity_data.clear();
                    pressure_data.clear();
                    light_data.clear();
                }
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Data collection stopped");
                ImGui::Text("Total Data Points: %zu (cached)", time_data.size());
                ImGui::Text("Collection Rate: Waiting for connection");
                
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Connect to Raspberry Pi to resume data collection");
            }
            
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