// This source code file was last time modified by Igor UA3DJY on September 24th, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.
#include "logqso.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QString>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMessageBox>

#include "logbook/adif.h"
#include "Configuration.hpp"
#include "Bands.hpp"

#include "ui_logqso.h"
#include "moc_logqso.cpp"

LogQSO::LogQSO(QString const& programTitle, QSettings * settings
              , Configuration const * config, QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::LogQSO)
  , m_settings (settings)
  , m_config {config}
{
  ui->setupUi(this);
  setWindowTitle(programTitle + " - Log QSO");
  loadSettings ();
}

LogQSO::~LogQSO ()
{
}

void LogQSO::loadSettings ()
{
  m_settings->beginGroup ("LogQSO");
  restoreGeometry (m_settings->value ("geometry", saveGeometry ()).toByteArray ());
  ui->cbTxPower->setChecked (m_settings->value ("SaveTxPower", false).toBool ());
  ui->cbComments->setChecked (m_settings->value ("SaveComments", false).toBool ());
  ui->cbEqslComments->setChecked (m_settings->value ("SaveEQSLComments", false).toBool ());
  m_txPower = m_settings->value ("TxPower", "").toString ();
  m_comments = m_settings->value ("LogComments", "").toString();
  m_eqslcomments = m_settings->value ("LogEQSLComments", "").toString();
  m_settings->endGroup ();
}

void LogQSO::storeSettings () const
{
  m_settings->beginGroup ("LogQSO");
  m_settings->setValue ("geometry", saveGeometry ());
  m_settings->setValue ("SaveTxPower", ui->cbTxPower->isChecked ());
  m_settings->setValue ("SaveComments", ui->cbComments->isChecked ());
  m_settings->setValue ("SaveEQSLComments", ui->cbEqslComments->isChecked ());
  m_settings->setValue ("TxPower", m_txPower);
  m_settings->setValue ("LogComments", m_comments);
  m_settings->setValue ("LogEQSLComments", m_eqslcomments);
  m_settings->endGroup ();
}

void LogQSO::initLogQSO(QString const& hisCall, QString const& hisGrid, QString mode,
                        QString const& rptSent, QString const& rptRcvd,
                        QDateTime const& dateTimeOn, QDateTime const& dateTimeOff,
                        Radio::Frequency dialFreq, bool noSuffix, bool autologging)
{
  m_send_to_eqsl=m_config->send_to_eqsl();
  ui->call->setText(hisCall);
  ui->grid->setText(hisGrid);
  ui->name->setText("");
  ui->txPower->setText("");
  ui->comments->setText("");
  ui->eqslcomments->setText("");
  ui->lab11->setVisible(m_send_to_eqsl);
  ui->eqslcomments->setVisible(m_send_to_eqsl);
  ui->cbEqslComments->setVisible(m_send_to_eqsl);
  if (ui->cbTxPower->isChecked ()) ui->txPower->setText(m_txPower);
  if (ui->cbComments->isChecked ()) ui->comments->setText(m_comments);
  if (ui->cbEqslComments->isChecked ()) ui->eqslcomments->setText(m_eqslcomments);
  if(m_config->report_in_comments()) {
    QString t=mode;
    if(rptSent!="") t+="  Sent: " + rptSent;
    if(rptRcvd!="") t+="  Rcvd: " + rptRcvd;
    ui->comments->setText(t);
  }
  if(noSuffix and mode.mid(0,3)=="JT9") mode="JT9";
  if(m_config->log_as_RTTY() and mode.mid(0,3)=="JT9") mode="RTTY";
  ui->mode->setText(mode);
  ui->sent->setText(rptSent);
  ui->rcvd->setText(rptRcvd);
  ui->start_date_time->setDateTime (dateTimeOn);
  ui->end_date_time->setDateTime (dateTimeOff);
  m_dialFreq=dialFreq;
  m_myCall=m_config->my_callsign();
  m_myGrid=m_config->my_grid();
  m_eqsl_username=m_config->eqsl_username();
  m_eqsl_passwd=m_config->eqsl_passwd();
  m_eqsl_nickname=m_config->eqsl_nickname();
  m_tcp_server_name=m_config->tcp_server_name();
  m_tcp_server_port=m_config->tcp_server_port();
  m_eqsltimer=m_config->eqsltimer();
  m_enable_tcp_connection=m_config->enable_tcp_connection();
  ui->band->setText(m_config->bands ()->find (dialFreq));

  if(!autologging) {
	 show ();
  }
  else {
	 accept();
  }
}

