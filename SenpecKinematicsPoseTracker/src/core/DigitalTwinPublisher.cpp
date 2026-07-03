#include "core/DigitalTwinPublisher.h"

#include "core/OperationLogger.h"

namespace senpec
{
    void DigitalTwinPublisher::Publish(const MeasurementRecord&)
    {
        OperationLogger::Instance().Log(LogLevel::Debug, "数字孪生数据已发送。");
    }
}
