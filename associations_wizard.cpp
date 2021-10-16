#include "associations_wizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <QPushButton>
#include <QLabel>

#include <QListView>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QComboBox>
#include <QMetaType>
#include <QStyledItemDelegate>
#include <QSignalBlocker>

#include <sstream>
#include <set>

namespace {

struct AssociationInfo {
    size_t deviceIndex;
    size_t channelIndex;
    size_t groupIndex;
    size_t targetDeviceIndex;
    std::optional<size_t> targetChannelIndex;
};

void updateComboModelWithSavingIndex(QComboBox* combo, QStringList stringList) {
    const auto prevIndex = combo->currentIndex();
    const auto newIndex = prevIndex < 0 || prevIndex >= stringList.size() ? 0 : prevIndex;

    QSignalBlocker blocker(combo);

    static_cast<QStringListModel*>(combo->model())->setStringList(stringList);

    combo->setCurrentIndex( newIndex );
}

void updateComboModelWithSavingText(QComboBox* combo, QStringList stringList) {
    auto foundIt = std::find( stringList.begin(), stringList.end(), combo->currentText());

    const auto newIndex = foundIt == stringList.end() ? 0 : static_cast<int>(foundIt - stringList.begin());

    QSignalBlocker blocker(combo);

    static_cast<QStringListModel*>(combo->model())->setStringList(stringList);

    combo->setCurrentIndex( newIndex );
}

class ItemDelegate : public QStyledItemDelegate {
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override {
        auto result = QStyledItemDelegate::sizeHint( option, index );
        return QSize( result.width(), result.height() + 16 );
    }
};

QString associationToString(const DevicesModel& model, const AssociationInfo& associationInfo) {
    auto& device = model.getDevices()[ associationInfo.deviceIndex ];
    auto& targetDevice = model.getDevices()[ associationInfo.targetDeviceIndex ];

    std::stringstream stream;
    stream << "Source: " << device.name << " [node: " << device.nodeId << "; channel: " << associationInfo.channelIndex << "; group: " << device.channelsToGroups[associationInfo.channelIndex][associationInfo.groupIndex].name << "]" <<
              "\nTarget: " << targetDevice.name << " [node: " << targetDevice.nodeId;
    if (associationInfo.targetChannelIndex) {
        stream << "; channel: " << *associationInfo.targetChannelIndex;
    }

    stream << "]";

    return QString::fromStdString(stream.str());
}

class BaseSourceModel : public QAbstractListModel {
public:

    BaseSourceModel(const DevicesModel& model, std::vector<AssociationInfo> references) :
        m_model(model),
        m_associationReferences(std::move(references))
    { }


    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if ( role == Qt::DisplayRole ) {
            return associationToString( m_model, m_associationReferences[index.row()] );
        }

        return {};
    }

    int rowCount(const QModelIndex &parent) const override {
        return m_associationReferences.size();
    }

    const std::vector<AssociationInfo>& getAssociationReferences() const {
        return m_associationReferences;
    }

    const DevicesModel& getDevicesModel() const {
        return m_model;
    }

private:
    const DevicesModel& m_model;
    std::vector<AssociationInfo> m_associationReferences;
};


class SourceModel : public BaseSourceModel {
public:

    static std::vector<AssociationInfo> createAssociationReferences(const DevicesModel& model) {
        std::vector<AssociationInfo> result;

        size_t deviceIndex = 0;
        for ( const auto& device : model.getDevices() ) {
            size_t channelIndex = 0;
            for ( const auto& groups : device.channelsToGroups ) {
                size_t groupIndex = 0;
                for ( const auto& group : groups ) {
                    size_t associationIndex = 0;
                    for ( const auto& association : group.associations ) {

                        AssociationInfo info;
                        info.deviceIndex = deviceIndex;
                        info.channelIndex = channelIndex;
                        info.groupIndex = groupIndex;
                        info.targetDeviceIndex = association.deviceIndex;
                        info.targetChannelIndex = association.channelIndex;

                        result.push_back( info );

                        ++associationIndex;
                    }

                    ++groupIndex;
                }

                ++channelIndex;
            }
            ++deviceIndex;
        }

        return result;
    }


    SourceModel(const DevicesModel& model) :
        BaseSourceModel(model, createAssociationReferences(model)) {
    }
};

class HintSourceModel : public BaseSourceModel {
public:

