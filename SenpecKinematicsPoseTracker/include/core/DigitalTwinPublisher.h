#pragma once

#include "core/MeasurementStore.h"

namespace senpec
{
    class DigitalTwinPublisher
    {
    public:
        void Publish(const MeasurementRecord& record);
    };
}
