#pragma once
#include <QObject>
#include <QString>
class MonarkDrone : public QObject
{
     Q_OBJECT
public:
     Q_PROPERTY(QString droneName READ droneName  CONSTANT)
     Q_PROPERTY(int droneId       READ droneId    CONSTANT)
    MonarkDrone(int droneId, QObject* p_parent=nullptr);
    MonarkDrone(MonarkDrone const& that);
    QString const& droneName() const{return m_droneName;}
    int droneId() const{return m_droneId;}
private:
    QString m_droneName;
    int m_droneId;
};
