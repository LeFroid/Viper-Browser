#include "UserScript.h"

#include <QFile>
#include <QTextStream>

UserScript::UserScript() :
    m_name(),
    m_namespace(),
    m_description(),
    m_version(),
    m_noSubFrames(false),
    m_isEnabled(true),
    m_injectionTime(ScriptInjectionTime::DocumentEnd),
    m_includes(),
    m_excludes(),
    m_dependencies(),
    m_scriptData(),
    m_dependencyData()
{
}

const QString &UserScript::getName() const
{
    return m_name;
}

const QString &UserScript::getDescription() const
{
    return m_description;
}

const QString &UserScript::getVersion() const
{
    return m_version;
}

bool UserScript::isEnabledOnSubFrames() const
{
    return !m_noSubFrames;
}

ScriptInjectionTime UserScript::getInjectionTime() const
{
    return m_injectionTime;
}

bool UserScript::isEnabled() const
{
    return m_isEnabled;
}

void UserScript::setEnabled(bool value)
{
    m_isEnabled = value;
}

const QByteArray &UserScript::getDependencyData() const
{
    return m_dependencyData;
}

const QString &UserScript::getScriptData() const
{
    return m_scriptData;
}

bool UserScript::load(const QString &file, const QString &templateData)
{
    QFile f(file);
    if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    m_dependencyData.clear();
    m_fileName = file;

    // Read file line by line, adding contents to local data buffer and initially parsing the metadata block
    bool foundMetaDataStart = false, foundMetaDataEnd = false;
    QRegularExpression metaDataStart("// ==UserScript=="),
            metaDataEnd("// ==/UserScript=="),
            keyValuePair("// @([\\w-]+)( +)?([^\\n]*)?");
    QByteArray metaData;
    QByteArray scriptData;

    QTextStream stream(&f);
    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        if (!foundMetaDataStart)
            foundMetaDataStart = metaDataStart.match(line).hasMatch();
        else if (!foundMetaDataEnd)
        {
            QRegularExpressionMatch keyValMatch = keyValuePair.match(line);
            if (keyValMatch.hasMatch())
            {
                QString key = keyValMatch.captured(1), value = (keyValMatch.lastCapturedIndex() > 1 ? keyValMatch.captured(3) : QString());
                if (key.compare("name") == 0)
                    m_name = value;
                else if (key.compare("namespace") == 0)
                    m_namespace = value;
                else if (key.compare("description") == 0)
                    m_description = value;
                else if (key.compare("version") == 0)
                    m_version = value;
                else if (key.compare("noframes") == 0)
                    m_noSubFrames = true;
                else if (key.compare("include") == 0)
                    m_includes.push_back(getRegExp(value));
                else if (key.compare("exclude") == 0)
                    m_excludes.push_back(getRegExp(value));
                else if (key.compare("match") == 0)
                    m_includes.push_back(getMatchRegExp(value));
                else if (key.compare("require") == 0)
                    m_dependencies.push_back(value);
                else if (key.compare("run-at") == 0)
                {
                    if (value.compare("document-end") == 0)
                        m_injectionTime = ScriptInjectionTime::DocumentEnd;
                    else if (value.compare("document-start") == 0)
                        m_injectionTime = ScriptInjectionTime::DocumentStart;
                    else if (value.compare("document-idle") == 0)
                        m_injectionTime = ScriptInjectionTime::DocumentIdle;
                }

                metaData.append(line.replace("'", "\\'"));
            }
            else
                foundMetaDataEnd = metaDataEnd.match(line).hasMatch();
        }
        else
        {
            scriptData.append(line);
            scriptData.append(QChar('\n'));
        }
    }
    f.close();

    // If there are no items in the include container, add a "match everything" rule
    if (m_includes.empty())
        m_includes.push_back(QRegularExpression(QStringLiteral(".*")));

    // Copy template file into script data member, then replace variables with user script specific data
    m_scriptData = templateData;
    m_scriptData.replace(QStringLiteral("{{SCRIPT_UUID}}"), QString("%1.%2").arg(m_namespace).arg(m_name));
    m_scriptData.replace(QStringLiteral("{{SCRIPT_OBJECT}}"), getScriptJSON());
    m_scriptData.replace(QStringLiteral("{{SCRIPT_META_STR}}"), QString(metaData));
    m_scriptData.replace(QStringLiteral("{{USER_SCRIPT}}"), QString(scriptData));

    return true;
}

