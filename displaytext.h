// This source code file was last time modified by Igor UA3DJY on August 31st, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

// -*- Mode: C++ -*-
#ifndef DISPLAYTEXT_H
#define DISPLAYTEXT_H

#include <QTextEdit>
#include "logbook/logbook.h"
#include "decodedtext.h"
#include "Radio.hpp"

class Configuration;

class DisplayText : public QTextEdit
{
    Q_OBJECT
public:
    explicit DisplayText(QWidget *parent = 0);
    void setConfiguration(Configuration const *);
    void setContentFont (QFont const&);
    void insertLineSpacer(QString const&);
    bool displayDecodedText(DecodedText decodedText, QString myCall, 
                            bool once_notified, LogBook logBook,
                            double dialFreq = 0, const QString app_mode = "",
                            bool bypassRxfFilters = false, bool bypassAllFilters = false,
                            int rx_frq = 0, bool windowPopup = false, QWidget* window = NULL);
    void displayTransmittedText(QString text, QString modeTx, qint32 txFreq,
                                QColor color_TxMsg, bool bFastMode);
    void displayQSY(QString text);

signals:
    void selectCallsign(bool shift, bool ctrl);

public slots:
  void appendText(QString const& text, QString const& bg = "white", QString const& color = "black", int std_type = 0, QString const& servis = "&nbsp;", QString const& cntry = "&nbsp;", bool forceBold = false, bool strikethrough = false, bool underline = false);

protected:
    void mouseDoubleClickEvent(QMouseEvent *e);

private:

    QTextCharFormat m_charFormat;
    Configuration const * m_config;
};

#endif // DISPLAYTEXT_H
