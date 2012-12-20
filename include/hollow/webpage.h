// -*- Mode: c++; tab-width: 2 -*-

#ifndef HOLLOW_WEBVIEW_H
#define HOLLOW_WEBVIEW_H

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QWebPage>
#include <QWebSettings>
#include <QBuffer>
#include <QImage>
#include <QPainter>

#include <hollow/core.h>
#include <hollow/networkaccessmanager.h>
#include <hollow/jserror.h>
#include <hollow/response.h>


// Statically allocating the data needed to create the default
// configuration passed to a web page
static ConfigPair map_data[] = {
  std::make_pair("cache", false),
  std::make_pair("screenshot", false),
};


static Config defaultConfig(map_data,
                            map_data +
                            sizeof(map_data) / sizeof(map_data[0]));

static UsernamePasswordPair emptyCredentials = std::make_pair("", "");

class WebPage : public QWebPage
{
  Q_OBJECT

public:
  WebPage(QObject *parent=0, UsernamePasswordPair& credentials=emptyCredentials, Config& config=defaultConfig);
  Response *lastResponse();
  bool finished();
  bool hasErrors();
  QImage renderImage();
  QByteArray renderPNGBase64();
  void setAuthUsername(std::string& username);
  std::string getAuthUsername(void);
  void setAuthPassword(std::string& password);
  std::string getAuthPassword(void);

public slots:
  void setJSReady();

private slots:
  void handleLoadFinished(bool);
  void handleResourceRequested(const QNetworkRequest&);
  void handleNetworkReplies(QNetworkReply *);
  bool shouldInterruptJavaScript();
  void prepareJS();
  void handleAuthentication(QNetworkReply*, QAuthenticator*);

private:
  bool m_hasErrors;
  bool m_shouldWaitForJS;
  bool m_jsReady;
  bool m_loadFinished;
  Response *m_lastResponse;
  JSErrorList m_js_errors;
  StringList m_requestedResources;
  NetworkAccessManager *m_networkAccessManager;
  Config m_config;
  std::string m_authUsername;
  std::string m_authPassword;
  int m_authAttempts;
  Response *buildResponseFromNetworkReply(QNetworkReply *reply, utimestamp when);
  void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID);
};

#endif  // HOLLOW_WEBVIEW_H
