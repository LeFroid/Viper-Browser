#-------------------------------------------------
#
# Project created by QtCreator 2017-08-06T15:51:56
#
#-------------------------------------------------

QT       += core concurrent gui sql svg printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += quickwidgets widgets webenginewidgets

TARGET = viper-browser
TEMPLATE = app
DESTDIR = $$VIPER_BUILD_DIR

QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS_RELEASE += -O2

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RESOURCES  = application.qrc
QTQUICK_COMPILER_SKIPPED_RESOURCES += application.qrc

top_srcdir=./

INCLUDEPATH += \
    $$PWD/AdBlock \
    $$PWD/AutoFill \
    $$PWD/Bookmarks \
    $$PWD/Cache \
    $$PWD/Cookies \
    $$PWD/Credentials \
    $$PWD/Downloads \
    $$PWD/Extensions \
    $$PWD/History \
    $$PWD/Highlighters \
    $$PWD/Network \
    $$PWD/Preferences \
    $$PWD/URLSuggestion \
    $$PWD/UserAgents \
    $$PWD/UserScripts \
    $$PWD/Web \
    $$PWD/Widgets \
    $$PWD/Window

SOURCES += \
    main.cpp \
    AdBlock/AdBlockManager.cpp \
    AdBlock/AdBlockModel.cpp \
    History/HistoryTableModel.cpp \
    Cookies/CookieJar.cpp \
    Cookies/CookieModifyDialog.cpp \
    Cookies/CookieTableModel.cpp \
    Cookies/CookieWidget.cpp \
    Cookies/DetailedCookieTableModel.cpp \
    History/ClearHistoryDialog.cpp \
    History/HistoryManager.cpp \
    History/HistoryWidget.cpp \
    BrowserApplication.cpp \
    DatabaseWorker.cpp \
    FaviconStorage.cpp \
    MainWindow.cpp \
    Bookmarks/BookmarkFolderModel.cpp \
    Bookmarks/BookmarkManager.cpp \
    Bookmarks/BookmarkTableModel.cpp \
    Bookmarks/BookmarkWidget.cpp \
    Downloads/DownloadItem.cpp \
    Downloads/InternalDownloadItem.cpp \
    Downloads/DownloadManager.cpp \
    Widgets/FindTextWidget.cpp \
    Web/WebActionProxy.cpp \
    Web/WebDialog.cpp \
    Web/WebPage.cpp \
    Web/WebView.cpp \
    Network/NetworkAccessManager.cpp \
    Network/RequestInterceptor.cpp \
    UserAgents/UserAgentManager.cpp \
    UserAgents/AddUserAgentDialog.cpp \
    Network/SecurityManager.cpp \
    Network/SecurityInfoDialog.cpp \
    Network/CertificateViewer.cpp \
    Preferences/Preferences.cpp \
    Preferences/GeneralTab.cpp \
    Settings.cpp \
    Bookmarks/BookmarkExporter.cpp \
    Bookmarks/BookmarkImporter.cpp \
    SearchEngineManager.cpp \
    Preferences/SearchTab.cpp \
    Preferences/AddSearchEngineDialog.cpp \
    Widgets/BookmarkBar.cpp \
    Window/BrowserTabWidget.cpp \
    Widgets/FileChooserLineEdit.cpp \
    Window/SearchEngineLineEdit.cpp \
    Window/URLLineEdit.cpp \
    Network/CertificateGeneralTab.cpp \
    Preferences/ContentTab.cpp \
    Highlighters/HTMLHighlighter.cpp \
    Window/BrowserTabBar.cpp \
    Bookmarks/BookmarkNode.cpp \
    UserAgents/UserAgentsWindow.cpp \
    SessionManager.cpp \
    Network/ViperNetworkReply.cpp \
    UserScripts/UserScript.cpp \
    UserScripts/UserScriptManager.cpp \
    UserScripts/UserScriptWidget.cpp \
    UserScripts/UserScriptModel.cpp \
    Highlighters/JavaScriptHighlighter.cpp \
    Widgets/CodeEditor.cpp \
    Extensions/ExtStorage.cpp \
    UserScripts/UserScriptEditor.cpp \
    AdBlock/AdBlockWidget.cpp \
    Widgets/CheckableTableView.cpp \
    UserScripts/AddUserScriptDialog.cpp \
    History/HistoryMenu.cpp \
    UserAgents/UserAgentMenu.cpp \
    Bookmarks/BookmarkMenu.cpp \
    AdBlock/AdBlockSubscribeDialog.cpp \
    AdBlock/CustomFilterEditor.cpp \
    Widgets/BookmarkDialog.cpp \
    Network/ViperSchemeHandler.cpp \
    UserScripts/WebEngineScriptAdapter.cpp \
    Preferences/PrivacyTab.cpp \
    Preferences/ExemptThirdPartyCookieDialog.cpp \
    Web/WebHitTestResult.cpp \
    Window/NavigationToolBar.cpp \
    URLSuggestion/URLSuggestionWidget.cpp \
    URLSuggestion/URLSuggestionItemDelegate.cpp \
    Network/BlockedSchemeHandler.cpp \
    URLSuggestion/URLSuggestionListModel.cpp \
    URLSuggestion/URLSuggestionWorker.cpp \
    Web/WebWidget.cpp \
    AdBlock/AdBlockButton.cpp \
    CommonUtil.cpp \
    Network/AuthDialog.cpp \
    Network/HttpRequest.cpp \
    Credentials/CredentialStore.cpp \
    AutoFill/AutoFill.cpp \
    AutoFill/AutoFillBridge.cpp