    static std::vector<AssociationInfo> createAssociationReferences(const DevicesModel& model) {
        std::vector<AssociationInfo> result;

        AssociationInfo reference = {};

        for ( const auto& device : model.getDevices() ) {
            reference.channelIndex = 0;
            for ( const auto& groups : device.channelsToGroups ) {
                reference.groupIndex = 0;
                for ( const auto& group : groups ) {
                    if ( group.associations.size() < group.maxAssociationsNumber ) {
                        std::set< std::pair< size_t, std::optional<size_t> > > existingAssociations;

                        for ( auto& association : group.associations ) {
                            existingAssociations.emplace( association.deviceIndex, association.channelIndex );
                        }

                        reference.targetDeviceIndex = 0;
                        for ( const auto& targetDevice : model.getDevices() ) {
                            if ( reference.deviceIndex != reference.targetDeviceIndex ) {
                                reference.targetChannelIndex = {};

                                auto addAssociationReference = [&]() {
                                    if ( existingAssociations.find( std::make_pair( reference.targetDeviceIndex, reference.targetChannelIndex ) ) == existingAssociations.end() ) {
                                        result.push_back(reference);
                                    }
                                };

                                addAssociationReference();

                                for ( reference.targetChannelIndex = 0;
                                      *reference.targetChannelIndex < targetDevice.channelsToGroups.size();
                                      ++(*reference.targetChannelIndex) ) {
                                    addAssociationReference();
                                }
                            }


                            ++reference.targetDeviceIndex;
                        }
                    }
                    ++reference.groupIndex;
                }

                ++reference.channelIndex;
            }
            ++reference.deviceIndex;
        }

        return result;
    }

    HintSourceModel(const DevicesModel& model) :
        BaseSourceModel(model, createAssociationReferences(model)) {
    }
};

struct FilterInfo {
    std::optional<size_t> deviceIndex;
    std::optional<size_t> channelIndex;
    std::string groupName;
    std::optional<size_t> targetDeviceIndex;
    std::optional<std::optional<size_t>> targetChannelIndex;
};

class AssociationListProxyModel : public QSortFilterProxyModel {
public:
    AssociationListProxyModel(BaseSourceModel* sourceModel) {
        setSourceModel( sourceModel );
    }

    void setFilter(FilterInfo filterInfo) {
        m_filterInfo = std::move(filterInfo);
        invalidate();
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &) const override {
        auto model = static_cast< BaseSourceModel* >( sourceModel() );

        const auto& associationReference = model->getAssociationReferences()[sourceRow];
        const auto& devicesModel = model->getDevicesModel();

        const auto& device = devicesModel.getDevices()[ associationReference.deviceIndex ];

        if ( m_filterInfo.deviceIndex && *m_filterInfo.deviceIndex != associationReference.deviceIndex )
            return false;

        if ( m_filterInfo.channelIndex && *m_filterInfo.channelIndex != associationReference.channelIndex )
            return false;

        const auto& group = device.channelsToGroups[associationReference.channelIndex][associationReference.groupIndex];

        if ( !m_filterInfo.groupName.empty() && m_filterInfo.groupName != group.name )
            return false;

        if ( m_filterInfo.targetDeviceIndex && *m_filterInfo.targetDeviceIndex != associationReference.targetDeviceIndex )
            return false;

        if ( m_filterInfo.targetChannelIndex && *m_filterInfo.targetChannelIndex != associationReference.targetChannelIndex )
            return false;

        return true;
    }

private:

    FilterInfo m_filterInfo;
};

}