QRegularExpression UserScript::getRegExp(const QString &str)
{
    // Check if string is either a single '*' or empty, and if so, set regular expression to match everything
    if (str.isEmpty() || (str.size() == 1 && str.at(0) == '*'))
        return QRegularExpression(QStringLiteral(".*"));

    QString converted;

    // Check if rule is a regular expression
    if (str.startsWith('/') && str.endsWith('/'))
    {
        converted = str.mid(1);
        converted = str.left(str.size() - 1);
        converted.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
        return QRegularExpression(converted);
    }

    // If the method has not returned at this point, replace each occurrence of a "*" with a ".*"
    converted = str;
    converted.replace(QStringLiteral("*"), QStringLiteral(".*"));
    return QRegularExpression(converted);
}

QRegularExpression UserScript::getMatchRegExp(const QString &str)
{
    QChar kleeneStar = QLatin1Char('*');
    int pos = str.indexOf("://");

    // If string is empty or does not specify scheme, return regex accepting anything with http or https scheme
    if (str.isEmpty() || pos <= 0)
        return QRegularExpression(QStringLiteral("http(s?)://"));
    else if (str.compare("<all_urls>") == 0)
        return QRegularExpression(QStringLiteral("(http(s?)|ftp|file)://.*"));

    // Get scheme in the form of a regular expression
    QString converted = str.left(pos);
    if (converted.compare(kleeneStar) == 0)
        converted = QStringLiteral("http(s?)");
    if (!converted.startsWith("http") && !converted.startsWith("ftp") && !converted.startsWith("file"))
        return QRegularExpression(QStringLiteral("http(s?)://"));
    converted.append(QStringLiteral("://"));

    // Get host in the form of a regular expression
    int pathPos = str.indexOf('/', pos + 3);
    if (pathPos < 0)
    {
        // Path character is mandatory
        return QRegularExpression(QStringLiteral("http(s?)://"));
    }
    else if (pathPos > 0)
    {
        QString host = str.left(pathPos);
        host = host.mid(pos + 3);

        int lastStarPos = host.lastIndexOf(kleeneStar);
        if (lastStarPos > 0)
            return QRegularExpression(QStringLiteral("http(s?)://"));

        if (lastStarPos == 0)
        {
            if (host.size() == 1)
                converted.append(QStringLiteral(".*/"));
            else if (host.at(1) == QLatin1Char('.'))
            {
                converted.append(".*");
                host = host.mid(1);
            }
            else
                return QRegularExpression(QStringLiteral("http(s?)://"));

        }

        converted.append(host);
    }

    converted.append(QChar('/'));

    // Get path in the form of a regular expression
    if (pathPos + 1 == str.size())
        return QRegularExpression(converted);

    QString path = str.mid(pathPos + 1);
    path.replace("*", ".*");
    converted.append(path);
    return QRegularExpression(converted);
}

QString UserScript::getScriptJSON() const
{
    QString excludes;
    for (const QRegularExpression &exp : m_excludes)
        excludes.append(QString("\"%1\",").arg(exp.pattern()));
    excludes = excludes.left(excludes.size() - 1);

    QString includes;
    for (const QRegularExpression &exp : m_includes)
        includes.append(QString("\"%1\",").arg(exp.pattern()));
    includes = includes.left(includes.size() - 1);

    QString descrFix = m_description;
    descrFix.replace("'", "\\'");
    QString nameFix = m_name;
    nameFix.replace("'", "\\'");
    QString namespaceFix = m_namespace;
    namespaceFix.replace("'", "\\'");
    QString runTime = (m_injectionTime == ScriptInjectionTime::DocumentEnd ? QStringLiteral("document-end") : QStringLiteral("document-start"));
    return QString("{ 'description': '%1', 'excludes': [ %2 ], "
    "'includes': [ %3 ], 'matches': [], 'name': '%4', "
    "'namespace': '%5', 'resources': {}, 'run-at': '%6', "
    "'version': '%7' }").arg(descrFix).arg(excludes).arg(includes).arg(nameFix).arg(namespaceFix).arg(runTime).arg(m_version);
}
