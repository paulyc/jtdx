// This source code file was last time modified by Igor Chernikov UA3DJY on August 23rd, 2016.
// All changes are shown in the patch file coming together with the full JTDX source code.

#include "messageaveraging.h"
#include <QSettings>
#include <QApplication>
#include "ui_messageaveraging.h"
#include "commons.h"

MessageAveraging::MessageAveraging(QSettings * settings, QWidget *parent) :
  QWidget(parent),
  settings_ {settings},
  ui(new Ui::MessageAveraging)
{
  ui->setupUi(this);
  setWindowTitle (QApplication::applicationName () + " - " + tr ("Message Averaging"));
  read_settings ();
}

MessageAveraging::~MessageAveraging()
{
  if (isVisible ()) write_settings ();
}

void MessageAveraging::closeEvent (QCloseEvent * e)
{
  write_settings ();
  QWidget::closeEvent (e);
}

void MessageAveraging::read_settings ()
{
  settings_->beginGroup ("MessageAveraging");
  restoreGeometry (settings_->value ("window/geometry").toByteArray ());
  settings_->endGroup ();
}

void MessageAveraging::write_settings ()
{
  settings_->beginGroup ("MessageAveraging");
  settings_->setValue ("window/geometry", saveGeometry ());
  settings_->endGroup ();
}

void MessageAveraging::displayAvg(QString t)
{
  ui->msgAvgTextBrowser->setText(t);
}