AssociationsWizard::AssociationsWizard(DevicesModel& model, size_t index, std::optional<size_t> subIndex, QWidget *parent) :
    QWidget(parent),
    m_model(model)
{
    auto mainLayout = new QVBoxLayout(this);

    const auto& device = model.getDevices()[index];

    {
        auto backButton = new QPushButton("< Back To Devices", this);

        mainLayout->addWidget( backButton );

        connect( backButton, &QPushButton::clicked, this, &AssociationsWizard::backClicked );
    }

    {
        auto layout = new QHBoxLayout;
        auto label = new QLabel( "Source node: " );
        label->setFixedWidth(100);

        layout->addWidget( label );

        m_sourceNodeCombo = new QComboBox(this);
        m_sourceNodeCombo->setModel( new QStringListModel );

        m_sourceNodeCombo->setCurrentIndex( index + 1 );


        layout->addWidget(m_sourceNodeCombo);

        mainLayout->addLayout(layout);
    }

    {
        auto layout = new QHBoxLayout;
        auto label = new QLabel( "Source channel: " );
        label->setFixedWidth(100);

        layout->addWidget( label );

        m_sourceChannelCombo = new QComboBox(this);
        m_sourceChannelCombo->setModel( new QStringListModel );
        layout->addWidget(m_sourceChannelCombo);


        mainLayout->addLayout(layout);
    }

    {
        auto layout = new QHBoxLayout;
        auto label = new QLabel( "Source group: " );
        label->setFixedWidth(100);

        layout->addWidget( label );

        m_sourceGroupCombo = new QComboBox(this);
        m_sourceGroupCombo->setModel( new QStringListModel );
        layout->addWidget(m_sourceGroupCombo);

        auto groupInfoButton = new QPushButton("Info", this);
        groupInfoButton->setToolTip("Fill source information (source node, channel, group) to see group details.");
        groupInfoButton->setSizePolicy( QSizePolicy::Fixed, groupInfoButton->sizePolicy().verticalPolicy() );
        connect(groupInfoButton, &QPushButton::clicked, this, [this]() {
            if ( m_sourceNodeCombo->currentIndex() > 0 && m_sourceChannelCombo->currentIndex() > 0 && m_sourceGroupCombo->currentIndex() > 0 ) {
                emit groupsWizardRequested(m_sourceNodeCombo->currentIndex() - 1, m_sourceChannelCombo->currentIndex() - 1, m_sourceGroupCombo->currentIndex() - 1);
            }
        });

        layout->addWidget(groupInfoButton);


        //connect( m_sourceGroupCombo, &QComboBox::currentIndexChanged, this, &AssociationsWizard::updateSourceGroupsCombo );

        mainLayout->addLayout(layout);


    }

    {
        auto layout = new QHBoxLayout;
        auto label = new QLabel( "Target node: " );
        label->setFixedWidth(100);

        layout->addWidget( label );

        m_targetNodeCombo = new QComboBox(this);
        m_targetNodeCombo->setModel( new QStringListModel );
        layout->addWidget(m_targetNodeCombo);

        mainLayout->addLayout(layout);
    }

    {
        auto layout = new QHBoxLayout;
        auto label = new QLabel( "Target channel: " );
        label->setFixedWidth(100);

        layout->addWidget( label );

        m_targetChannelCombo = new QComboBox(this);
        m_targetChannelCombo->setModel( new QStringListModel );
        layout->addWidget(m_targetChannelCombo);

        mainLayout->addLayout(layout);
    }

    {
        auto viewsLayout = new QGridLayout;
        mainLayout->addLayout(viewsLayout);

        {
            viewsLayout->addWidget( new QLabel("Existing Associations", this), 0, 0 );

            m_existingAssociationsView = new QListView(this);
            m_existingAssociationsView->setModel( new AssociationListProxyModel( new SourceModel( m_model ) ) );
            m_existingAssociationsView->setItemDelegate(new ItemDelegate);
            m_existingAssociationsView->setToolTip(
R"(This is a list of existing associations mathing to the chosen filter (source node, source channel, source group, target node, target channel).
You may remove association by selecting it and pressing the button.)");

            viewsLayout->addWidget(m_existingAssociationsView, 1, 0);

            auto removeAssociationButton = new QPushButton("Remove Selected Association", m_existingAssociationsView);
            //removeAssociationButton->setEnabled(false);

            viewsLayout->addWidget(removeAssociationButton, 2, 0);


            connect(removeAssociationButton, &QPushButton::clicked, this, [=]() {
                auto selectedIndexes = m_existingAssociationsView->selectionModel()->selectedIndexes();
                if ( !selectedIndexes.empty() ) {
                    auto proxyModel = static_cast<QAbstractProxyModel*>( m_existingAssociationsView->model() );
                    auto sourceIndex = proxyModel->mapToSource(selectedIndexes.front());
                    auto sourceModel = static_cast<BaseSourceModel*>(proxyModel->sourceModel());

                    size_t index = static_cast<size_t>(sourceIndex.row());
                    if ( index < sourceModel->getAssociationReferences().size() ) {
                        const auto& reference = sourceModel->getAssociationReferences()[index];

                        m_model.removeAssociation( reference.deviceIndex, reference.channelIndex, reference.groupIndex, { reference.targetDeviceIndex, reference.targetChannelIndex } );

                        proxyModel->setSourceModel( new HintSourceModel( m_model ) );
                        static_cast<QAbstractProxyModel*>( m_existingAssociationsView->model() )->setSourceModel( new SourceModel( m_model ) );
                    }
                }
            });
        }

        {
            viewsLayout->addWidget( new QLabel("Hint For Association Adding", this), 0, 1 );


            m_hintAssociationsView = new QListView(this);
            m_hintAssociationsView->setModel( new AssociationListProxyModel( new HintSourceModel( m_model ) ) );
            m_hintAssociationsView->setItemDelegate(new ItemDelegate);

            m_hintAssociationsView->setToolTip(
R"(This is a list all the associations that could be added.
They are matching to the selected filter (source node, source channel, source group, target node, target channel).
If you want to add the association select it and press the button.
Note, the list should contain more relevant associations at the top. Less relevant ones should be below and lit with another color.
Flow of adding a association for a simple user:
    He just selects the association from the list with filtered source node only.
    User doesn't have to know all the details about zwave associations.
Flow of adding a association for an advanced user:
    He may input all the required information to filters manually and then chose only one option from the list.)");

            viewsLayout->addWidget(m_hintAssociationsView, 1, 1);

            auto addAssociationButton = new QPushButton("Add Selected Association", this);
            viewsLayout->addWidget(addAssociationButton, 2, 1);

            addAssociationButton->setToolTip(
R"(What we add:
Association is mapping of a source to a target.
Source is node+channel+association group:
    Node contains a list of channels
        Each channel contains a list of association groups
            Each group has maximum number of targets
Target is node+channel or just node (whole node))");


            connect(addAssociationButton, &QPushButton::clicked, this, [=]() {
                auto selectedIndexes = m_hintAssociationsView->selectionModel()->selectedIndexes();
                if ( !selectedIndexes.empty() ) {
                    auto proxyModel = static_cast<QAbstractProxyModel*>( m_hintAssociationsView->model() );
                    auto sourceIndex = proxyModel->mapToSource(selectedIndexes.front());
                    auto sourceModel = static_cast<BaseSourceModel*>(proxyModel->sourceModel());

                    size_t index = static_cast<size_t>(sourceIndex.row());
                    if ( index < sourceModel->getAssociationReferences().size() ) {
                        const auto& reference = sourceModel->getAssociationReferences()[index];

                        m_model.addAssociation( reference.deviceIndex, reference.channelIndex, reference.groupIndex, { reference.targetDeviceIndex, reference.targetChannelIndex } );

                        proxyModel->setSourceModel( new HintSourceModel( m_model ) );
                        static_cast<QAbstractProxyModel*>( m_existingAssociationsView->model() )->setSourceModel( new SourceModel( m_model ) );
                    }
                }
            });
        }
    }

    updateSourceNodeCombo(index + 1);

    updateTargetNodeCombo();

    connect( m_targetNodeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AssociationsWizard::updateTargetChannelCombo );
    connect( m_sourceChannelCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AssociationsWizard::updateSourceGroupsCombo );
    connect( m_sourceNodeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AssociationsWizard::updateSourceChannelsCombo );

}

