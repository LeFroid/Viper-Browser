#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QString>
#include <vector>

class MainWindow;

/**
 * @class SessionManager
 * @brief Handles the serialization of active browsing sessions when the application is being closed, and the
 *        deserialization of a saved browsing session when the application is being initialized
 */
class SessionManager
{
public:
    /// Default constructor
    SessionManager() = default;

    /// Returns true if the session has already been saved, false if else
    bool alreadySaved() const;

    /// Sets the path of the data file in which session information is stored
    void setSessionFile(const QString &fullPath);

    /// Saves the state of each window in the container
    void saveState(std::vector<MainWindow*> &windows);

    /**
     * @brief restoreSession Restores the previous browsing session
     * @param firstWindow Pointer to the first browsing window, which is automatically opened when the application initializes
     */
    void restoreSession(MainWindow *firstWindow);

private:
    /// Path of the file in which session data is stored
    QString m_dataFile;

    /// True if session has already been saved, false if else
    bool m_savedSession;
};

#endif // SESSIONMANAGER_H
