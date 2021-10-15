#include "devices_model.h"

DevicesModel::DevicesModel()
{
    {
        Device device;
        device.nodeId = 1;
        device.name = "eZLO hub";
        device.icon = "";

        addDevice( std::move(device) );
    }

    {
        Device device;
        device.nodeId = 2;
        device.name = "Siren";
        device.icon = "";

        {
            Item item;
            item.name = "battery";
            item.references.push_back({ 0, "battery" });

            device.items.push_back(item);
        }

        {
            Item item;
            item.name = "siren";
            item.references.push_back({ 0, "siren" });
            item.references.push_back({ 0, "basic" });

            device.items.push_back(item);
        }

        {
            AssociationGroup group;
            group.name = "Lifeline";
            group.maxAssociationsNumber = 10;
            group.profile = "Siren:Siren";
            group.associations.push_back( { 0, 0 } );
            group.commands.push_back( 60 );

            std::vector< AssociationGroup > groups;
            groups.push_back( std::move( group ) );

            device.channelsToGroups.push_back(std::move(groups));
        }

        {
            AssociationGroup group;
            group.name = "Next Group";
            group.maxAssociationsNumber = 2;
            group.profile = "ElectricMeter";
            group.associations.push_back( { 1, 0 } );
            group.commands.push_back( 20 );

            std::vector< AssociationGroup > groups;
            groups.push_back( std::move( group ) );

            device.channelsToGroups.push_back(std::move(groups));
        }


        addDevice( std::move(device) );
    }

    {
        Device device;
        device.nodeId = 4;
        device.name = "Switch";
        device.icon = "";

        {
            AssociationGroup group;
            group.name = "Lifeline";
            group.maxAssociationsNumber = 10;
            group.profile = "some profile";
            group.associations.push_back( { 0, 0 } );
            group.commands.push_back( 60 );

            std::vector< AssociationGroup > groups;
            groups.push_back( std::move( group ) );

            device.channelsToGroups.push_back(std::move(groups));
        }

        {
            Item item;
            item.name = "switch";
            item.references.push_back({ 0, "switch" });
            item.references.push_back({ 0, "basic" });

            device.items.push_back(item);
        }

        addDevice( std::move(device) );
    }
}

const std::vector<DevicesModel::Device>& DevicesModel::getDevices() const {
    return m_devices;
}

void DevicesModel::addDevice(Device device) {
    if ( device.nodeId == 0 ) {
        while (findDeviceByNode(++device.nodeId));
    }

    m_devices.push_back( std::move( device ) );
}

void DevicesModel::removeAssociation(size_t deviceIndex, size_t channelIndex, size_t groupIndex, Association association) {
    if ( m_devices.size() <= deviceIndex )
        return;

    if ( m_devices[deviceIndex].channelsToGroups.size() <= channelIndex )
        return;

    if ( m_devices[deviceIndex].channelsToGroups[channelIndex].size() <= groupIndex )
        return;

    auto& associations = m_devices[deviceIndex].channelsToGroups[channelIndex][groupIndex].associations;

    associations.erase(std::find(associations.begin(), associations.end(), association));
}

void DevicesModel::addAssociation(size_t deviceIndex, size_t channelIndex, size_t groupIndex, Association association) {
    if ( m_devices.size() <= deviceIndex )
        return;

    if ( m_devices[deviceIndex].channelsToGroups.size() <= channelIndex )
        return;

    if ( m_devices[deviceIndex].channelsToGroups[channelIndex].size() <= groupIndex )
        return;

    if ( m_devices[deviceIndex].channelsToGroups[channelIndex][groupIndex].associations.size() >=
         m_devices[deviceIndex].channelsToGroups[channelIndex][groupIndex].maxAssociationsNumber )
        return;


    m_devices[deviceIndex].channelsToGroups[channelIndex][groupIndex].associations.push_back( association );
}

const DevicesModel::Device* DevicesModel::findDeviceByNode(size_t nodeIndex) const {
    auto it = std::find_if( m_devices.begin(), m_devices.end(), [nodeIndex](const Device& device) {
        return device.nodeId == nodeIndex;
    } );

    return it == m_devices.end() ? nullptr : &*it;
}