void AssociationsWizard::updateSourceNodeCombo(std::optional<size_t> currentIndex) {
    QStringList stringList;

    stringList.append("Not specified");

    for ( auto& device : m_model.getDevices() ) {
        stringList.append( QString("Node ") + QString::number(device.nodeId) + " (" + QString::fromStdString( device.name ) + ")" );
    }

    static_cast< QStringListModel* >( m_sourceNodeCombo->model() )->setStringList(stringList);

    if (currentIndex)
        m_sourceNodeCombo->setCurrentIndex(*currentIndex);

    updateSourceChannelsCombo();
}

void AssociationsWizard::updateSourceChannelsCombo() {
    QStringList stringList;

    stringList.append("Not specified");

    size_t maxChannelsNumber = 0;

    auto handleDevice = [&](const DevicesModel::Device& device) {
          for ( ; maxChannelsNumber < device.channelsToGroups.size(); ++maxChannelsNumber ) {
              stringList.append( "Channel " + QString::number( maxChannelsNumber ) );
          }
    };

    if ( m_sourceNodeCombo->currentIndex() <= 0 ) {
        for ( auto& device : m_model.getDevices() ) {
             handleDevice( device );
        }
    }
    else {
        handleDevice( m_model.getDevices()[ m_sourceNodeCombo->currentIndex() - 1 ] );
    }

    updateComboModelWithSavingText(m_sourceChannelCombo, stringList);

    updateSourceGroupsCombo();
}

