#pragma once

#include "core/DigitalTwinPublisher.h"
#include "core/AlignmentSolver.h"
#include "core/MeasurementStore.h"
#include "core/TrackerDevice.h"
#include "ui/UiLayer.h"

struct GLFWwindow;

namespace senpec
{
    class App
    {
    public:
        App();
        ~App();
        int Run();

    private:
        bool InitWindow();
        void Shutdown();

        GLFWwindow* window_;
        TrackerDevice device_;
        MeasurementStore store_;
        AlignmentSolver solver_;
        DigitalTwinPublisher publisher_;
        UiLayer uiLayer_;
    };
}
