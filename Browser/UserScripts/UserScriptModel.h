#ifndef USERSCRIPTMODEL_H
#define USERSCRIPTMODEL_H

#include "Settings.h"
#include "UserScript.h"
#include <memory>
#include <vector>
#include <QAbstractTableModel>

/**
 * @class UserScriptModel
 * @brief Acts as an abstraction to allow for storage, viewing and modification of user scripts
 */
class UserScriptModel : public QAbstractTableModel
{
    friend class UserScriptManager;

    Q_OBJECT

public:
    /// Constructs the user script model with the given parent
    explicit UserScriptModel(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    /// Saves user script information before destruction of the model
    virtual ~UserScriptModel();

    /// Adds the given user script to the collection
    void addScript(const UserScript &script);

    /// Adds the given user script to the collection
    void addScript(UserScript &&script);

    /// Returns the file name of the user script at the given index
    QString getScriptFileName(int indexRow) const;

    /// Returns the source code of the user script at the given index. If index is invalid, an empty string is returned
    QString getScriptSource(int indexRow);

    /// Displays header information for the given section, header orientation and the header role
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /// Returns the number of rows available
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /// Returns the data stored at the given index
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /// Allow user to modify first column (enabled/disabled checkbox)
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    /// Sets the data at the given index.
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

public slots:
    /// Reloads the script at the given index in the script container
    void reloadScript(int indexRow);

private:
    /// Loads user scripts from the script directory
    void load();

    /// Loads dependencies for the user script in the container at the given index
    void loadDependencies(int scriptIdx);

    /// Saves user script information to a user script configuration file
    void save();

protected:
    /// Container used to store user scripts
    std::vector<UserScript> m_scripts;

    /// User script template file contents
    QString m_scriptTemplate;

    /// Absolute path to user script dependency directory
    QString m_scriptDepDir;

    /// True if scripts are enabled, false if else
    bool m_enabled;

private:
    /// Application settings
    std::shared_ptr<Settings> m_settings;
};

#endif // USERSCRIPTMODEL_H
