#include "MonarkDrone.h"

MonarkDrone::MonarkDrone(int droneId, QObject* p_parent)
    : QObject(p_parent)
    , m_droneName{QString("MONARK ")+droneId}
    , m_droneId{droneId}
{
}

MonarkDrone::MonarkDrone(MonarkDrone const& that)
    : QObject{that.parent()}
    , m_droneName{that.m_droneName}
    , m_droneId{that.m_droneId}
{
}
