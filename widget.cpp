#include "widget.h"
#include "devices_wizard.h"
#include "associations_wizard.h"
#include "groups_wizard.h"

#include <QVBoxLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    resize(800, 800);

    openDevicesWizard();
}

Widget::~Widget()
{

}

void Widget::openDevicesWizard() {
    auto devicesWizard = new DevicesWizard(m_devicesModel, this);

    connect( devicesWizard, &DevicesWizard::deviceSelected, this, &Widget::openAssociationsWizard );

    setWindowTitle( "Devices Editor" );

    setCurrentWizard( devicesWizard );
}

void Widget::setCurrentWizard(QWidget* wizard) {
    if (m_currentWizard) {
        m_currentWizard->hide();
        m_currentWizard->deleteLater();
    }

    layout()->addWidget( wizard );
    m_currentWizard = wizard;
}

void Widget::openGroupsWizard(size_t devicesIndex, size_t channelIndex, size_t groupIndex) {
    auto wizard = new GroupsWizard(m_devicesModel, devicesIndex, channelIndex, groupIndex, this);

    connect( wizard, &GroupsWizard::backButtonClicked, this, [=]() {
        openAssociationsWizard(wizard->getDeviceIndex(), {});
    } );

    setWindowTitle("Association Groups");

    setCurrentWizard( wizard );
}

void Widget::openAssociationsWizard(size_t deviceIndex, std::optional<size_t> subIndex) {
    auto wizard = new AssociationsWizard(m_devicesModel, deviceIndex, subIndex, this);


   setWindowTitle( "Associations Editor" );

    connect(wizard, &AssociationsWizard::backClicked, this, &Widget::openDevicesWizard);
    connect(wizard, &AssociationsWizard::groupsWizardRequested, this, &Widget::openGroupsWizard);

    setCurrentWizard( wizard );
}