void LogQSO::accept()
{
  QString hisCall,hisGrid,mode,rptSent,rptRcvd,date,time,band;
  QString comments,eqslcomments,name;
  hisCall=ui->call->text();
  hisGrid=ui->grid->text();
  mode=ui->mode->text();
  rptSent=ui->sent->text();
  rptRcvd=ui->rcvd->text();
  m_dateTimeOn = ui->start_date_time->dateTime ();
  m_dateTimeOff = ui->end_date_time->dateTime ();
  band=ui->band->text();
  name=ui->name->text();
  m_txPower=ui->txPower->text();
  comments=ui->comments->text();
  m_comments=comments;
  eqslcomments=ui->eqslcomments->text();
  m_eqslcomments=eqslcomments;
  QString strDialFreq(QString::number(m_dialFreq / 1.e6,'f',6));

  //Log this QSO to ADIF file "wsjtx_log.adi"
  QString filename = "wsjtx_log.adi";  // TODO allow user to set
  ADIF adifile;
  auto adifilePath = QDir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}.absoluteFilePath ("wsjtx_log.adi");
  adifile.init(adifilePath);
  if (!adifile.addQSOToFile(hisCall,hisGrid,mode,rptSent,rptRcvd,m_dateTimeOn,m_dateTimeOff,band,comments,name,strDialFreq,m_myCall,m_myGrid,m_txPower,m_send_to_eqsl))
  {
      QMessageBox m;
      m.setText("Cannot open file \"" + adifilePath + "\".");
      m.exec();
   }

//Log this QSO to file "wsjtx.log"
  static QFile f {QDir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}.absoluteFilePath ("wsjtx.log")};
  if(!f.open(QIODevice::Text | QIODevice::Append)) {
    QMessageBox m;
    m.setText("Cannot open file \"" + f.fileName () + "\" for append:" + f.errorString ());
    m.exec();
  } else {
    QString logEntry=m_dateTimeOn.date().toString("yyyy-MMM-dd,") +
      m_dateTimeOn.time().toString("hh:mm,") + 
      m_dateTimeOff.date().toString("yyyy-MM-dd,") +
      m_dateTimeOff.time().toString("hh:mm:ss,") + hisCall + "," +
      hisGrid + "," + strDialFreq + "," + mode +
      "," + rptSent + "," + rptRcvd + "," + m_txPower +
      "," + comments + "," + name;
    QTextStream out(&f);
    out << logEntry << endl;
    f.close();
  }

