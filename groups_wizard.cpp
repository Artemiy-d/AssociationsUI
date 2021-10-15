#include "groups_wizard.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>

#include <QListView>
#include <QStringListModel>
#include <QLineEdit>
#include <QGridLayout>

GroupsWizard::GroupsWizard(DevicesModel& model, size_t deviceIndex, size_t channelIndex, size_t groupIndex, QWidget* parent) :
    QWidget(parent),
    m_devicesModel(model),
    m_deviceIndex(deviceIndex),
    m_channelIndex(channelIndex),
    m_groupIndex(groupIndex)
{
    auto mainLayout = new QVBoxLayout(this);

    auto backButton = new QPushButton("< Back To Associations");
    mainLayout->addWidget(backButton);
    connect(backButton, &QPushButton::clicked, this, &GroupsWizard::backButtonClicked);

    auto& device = m_devicesModel.getDevices()[m_deviceIndex];

    mainLayout->addWidget(new QLabel("Device: " + QString::fromStdString(device.name) +
                                     "\nChannel: " + QString::number(m_channelIndex) +
                                     "\nGroup Name: " + QString::fromStdString(getGroup().name) +
                                     "\nGroup Profile: " + QString::fromStdString(getGroup().profile) +
                                     "\nCurrent Associations Number: " + QString::number(getGroup().associations.size()) +
                                     "\nMax Associations Number: " + QString::number(getGroup().maxAssociationsNumber) ) );

    mainLayout->addSpacing( 10 );

    mainLayout->addWidget( new QLabel("Commands", this) );

    {
        auto commandsView = new QListView(this);

        QStringList commands;

        commands.append("BASIC_REPORT");
        commands.append("SIREN_REPORT");

        commandsView->setModel(new QStringListModel(commands));

        mainLayout->addWidget(commandsView);
    }


    mainLayout->addSpacing(10);

    {
        auto layout = new QHBoxLayout;

        layout->addWidget(new QLabel("Specific Commands For Target Node"));

        auto targetNodeCombo = new QComboBox(this);

        {
            QStringList nodesList;

            size_t index = 0;
            for ( auto& device : m_devicesModel.getDevices() ) {
                if (index != deviceIndex) {
                    nodesList.append( QString::number(device.nodeId) + " (" + QString::fromStdString(device.name) + ")" );
                }

                ++index;
            }

            targetNodeCombo->setModel( new QStringListModel(nodesList) );
        }

        layout->addWidget(targetNodeCombo);

        mainLayout->addLayout(layout);
    }

    {
        auto specificCommandsView = new QListView(this);

        QStringList commands;
        commands.append("SIREN_REPORT (Data: 0x018D)");
        commands.append("BATTERY_REPORT");

        specificCommandsView->setModel(new QStringListModel(commands));

        mainLayout->addWidget(specificCommandsView);
    }


    auto addCommandLayout = new QHBoxLayout;

    {
        auto label = new QLabel("Command: ");

        addCommandLayout->addWidget(label);

        auto combo = new QComboBox(this);
        addCommandLayout->addWidget(combo);

        QStringList commands;

        commands.append("BASIC_REPORT");
        commands.append("BASIC_SET");
        commands.append("SWITCH_BINARY_REPORT");
        commands.append("NOTIFICATION_REPORT");

        combo->setModel(new QStringListModel(commands));

    }

    {
        auto label = new QLabel("Data: ");
        label->setFixedWidth(100);

        addCommandLayout->addWidget(label);

        auto edit = new QLineEdit(this);
        edit->setText("0xF1");
        edit->setMinimumWidth(150);
        addCommandLayout->addWidget(edit);


    }

    auto addCommandButton = new QPushButton("Add", this);
    addCommandLayout->addWidget(addCommandButton);

    mainLayout->addLayout(addCommandLayout);

}

const DevicesModel::AssociationGroup& GroupsWizard::getGroup() const {
    return m_devicesModel.getDevices()[m_deviceIndex].channelsToGroups[m_channelIndex][m_groupIndex];
}

size_t GroupsWizard::getDeviceIndex() const {
    return m_deviceIndex;
}
