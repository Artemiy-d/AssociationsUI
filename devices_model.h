#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <map>

class DevicesModel
{
public:
    struct ZWaveReference {
        size_t channelIndex;
        std::string cc;
    };

    struct Item {
        std::string name;

        std::vector< ZWaveReference > references;
    };

    struct Association {
        size_t deviceIndex;
        std::optional<size_t> channelIndex;

        bool operator == (const Association& other) const {
            return deviceIndex == other.deviceIndex && channelIndex == other.channelIndex;
        }
    };

    struct AssociationGroup {
        std::string name;
        uint8_t maxAssociationsNumber;
        std::string profile;
        std::vector<Association> associations;
        std::vector<size_t> commands;
    };


    struct SubDeivice {
        std::string name;
        std::string icon;
        std::vector<Item> items;
    };

    struct Device : SubDeivice {
          size_t nodeId = 0;
          std::vector<std::vector<AssociationGroup>> channelsToGroups;
          std::vector<SubDeivice> children;
    };

    DevicesModel();

    const std::vector<Device>& getDevices() const;

    void addDevice(Device device);

    void removeAssociation(size_t deviceIndex, size_t channelIndex, size_t groupIndex, Association association);

    void addAssociation(size_t deviceIndex, size_t channelIndex, size_t groupIndex, Association association);

    const Device* findDeviceByNode(size_t nodeIndex) const;

private:

    std::vector<Device> m_devices;
};


