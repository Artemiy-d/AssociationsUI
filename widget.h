#pragma once

#include <QWidget>

#include <optional>
#include "devices_model.h"

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void openDevicesWizard();

    void openGroupsWizard(size_t devicesIndex, size_t channelIndex, size_t groupIndex);

    void openAssociationsWizard(size_t deviceIndex, std::optional<size_t> subIndex);

private:

    void setCurrentWizard(QWidget* wizard);

private:
    QWidget* m_currentWizard = nullptr;
    DevicesModel m_devicesModel;
};