void AssociationsWizard::updateSourceGroupsCombo() {
    QStringList stringList;

    stringList.append("Not specified");

    auto handleGroups = [&](const std::vector<DevicesModel::AssociationGroup>& groups) {
          for ( auto& group : groups ) {
              stringList.append( QString::fromStdString( group.name ) );
          }
    };

    auto handleDevice = [&](const DevicesModel::Device& device) {
        if ( m_sourceChannelCombo->currentIndex() <= 0 ) {
            const size_t index = m_sourceChannelCombo->currentIndex() - 1;
            if ( index < device.channelsToGroups.size() ) {
                handleGroups( device.channelsToGroups[index] );
            }
        }
        else {
            for ( auto& groups : device.channelsToGroups ) {
                handleGroups( groups );
            }
        }
    };

    if ( m_sourceNodeCombo->currentIndex() <= 0 ) {
        for ( auto& device : m_model.getDevices() ) {
             handleDevice( device );
        }
    }
    else {
        handleDevice( m_model.getDevices()[ m_sourceNodeCombo->currentIndex() - 1 ] );
    }

    std::sort( stringList.begin() + 1, stringList.end() );
    stringList.erase( std::unique(stringList.begin() + 1, stringList.end()), stringList.end() );

    updateComboModelWithSavingText(m_sourceGroupCombo, stringList);

    invalidateFilters();
}

void AssociationsWizard::updateTargetNodeCombo() {
    QStringList stringList;

    stringList.append("Not specified");

    for ( auto& device : m_model.getDevices() ) {
        stringList.append( QString("Node ") + QString::number(device.nodeId) + " (" + QString::fromStdString( device.name ) + ")" );
    }

    updateComboModelWithSavingText(m_targetNodeCombo, stringList);

    updateTargetChannelCombo();

    invalidateFilters();
}

void AssociationsWizard::updateTargetChannelCombo() {
    QStringList stringList;

    stringList.append("Not specified");
    stringList.append("Whole node");

    size_t maxChannelsNumber = 0;

    auto handleDevice = [&](const DevicesModel::Device& device) {
          for ( ; maxChannelsNumber < device.channelsToGroups.size(); ++maxChannelsNumber ) {
              stringList.append( "Channel " + QString::number( maxChannelsNumber ) );
          }
    };

    if ( m_targetNodeCombo->currentIndex() <= 0 ) {
        for ( auto& device : m_model.getDevices() ) {
             handleDevice( device );
        }
    }
    else {
        handleDevice( m_model.getDevices()[ m_targetNodeCombo->currentIndex() - 1 ] );
    }

    updateComboModelWithSavingText(m_targetChannelCombo, stringList);

    invalidateFilters();
}

void AssociationsWizard::invalidateFilters() {
    if (!m_filtersInvalidated) {
        m_filtersInvalidated = true;

        QMetaObject::invokeMethod(this, &AssociationsWizard::updateFilters, Qt::ConnectionType::QueuedConnection);
    }
}

void AssociationsWizard::updateFilters() {
    m_filtersInvalidated = false;

    FilterInfo filterInfo;

    if ( m_sourceNodeCombo->currentIndex() > 0 ) {
        filterInfo.deviceIndex = m_sourceNodeCombo->currentIndex() - 1;
    }

    if ( m_sourceChannelCombo->currentIndex() > 0 ) {
        filterInfo.channelIndex = m_sourceChannelCombo->currentIndex() - 1;
    }

    if ( m_sourceGroupCombo->currentIndex() > 0 ) {
        filterInfo.groupName = m_sourceGroupCombo->currentText().toStdString();
    }

    if ( m_targetNodeCombo->currentIndex() > 0 ) {
        filterInfo.targetDeviceIndex = m_targetNodeCombo->currentIndex() - 1;
    }

    if ( m_targetChannelCombo->currentIndex() > 0 ) {
        filterInfo.targetChannelIndex = m_targetChannelCombo->currentIndex() == 1 ?
                    std::optional<size_t>() :
                    std::optional<size_t>( m_targetChannelCombo->currentIndex() - 2 );
    }

    static_cast< AssociationListProxyModel* >( m_existingAssociationsView->model() )->setFilter( filterInfo );
    m_existingAssociationsView->update();

    static_cast< AssociationListProxyModel* >( m_hintAssociationsView->model() )->setFilter( filterInfo );
    m_hintAssociationsView->update();
}