//Clean up and finish logging
  Q_EMIT acceptQSO (m_dateTimeOff, hisCall, hisGrid, m_dialFreq, mode, rptSent, rptRcvd, m_txPower, comments, name, m_dateTimeOn);
  QString myadif;
  if (m_enable_tcp_connection){
    QByteArray myadif2;
    myadif="<BAND:" + QString::number(band.length()) + ">" + band;
    myadif+=" <CALL:" + QString::number(hisCall.length()) + ">" + hisCall;
    myadif+=" <FREQ:" + QString::number(strDialFreq.length()) + ">" + strDialFreq;
    myadif+=" <MODE:"  + QString::number(mode.length()) + ">" + mode;
    myadif+=" <QSO_DATE:8>" + m_dateTimeOn.date().toString("yyyyMMdd");
    myadif+=" <TIME_ON:6>" + m_dateTimeOn.time().toString("hhmmss");
    myadif+=" <RST_SENT:" + QString::number(rptSent.length()) + ">" + rptSent;
    myadif+=" <RST_RCVD:" + QString::number(rptRcvd.length()) + ">" + rptRcvd;
    myadif+=" <TX_PWR:" + QString::number(m_txPower.length()) + ">" + m_txPower;
    if (hisGrid.length() > 3) {
      myadif+=" <GRIDSQUARE:" + QString::number(hisGrid.length()) + ">" + hisGrid;
    }
    if (name.length() > 0) {
      myadif+=" <NAME:" + QString::number(name.length()) + ">" + name;
    }
    if (comments.length() > 0) {
      myadif+=" <COMMENT:" + QString::number(comments.length()) + ">" + comments;
    }
    if (m_send_to_eqsl) {
      myadif+=" <EQSL_QSL_SENT:1>Y <EQSL_QSLSDATE:8>" + date;
    }
    myadif+=" <EOR> ";
    myadif="<command:3>Log <parameters:" + QString::number(myadif.length()) + "> " + myadif;
    myadif2 = myadif.toUtf8();
    QTcpSocket socket;
    socket.connectToHost(m_tcp_server_name, m_tcp_server_port);
    if (socket.waitForConnected(1000)) {
      socket.write(myadif2);
      if (socket.waitForReadyRead(1000)){
        myadif2 = socket.readAll();
        if (myadif2.left(3) == "NAK") {
          QMessageBox::critical(0, "Critical",myadif2 + " QSO data rejected by external software");
        }
      }  
      socket.close();
    } else {
      QMessageBox::critical(0, "Critical", socket.errorString());
    }
  }
  if (m_send_to_eqsl) {
    myadif="<ADIF_VER:5>2.1.9";
    myadif+="<EQSL_USER:" + QString::number(m_eqsl_username.length()) + ">" + m_eqsl_username;
    myadif+="<EQSL_PSWD:" + QString::number(m_eqsl_passwd.length()) + ">" + m_eqsl_passwd;
    myadif+="<PROGRAMID:4>JTDX<EOH><APP_EQSL_QTH_NICKNAME:" + QString::number(m_eqsl_nickname.length()) + ">" + m_eqsl_nickname;
    myadif+="<CALL:" + QString::number(hisCall.length()) + ">" + hisCall;
    myadif+="<MODE:"  + QString::number(mode.length()) + ">" + mode;
    myadif+="<QSO_DATE:8>" + m_dateTimeOn.date().toString("yyyyMMdd");;
    myadif+="<TIME_ON:4>" + m_dateTimeOn.time().toString("hhmm");;
    myadif+="<RST_SENT:" + QString::number(rptSent.length()) + ">" + rptSent;
    myadif+="<BAND:" + QString::number(band.length()) + ">" + band;
    if(eqslcomments!="") myadif+="<QSLMSG:" + QString::number(eqslcomments.length()) + ">" + eqslcomments;
//    myadif+="<QSLMSG:19>TNX For QSO TU 73!.";
    myadif+="<EOR>";
    QUrl url("http://www.eqsl.cc/qslcard/importADIF.cfm");
    QUrlQuery query;
    query.addQueryItem("ADIFdata", myadif);
    url.setQuery(query.query());
    QEventLoop eventLoop;
    QTimer timer;
    timer.setSingleShot(true);
    QNetworkAccessManager mgr;
    QNetworkRequest req( url );
//    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    QNetworkReply *reply = mgr.get(req);
    QObject::connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    QObject::connect( reply, SIGNAL(finished()), &eventLoop, SLOT(quit()) );
    timer.start(m_eqsltimer*1000);
    eventLoop.exec( QEventLoop::ExcludeUserInputEvents );
    if(timer.isActive()) {
      timer.stop();

//    if (reply->error() == QNetworkReply::NoError) {
//      printf("Success : %s\n",reply->readAll().toStdString().c_str());
//    } else {
//      printf("Failure : %s\n",reply->errorString().toStdString().c_str());
//    }
    } else {
   // timeout
     QObject::disconnect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));

     reply->abort();
     QMessageBox::critical(0, "Critical", "Can not establish/complete connection to eQSL server");
    }
    delete reply;
  }
  QDialog::accept();
}

// closeEvent is only called from the system menu close widget for a
// modeless dialog so we use the hideEvent override to store the
// window settings
void LogQSO::hideEvent (QHideEvent * e)
{
  storeSettings ();
  QDialog::hideEvent (e);
}