HEADERS += \
    TreeNode.h \
    AdBlock/AdBlockModel.h \
    Cache/LRUCache.h \
    History/HistoryTableModel.h \
    Cookies/CookieJar.h \
    Cookies/CookieModifyDialog.h \
    Cookies/CookieTableModel.h \
    Cookies/CookieWidget.h \
    Cookies/DetailedCookieTableModel.h \
    History/ClearHistoryDialog.h \
    History/ClearHistoryOptions.h \
    History/HistoryManager.h \
    History/HistoryWidget.h \
    BrowserApplication.h \
    DatabaseWorker.h \
    FaviconStorage.h \
    MainWindow.h \
    Bookmarks/BookmarkFolderModel.h \
    Bookmarks/BookmarkManager.h \
    Bookmarks/BookmarkTableModel.h \
    Bookmarks/BookmarkWidget.h \
    Downloads/DownloadItem.h \
    Downloads/InternalDownloadItem.h \
    Downloads/DownloadManager.h \
    Widgets/FindTextWidget.h \
    Web/WebActionProxy.h \
    Web/WebDialog.h \
    Web/WebPage.h \
    Web/WebView.h \
    Network/NetworkAccessManager.h \
    Network/RequestInterceptor.h \
    UserAgents/UserAgentManager.h \
    UserAgents/AddUserAgentDialog.h \
    Network/SecurityManager.h \
    Network/SecurityInfoDialog.h \
    Network/CertificateViewer.h \
    Preferences/Preferences.h \
    Preferences/GeneralTab.h \
    Settings.h \
    Bookmarks/BookmarkExporter.h \
    Bookmarks/BookmarkImporter.h \
    SearchEngineManager.h \
    Preferences/SearchTab.h \
    Preferences/AddSearchEngineDialog.h \
    Widgets/BookmarkBar.h \
    Window/BrowserTabWidget.h \
    Widgets/FileChooserLineEdit.h \
    Window/SearchEngineLineEdit.h \
    Window/URLLineEdit.h \
    Network/CertificateGeneralTab.h \
    Preferences/ContentTab.h \
    Highlighters/HTMLHighlighter.h \
    Window/BrowserTabBar.h \
    Bookmarks/BookmarkNode.h \
    UserAgents/UserAgentsWindow.h \
    SessionManager.h \
    Network/ViperNetworkReply.h \
    UserScripts/UserScript.h \
    UserScripts/UserScriptManager.h \
    UserScripts/UserScriptWidget.h \
    UserScripts/UserScriptModel.h \
    Highlighters/JavaScriptHighlighter.h \
    Widgets/CodeEditor.h \
    Extensions/ExtStorage.h \
    UserScripts/UserScriptEditor.h \
    AdBlock/AdBlockWidget.h \
    Widgets/CheckableTableView.h \
    UserScripts/AddUserScriptDialog.h \
    DatabaseFactory.h \
    History/HistoryMenu.h \
    UserAgents/UserAgentMenu.h \
    Bookmarks/BookmarkMenu.h \
    AdBlock/AdBlockSubscribeDialog.h \
    AdBlock/CustomFilterEditor.h \
    Widgets/BookmarkDialog.h \
    Network/ViperSchemeHandler.h \
    UserScripts/WebEngineScriptAdapter.h \
    Preferences/PrivacyTab.h \
    Preferences/ExemptThirdPartyCookieDialog.h \
    Web/WebHitTestResult.h \
    Window/NavigationToolBar.h \
    URLSuggestion/URLSuggestionWidget.h \
    URLSuggestion/URLSuggestionItemDelegate.h \
    Network/BlockedSchemeHandler.h \
    URLSuggestion/URLSuggestionListModel.h \
    URLSuggestion/URLSuggestionWorker.h \
    Web/WebWidget.h \
    AdBlock/AdBlockButton.h \
    CommonUtil.h \
    Network/AuthDialog.h \
    Network/HttpRequest.h \
    Credentials/CredentialStore.h \
    Credentials/CredentialStoreImpl.h \
    AutoFill/AutoFill.h \
    AutoFill/AutoFillBridge.h

FORMS += \
    Cookies/CookieWidget.ui \
    Cookies/CookieModifyDialog.ui \
    History/ClearHistoryDialog.ui \
    History/HistoryWidget.ui \
    Bookmarks/BookmarkWidget.ui \
    Downloads/DownloadManager.ui \
    Downloads/DownloadItem.ui \
    Widgets/FindTextWidget.ui \
    UserAgents/AddUserAgentDialog.ui \
    Network/SecurityInfoDialog.ui \
    Network/CertificateViewer.ui \
    Preferences/Preferences.ui \
    Preferences/GeneralTab.ui \
    Preferences/SearchTab.ui \
    Preferences/AddSearchEngineDialog.ui \
    Network/CertificateGeneralTab.ui \
    Preferences/ContentTab.ui \
    UserAgents/UserAgentsWindow.ui \
    UserScripts/UserScriptWidget.ui \
    UserScripts/UserScriptEditor.ui \
    AdBlock/AdBlockWidget.ui \
    UserScripts/AddUserScriptDialog.ui \
    AdBlock/AdBlockSubscribeDialog.ui \
    AdBlock/CustomFilterEditor.ui \
    Widgets/BookmarkDialog.ui \
    Preferences/PrivacyTab.ui \
    Preferences/ExemptThirdPartyCookieDialog.ui \
    MainWindow.ui \
    Network/AuthDialog.ui

packagesExist(KWallet) {
    message("Using KWallet for secure storage")
    QT += KWallet
    DEFINES += USE_KWALLET
    SOURCES += Credentials/CredentialStoreKWallet.cpp
    HEADERS += Credentials/CredentialStoreKWallet.h
}

include(Browser.pri)
