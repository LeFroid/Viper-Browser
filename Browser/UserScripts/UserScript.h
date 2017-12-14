#ifndef USERSCRIPT_H
#define USERSCRIPT_H

#include <vector>
#include <QRegularExpression>
#include <QString>

/**
 * @class UserScript
 * @brief An object storing a single GreaseMonkey-style user script, including
 *        its metadata and the formatted JavaScript code.
 */
class UserScript
{
    friend class UserScriptManager;

public:
    /// Default constructor
    UserScript() = default;

protected:
    /// Attempts to load and parse the user script file, given the user script template.
    /// Returns true on success, false on failure
    bool load(const QString &file, const QString &templateData);

private:
    /// Converts the include or exclude rule into a QRegularExpression
    QRegularExpression getRegExp(const QString &str);

    /// Converts the match rule into a QRegularExpression
    QRegularExpression getMatchRegExp(const QString &str);

    /// Converts the extracted script metadata into a JSON object
    QString getScriptJSON() const;

protected:
    /// Name of the user script
    QString m_name;

    /// Used in combination with the user script name in order to form a unique identifier
    QString m_namespace;

    /// A brief summary of what the script does
    QString m_description;

    /// The version of the user script
    QString m_version;

    /// When set to true, the script will run only in the top-level document, never in nested frames.
    bool m_noSubFrames;

    /// Container of url include and matching rules, where the script will be injected
    std::vector<QRegularExpression> m_includes;

    /// Container of url excluding rules, where the script will never be injected
    std::vector<QRegularExpression> m_excludes;

    /// JavaScript dependencies
    std::vector<QString> m_dependencies;

    /// The processed version of the user script
    QString m_scriptData;

    /// Stores script dependencies in a data buffer, which will be injected onto target sites before the user script
    QByteArray m_dependencyData;
};

#endif // USERSCRIPT_H
