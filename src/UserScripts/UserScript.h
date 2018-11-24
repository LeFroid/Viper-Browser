#ifndef USERSCRIPT_H
#define USERSCRIPT_H

#include <vector>
#include <QRegularExpression>
#include <QString>

/// List of time periods in which a user script may be injected onto a page
enum class ScriptInjectionTime
{
    DocumentEnd,
    DocumentStart,
    DocumentIdle
};

/**
 * @class UserScript
 * @brief An object storing a single GreaseMonkey-style user script, including
 *        its metadata and the formatted JavaScript code.
 */
class UserScript
{
    friend class UserScriptManager;
    friend class UserScriptModel;

public:
    /// Default constructor
    UserScript();

    /// Returns the name of the user script
    const QString &getName() const;

    /// Returns the description of the user script
    const QString &getDescription() const;

    /// Returns the version string of the user script
    const QString &getVersion() const;

    /// Returns true if the script is enabled to run on subframes, false if else
    bool isEnabledOnSubFrames() const;

    /// Returns the injection time of the script
    ScriptInjectionTime getInjectionTime() const;

    /// Returns true if the script is enabled, false if else
    bool isEnabled() const;

    /// Sets the state of the user script. If value is true, script will be enabled, otherwise script will be disabled
    void setEnabled(bool value);

    /// Returns a QByteArray containing the JavaScript dependencies of the user script, concatenated together
    const QByteArray &getDependencyData() const;

    /// Returns the user script in string form
    const QString &getScriptData() const;

protected:
    /// Attempts to load and parse the user script file, given the user script template.
    /// Returns true on success, false on failure
    bool load(const QString &file, const QString &templateData);

private:
    /// Converts the include or exclude rule into a QRegularExpression
    QRegularExpression getRegExp(const QString &str);

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

    /// Name of the locally stored file from which this script was loaded and/or saved into
    QString m_fileName;

    /// When set to true, the script will run only in the top-level document, never in nested frames.
    bool m_noSubFrames;

    /// When set to true, user script will be injected as per include/exclude rules. If false, script will not be used
    bool m_isEnabled;

    /// When the script will be injected onto a page
    ScriptInjectionTime m_injectionTime;

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
