#include <iostream>
#include <QObject>
#include <QWebFrame>
#include <QNetworkRequest>
#include <QApplication>

#include <hollow/hollow.h>
#include <hollow/error.h>
#include <hollow/webpage.h>
#include <hollow/response.h>
#include <QRegExp>
#include "config.h"

// Mocking the values to pass to QApplication
static int argc = 1;
static char *argv[] = { (char *) "sleepy-hollow", 0 };
static QApplication *app;

Hollow::Hollow(QObject *parent)
  : QObject(parent)
{ }

Hollow::~Hollow()
{ }

void
Hollow::setup(void)
{
  app = new QApplication(argc, argv);

  // Setting some cool parameters in our app!
  app->setApplicationName(QString(PACKAGE_NAME));
  app->setApplicationVersion(QString(PACKAGE_VERSION));
}

void
Hollow::teardown(void)
{
  delete app;
  app = 0;
}

Response *
Hollow::request (const char* method,
                 const char* url,
                 const char* payload,
                 StringHashMap& headers,
                 UsernamePasswordPair& credentials,
                 Config& config)
{
  QString operation(method);
  QNetworkRequest request;

  // First of all, let's see if this url is valid and contains a valid
  // scheme
  QUrl qurl(QString::fromStdString(url));
  if (!qurl.isValid() || qurl.scheme().isEmpty()) {
    QString qerr = qurl.errorString();
    QString err("The url \"%1\" is not valid: %2");
    err = err.arg(url, (!qerr.isEmpty() ? qerr : "You need to inform a scheme"));

    // Reporting the error
    Error::set(Error::INVALID_URL, err.toUtf8().data());
    return NULL;
  }

  qurl.setUserName(QString::fromStdString(credentials.first));
  qurl.setPassword(QString::fromStdString(credentials.second));

  // setting up the page and connecting it's loadFinished signal to our
  // exit function
  WebPage page(this, config);
  page.triggerAction(QWebPage::Stop);

  QNetworkAccessManager::Operation networkOp = QNetworkAccessManager::UnknownOperation;

  operation = operation.toLower();
  if (operation == "get") {
    networkOp = QNetworkAccessManager::GetOperation;
  } else if (operation == "head") {
    networkOp = QNetworkAccessManager::HeadOperation;
  } else if (operation == "put") {
    networkOp = QNetworkAccessManager::PutOperation;
  } else if (operation == "post") {
    networkOp = QNetworkAccessManager::PostOperation;
  } else if (operation == "delete") {
    networkOp = QNetworkAccessManager::DeleteOperation;
  } else {
    Error::set(Error::INVALID_METHOD, operation.toUtf8().data());
    return NULL;
  }

  // setting the request headers coming from the python layer
  StringHashMapIterator headerIter;
  for (headerIter = headers.begin(); headerIter != headers.end(); headerIter++)
    request.setRawHeader(headerIter->first.c_str(), headerIter->second.c_str());

  // Setting the payload
  QByteArray body(payload);
  request.setUrl(qurl);

  page.mainFrame()->load(request, networkOp, body);
  page.setViewportSize(page.mainFrame()->contentsSize());


  // Mainloop
  while (!page.finished()) {
    app->processEvents();
    SleeperThread::msleep(0.01);
  }

  // if (page.mainFrame()->toHtml().contains(QRegExp("meta.*Refresh.*content[=]", Qt::CaseInsensitive))) {
  //   page.triggerAction(QWebPage::Reload);
  //   while (!page.finished()) {
  //     app->processEvents();
  //     SleeperThread::msleep(0.01);
  //   }
  // }

  if (page.hasErrors()) {
    // The error was properly set in the WebPage::handleNetworkReplies()
    // method, so we just need to return NULL to notify the caller that
    // something didn't work.
    return NULL;
  } else {
    // Yay! Let's return the response object created by the webpage
    // after receiving a network reply.
    return page.lastResponse();
  }
}
