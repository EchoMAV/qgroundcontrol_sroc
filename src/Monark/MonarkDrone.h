#pragma once
#include <QObject>
#include <QString>
class MonarkDrone : public QObject
{
     Q_OBJECT
public:

     enum class UpdateState : int{
        BeforeUpdate=0,
        UpdateInProgress=1,
        UpdateSuccessful=2,
        UpdateFailed=3,
    };

    Q_PROPERTY(int updateState READ updateState NOTIFY updateStateChanged);

    Q_PROPERTY(QString droneName READ droneName  CONSTANT)
    Q_PROPERTY(int droneId       READ droneId    CONSTANT)

    int updateState() const { return m_updateState;}

    void setUpdateState(int updateState){m_updateState=updateState;
        emit updateStateChanged(updateState);
    }

    MonarkDrone(int droneId, QObject* p_parent=nullptr);
    MonarkDrone(MonarkDrone const& that);
    QString const& droneName() const{return m_droneName;}
    int droneId() const{return m_droneId;}



private:
signals:


    void updateStateChanged(int updateState);
private:
    QString m_droneName;
    int m_droneId;
    QAtomicInteger<int> m_updateState;
};
