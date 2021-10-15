#pragma once
#include <QWidget>

#include "devices_model.h"
#include <optional>
#include <memory>

class QComboBox;
class QListView;

class AssociationsWizard : public QWidget
{
    struct FiltersUpdater {};

    Q_OBJECT
public:
    explicit AssociationsWizard(DevicesModel& model, size_t index, std::optional<size_t> subIndex, QWidget *parent = nullptr);


signals:
    void backClicked();

    void groupsWizardRequested(size_t, size_t, size_t);

private:

    void updateSourceNodeCombo(std::optional<size_t> currentIndex);

    void updateSourceChannelsCombo();

    void updateSourceGroupsCombo();


    void updateTargetNodeCombo();

    void updateTargetChannelCombo();

    void invalidateFilters();

    void updateFilters();

    //std::shared_ptr<FiltersUpdater> createFiltersUpdater

private:
    DevicesModel& m_model;

    QComboBox* m_sourceNodeCombo = nullptr;
    QComboBox* m_sourceChannelCombo = nullptr;
    QComboBox* m_sourceGroupCombo = nullptr;

    QComboBox* m_targetNodeCombo = nullptr;
    QComboBox* m_targetChannelCombo = nullptr;

    QListView* m_existingAssociationsView = nullptr;
    QListView* m_hintAssociationsView = nullptr;

    bool m_filtersInvalidated = false;
};


