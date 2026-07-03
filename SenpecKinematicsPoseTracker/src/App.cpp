#include "App.h"

#pragma execution_character_set("utf-8")

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "core/OperationLogger.h"


#include <algorithm>
#include <chrono>
#include <thread>

namespace senpec
{
    static void LoadSimHeiFont(ImGuiIO& io)
    {
        const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesChineseFull();
        if (io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\simhei.ttf", 18.0f, nullptr, glyphRanges))
        {
            return;
        }

        io.Fonts->AddFontDefault();
    }

    App::App()
        : window_(nullptr)
        , uiLayer_(device_, store_, solver_, publisher_)
    {
    }

    App::~App()
    {
        Shutdown();
    }

    bool App::InitWindow()
    {
        if (!glfwInit())
        {
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window_ = glfwCreateWindow(1280, 720, u8"压力容器对接测量和计算软件", nullptr, nullptr);
        if (!window_)
        {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        LoadSimHeiFont(io);

        const ImGuiStyle& style = ImGui::GetStyle();
        const ImVec4 defaultPaneColor = style.Colors[ImGuiCol_ChildBg];
        const ImVec4 defaultWidgetColor = style.Colors[ImGuiCol_Button];
        const ImVec4 defaultTextColor = style.Colors[ImGuiCol_Text];
        uiLayer_.SetThemeDefaults(
            {defaultPaneColor.x, defaultPaneColor.y, defaultPaneColor.z, defaultPaneColor.w},
            {defaultWidgetColor.x, defaultWidgetColor.y, defaultWidgetColor.z, defaultWidgetColor.w},
            {defaultTextColor.x, defaultTextColor.y, defaultTextColor.z, defaultTextColor.w});

        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        OperationLogger::Instance().Log("应用窗口初始化完成。");
        return true;
    }

    void App::Shutdown()
    {
        if (window_)
        {
            OperationLogger::Instance().Log("应用正在退出。");
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            glfwDestroyWindow(window_);
            window_ = nullptr;
            glfwTerminate();
        }
    }

    int App::Run()
    {
        if (!InitWindow())
        {
            return -1;
        }

        while (!glfwWindowShouldClose(window_))
        {
            const auto frameStart = std::chrono::steady_clock::now();
            glfwPollEvents();

            ImGuiIO& io = ImGui::GetIO();
            io.FontGlobalScale = uiLayer_.GetUiScale();

            ImGui::StyleColorsDark();

            if (uiLayer_.HasCustomWidgetColor())
            {
                ImGuiStyle& style = ImGui::GetStyle();
                const auto& widgetColor = uiLayer_.GetWidgetColor();
                style.Colors[ImGuiCol_FrameBg] = ImVec4(widgetColor[0] * 0.35f, widgetColor[1] * 0.35f, widgetColor[2] * 0.35f, 0.85f);
                style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(widgetColor[0] * 0.55f, widgetColor[1] * 0.55f, widgetColor[2] * 0.55f, 0.95f);
                style.Colors[ImGuiCol_FrameBgActive] = ImVec4(widgetColor[0] * 0.75f, widgetColor[1] * 0.75f, widgetColor[2] * 0.75f, 1.0f);
                style.Colors[ImGuiCol_Button] = ImVec4(widgetColor[0] * 0.45f, widgetColor[1] * 0.45f, widgetColor[2] * 0.45f, 0.90f);
                style.Colors[ImGuiCol_ButtonHovered] = ImVec4(widgetColor[0] * 0.65f, widgetColor[1] * 0.65f, widgetColor[2] * 0.65f, 1.0f);
                style.Colors[ImGuiCol_ButtonActive] = ImVec4(widgetColor[0] * 0.85f, widgetColor[1] * 0.85f, widgetColor[2] * 0.85f, 1.0f);
                style.Colors[ImGuiCol_Header] = ImVec4(widgetColor[0] * 0.40f, widgetColor[1] * 0.40f, widgetColor[2] * 0.40f, 0.80f);
                style.Colors[ImGuiCol_HeaderHovered] = ImVec4(widgetColor[0] * 0.60f, widgetColor[1] * 0.60f, widgetColor[2] * 0.60f, 0.90f);
                style.Colors[ImGuiCol_HeaderActive] = ImVec4(widgetColor[0] * 0.80f, widgetColor[1] * 0.80f, widgetColor[2] * 0.80f, 1.0f);
                style.Colors[ImGuiCol_CheckMark] = ImVec4(widgetColor[0], widgetColor[1], widgetColor[2], 1.0f);
                style.Colors[ImGuiCol_SliderGrab] = ImVec4(widgetColor[0], widgetColor[1], widgetColor[2], 0.85f);
                style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(widgetColor[0], widgetColor[1], widgetColor[2], 1.0f);
            }

            if (uiLayer_.HasCustomTextColor())
            {
                ImGuiStyle& style = ImGui::GetStyle();
                const auto& textColor = uiLayer_.GetTextColor();
                style.Colors[ImGuiCol_Text] = ImVec4(textColor[0], textColor[1], textColor[2], textColor[3]);
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            uiLayer_.Render();

            ImGui::Render();
            int displayWidth = 0;
            int displayHeight = 0;
            glfwGetFramebufferSize(window_, &displayWidth, &displayHeight);
            glViewport(0, 0, displayWidth, displayHeight);
            const auto& bg = uiLayer_.GetBackgroundColor();
            glClearColor(bg[0], bg[1], bg[2], bg[3]);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window_);

            const float targetFps = std::max(uiLayer_.GetTargetFps(), 1.0f);
            const auto targetFrameDuration = std::chrono::duration<double>(1.0 / targetFps);
            const auto elapsed = std::chrono::steady_clock::now() - frameStart;
            if (elapsed < targetFrameDuration)
            {
                std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(targetFrameDuration - elapsed));
            }
        }

        Shutdown();
        return 0;
    }
}

