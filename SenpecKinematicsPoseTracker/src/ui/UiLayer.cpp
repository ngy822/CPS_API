#include "ui/UiLayer.h"
#include "LogoImage.h" 
#include "LogoImage1.h" // 【新增】引入软件说明图片

#pragma execution_character_set("utf-8")

#include <chrono>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "core/OperationLogger.h"

#include <GLFW/glfw3.h> 

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace senpec
{
    static std::string FormatTime(const std::chrono::system_clock::time_point& timePoint)
    {
        const std::time_t timeValue = std::chrono::system_clock::to_time_t(timePoint);
        std::tm tmValue{};
#ifdef _WIN32
        localtime_s(&tmValue, &timeValue);
#else
        localtime_r(&timeValue, &tmValue);
#endif
        std::ostringstream stream;
        stream << std::put_time(&tmValue, "%Y-%m-%d %H:%M:%S");
        return stream.str();
    }

    static const char* MotionModeToText(MotionMode mode)
    {
        switch (mode)
        {
        case MotionMode::Idle: return u8"Idle 模式";
        case MotionMode::Position: return u8"Position 模式";
        case MotionMode::Track: return u8"Track 模式";
        default: return u8"未知模式";
        }
    }

    static const char* HardwareOperationModeToText(api::lt::OperationMode mode)
    {
        switch (mode)
        {
        case api::lt::OperationMode::Idle: return u8"Idle";
        case api::lt::OperationMode::Tracking: return u8"Tracking";
        case api::lt::OperationMode::Position: return u8"Position";
        case api::lt::OperationMode::TrackIdle: return u8"TrackIdle";
        case api::lt::OperationMode::Searching: return u8"Searching";
        case api::lt::OperationMode::Internal: return u8"Internal";
        default: return u8"Unknown";
        }
    }

    UiLayer::UiLayer(TrackerDevice& device, MeasurementStore& store, AlignmentSolver& solver, DigitalTwinPublisher& publisher)
        : device_(device)
        , store_(store)
        , solver_(solver)
        , publisher_(publisher)
        , exportPath_("measurements.csv")
        , targetSizeIndex_(0)
        , uiScale_(1.0f)
        , targetFps_(60.0f)
        , backgroundColor_{ 0.10f, 0.12f, 0.15f, 1.0f }
        , defaultBackgroundColor_{ 0.10f, 0.12f, 0.15f, 1.0f }
        , widgetColor_{ 0.26f, 0.59f, 0.98f, 1.0f }
        , defaultWidgetColor_{ 0.26f, 0.59f, 0.98f, 1.0f }
        , textColor_{ 1.0f, 1.0f, 1.0f, 1.0f }
        , defaultTextColor_{ 1.0f, 1.0f, 1.0f, 1.0f }
        , defaultUiScale_(1.0f)
        , defaultTargetFps_(60.0f)
        , useCustomBackgroundColor_(false)
        , useCustomWidgetColor_(false)
        , useCustomTextColor_(false)
        , enableCameraSearch_(false)
        , showDisplaySettings_(false)
        , showAboutWindow_(false)
        , showUserManualWindow_(false)
        , requestMeasure_(false)
        , kinematicsCsvPath_("measurements.csv")
        , simulatedKinematicsResult_{}
        , hasSimulatedKinematicsResult_(false)
        , leftPaneRatio_(0.66f)
        , topPaneRatio_(0.75f)
        , runtimeLog_(u8"系统已启动，等待测量。\n")
        , autoScrollLog_(true)
        , scrollToBottom_(true)
        , isSimulationMode_(false)
        , simulatedConnected_(false)
        , isDetectingIp_(false)
        , detectCompleted_(false)
        , detectSuccess_(false)
        , currentRoundIndex_(1)
        , lastSavedPointCount_(0)
        , reportExportPath_("DockingReport.csv")
    {
        std::filesystem::path configPath = std::filesystem::current_path() / "config.json";
        std::ifstream configFile(configPath);
        ipAddress_ = "192.168.0.10";

        if (configFile.is_open())
        {
            std::stringstream buffer;
            buffer << configFile.rdbuf();
            std::string content = buffer.str();

            size_t ipKeyPos = content.find("\"ip\"");
            if (ipKeyPos != std::string::npos)
            {
                size_t colonPos = content.find(":", ipKeyPos);
                if (colonPos != std::string::npos)
                {
                    size_t startQuote = content.find("\"", colonPos);
                    if (startQuote != std::string::npos)
                    {
                        size_t endQuote = content.find("\"", startQuote + 1);
                        if (endQuote != std::string::npos)
                        {
                            ipAddress_ = content.substr(startQuote + 1, endQuote - startQuote - 1);
                        }
                    }
                }
            }
        }
        else
        {
            std::ofstream newConfig(configPath);
            if (newConfig.is_open())
            {
                newConfig << "{\n  \"ip\": \"192.168.0.10\"\n}\n";
            }
        }
    }

    UiLayer::~UiLayer()
    {
        std::lock_guard<std::mutex> lock(detectMutex_);
        if (detectThread_.joinable())
        {
            detectThread_.join();
        }

        if (logoTextureId_ != 0)
        {
            glDeleteTextures(1, &logoTextureId_);
        }

        // 【新增】释放说明图片的纹理
        if (manualTextureId_ != 0)
        {
            glDeleteTextures(1, &manualTextureId_);
        }
    }

    float UiLayer::GetUiScale() const { return uiScale_; }
    float UiLayer::GetTargetFps() const { return targetFps_; }
    const std::array<float, 4>& UiLayer::GetBackgroundColor() const { return backgroundColor_; }
    const std::array<float, 4>& UiLayer::GetWidgetColor() const { return widgetColor_; }
    const std::array<float, 4>& UiLayer::GetTextColor() const { return textColor_; }
    const std::string& UiLayer::GetLogFilePath() const { return OperationLogger::Instance().GetLogFilePath(); }
    bool UiLayer::OpenLogFile() const { return OperationLogger::Instance().OpenLogFile(); }
    bool UiLayer::HasCustomWidgetColor() const { return useCustomWidgetColor_; }
    bool UiLayer::HasCustomTextColor() const { return useCustomTextColor_; }

    void UiLayer::SetThemeDefaults(const std::array<float, 4>& backgroundColor, const std::array<float, 4>& widgetColor, const std::array<float, 4>& textColor)
    {
        defaultBackgroundColor_ = backgroundColor;
        defaultWidgetColor_ = widgetColor;
        defaultTextColor_ = textColor;
        backgroundColor_ = backgroundColor;
        widgetColor_ = widgetColor;
        textColor_ = textColor;
        useCustomBackgroundColor_ = false;
        useCustomWidgetColor_ = false;
        useCustomTextColor_ = false;
    }

    void UiLayer::RestoreDefaultDisplaySettings()
    {
        uiScale_ = defaultUiScale_;
        targetFps_ = defaultTargetFps_;
        backgroundColor_ = defaultBackgroundColor_;
        widgetColor_ = defaultWidgetColor_;
        textColor_ = defaultTextColor_;
        useCustomBackgroundColor_ = false;
        useCustomWidgetColor_ = false;
        useCustomTextColor_ = false;
        OperationLogger::Instance().Log("显示设置已恢复默认。 ");
        AppendRuntimeLog(u8"显示设置已恢复默认。");
    }

    void UiLayer::Render()
    {
        RenderMainMenu();
        RenderMainWorkspace();
        RenderDisplaySettingsWindow();
        RenderAboutWindow();
        RenderUserManualWindow();

        if (!isSimulationMode_ && device_.measureCompleted_)
        {
            device_.measureCompleted_ = false;
            if (device_.measureSuccess_)
            {
                requestMeasure_ = true;
            }
            else
            {
                AppendRuntimeLog(u8"硬件测量失败，获取数据超时或异常。");
                OperationLogger::Instance().Log(LogLevel::Error, u8"硬件测量失败，获取数据超时或异常。");
            }
        }

        if (requestMeasure_)
        {
            HandleMeasurement();
            requestMeasure_ = false;
        }
    }

    void UiLayer::OpenSystemFile(const std::string& pathStr, const std::string& fileDesc)
    {
        std::filesystem::path p = std::filesystem::u8path(pathStr);
        if (!std::filesystem::exists(p))
        {
            std::string msg = u8"错误：找不到" + fileDesc + u8" (" + pathStr + u8")。请确认文件是否存在！";
            AppendRuntimeLog(msg);
            OperationLogger::Instance().Log(LogLevel::Error, "尝试打开文件失败，文件不存在: " + pathStr);
            return;
        }

#ifdef _WIN32
        std::wstring wpath = p.wstring();
        HINSTANCE result = ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(result) <= 32)
        {
            AppendRuntimeLog(u8"错误：无法打开" + fileDesc + u8"，可能没有关联的默认程序或权限不足。");
            OperationLogger::Instance().Log(LogLevel::Error, "ShellExecuteW 失败: " + pathStr);
        }
        else
        {
            AppendRuntimeLog(u8"已调用系统默认程序打开" + fileDesc + u8"：" + pathStr);
            OperationLogger::Instance().Log(LogLevel::Info, "成功在外部打开文件: " + pathStr);
        }
#else
        AppendRuntimeLog(u8"错误：当前操作系统不支持该外部打开调用。");
#endif
    }

    void UiLayer::RenderMainWorkspace()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 workPos = viewport->WorkPos;
        const ImVec2 workSize = viewport->WorkSize;
        const ImVec4 defaultPaneColor = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
        const ImVec4 paneColor = useCustomBackgroundColor_
            ? ImVec4(backgroundColor_[0], backgroundColor_[1], backgroundColor_[2], backgroundColor_[3])
            : defaultPaneColor;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, paneColor);
        ImGui::SetNextWindowPos(workPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(workSize, ImGuiCond_Always);

        ImGui::Begin(u8"压力容器对接测量和计算软件", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);

        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float splitterThickness = 6.0f;
        const float minLeftWidth = avail.x * 0.30f;
        const float minRightWidth = avail.x * 0.20f;
        const float minTopHeight = avail.y * 0.45f;
        const float minBottomHeight = avail.y * 0.15f;

        float leftWidth = std::clamp(avail.x * leftPaneRatio_, minLeftWidth, avail.x - minRightWidth - splitterThickness);
        float topHeight = std::clamp(avail.y * topPaneRatio_, minTopHeight, avail.y - minBottomHeight - splitterThickness);
        float rightWidth = avail.x - leftWidth - splitterThickness;
        float bottomHeight = avail.y - topHeight - splitterThickness;

        auto RenderLargeHeader = [](const char* title) {
            ImGui::SetWindowFontScale(1.15f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.75f, 1.0f, 1.0f));
            ImGui::TextUnformatted(title);
            ImGui::PopStyleColor();
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Separator();
            ImGui::Spacing();
            };

        ImGui::SetCursorScreenPos(origin);
        ImGui::BeginChild("##TopLeftPane", ImVec2(leftWidth, topHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
        RenderLargeHeader(u8"跟踪仪操作和测量");
        RenderConnectionPanel();
        RenderTargetPanel();
        RenderControlPanel();
        RenderTablePanel();
        ImGui::EndChild();

        ImGui::SetCursorScreenPos(ImVec2(origin.x + leftWidth, origin.y));
        const ImRect verticalSplitRect(ImVec2(origin.x + leftWidth, origin.y), ImVec2(origin.x + leftWidth + splitterThickness, origin.y + topHeight));
        ImGui::SplitterBehavior(verticalSplitRect, ImGui::GetID("##VerticalSplitter"), ImGuiAxis_X, &leftWidth, &rightWidth, minLeftWidth, minRightWidth, 0.0f, 0.0f, 0);
        ImGui::GetWindowDrawList()->AddRectFilled(verticalSplitRect.Min, verticalSplitRect.Max, IM_COL32(120, 120, 120, 255));

        ImGui::SetCursorScreenPos(ImVec2(origin.x + leftWidth + splitterThickness, origin.y));
        ImGui::BeginChild("##TopRightPane", ImVec2(rightWidth, topHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
        RenderLargeHeader(u8"偏差和运动学计算");
        RenderKinematicsWindow();
        ImGui::EndChild();

        ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + topHeight));
        const ImRect horizontalSplitRect(ImVec2(origin.x, origin.y + topHeight), ImVec2(origin.x + avail.x, origin.y + topHeight + splitterThickness));
        ImGui::SplitterBehavior(horizontalSplitRect, ImGui::GetID("##HorizontalSplitter"), ImGuiAxis_Y, &topHeight, &bottomHeight, minTopHeight, minBottomHeight, 0.0f, 0.0f, 0);
        ImGui::GetWindowDrawList()->AddRectFilled(horizontalSplitRect.Min, horizontalSplitRect.Max, IM_COL32(120, 120, 120, 255));

        ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + topHeight + splitterThickness));
        ImGui::BeginChild("##BottomPane", ImVec2(avail.x, bottomHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
        RenderLargeHeader(u8"日志打印");
        RenderRuntimeLogWindow();
        ImGui::EndChild();

        leftPaneRatio_ = leftWidth / avail.x;
        topPaneRatio_ = topHeight / avail.y;

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void UiLayer::RenderSplitterWindows(float& leftWidth, float& topHeight)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 workPos = viewport->WorkPos;
        const ImVec2 workSize = viewport->WorkSize;
        const float splitterThickness = 6.0f;
        const float minLeftWidth = workSize.x * 0.30f;
        const float maxLeftWidth = workSize.x * 0.80f;
        const float minTopHeight = workSize.y * 0.45f;
        const float maxTopHeight = workSize.y * 0.85f;

        const float verticalX = workPos.x + leftWidth - splitterThickness * 0.5f;
        const float horizontalY = workPos.y + topHeight - splitterThickness * 0.5f;

        ImGui::SetNextWindowPos(ImVec2(verticalX, workPos.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(splitterThickness, topHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("##VerticalSplitter", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);
        ImGui::InvisibleButton("##VerticalSplitterButton", ImVec2(splitterThickness, topHeight));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive())
        {
            leftWidth += ImGui::GetIO().MouseDelta.x;
            leftWidth = std::clamp(leftWidth, minLeftWidth, maxLeftWidth);
            leftPaneRatio_ = leftWidth / workSize.x;
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

        ImGui::SetNextWindowPos(ImVec2(workPos.x, horizontalY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(workSize.x, splitterThickness), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("##HorizontalSplitter", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);
        ImGui::InvisibleButton("##HorizontalSplitterButton", ImVec2(workSize.x, splitterThickness));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        if (ImGui::IsItemActive())
        {
            topHeight += ImGui::GetIO().MouseDelta.y;
            topHeight = std::clamp(topHeight, minTopHeight, maxTopHeight);
            topPaneRatio_ = topHeight / workSize.y;
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void UiLayer::RenderMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu(u8"打开"))
            {
                if (ImGui::MenuItem(u8"打开测量结果文件 (从保存路径)"))
                {
                    OpenSystemFile(exportPath_, u8"测量结果文件");
                }
                if (ImGui::MenuItem(u8"打开导入的数据文件 (从导入路径)"))
                {
                    OpenSystemFile(kinematicsCsvPath_, u8"导入的数据文件");
                }
                if (ImGui::MenuItem(u8"打开综合报告文件 (从报告路径)"))
                {
                    OpenSystemFile(reportExportPath_, u8"综合报告文件");
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(u8"设置"))
            {
                if (ImGui::MenuItem(u8"显示设置")) showDisplaySettings_ = true;
                if (ImGui::BeginMenu(u8"使用模式"))
                {
                    if (ImGui::MenuItem(u8"实际模式", nullptr, !isSimulationMode_))
                    {
                        isSimulationMode_ = false;
                        AppendRuntimeLog(u8"已切换到【实际模式】。");
                        OperationLogger::Instance().Log(LogLevel::Info, u8"工作模式已切换为: 实际模式");
                    }
                    if (ImGui::MenuItem(u8"模拟模式", nullptr, isSimulationMode_))
                    {
                        isSimulationMode_ = true;
                        AppendRuntimeLog(u8"已切换到【模拟模式】。");
                        OperationLogger::Instance().Log(LogLevel::Info, u8"工作模式已切换为: 模拟模式");
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(u8"日志"))
            {
                if (ImGui::MenuItem(u8"显示日志"))
                {
                    const bool opened = OpenLogFile();
                    AppendRuntimeLog(opened ? u8"已打开日志文件。" : u8"日志文件打开失败。");
                    OperationLogger::Instance().Log(opened ? LogLevel::Info : LogLevel::Error, opened ? "已打开日志文件。" : "日志文件打开失败。");
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(u8"帮助"))
            {
                if (ImGui::MenuItem(u8"关于")) showAboutWindow_ = true;
                if (ImGui::MenuItem(u8"软件使用说明")) showUserManualWindow_ = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void UiLayer::RenderUserManualWindow()
    {
        if (!showUserManualWindow_) return;

        // 【新增】延迟加载说明文档图片
        if (!manualLoadAttempted_)
        {
            manualLoadAttempted_ = true;
            int channels;
            unsigned char* imgData = stbi_load_from_memory(manual_png_data, manual_png_size, &manualWidth_, &manualHeight_, &channels, 4);
            if (imgData)
            {
                glGenTextures(1, &manualTextureId_);
                glBindTexture(GL_TEXTURE_2D, manualTextureId_);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, manualWidth_, manualHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
                stbi_image_free(imgData);

                OperationLogger::Instance().Log(LogLevel::Info, "说明图片加载并生成纹理成功");
            }
            else
            {
                std::string failReason = stbi_failure_reason() ? stbi_failure_reason() : "未知错误";
                OperationLogger::Instance().Log(LogLevel::Error, "说明图片解码失败: " + failReason);
            }
        }

        ImGui::SetNextWindowSize(ImVec2(600 * uiScale_, 650 * uiScale_), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(u8"软件操作流程与使用说明", &showUserManualWindow_))
        {
            ImGui::TextWrapped(u8"欢迎使用【压力容器对接测量和计算软件】。本指南将引导您完成从硬件连接到多轮对接迭代的标准化作业流程。");
            ImGui::Spacing();

            // 【新增】在欢迎语下方居中渲染说明图片
            if (manualTextureId_ != 0)
            {
                float aspect = static_cast<float>(manualWidth_) / static_cast<float>(manualHeight_);
                float windowWidth = ImGui::GetWindowSize().x;

                // 动态计算显示尺寸：占窗口宽度的 90%，两边留白更美观
                float displayWidth = windowWidth * 0.90f;
                float displayHeight = displayWidth / aspect;

                // 居中偏移
                float cursorPosX = (windowWidth - displayWidth) * 0.5f;
                if (cursorPosX > 0.0f)
                {
                    ImGui::SetCursorPosX(cursorPosX);
                }

                ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(manualTextureId_)), ImVec2(displayWidth, displayHeight));
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            if (ImGui::CollapsingHeader(u8"第一步：硬件连接与初始化", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BulletText(u8"确认激光跟踪仪及控制柜已上电，且与工控机处于同一局域网内。");
                ImGui::BulletText(u8"如果是默认网络配置，点击【连接】；否则，请先点击【自动搜索】获取设备IP。");
                ImGui::BulletText(u8"连接成功后，仪表盘中的“光路锁定”状态应亮起，设备电机开始使能。");
            }

            if (ImGui::CollapsingHeader(u8"第二步：靶标捕获与锁定", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BulletText(u8"将靶标球(SMR)放置在压力容器对接面的基准点上。");
                ImGui::BulletText(u8"长按【方位角/俯仰角】点动按钮，观察跟踪仪红光，将其引导至靶标球附近。");
                ImGui::BulletText(u8"点击【螺旋搜索靶标 (自动捕获)】，仪器将自动画圈寻找并锁定靶标。");
                ImGui::BulletText(u8"锁定成功后，仪表盘中的信号强度条将变绿，距离与坐标数据开始实时跳动。");
            }

            if (ImGui::CollapsingHeader(u8"第三步：空间采数与数据固化", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BulletText(u8"确保人员未遮挡光路且环境无剧烈震动，点击【开始测量 (单点)】。");
                ImGui::BulletText(u8"等待约1~2秒，采集到的高精度坐标将自动填入下方的“测量结果”表格中。");
                ImGui::BulletText(u8"移动靶标球至下一个基准点，重复“找球 -> 锁定 -> 测量”动作，直到采集完本轮所需的所有特征点。");
            }

            if (ImGui::CollapsingHeader(u8"第四步：运动学解算与对接评估", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BulletText(u8"本轮所有点位测量完毕后，点击【导出】将数据存为CSV文件，并在右侧【导入路径】确认文件名。");
                ImGui::BulletText(u8"点击右侧面板的【开始计算】。");
                ImGui::BulletText(u8"系统将自动评估当前的【筒体对接质量】（如轴线夹角偏差、错边量等）。");
                ImGui::BulletText(u8"依据下方生成的【运动学计算结果】，指导现场液压系统或顶升机构对底盘车体进行微调。");
            }

            if (ImGui::CollapsingHeader(u8"第五步：多轮迭代与报告导出", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BulletText(u8"微调机构动作完成后，进入下一轮迭代：再次测量靶标球，导出新数据，并再次点击【开始计算】。");
                ImGui::BulletText(u8"系统后台会自动为您按轮次（第1轮、第2轮...）封存所有的计算和评估数据。");
                ImGui::BulletText(u8"当【筒体对接质量评估】中的所有公差满足工艺要求时，对接完成！");
                ImGui::BulletText(u8"最后，点击右下角的【导出综合报告】，系统将生成完整的《多轮对接综合记录报告.csv》用于工程存档。");
            }
        }
        ImGui::End();
    }

    void UiLayer::RenderConnectionPanel()
    {
        ImGui::SeparatorText(u8"硬件连接");

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(u8"IP地址:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##IpAddress", &ipAddress_);

        bool isActuallyConnected = isSimulationMode_ ? simulatedConnected_ : device_.IsConnected();

        bool isSearching = isSimulationMode_ ? false : device_.isSearching_.load();
        ImGui::BeginDisabled(isSearching);
        const char* detectLabel = isSearching ? u8"搜索中(5s)..." : u8"自动搜索";
        if (ImGui::Button(detectLabel))
        {
            if (isSimulationMode_)
            {
                static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
                std::uniform_int_distribution<int> dist(100, 254);
                ipAddress_ = "192.168.0." + std::to_string(dist(rng));
                AppendRuntimeLog(u8"【模拟】自动搜索完成，已生成虚拟 IP。");
            }
            else device_.StartAutoDetect();
        }
        ImGui::EndDisabled();

        if (!isSimulationMode_ && device_.detectCompleted_)
        {
            device_.detectCompleted_ = false;
            if (device_.detectSuccess_)
            {
                ipAddress_ = device_.detectedIp_;
                AppendRuntimeLog(u8"自动搜索完成，已填入 IP。");
            }
            else if (device_.detectedIp_.empty())
            {
                AppendRuntimeLog(u8"当前 SDK 不支持自动搜索，请手动输入 IP 后连接。");
            }
            else
            {
                AppendRuntimeLog(u8"自动搜索失败或超时。");
            }
        }

        ImGui::SameLine();
        bool isConnecting = isSimulationMode_ ? false : device_.isConnecting_.load();
        ImGui::BeginDisabled(isConnecting || isActuallyConnected);
        const char* connectLabel = isConnecting ? u8"连接中..." : u8"连接";
        if (ImGui::Button(connectLabel))
        {
            if (isSimulationMode_)
            {
                simulatedConnected_ = true;
                AppendRuntimeLog(u8"【模拟】硬件连接成功，已虚拟初始化！");
            }
            else
            {
                device_.StartConnectAsync(ipAddress_);
                AppendRuntimeLog(u8"正在后台连接硬件...");
            }
        }
        ImGui::EndDisabled();

        if (!isSimulationMode_ && device_.connectCompleted_)
        {
            device_.connectCompleted_ = false;
            if (device_.connectSuccess_) AppendRuntimeLog(u8"连接成功，硬件已初始化！");
            else AppendRuntimeLog(u8"连接失败，请检查 IP 或网络！");
        }

        ImGui::SameLine();
        bool isDisconnecting = isSimulationMode_ ? false : device_.isDisconnecting_.load();
        ImGui::BeginDisabled(isDisconnecting || !isActuallyConnected);
        const char* disconnectLabel = isDisconnecting ? u8"断开中..." : u8"断开连接";
        if (ImGui::Button(disconnectLabel))
        {
            if (isSimulationMode_)
            {
                simulatedConnected_ = false;
                AppendRuntimeLog(u8"【模拟】已安全断开虚拟硬件。");
            }
            else
            {
                device_.StartDisconnectAsync();
                AppendRuntimeLog(u8"正在后台安全断开硬件...");
            }
        }
        ImGui::EndDisabled();

        if (!isSimulationMode_ && device_.disconnectCompleted_)
        {
            device_.disconnectCompleted_ = false;
            AppendRuntimeLog(u8"断开连接成功。");
        }

        ImGui::SameLine();
        const ImVec4 statusColor = isActuallyConnected ? ImVec4(0.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
        ImGui::ColorButton("##status", statusColor, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(16.0f, 16.0f));
        ImGui::SameLine();
        ImGui::TextUnformatted(isActuallyConnected ? u8"已连接" : u8"未连接");

        // 【新增】Home 按钮及逻辑
        ImGui::SameLine();
        ImGui::BeginDisabled((!isSimulationMode_ && !device_.IsConnected()) || (isSimulationMode_ && !simulatedConnected_));
        if (ImGui::Button("Home"))
        {
            // 根据 targetSizeIndex_ 映射到 api::lt::SmrSize
            api::lt::SmrSize smrSize = api::lt::SmrSize::OneHalfInch;
            switch (targetSizeIndex_)
            {
            case 0:
                smrSize = api::lt::SmrSize::OneHalfInch;
                break;
            case 1:
                smrSize = api::lt::SmrSize::HalfInch;
                break;
            case 2:
                smrSize = api::lt::SmrSize::SevenEighthInch;
                break;
            default:
                smrSize = api::lt::SmrSize::OneHalfInch;
                break;
            }

            if (isSimulationMode_)
            {
                OperationLogger::Instance().Log(LogLevel::Info, u8"【模拟操作】点击了 Home 按钮。");
                AppendRuntimeLog(u8"【模拟】请求执行 Home 归零操作...");
            }
            else
            {
                OperationLogger::Instance().Log(LogLevel::Info, u8"【操作】点击了 Home 按钮。");
                AppendRuntimeLog(u8"正在请求硬件执行 Home 归零操作...");
                device_.Home(smrSize);
            }
        }
        ImGui::EndDisabled();
    }

    void UiLayer::RenderTargetPanel()
    {
        ImGui::SeparatorText(u8"靶标球规格");
        const char* sizes[] = { u8"1.5 英寸", u8"0.5 英寸", u8"7/8 英寸" };

        ImGui::SetNextItemWidth(150.0f * uiScale_);
        if (ImGui::Combo(u8"靶标球", &targetSizeIndex_, sizes, IM_ARRAYSIZE(sizes)))
        {
        }
    }

    void UiLayer::RenderDisplaySettingsWindow()
    {
        if (!showDisplaySettings_) return;
        ImGui::Begin("显示设置", &showDisplaySettings_);
        ImGui::SliderFloat("UI缩放倍数", &uiScale_, 0.5f, 2.0f, "%.2f");
        ImGui::SliderFloat("刷新率(FPS)", &targetFps_, 10.0f, 240.0f, "%.0f");
        if (ImGui::ColorEdit4("背景颜色", backgroundColor_.data())) useCustomBackgroundColor_ = true;
        if (ImGui::ColorEdit4("组件颜色", widgetColor_.data())) useCustomWidgetColor_ = true;
        if (ImGui::ColorEdit4("文字颜色", textColor_.data())) useCustomTextColor_ = true;
        if (ImGui::Button(u8"恢复默认")) RestoreDefaultDisplaySettings();
        ImGui::Text(u8"当前刷新率: %.1f FPS", ImGui::GetIO().Framerate);
        ImGui::End();
    }

    void UiLayer::RenderAboutWindow()
    {
        if (!showAboutWindow_) return;

        if (!logoLoadAttempted_)
        {
            logoLoadAttempted_ = true;
            int channels;
            unsigned char* imgData = stbi_load_from_memory(logo_png_data, logo_png_size, &logoWidth_, &logoHeight_, &channels, 4);
            if (imgData)
            {
                glGenTextures(1, &logoTextureId_);
                glBindTexture(GL_TEXTURE_2D, logoTextureId_);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logoWidth_, logoHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
                stbi_image_free(imgData);

                OperationLogger::Instance().Log(LogLevel::Info, "标志图片加载并生成纹理成功，尺寸: " + std::to_string(logoWidth_) + "x" + std::to_string(logoHeight_));
            }
            else
            {
                std::string failReason = stbi_failure_reason() ? stbi_failure_reason() : "未知错误";
                OperationLogger::Instance().Log(LogLevel::Error, "标志图片解码失败: " + failReason);
                AppendRuntimeLog(u8"内部组件：标志图片加载失败 (" + failReason + u8")，详情请查看日志。");
            }
        }

        ImGui::Begin(u8"关于", &showAboutWindow_, ImGuiWindowFlags_AlwaysAutoResize);

        if (logoTextureId_ != 0)
        {
            float aspect = static_cast<float>(logoWidth_) / static_cast<float>(logoHeight_);
            float displayHeight = 60.0f * uiScale_;
            float displayWidth = displayHeight * aspect;

            float windowWidth = ImGui::GetWindowSize().x;
            float cursorPosX = (windowWidth - displayWidth) * 0.5f;
            if (cursorPosX > 0.0f)
            {
                ImGui::SetCursorPosX(cursorPosX);
            }

            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(logoTextureId_)), ImVec2(displayWidth, displayHeight));
            ImGui::Separator();
        }

        ImGui::Text(u8"软件名称: 压力容器对接测量和计算软件");
        ImGui::Text(u8"软件版本: 1.0.0");
        ImGui::Text(u8"编译时间: %s %s", __DATE__, __TIME__);
        ImGui::Separator();
        ImGui::Text(u8"Copyright (C) 2026 上海交通大学. All right reserved!");
        ImGui::End();
    }

    void UiLayer::RenderControlPanel()
    {
        ImGui::SeparatorText(u8"硬件实时状态");

        auto now = std::chrono::steady_clock::now();
        static auto lastReadTime = now;

        if (!isSimulationMode_)
        {
            if (device_.IsConnected())
            {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastReadTime).count() > 500)
                {
                    cachedStatus_ = device_.GetRealTimeStatus();
                    lastReadTime = now;
                }
            }
            else
            {
                cachedStatus_ = TrackerRealTimeStatus{};
            }
        }
        else
        {
            if (simulatedConnected_)
            {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastReadTime).count() > 100)
                {
                    cachedStatus_.azimuth = simulatedAzimuth_;
                    cachedStatus_.elevation = simulatedElevation_;
                    cachedStatus_.intensity = 0.96;
                    cachedStatus_.isLaserPathError = false;
                    cachedStatus_.distance = 1250.0 + (std::rand() % 10) / 10.0;
                    cachedStatus_.x = 880.2 + (std::rand() % 10) / 10.0;
                    cachedStatus_.y = -350.4 + (std::rand() % 10) / 10.0;
                    cachedStatus_.z = 520.1 + (std::rand() % 10) / 10.0;
                    cachedStatus_.temperature = 24.5;
                    cachedStatus_.pressure = 760.1;
                    cachedStatus_.humidity = 45.2;
                    lastReadTime = now;
                }
            }
            else
            {
                cachedStatus_ = TrackerRealTimeStatus{};
            }
        }

        if (cachedStatus_.isWarmingUp)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), u8"【仪器正在预热中，请等待...】");
        }

        ImGui::TextUnformatted(u8"光路锁定:");
        ImGui::SameLine();
        if (cachedStatus_.isLaserPathError || (!isSimulationMode_ && !device_.IsConnected()) || (isSimulationMode_ && !simulatedConnected_))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), u8"未就绪 / 断开");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), u8"正常追踪");
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(u8"  信号强度:");
        ImGui::SameLine();
        float intensityClamped = std::clamp(static_cast<float>(cachedStatus_.intensity), 0.0f, 1.0f);
        ImGui::ProgressBar(intensityClamped, ImVec2(-FLT_MIN, 0), "");

        if (ImGui::BeginTable("RealTimeStatusTable", 2, ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableNextColumn();
            ImGui::Text(u8"  X: %.3f", cachedStatus_.x);
            ImGui::Text(u8"  Y: %.3f", cachedStatus_.y);
            ImGui::Text(u8"  Z: %.3f", cachedStatus_.z);
            ImGui::Text(u8"距离: %.3f mm", cachedStatus_.distance);

            ImGui::TableNextColumn();
            ImGui::Text(u8"方位角: %.4f°", cachedStatus_.azimuth);
            ImGui::Text(u8"俯仰角: %.4f°", cachedStatus_.elevation);
            ImGui::Text(u8"温 度: %.1f °C", cachedStatus_.temperature);
            ImGui::Text(u8"气 压: %.1f mmHg", cachedStatus_.pressure);
            ImGui::Text(u8"硬件模式: %s", HardwareOperationModeToText(cachedStatus_.operationMode));

            ImGui::EndTable();
        }

        ImGui::SeparatorText(u8"运动控制 (按住连续运动，松开停止)");

        const char* motionModeLabels[] = { u8"Idle 模式", u8"Position 模式", u8"Track 模式" };
        const char* currentMotionModeLabel = motionModeLabels[static_cast<int>(currentMotionMode_)];
        const bool canApplyMotionMode = isSimulationMode_ ? simulatedConnected_ : device_.IsConnected();

        auto applyMode = [&](MotionMode newMode)
        {
            const char* targetText = MotionModeToText(newMode);
            OperationLogger::Instance().Log(LogLevel::Info, std::string(u8"请求设定电机模式为: ") + targetText);

            if (currentMotionMode_ == newMode)
            {
                AppendRuntimeLog(std::string(u8"电机模式已选择为 ") + targetText + u8"。点击设定按钮将尝试切换。") ;
                OperationLogger::Instance().Log(LogLevel::Info, std::string(u8"电机模式选择保持为: ") + targetText);
                return;
            }

            currentMotionMode_ = newMode;
            AppendRuntimeLog(std::string(u8"已选择电机模式: ") + targetText);
            OperationLogger::Instance().Log(LogLevel::Info, std::string(u8"电机模式已选择: ") + targetText);
        };

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(u8"电机模式:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(180.0f * uiScale_);
        if (ImGui::BeginCombo("##MotionModeCombo", currentMotionModeLabel))
        {
            if (ImGui::Selectable(motionModeLabels[0], currentMotionMode_ == MotionMode::Idle))
            {
                applyMode(MotionMode::Idle);
            }
            if (ImGui::Selectable(motionModeLabels[1], currentMotionMode_ == MotionMode::Position))
            {
                applyMode(MotionMode::Position);
            }
            if (ImGui::Selectable(motionModeLabels[2], currentMotionMode_ == MotionMode::Track))
            {
                applyMode(MotionMode::Track);
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();
        if (!canApplyMotionMode)
        {
            ImGui::BeginDisabled(true);
        }
        if (ImGui::Button(u8"设定模式"))
        {
            const char* targetText = MotionModeToText(currentMotionMode_);
            if (isSimulationMode_)
            {
                AppendRuntimeLog(std::string(u8"【模拟】已设定电机模式为 ") + targetText + u8"。") ;
                OperationLogger::Instance().Log(LogLevel::Info, std::string(u8"【模拟】设定电机模式: ") + targetText);
            }
            else
            {
                api::lt::ErrorType ret = api::lt::ErrorType::Success;
                switch (currentMotionMode_)
                {
                case MotionMode::Idle:
                    ret = device_.SwitchToIdle();
                    break;
                case MotionMode::Position:
                    ret = device_.SwitchToPosition();
                    break;
                case MotionMode::Track:
                    ret = device_.SwitchToTrack();
                    break;
                }

                if (ret == api::lt::ErrorType::Success)
                {
                    AppendRuntimeLog(std::string(u8"已设定电机模式为 ") + targetText + u8"。") ;
                    OperationLogger::Instance().Log(LogLevel::Info, std::string(u8"电机模式设定成功: ") + targetText);
                }
                else
                {
                    AppendRuntimeLog(std::string(u8"设定电机模式失败: ") + targetText);
                    OperationLogger::Instance().Log(LogLevel::Error, std::string(u8"电机模式设定失败: ") + targetText + u8"，错误码=" + std::to_string(static_cast<unsigned int>(ret)));
                }
            }
        }
        if (!canApplyMotionMode)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(canApplyMotionMode ? u8"" : u8"(未连接时不可设定)");

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(u8"使能相机搜索:");
        ImGui::SameLine();
        ImGui::BeginDisabled(!canApplyMotionMode);
        bool cameraSearchChanged = ImGui::Checkbox("##EnableCameraSearch", &enableCameraSearch_);
        ImGui::EndDisabled();
        if (cameraSearchChanged)
        {
            if (isSimulationMode_)
            {
                AppendRuntimeLog(enableCameraSearch_ ? u8"【模拟】已勾选使能相机搜索。" : u8"【模拟】已取消使能相机搜索。");
                OperationLogger::Instance().Log(LogLevel::Info, enableCameraSearch_ ? u8"【模拟】EnableCameraSearch(true)" : u8"【模拟】EnableCameraSearch(false)");
            }
            else
            {
                const api::lt::ErrorType ret = device_.EnableCameraSearch(enableCameraSearch_);
                if (ret == api::lt::ErrorType::Success)
                {
                    AppendRuntimeLog(enableCameraSearch_ ? u8"已使能相机搜索。" : u8"已关闭相机搜索。");
                    OperationLogger::Instance().Log(LogLevel::Info, enableCameraSearch_ ? u8"EnableCameraSearch(true) 成功" : u8"EnableCameraSearch(false) 成功");
                }
                else
                {
                    enableCameraSearch_ = !enableCameraSearch_;
                    AppendRuntimeLog(enableCameraSearch_ ? u8"使能相机搜索失败，已恢复为开启前状态。" : u8"关闭相机搜索失败，已恢复为开启前状态。");
                    OperationLogger::Instance().Log(LogLevel::Error, std::string(u8"EnableCameraSearch 切换失败，错误码=") + std::to_string(static_cast<unsigned int>(ret)));
                }
            }
        }

        bool isSearchingTarget = isSimulationMode_ ? false : device_.isSearchingTarget_.load();

        ImGui::BeginDisabled(isSearchingTarget || (!isSimulationMode_ && !device_.IsConnected()) || (isSimulationMode_ && !simulatedConnected_));

        ImGui::PushID("JogButtons");

        ImGui::Button(u8"方位角(正向)");
        const bool azimuthPositiveActivated = ImGui::IsItemActivated();
        const bool azimuthPositiveActive = ImGui::IsItemActive();
        const bool azimuthPositiveDeactivated = ImGui::IsItemDeactivated();
        ImGui::SameLine();
        ImGui::Button(u8"方位角(反向)");
        const bool azimuthNegativeActivated = ImGui::IsItemActivated();
        const bool azimuthNegativeActive = ImGui::IsItemActive();
        const bool azimuthNegativeDeactivated = ImGui::IsItemDeactivated();

        ImGui::Button(u8"俯仰角(正向)");
        const bool elevationPositiveActivated = ImGui::IsItemActivated();
        const bool elevationPositiveActive = ImGui::IsItemActive();
        const bool elevationPositiveDeactivated = ImGui::IsItemDeactivated();
        ImGui::SameLine();
        ImGui::Button(u8"俯仰角(反向)");
        const bool elevationNegativeActivated = ImGui::IsItemActivated();
        const bool elevationNegativeActive = ImGui::IsItemActive();
        const bool elevationNegativeDeactivated = ImGui::IsItemDeactivated();

        ImGui::PopID();
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::BeginDisabled((!isSimulationMode_ && !device_.IsConnected()) || (isSimulationMode_ && !simulatedConnected_));

        if (isSearchingTarget)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

            if (ImGui::Button(u8"停止搜索 (急停)", ImVec2(-FLT_MIN, 28.0f * uiScale_)))
            {
                if (!isSimulationMode_)
                {
                    device_.StopMotion();
                    OperationLogger::Instance().Log(LogLevel::Info, u8"【操作】点击了停止搜索(急停)。");
                    AppendRuntimeLog(u8"请求终止靶标搜索...");
                }
            }
            ImGui::PopStyleColor(3);
        }
        else
        {
            if (ImGui::Button(u8"螺旋搜索靶标 (自动捕获)", ImVec2(-FLT_MIN, 28.0f * uiScale_)))
            {
                if (isSimulationMode_)
                {
                    OperationLogger::Instance().Log(LogLevel::Info, u8"【模拟操作】点击了搜索靶标。");
                    AppendRuntimeLog(u8"【模拟】正在搜索靶标... (已虚拟锁定)");
                    cachedStatus_.isLaserPathError = false;
                }
                else
                {
                    device_.StartSearchTargetAsync();
                    OperationLogger::Instance().Log(LogLevel::Info, u8"【操作】点击了螺旋搜索靶标。");
                    AppendRuntimeLog(u8"请求后台执行靶标螺旋搜索...");
                }
            }
        }
        ImGui::EndDisabled();

        if (!isSimulationMode_)
        {
            const double LARGE_JOG_DEGREE = 180;

            if (azimuthPositiveActivated) {
                device_.MoveAzimuth(LARGE_JOG_DEGREE);
                OperationLogger::Instance().Log(LogLevel::Info, u8"【操作】按下：方位角正向点动");
                AppendRuntimeLog(u8"开始正向方位角转动...");
            }
            else if (azimuthNegativeActivated) {
                device_.MoveAzimuth(-LARGE_JOG_DEGREE);
                OperationLogger::Instance().Log(LogLevel::Info, u8"【操作】按下：方位角反向点动");
                AppendRuntimeLog(u8"开始反向方位角转动...");
            }
            else if (elevationPositiveActivated) {
                device_.MoveElevation(LARGE_JOG_DEGREE);
                OperationLogger::Instance().Log(LogLevel::Info, u8"【操作】按下：俯仰角正向点动");
                AppendRuntimeLog(u8"开始正向俯仰角转动...");
            }
            else if (elevationNegativeActivated) {
                device_.MoveElevation(-LARGE_JOG_DEGREE);
                OperationLogger::Instance().Log(LogLevel::Info, u8"【操作】按下：俯仰角反向点动");
                AppendRuntimeLog(u8"开始反向俯仰角转动...");
            }

            if (azimuthPositiveDeactivated || azimuthNegativeDeactivated ||
                elevationPositiveDeactivated || elevationNegativeDeactivated)
            {
                device_.StopMotionByModeChange();
                OperationLogger::Instance().Log(LogLevel::Info, u8"【操作】松开：通过模式切换停止运动");
                AppendRuntimeLog(u8"运动已停止 (松开按钮，模式切换)。");
            }
        }
        else if (simulatedConnected_)
        {
            if (azimuthPositiveActivated) { AppendRuntimeLog(u8"【模拟】开始正向方位角转动..."); OperationLogger::Instance().Log(LogLevel::Info, u8"【模拟操作】按下：方位角正向点动"); }
            if (azimuthNegativeActivated) { AppendRuntimeLog(u8"【模拟】开始反向方位角转动..."); OperationLogger::Instance().Log(LogLevel::Info, u8"【模拟操作】按下：方位角反向点动"); }
            if (elevationPositiveActivated) { AppendRuntimeLog(u8"【模拟】开始正向俯仰角转动..."); OperationLogger::Instance().Log(LogLevel::Info, u8"【模拟操作】按下：俯仰角正向点动"); }
            if (elevationNegativeActivated) { AppendRuntimeLog(u8"【模拟】开始反向俯仰角转动..."); OperationLogger::Instance().Log(LogLevel::Info, u8"【模拟操作】按下：俯仰角反向点动"); }

            if (azimuthPositiveActive) simulatedAzimuth_ += 0.2;
            if (azimuthNegativeActive) simulatedAzimuth_ -= 0.2;
            if (elevationPositiveActive) simulatedElevation_ += 0.2;
            if (elevationNegativeActive) simulatedElevation_ -= 0.2;

            if (azimuthPositiveDeactivated || azimuthNegativeDeactivated ||
                elevationPositiveDeactivated || elevationNegativeDeactivated)
            {
                AppendRuntimeLog(u8"【模拟】运动已停止 (松开按钮)。");
                OperationLogger::Instance().Log(LogLevel::Info, u8"【模拟操作】松开：停止运动");
            }
        }

        bool isMeasuring = isSimulationMode_ ? false : device_.isMeasuring_.load();

        ImGui::BeginDisabled(isMeasuring || isSearchingTarget || (!isSimulationMode_ && !device_.IsConnected()) || (isSimulationMode_ && !simulatedConnected_));
        const char* measureLabel = isMeasuring ? u8"测量中..." : u8"开始测量 (单点)";
        if (ImGui::Button(measureLabel, ImVec2(-FLT_MIN, 35.0f * uiScale_)))
        {
            if (isSimulationMode_)
            {
                requestMeasure_ = true;
                OperationLogger::Instance().Log(LogLevel::Info, u8"点击开始测量按钮 (模拟模式)。");
                AppendRuntimeLog(u8"【模拟】请求开始测量。");
            }
            else
            {
                device_.StartMeasureAsync();
                OperationLogger::Instance().Log(LogLevel::Info, u8"点击开始测量按钮 (实际模式)。");
                AppendRuntimeLog(u8"正在后台请求硬件测量...");
            }
        }
        ImGui::EndDisabled();
    }

    void UiLayer::RenderTablePanel()
    {
        ImGui::SeparatorText(u8"测量结果");

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(u8"保存路径:");
        ImGui::SameLine();

        float btnExportWidth = ImGui::CalcTextSize(u8"导出").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float btnClearWidth = ImGui::CalcTextSize(u8"清空").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float btnCopyWidth = ImGui::CalcTextSize(u8"一键复制").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float spacingX = ImGui::GetStyle().ItemSpacing.x;
        float totalRightWidth = btnExportWidth + btnClearWidth + btnCopyWidth + spacingX * 3.0f;

        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(80.0f, availWidth - totalRightWidth));
        ImGui::InputText("##ExportPath", &exportPath_);

        ImGui::SameLine();
        if (ImGui::Button(u8"导出"))
        {
            const bool exported = store_.ExportCsv(exportPath_);
            AppendRuntimeLog(exported ? u8"已导出CSV。" : u8"导出CSV失败。");
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"清空"))
        {
            store_.Clear();
            lastSavedPointCount_ = 0;
            AppendRuntimeLog(u8"已清空测量表格和后台存储。");
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"一键复制"))
        {
            std::ostringstream oss;
            oss << "序号\tX\tY\tZ\t靶标球\t日期时间\n";
            for (const auto& record : store_.GetRecords())
            {
                oss << record.index << "\t"
                    << std::fixed << std::setprecision(3) << record.x << "\t"
                    << record.y << "\t"
                    << record.z << "\t"
                    << record.targetSize << "\t"
                    << FormatTime(record.timestamp) << "\n";
            }
            ImGui::SetClipboardText(oss.str().c_str());
            AppendRuntimeLog(u8"已将测量结果表格整体复制到系统剪贴板。");
        }

        ImGui::BeginChild("##MeasurementsTableRegion", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginTable("MeasurementsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn(u8"序号");
            ImGui::TableSetupColumn(u8"X");
            ImGui::TableSetupColumn(u8"Y");
            ImGui::TableSetupColumn(u8"Z");
            ImGui::TableSetupColumn(u8"靶标球");
            ImGui::TableSetupColumn(u8"日期时间");
            ImGui::TableHeadersRow();

            auto RenderReadOnlyInput = [](int col, const char* text) {
                ImGui::TableSetColumnIndex(col);
                ImGui::PushID(col);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                ImGui::PushItemWidth(-FLT_MIN);

                char buf[256];
                snprintf(buf, sizeof(buf), "%s", text);
                ImGui::InputText("##cell", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

                ImGui::PopItemWidth();
                ImGui::PopStyleColor();
                ImGui::PopID();
                };

            for (const auto& record : store_.GetRecords())
            {
                ImGui::TableNextRow();
                ImGui::PushID(record.index);

                char buf[64];
                snprintf(buf, sizeof(buf), "%d", record.index); RenderReadOnlyInput(0, buf);
                snprintf(buf, sizeof(buf), "%.3f", record.x); RenderReadOnlyInput(1, buf);
                snprintf(buf, sizeof(buf), "%.3f", record.y); RenderReadOnlyInput(2, buf);
                snprintf(buf, sizeof(buf), "%.3f", record.z); RenderReadOnlyInput(3, buf);
                RenderReadOnlyInput(4, record.targetSize.c_str());
                RenderReadOnlyInput(5, FormatTime(record.timestamp).c_str());

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }

    void UiLayer::RenderKinematicsWindow()
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), u8"● 正在进行第 %d 轮对接测量", currentRoundIndex_);
        ImGui::Spacing();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(u8"导入路径:");
        ImGui::SameLine();

        float btnCalcWidth = ImGui::CalcTextSize(u8"开始计算").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float spacingX = ImGui::GetStyle().ItemSpacing.x;

        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(80.0f, availWidth - btnCalcWidth - spacingX));
        ImGui::InputText("##KinematicsCsvPath", &kinematicsCsvPath_);

        ImGui::SameLine();
        if (ImGui::Button(u8"开始计算"))
        {
            StartKinematicsSolve();
        }

        auto RenderLabelAndCopyableText = [](const char* label, const char* fmt, ...) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::SameLine();

            char buf[256];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buf, sizeof(buf), fmt, args);
            va_end(args);

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushItemWidth(-FLT_MIN);
            ImGui::PushID(label);
            ImGui::InputText("##val", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
            ImGui::PopID();
            ImGui::PopItemWidth();
            ImGui::PopStyleColor();
            };

        ImGui::SeparatorText(u8"筒体对接质量评估");
        if (hasSimulatedKinematicsResult_)
        {
            RenderLabelAndCopyableText(u8"轴线夹角偏差(°): ", "%.3f", simulatedKinematicsResult_.axisAngleDeviation);
            RenderLabelAndCopyableText(u8"端面间距(mm): ", "%.3f", simulatedKinematicsResult_.endFaceSpacing);
            RenderLabelAndCopyableText(u8"错边量(mm): ", "%.3f", simulatedKinematicsResult_.misalignment);
        }
        else
        {
            ImGui::TextUnformatted(u8"暂无计算结果。");
        }

        // ==========================================
        // 渲染矩阵 及 3平3转 分量表
        // ==========================================
        if (hasSimulatedKinematicsResult_)
        {
            RenderMatrix4x4(u8"当前位姿矩阵", simulatedKinematicsResult_.currentPoseMatrix);
            RenderPoseVector(u8"当前位姿分量 (3平3转)", simulatedKinematicsResult_.currentPoseVector);

            RenderMatrix4x4(u8"目标位姿矩阵", simulatedKinematicsResult_.targetPoseMatrix);
            RenderPoseVector(u8"目标位姿分量 (3平3转)", simulatedKinematicsResult_.targetPoseVector);
        }

        ImGui::SeparatorText(u8"运动学计算结果");
        if (hasSimulatedKinematicsResult_)
        {
            RenderLabelAndCopyableText(u8"轮1高度: ", "%.3f", simulatedKinematicsResult_.wheel1Height);
            RenderLabelAndCopyableText(u8"轮2高度: ", "%.3f", simulatedKinematicsResult_.wheel2Height);
            RenderLabelAndCopyableText(u8"轮3高度: ", "%.3f", simulatedKinematicsResult_.wheel3Height);
            RenderLabelAndCopyableText(u8"轮4高度: ", "%.3f", simulatedKinematicsResult_.wheel4Height);
            RenderLabelAndCopyableText(u8"滚转角度: ", "%.3f", simulatedKinematicsResult_.rollAngle);
            RenderLabelAndCopyableText(u8"前进距离: ", "%.3f", simulatedKinematicsResult_.forwardDistance);
        }
        else
        {
            ImGui::TextUnformatted(u8"暂无计算结果。");
        }

        ImGui::Spacing();
        ImGui::SeparatorText(u8"多轮对接记录与报告导出");
        ImGui::Text(u8"后台已保存 %zu 轮对接计算数据。", historyRounds_.size());

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(u8"报告路径:");
        ImGui::SameLine();

        float btnExportRepWidth = ImGui::CalcTextSize(u8"导出综合报告").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetNextItemWidth(std::max(80.0f, availWidth - btnExportRepWidth - spacingX));
        ImGui::InputText("##ReportExportPath", &reportExportPath_);
        ImGui::SameLine();

        ImGui::BeginDisabled(historyRounds_.empty());
        if (ImGui::Button(u8"导出综合报告"))
        {
            ExportDockingReport(reportExportPath_);
        }
        ImGui::EndDisabled();
    }

    void UiLayer::RenderMatrix4x4(const char* label, const std::array<double, 16>& matrix)
    {
        ImGui::Spacing();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine();

        std::string btnId = std::string(u8"一键复制##") + label;
        if (ImGui::SmallButton(btnId.c_str()))
        {
            std::ostringstream oss;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    oss << std::fixed << std::setprecision(3) << matrix[row * 4 + col];
                    if (col < 3) oss << "\t";
                }
                oss << "\n";
            }
            ImGui::SetClipboardText(oss.str().c_str());
            AppendRuntimeLog(std::string(u8"已将 [") + label + u8"] 整体复制到系统剪贴板。");
        }
        ImGui::Separator();

        if (ImGui::BeginTable(label, 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
        {
            for (int row = 0; row < 4; ++row)
            {
                ImGui::TableNextRow();
                for (int col = 0; col < 4; ++col)
                {
                    ImGui::TableSetColumnIndex(col);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.3f", matrix[row * 4 + col]);

                    ImGui::PushID(row * 4 + col);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                    ImGui::PushItemWidth(-FLT_MIN);
                    ImGui::InputText("##cell", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopItemWidth();
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    }

    // =========================================================================
    // 【新增】渲染 6-DOF 分量表
    // =========================================================================
    void UiLayer::RenderPoseVector(const char* label, const std::array<double, 6>& vec)
    {
        ImGui::Spacing();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine();

        std::string btnId = std::string(u8"一键复制##") + label;
        if (ImGui::SmallButton(btnId.c_str()))
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3)
                << vec[0] << "\t" << vec[1] << "\t" << vec[2] << "\t"
                << vec[3] << "\t" << vec[4] << "\t" << vec[5] << "\n";
            ImGui::SetClipboardText(oss.str().c_str());
            AppendRuntimeLog(std::string(u8"已将 [") + label + u8"] 复制到系统剪贴板。");
        }
        ImGui::Separator();

        if (ImGui::BeginTable(label, 6, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn(u8"X(mm)");
            ImGui::TableSetupColumn(u8"Y(mm)");
            ImGui::TableSetupColumn(u8"Z(mm)");
            ImGui::TableSetupColumn(u8"Rx(°)");
            ImGui::TableSetupColumn(u8"Ry(°)");
            ImGui::TableSetupColumn(u8"Rz(°)");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            for (int col = 0; col < 6; ++col)
            {
                ImGui::TableSetColumnIndex(col);
                char buf[32];
                snprintf(buf, sizeof(buf), "%.3f", vec[col]);

                ImGui::PushID(col);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                ImGui::PushItemWidth(-FLT_MIN);
                ImGui::InputText("##cell", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
                ImGui::PopItemWidth();
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    void UiLayer::RenderRuntimeLogWindow()
    {
        if (ImGui::Checkbox(u8"自动滚动", &autoScrollLog_))
        {
            if (autoScrollLog_) scrollToBottom_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"清空"))
        {
            runtimeLog_.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"复制"))
        {
            ImGui::SetClipboardText(runtimeLog_.c_str());
        }

        ImGui::Separator();

        ImGui::BeginChild("##RuntimeLogScrollRegion", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImVec2 textSize = ImGui::CalcTextSize(runtimeLog_.c_str());
        float inputHeight = std::max(ImGui::GetContentRegionAvail().y, textSize.y + ImGui::GetStyle().FramePadding.y * 2.0f);
        ImGui::InputTextMultiline("##RuntimeLogText", &runtimeLog_, ImVec2(-FLT_MIN, inputHeight), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        if (scrollToBottom_)
        {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom_ = false;
        }
        ImGui::EndChild();
    }

    void UiLayer::AppendRuntimeLog(const std::string& line)
    {
        runtimeLog_ += line;
        runtimeLog_ += '\n';

        if (autoScrollLog_)
        {
            scrollToBottom_ = true;
        }
    }

    void UiLayer::HandleMeasurement()
    {
        MeasurementPoint point{};
        bool bDone = true;
        bool bValid = true;
        double avg = 0.0;
        double max = 0.0;
        double rms = 0.0;

        if (isSimulationMode_)
        {
            if (!simulatedConnected_)
            {
                AppendRuntimeLog(u8"【模拟】测量失败：硬件未连接。 ");
                OperationLogger::Instance().Log(LogLevel::Error, "【模拟】测量失败：硬件未连接。");
                return;
            }

            static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
            std::uniform_real_distribution<double> dist(-2000.0, 2000.0);
            point.x = dist(rng);
            point.y = dist(rng);
            point.z = dist(rng);
            avg = 0.015;
            max = 0.025;
            rms = 0.018;
        }
        else
        {
            point = device_.lastMeasurePoint_;
            avg = device_.lastMeasureAvg_;
            max = device_.lastMeasureMax_;
            rms = device_.lastMeasureRms_;
        }

        std::string currentTargetStr = "未知";
        if (targetSizeIndex_ == 0) currentTargetStr = "1.5";
        else if (targetSizeIndex_ == 1) currentTargetStr = "0.5";
        else if (targetSizeIndex_ == 2) currentTargetStr = "7/8";

        MeasurementRecord record{};
        record.index = static_cast<int>(store_.GetRecords().size()) + 1;
        record.x = point.x;
        record.y = point.y;
        record.z = point.z;
        record.targetSize = currentTargetStr;
        record.timestamp = std::chrono::system_clock::now();

        store_.AddRecord(record);

        std::ostringstream stream;
        stream << (isSimulationMode_ ? u8"【模拟测量】" : u8"硬件测量")
            << " #" << record.index
            << " x=" << std::fixed << std::setprecision(3) << point.x
            << " y=" << point.y
            << " z=" << point.z
            << " | done=" << (bDone ? "true" : "false")
            << " valid=" << (bValid ? "true" : "false")
            << " avg=" << avg
            << " max=" << max
            << " rms=" << rms;
        AppendRuntimeLog(stream.str());
        OperationLogger::Instance().Log(LogLevel::Info, stream.str());
    }

    void UiLayer::StartKinematicsSolve()
    {
        const auto& allRecords = store_.GetRecords();
        std::vector<MeasurementRecord> currentRoundPoints;
        for (size_t i = lastSavedPointCount_; i < allRecords.size(); ++i)
        {
            currentRoundPoints.push_back(allRecords[i]);
        }

        if (currentRoundPoints.empty())
        {
            AppendRuntimeLog(u8"提示：当前轮次无新测量点，将基于已有数据重新计算。");
            OperationLogger::Instance().Log(LogLevel::Info, u8"【提示】未收集到新测量点，将基于已有数据重复计算。");
        }

        std::filesystem::path csvPath = std::filesystem::u8path(kinematicsCsvPath_);
        if (!std::filesystem::exists(csvPath))
        {
            AppendRuntimeLog(u8"错误：找不到指定的CSV文件路径，请确认是否已导出测量数据！");
            OperationLogger::Instance().Log(LogLevel::Error, "未找到CSV文件：" + kinematicsCsvPath_);
            return;
        }

        OperationLogger::Instance().Log(LogLevel::Info, "正在调用 AlignmentSolver 核心解算模块读取 CSV...");

        solver_.computeAlignment(kinematicsCsvPath_, 1);

        if (!solver_.is_success)
        {
            std::string errorMsg = u8"解算失败！详细原因: " + solver_.last_error_message;
            AppendRuntimeLog(errorMsg);
            return;
        }

        simulatedKinematicsResult_.wheel1Height = solver_.H_cols_cmd[0];
        simulatedKinematicsResult_.wheel2Height = solver_.H_cols_cmd[1];
        simulatedKinematicsResult_.wheel3Height = solver_.H_cols_cmd[2];
        simulatedKinematicsResult_.wheel4Height = solver_.H_cols_cmd[3];
        simulatedKinematicsResult_.rollAngle = solver_.roll_cmd_fixed;
        simulatedKinematicsResult_.forwardDistance = solver_.dy_cmd;

        simulatedKinematicsResult_.axisAngleDeviation = solver_.axis_angle_dev;
        simulatedKinematicsResult_.endFaceSpacing = solver_.end_face_spacing;
        simulatedKinematicsResult_.misalignment = solver_.misalignment;

        // 提取 4x4 矩阵
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                simulatedKinematicsResult_.currentPoseMatrix[row * 4 + col] = solver_.mat_M0(row, col);
                simulatedKinematicsResult_.targetPoseMatrix[row * 4 + col] = solver_.mat_Mnew(row, col);
            }
        }

        // 【新增】提取 6-DOF (欧拉角) 分量并映射到结构体
        for (int i = 0; i < 6; ++i) {
            if (i < solver_.vec_M0.size()) {
                simulatedKinematicsResult_.currentPoseVector[i] = solver_.vec_M0[i];
            }
            if (i < solver_.vec_Mnew.size()) {
                simulatedKinematicsResult_.targetPoseVector[i] = solver_.vec_Mnew[i];
            }
        }

        hasSimulatedKinematicsResult_ = true;

        const bool poseAccepted = publisher_.PublishPosePair(
            simulatedKinematicsResult_.currentPoseMatrix,
            simulatedKinematicsResult_.currentPoseVector,
            simulatedKinematicsResult_.targetPoseMatrix,
            simulatedKinematicsResult_.targetPoseVector);

        if (poseAccepted)
        {
            AppendRuntimeLog(
                publisher_.IsConnected()
                ? u8"当前/目标位姿矩阵及 3平3转分量已提交 CPS 总线，并将按 50 ms 周期持续发送。"
                : u8"当前/目标位姿矩阵及 3平3转分量已缓存；CPS 总线连接后将自动持续发送。");
        }
        else
        {
            AppendRuntimeLog(u8"CPS 位姿发布失败：数据包含无效数值或发布器未初始化。");
        }

        std::ostringstream stream;
        stream << u8"【算法核心】完成第 " << currentRoundIndex_ << u8" 轮对接计算！";
        AppendRuntimeLog(stream.str());
        OperationLogger::Instance().Log(LogLevel::Info, stream.str());

        RoundRecord roundRecord;
        roundRecord.roundIndex = currentRoundIndex_;
        roundRecord.timestamp = FormatTime(std::chrono::system_clock::now());
        roundRecord.points = currentRoundPoints;
        roundRecord.kinematics = simulatedKinematicsResult_;

        historyRounds_.push_back(roundRecord);

        lastSavedPointCount_ = allRecords.size();
        currentRoundIndex_++;
    }

    void UiLayer::ExportDockingReport(const std::string& path)
    {
        std::filesystem::path fullPath = std::filesystem::absolute(std::filesystem::u8path(path));
        std::ofstream file(fullPath, std::ios::trunc);

        if (!file.is_open())
        {
            OperationLogger::Instance().Log(LogLevel::Error, "导出综合报告失败: 无法打开文件 " + fullPath.u8string());
            AppendRuntimeLog(u8"导出综合报告失败，请检查文件是否被其它程序占用。");
            return;
        }

        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        file.write(reinterpret_cast<const char*>(bom), sizeof(bom));

        file << "压力容器多轮对接综合记录报告\n\n";

        for (const auto& round : historyRounds_)
        {
            file << "================================================================\n";
            file << "第 " << round.roundIndex << " 轮对接记录\n";
            file << "计算完成时间: " << round.timestamp << "\n";
            file << "----------------------------------------------------------------\n";

            file << "\n[1. 靶标球空间测量数据]\n";
            file << "序号,X(mm),Y(mm),Z(mm),靶标球规格,测量时间\n";
            for (const auto& pt : round.points)
            {
                file << pt.index << ","
                    << std::fixed << std::setprecision(3) << pt.x << ","
                    << pt.y << ","
                    << pt.z << ","
                    << "=\"" << pt.targetSize << "\","
                    << FormatTime(pt.timestamp) << "\n";
            }

            file << "\n[2. 筒体对接质量评估]\n";
            file << "轴线夹角偏差(°)," << round.kinematics.axisAngleDeviation << "\n";
            file << "端面间距(mm)," << round.kinematics.endFaceSpacing << "\n";
            file << "错边量(mm)," << round.kinematics.misalignment << "\n";

            file << "\n[3. 运动学调平控制指令]\n";
            file << "1号轮调平高度(mm)," << round.kinematics.wheel1Height << "\n";
            file << "2号轮调平高度(mm)," << round.kinematics.wheel2Height << "\n";
            file << "3号轮调平高度(mm)," << round.kinematics.wheel3Height << "\n";
            file << "4号轮调平高度(mm)," << round.kinematics.wheel4Height << "\n";
            file << "整体滚转角度(°)," << round.kinematics.rollAngle << "\n";
            file << "整体前进距离(mm)," << round.kinematics.forwardDistance << "\n";

            file << "\n[4. 空间位姿变化矩阵及分量]\n";

            // 【新增】将欧拉角和分量也输出到最终报告中
            file << "-- 当前位姿分量 (X, Y, Z, Rx, Ry, Rz) --\n";
            file << round.kinematics.currentPoseVector[0] << "," << round.kinematics.currentPoseVector[1] << ","
                << round.kinematics.currentPoseVector[2] << "," << round.kinematics.currentPoseVector[3] << ","
                << round.kinematics.currentPoseVector[4] << "," << round.kinematics.currentPoseVector[5] << "\n\n";

            file << "-- 当前位姿矩阵 --\n";
            for (int i = 0; i < 4; i++) {
                file << round.kinematics.currentPoseMatrix[i * 4] << ","
                    << round.kinematics.currentPoseMatrix[i * 4 + 1] << ","
                    << round.kinematics.currentPoseMatrix[i * 4 + 2] << ","
                    << round.kinematics.currentPoseMatrix[i * 4 + 3] << "\n";
            }

            file << "\n-- 目标位姿分量 (X, Y, Z, Rx, Ry, Rz) --\n";
            file << round.kinematics.targetPoseVector[0] << "," << round.kinematics.targetPoseVector[1] << ","
                << round.kinematics.targetPoseVector[2] << "," << round.kinematics.targetPoseVector[3] << ","
                << round.kinematics.targetPoseVector[4] << "," << round.kinematics.targetPoseVector[5] << "\n\n";

            file << "-- 目标位姿矩阵 --\n";
            for (int i = 0; i < 4; i++) {
                file << round.kinematics.targetPoseMatrix[i * 4] << ","
                    << round.kinematics.targetPoseMatrix[i * 4 + 1] << ","
                    << round.kinematics.targetPoseMatrix[i * 4 + 2] << ","
                    << round.kinematics.targetPoseMatrix[i * 4 + 3] << "\n";
            }
            file << "\n\n";
        }

        OperationLogger::Instance().Log(LogLevel::Info, "已成功导出综合对接报告: " + fullPath.u8string());

        std::ostringstream msg;
        msg << u8"已成功导出共 " << historyRounds_.size() << u8" 轮对接综合报告。";
        AppendRuntimeLog(msg.str());
    }
}
