#ifndef VIPERNETWORKREPLY_H
#define VIPERNETWORKREPLY_H

#include <QNetworkReply>
#include <QNetworkRequest>

/**
 * @class ViperNetworkReply
 * @brief Handles the viper:// scheme, which is in itself a simple wrapper for
 *        qrc files
 */
class ViperNetworkReply : public QNetworkReply
{
    Q_OBJECT
public:
    /// Constructs the network reply, given the original request and an optional parent
    explicit ViperNetworkReply(const QNetworkRequest &request, QObject *parent = nullptr);

    /// Unused
    void abort() override {}

    /// Returns the number of bytes available for reading
    qint64 bytesAvailable() const override;

protected:
    /**
     * @brief readData Reads up to maxSize bytes of data into the buffer
     * @param data The data buffer
     * @param maxSize The size constraint of bytes to be read into the data buffer
     * @return The number of bytes read into data
     */
    qint64 readData(char *data, qint64 maxSize) override;

private slots:
    /// Called when the reply is finished due to a read error
    void errorFinished();

private:
    /// Attempts to load the resource associated with the request
    void loadFile();

private:
    /// The data buffer
    QByteArray m_data;

    /// Current offset into the data buffer
    qint64 m_offset;
};

#endif // VIPERNETWORKREPLY_H
