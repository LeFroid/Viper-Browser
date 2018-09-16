#ifndef AUTOFILL_H
#define AUTOFILL_H

#include "CredentialStore.h"

#include <memory>

#include <QByteArray>
#include <QObject>
#include <QString>

/**
 * @class AutoFill
 * @brief Handles automatic filling of form data that the user allows
 *        the system to manage for them.
 */
class AutoFill : public QObject
{
    Q_OBJECT

public:
    /// AutoFill constructor
    explicit AutoFill(QObject *parent = nullptr);

    /// Destructor
    ~AutoFill();

    /**
     * @brief Called by \ref AutoFillBridge when a form was submitted on a \ref WebPage
     * @param pageUrl The URL of the page containing the form
     * @param username The username field of the form
     * @param password The password entered by the user
     * @param formData All of the information included in the form, as a urlencoded byte array
     */
    void onFormSubmitted(const QString &pageUrl, const QString &username, const QString &password, const QByteArray &formData);

private:
    /// Credential storage system
    std::unique_ptr<CredentialStore> m_credentialStore;
};


#endif // AUTOFILL_H
