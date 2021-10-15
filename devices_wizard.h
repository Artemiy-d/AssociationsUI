#pragma once

#include <QWidget>

#include <optional>

#include "devices_model.h"

class QAbstractItemModel;

class DevicesWizard : public QWidget
{
    Q_OBJECT

public:
    DevicesWizard(DevicesModel& model, QWidget* parent);

signals:
    void deviceSelected(size_t deviceIndex, std::optional<size_t> subDeviceIndex);

private:
    void addDeviceToListView( const DevicesModel::Device& device );

private:
    DevicesModel& m_devicesModel;
    QAbstractItemModel* m_listViewModel;
    std::vector<int> m_parents;
};
