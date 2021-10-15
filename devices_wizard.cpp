#include "devices_wizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QListView>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValueRef>
#include <QJsonObject>

#include <QTextEdit>
#include <QTreeView>
#include <QStyledItemDelegate>

namespace {
class ItemDelegate : public QStyledItemDelegate {
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override {
        auto result = QStyledItemDelegate::sizeHint( option, index );
        return QSize( result.width(), result.height() + 10 );
    }
};
}

DevicesWizard::DevicesWizard(DevicesModel& model, QWidget* parent) : QWidget(parent), m_devicesModel(model)
{
    auto mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel("Current ZWave devices List (Double click -> Associations Editor)", this));

    auto listView = new QListView(this);
    mainLayout->addWidget(listView);
    listView->setItemDelegate( new ItemDelegate );

    m_listViewModel = new QStringListModel;

    for ( const auto& device : model.getDevices() ) {
        addDeviceToListView(device);
    }

    listView->setModel( m_listViewModel );

    listView->setToolTip(
R"(This is a list of existing included zwave devices. Each main device is a zwave node.
Some devices have set of child devices. Each subdevice is under the same node as a parent.
You may add a new device to the list declaring it in the textedit below and pressing the button)"
);

    listView->setEditTriggers( QListView::NoEditTriggers );

    connect( listView, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        auto it = std::upper_bound( m_parents.begin(), m_parents.end(), index.row() );

        if ( !m_parents.empty() && it != m_parents.begin() ) {
            --it;
            emit deviceSelected( *it,
                                 index.row() == *it ? std::optional<size_t>() : index.row() - *it - 1 );
        }
    });


    auto textEdit = new QTextEdit(this);
    textEdit->setFixedHeight(200);
    textEdit->setText(
R"({
    "name": "My Device",
    "subdevices": [
        {
            "name": "My Device 1",
            "items": [
                {
                    "name": "second_item",
                    "zwave_references": [
                        {
                            "channel": 1,
                            "cc": "siren"
                        }
                    ]
                }
            ]
        },
        {
            "name": "My Device 2"
        }
    ],
    "items": [
        {
            "name": "first_item",
            "zwave_references": [
                {
                    "channel": 0,
                    "cc": "notification"
                },
                {
                    "channel": 0,
                    "cc": "binary_sensor"
                }
            ]
        }
    ],
    "channels": [
        {
            "groups": [
                {
                    "name": "Lifeline",
                    "profile": "default",
                    "maxAssociationsNumber": 10
                },
                {
                    "name": "Secondary Group",
                    "profile": "Electric:Meter",
                    "maxAssociationsNumber": 5
                }
            ]
        }
    ]
}
)");

    mainLayout->addSpacing(15);

    mainLayout->addWidget(new QLabel("New Device Json Description", this));
    mainLayout->addWidget(textEdit);


    auto addNewDeviceButton = new QPushButton("Add New Device By Json", this);
    connect(addNewDeviceButton, &QPushButton::clicked, this, [textEdit, this]() {
        auto jsonDoc = QJsonDocument::fromJson( QByteArray( textEdit->toPlainText().toLocal8Bit() ) );

        if (jsonDoc.isObject()) {
            DevicesModel::Device device;

            device.name = jsonDoc["name"].toString().toStdString();


            auto channels = jsonDoc["channels"];
            for ( auto channel : channels.toArray() ) {
                std::vector< DevicesModel::AssociationGroup > associationGroups;

                auto groups = channel.toObject()["groups"];

                for ( auto group : groups.toArray() ) {
                    DevicesModel::AssociationGroup associationGroup;
                    auto groupObject = group.toObject();
                    associationGroup.name = groupObject["name"].toString().toStdString();
                    associationGroup.profile = groupObject["profile"].toString().toStdString();
                    associationGroup.maxAssociationsNumber = groupObject["maxAssociationsNumber"].toInt();

                    associationGroups.push_back( std::move( associationGroup ) );
                }

                device.channelsToGroups.push_back(std::move(associationGroups));
            }

            auto subdevices = jsonDoc["subdevices"];
            for ( auto subdeviceJson : subdevices.toArray() ) {
                DevicesModel::SubDeivice subdevice;

                subdevice.name = subdeviceJson.toObject()["name"].toString().toStdString();

                device.children.push_back( std::move( subdevice ) );
            }

            m_devicesModel.addDevice( std::move( device ) );
            addDeviceToListView( m_devicesModel.getDevices().back() );

            //listView->
            //listView->update();
        }
    });
    mainLayout->addWidget(addNewDeviceButton);

}

void DevicesWizard::addDeviceToListView( const DevicesModel::Device& device ) {
    auto row = m_listViewModel->rowCount();

    m_listViewModel->insertRow( row );
    auto parentIndex = m_listViewModel->index( row, 0 );
    m_listViewModel->setData( parentIndex, QString::fromStdString(device.name) + " (node: " + QString::number( device.nodeId ) + ")" );

    m_listViewModel->insertRows( ++row, device.children.size());
    m_parents.push_back(parentIndex.row());

    for ( auto& subdevice : device.children ) {
        m_listViewModel->setData( m_listViewModel->index( row, 0 ), "    " + QString::fromStdString(subdevice.name) );
        ++row;
    }
}
