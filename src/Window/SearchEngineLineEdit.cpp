#include "BrowserApplication.h"
#include "FaviconStorage.h"
#include "HttpRequest.h"
#include "SearchEngineLineEdit.h"

#include <QMenu>
#include <QResizeEvent>
#include <QToolButton>

SearchEngineLineEdit::SearchEngineLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    // Set minimum width and size policy
    setMinimumWidth(parent->width() / 5);

    QSizePolicy policy = sizePolicy();
    setSizePolicy(QSizePolicy::Preferred, policy.verticalPolicy());

    // Setup search button
    m_searchButton = new QToolButton(this);
    m_searchButton->setPopupMode(QToolButton::InstantPopup);
    m_searchButton->setCursor(Qt::ArrowCursor);
    m_searchButton->setToolTip(tr("Choose Search Engine"));
    m_searchButton->setStyleSheet("QToolButton { border: none; padding: 0px; background-position: left center; "
                                  "background-repeat: no-repeat; background-origin: content; background-image: url(\":/search_icon.png\"); }"
                                  "QToolButton::menu-indicator { subcontrol-position: bottom right; subcontrol-origin: content; top: 3px; } ");
    m_searchButton->setFixedWidth(28);

    // Set appearance
    setStyleSheet(QString("QLineEdit { border: 1px solid #666666; padding-left: %1px; margin-right: 3px; }"
                          "QLineEdit:focus { border: 1px solid #6c91ff; }").arg(m_searchButton->sizeHint().width() + 6));
    setPlaceholderText(tr("Search"));

    // Load search engines
    loadSearchEngines();

    // Set behavior of line edit
    connect(this, &SearchEngineLineEdit::returnPressed, [=](){
        QString term = text();
        if (term.isNull() || term.isEmpty())
            return;

        HttpRequest request = SearchEngineManager::instance().getSearchRequest(term, m_currentEngineName);
        emit requestPageLoad(request);
    });

    // Connect search engine manager's signals for search engines being added or removed to appropriate slots
    SearchEngineManager *manager = &SearchEngineManager::instance();
    connect(manager, &SearchEngineManager::defaultEngineChanged, this, &SearchEngineLineEdit::setSearchEngine);
    connect(manager, &SearchEngineManager::engineAdded, this, &SearchEngineLineEdit::addSearchEngine);
    connect(manager, &SearchEngineManager::engineRemoved, this, &SearchEngineLineEdit::removeSearchEngine);
}

void SearchEngineLineEdit::loadSearchEngines()
{
    m_searchEngineMenu = new QMenu(this);

    FaviconStorage *faviconStore = sBrowserApplication->getFaviconStorage();
    SearchEngineManager &manager = SearchEngineManager::instance();
    QList<QString> searchEngines = manager.getSearchEngineNames();

    // Add search engines to the options menu
    for (auto engineName : searchEngines)
    {
        QIcon menuIcon = faviconStore->getFavicon(QUrl(manager.getQueryString(engineName)));

        // Add search engine to the options menu
        QAction *action = m_searchEngineMenu->addAction(menuIcon, engineName);
        connect(action, &QAction::triggered, [=]() {
            setSearchEngine(action->text());
        });
    }

    m_searchButton->setMenu(m_searchEngineMenu);

    // Set current engine to the default setting
    setSearchEngine(manager.getDefaultSearchEngine());
}

void SearchEngineLineEdit::setSearchEngine(const QString &name)
{
    SearchEngine engine = SearchEngineManager::instance().getSearchEngineInfo(name);
    if (!engine.QueryString.isNull())
    {
        m_currentEngineName = name;
        m_currentEngine = engine;
        setPlaceholderText(tr("Search %1").arg(name));
    }
}

void SearchEngineLineEdit::addSearchEngine(const QString &name)
{
    FaviconStorage *faviconStore = sBrowserApplication->getFaviconStorage();
    QIcon menuIcon = faviconStore->getFavicon(QUrl(SearchEngineManager::instance().getQueryString(name)));

    // Add search engine to the options menu
    QAction *action = m_searchEngineMenu->addAction(menuIcon, name);
    connect(action, &QAction::triggered, [=]() {
        setSearchEngine(action->text());
    });
}

void SearchEngineLineEdit::removeSearchEngine(const QString &name)
{
    // Find the item in the menu, and its index so it may be removed
    QList<QAction*> menuActions = m_searchEngineMenu->actions();
    for (QAction *currAction : menuActions)
    {
        if (currAction->text().compare(name) == 0)
        {
            m_searchEngineMenu->removeAction(currAction);
            break;
        }
    }

    // Check if the current search engine is the one that was just removed, and if so change
    // the current engine to the search engine manager's default
    if (m_currentEngineName.compare(name) == 0)
        setSearchEngine(SearchEngineManager::instance().getDefaultSearchEngine());
}

void SearchEngineLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);

    QSize sz = m_searchButton->sizeHint();
    const QRect widgetRect = rect();
    m_searchButton->move(widgetRect.left(), (widgetRect.bottom() + 1 - sz.height()) / 2);
}
