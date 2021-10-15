#pragma once

#include <QWidget>

#include "devices_model.h"

class GroupsWizard : public QWidget
{
    Q_OBJECT

public:
    GroupsWizard(DevicesModel& model, size_t deviceIndex, size_t channelIndex, size_t groupIndex, QWidget* parent);

    size_t getDeviceIndex() const;

private:
    const DevicesModel::AssociationGroup& getGroup() const;

signals:
    void backButtonClicked();

private:
    DevicesModel& m_devicesModel;
    size_t m_deviceIndex;
    size_t m_channelIndex;
    size_t m_groupIndex;
};


