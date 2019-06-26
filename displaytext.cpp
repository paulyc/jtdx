// This source code file was last time modified by Igor UA3DJY on October 12th, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

#include "displaytext.h"

#include <QtGlobal>
#include <QApplication>
#include <QMouseEvent>
#include <QDateTime>
#include <QTextCharFormat>
#include <QFont>
#include <QTextCursor>

#include "Configuration.hpp"
#include "qt_helpers.hpp"

#include "moc_displaytext.cpp"

DisplayText::DisplayText(QWidget *parent) :
    QTextEdit(parent)
{
    setReadOnly (true);
    viewport ()->setCursor (Qt::ArrowCursor);
    setWordWrapMode (QTextOption::NoWrap);
    setStyleSheet ("");
}

void DisplayText::setConfiguration(Configuration const * config)
{
  m_config = config;
}
void DisplayText::setContentFont(QFont const& font)
{
//  setFont (font);
  m_charFormat.setFont (font);
//  selectAll ();
/*
  auto cursor = textCursor ();
  cursor.select(QTextCursor::Document);
  cursor.mergeCharFormat (m_charFormat);
  cursor.clearSelection ();
  cursor.movePosition (QTextCursor::End);

  // position so viewport scrolled to left
  cursor.movePosition (QTextCursor::Up);
  cursor.movePosition (QTextCursor::StartOfLine);

  setTextCursor (cursor);
  ensureCursorVisible (); */
}

void DisplayText::mouseDoubleClickEvent(QMouseEvent *e)
{
  bool ctrl = (e->modifiers() & Qt::ControlModifier);
  bool shift = (e->modifiers() & Qt::ShiftModifier);
  emit(selectCallsign(shift,ctrl));
  QTextEdit::mouseDoubleClickEvent(e);
}

void DisplayText::insertLineSpacer(QString const& line)
{
  appendText (line, "#d3d3d3");
}

void DisplayText::appendText(QString const& text, QString const& bg, QString const& color, int std_type, QString const& servis, QString const& cntry, bool forceBold, bool strikethrough, bool underlined)
{

    QString servbg, s;
    if (std_type == 2) servbg = "red";
    else if (servis == "?") servbg = "yellow";
    else servbg = "white";
    QString escaped;
    if (text.length() < 44) {
        escaped = text.left(40).replace("<","&lt;").replace(">","&gt;").replace(" ", "&nbsp;");
        s = "<table border=0 cellspacing=0 width=60%><tr><td bgcolor=\"" + bg + "\" style=\"color: " + 
        color + "\">" + escaped + "</td><td  bgcolor=\""+servbg+"\" style=\"color: black\">"+servis+"</td><td>"+cntry+"</td></tr></table>";
//        s = "<table border=0 cellspacing=0 width=60%><tr><td bgcolor=\"" + bg + "\" style=\"color: " + 
//        color + "\">" + escaped + "</td><td  bgcolor=\""+servbg+"\" style=\"color: black\">"+servis+"</td><td bgcolor=\"" + bg + "\" style=\"color: " + 
//        color + "\">"+cntry+"</td></tr></table>";
    } else {
        escaped = text.trimmed().replace("<","&lt;").replace(">","&gt;").replace(" ", "&nbsp;");
        s = "<table border=0 cellspacing=0 width=60%><tr><td colspan=\"3\" bgcolor=\"" + bg + "\" style=\"color: " + 
        color + "\">" + escaped + "</td></tr></table>";
    }
    auto cursor = textCursor ();
    cursor.movePosition (QTextCursor::End);
    auto pos = cursor.position ();
    m_charFormat.setFontStrikeOut(strikethrough);
    m_charFormat.setFontUnderline(underlined);
    auto weight = m_charFormat.fontWeight();
    if (forceBold) {
        if (weight < 70) {
            m_charFormat.setFontWeight(QFont::Bold);
        } else {
            m_charFormat.setFontWeight(QFont::Black);
        }
    }
    cursor.insertHtml (s);
    cursor.setPosition (pos, QTextCursor::MoveAnchor);
    cursor.movePosition (QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.mergeCharFormat (m_charFormat);
    m_charFormat.setFontWeight(weight);
    cursor.clearSelection ();

    // position so viewport scrolled to left
    cursor.movePosition (QTextCursor::Up);
    cursor.movePosition (QTextCursor::StartOfLine);
    setTextCursor (cursor);
    ensureCursorVisible ();
}

bool DisplayText::displayDecodedText(DecodedText decodedText, QString myCall,
                            bool once_notified, LogBook logBook,
                            double dialFreq, const QString app_mode,
                            bool bypassRxfFilters,bool bypassAllFilters,
                            int rx_frq, bool windowPopup, QWidget* window)
{
    QString bgColor = "white";
    QString txtColor = "black";
    QString swpColor = "";
    bool forceBold = false;
    bool strikethrough = false;
    bool underlined = false;
    bool beep = false;
    bool actwind = false;
    bool show_line = true;
    bool jt65bc = false;
	bool notified = false;
    int std_type = 0;
    bool displayCountryName = m_config->countryName();
    bool displayNewDXCC = m_config->newDXCC();
    bool displayNewDXCCBand = m_config->newDXCCBand();
    bool displayNewDXCCBandMode = m_config->newDXCCBandMode();
    bool displayNewGrid = m_config->newGrid();
    bool displayNewGridBand = m_config->newGridBand();
    bool displayNewGridBandMode = m_config->newGridBandMode();
    bool displayNewCall = m_config->newCall();
    bool displayNewCallBand = m_config->newCallBand();
    bool displayNewCallBandMode = m_config->newCallBandMode();
    bool displayPotential = m_config->newPotential();
    bool displayTxtColor = m_config->txtColor();
    bool displayWorkedColor = m_config->workedColor();
    bool displayWorkedStriked = m_config->workedStriked();
    bool displayWorkedUnderlined = m_config->workedUnderlined();
    bool displayWorkedDontShow = m_config->workedDontShow();
    bool beepOnNewDXCC = m_config->beepOnNewDXCC();
    bool beepOnNewGrid = m_config->beepOnNewGrid();
    bool beepOnNewCall = m_config->beepOnNewCall();
//    QString dt = "1135 -23  0.0 1314 @ CQ K4SHQ EM64      ";
//    decodedText=dt;
    QString messageText = decodedText.string().left(40);
    QString servis = "&nbsp;";
    QString cntry = "&nbsp;";
    QString checkCall;
    QString justCall;
    QString grid;
    checkCall = decodedText.CQersCall();
    if(messageText.contains("2nd-h") || messageText.contains("3rd-h")) jt65bc = true;
    if (checkCall != ""){
        std_type = 1;
        txtColor = m_config->color_CQ().name();
        decodedText.deCallAndGrid(justCall, grid);
    }
    else if (myCall != "" && Radio::base_callsign (decodedText.call()) == myCall){
            std_type = 2;
            txtColor = m_config->color_MyCall().name();
            decodedText.deCallAndGrid(checkCall, grid);
            actwind = true;
            if (m_config->beepOnMyCall()) {
                beep = true;
            }
    }
    else if (!decodedText.isNonStd1() && !decodedText.isNonStd2()) {
            decodedText.deCallAndGrid(checkCall, grid);
            if (checkCall != "") {
                std_type = 3;
            }
    }
    if (checkCall != "") {

        bool countryB4 = true;
        bool callB4 = true;
        bool countryB4BandMode = true;
        bool callB4BandMode = true;
        bool gridB4 = true;
        bool gridB4BandMode = true;
        QString countryName;
        QString checkMode;

        if (displayPotential && std_type == 3) {
            txtColor = m_config->color_StandardCall().name();
        }
        if (!jt65bc && (displayNewDXCC || displayNewCall || displayNewGrid) && ((displayPotential && std_type == 3) || std_type != 3)) {
            if (displayCountryName && !displayNewDXCC && !displayNewCall) {
                        logBook.getDXCC(/*in*/ checkCall, /*out*/ countryName);
                    }
            if (app_mode == "JT9+JT65") {
                if (decodedText.isJT9()) {
                    checkMode = "JT9";
                } else if (decodedText.isJT65()) { // TODO: is this if-condition necessary?
                    checkMode = "JT65";
                }
            } else {
                checkMode = app_mode;
            }
            if (displayNewDXCC) {
                if (displayNewDXCCBand || displayNewDXCCBandMode) {
                    if (displayNewDXCCBand && displayNewDXCCBandMode) {
                        logBook.matchDXCC(/*in*/checkCall,/*out*/countryName,countryB4,countryB4BandMode,/*in*/dialFreq,checkMode);
                    } else if (displayNewDXCCBand){
                        logBook.matchDXCC(/*in*/checkCall,/*out*/countryName,countryB4,countryB4BandMode,/*in*/dialFreq);
                    } else {
                        logBook.matchDXCC(/*in*/checkCall,/*out*/countryName,countryB4,countryB4BandMode,/*in*/0,checkMode);
                    }
                } else {
                    logBook.matchDXCC(/*in*/ checkCall, /*out*/ countryName, countryB4 ,countryB4BandMode);
                }
            }
            if (displayNewGrid) {
                if (displayNewGridBand || displayNewGridBandMode) {
                    if (displayNewGridBand && displayNewGridBandMode) {
                        logBook.matchGrid(/*in*/grid.trimmed(),/*out*/gridB4,gridB4BandMode,/*in*/dialFreq,checkMode);
                    } else if (displayNewGridBand) {
                        logBook.matchGrid(/*in*/grid.trimmed(),/*out*/gridB4,gridB4BandMode,/*in*/dialFreq);
                    } else {
                        logBook.matchGrid(/*in*/grid.trimmed(),/*out*/gridB4,gridB4BandMode,/*in*/0,checkMode);
                    }
                } else {
                    logBook.matchGrid(/*in*/ grid.trimmed(), /*out*/ gridB4 ,gridB4BandMode);
                }
            }
            if (displayNewCall) {
                if (displayNewCallBand || displayNewCallBandMode) {
                    if (displayNewCallBand && displayNewCallBandMode) {
                        logBook.matchCall(/*in*/checkCall,/*out*/countryName,callB4,callB4BandMode,/*in*/dialFreq,checkMode);
                    } else if (displayNewCallBand) {
                        logBook.matchCall(/*in*/checkCall,/*out*/countryName,callB4,callB4BandMode,/*in*/dialFreq);
                    } else {
                        logBook.matchCall(/*in*/checkCall,/*out*/countryName,callB4,callB4BandMode,/*in*/0,checkMode);
                    }
                } else {
                    logBook.matchCall(/*in*/ checkCall, /*out*/ countryName, callB4 ,callB4BandMode);
                }
            }

            if (displayNewDXCC || displayNewCall || displayNewGrid) {
//Worked
                if (displayWorkedColor) {
                    bgColor = m_config->color_WorkedCall().name();
                }
                if (displayWorkedStriked) {
                    strikethrough = true;
                } else if (displayWorkedUnderlined) {
                    underlined = true;
                }
                if (displayNewDXCC && !countryB4) {
                    forceBold = true;
                    bgColor = m_config->color_NewDXCC().name();
                    strikethrough = false;
                    underlined = false;
                    actwind = true;
                    if (beepOnNewDXCC) {
                        beep = true;
                    }
                } else if (displayNewGrid && !gridB4) {
                    forceBold = true;
                    bgColor = m_config->color_NewGrid().name();
                    strikethrough = false;
                    underlined = false;
                    actwind = true;
                    if (beepOnNewGrid) {
                        beep = true;
                    }
                } else if (displayNewDXCCBand && !countryB4BandMode) {
                    forceBold = true;
                    bgColor = m_config->color_NewDXCCBand().name();
                    strikethrough = false;
                    underlined = false;
                    actwind = true;
                    if (beepOnNewDXCC) {
                        beep = true;
                    }
                } else if (displayNewGridBand && !gridB4BandMode) {
                    forceBold = true;
                    bgColor = m_config->color_NewGridBand().name();
                    strikethrough = false;
                    underlined = false;
                    actwind = true;
                    if (beepOnNewGrid) {
                        beep = true;
                    }
                } else  if (displayNewCall && !callB4) {
                    forceBold = true;
                    bgColor = m_config->color_NewCall().name();
                    strikethrough = false;
                    underlined = false;
                    actwind = true;
                    if (beepOnNewCall) {
                        beep = true;
                    }
                } else if (displayNewCallBand && !callB4BandMode) {
                    forceBold = true;
                    bgColor = m_config->color_NewCallBand().name();
                    strikethrough = false;
                    underlined = false;
                    actwind = true;
                    if (beepOnNewCall) {
                        beep = true;
                    }
                } 
                if (!forceBold && std_type != 2 && displayWorkedDontShow) {
                    show_line = false;
                }
            }
        } else if (displayCountryName) {
            logBook.getDXCC(/*in*/ checkCall, /*out*/ countryName);
        }

        if (displayTxtColor && (displayPotential || std_type != 3)) {
            swpColor = bgColor;
            bgColor = txtColor;
            txtColor = swpColor;
        }

        if (displayCountryName) {
            // do some obvious abbreviations, don't care if we using just prefixes here, not big deal to run some replace's
            countryName.replace ("Islands", "Is.");
            countryName.replace ("Island", "Is.");
            countryName.replace ("North ", "N. ");
            countryName.replace ("Northern ", "N. ");
            countryName.replace ("South ", "S. ");
            countryName.replace ("East ", "E. ");
            countryName.replace ("Eastern ", "E. ");
            countryName.replace ("West ", "W. ");
            countryName.replace ("Western ", "W. ");
            countryName.replace ("Central ", "C. ");
            countryName.replace (" and ", " & ");
            countryName.replace ("Republic", "Rep.");
            countryName.replace ("United States", "U.S.A.");
            countryName.replace ("Fed. Rep. of ", "");
            countryName.replace ("French ", "Fr.");
            countryName.replace ("Asiatic", "AS");
            countryName.replace ("European", "EU");
            countryName.replace ("African", "AF");
//            messageText += countryName.trimmed();
            cntry = countryName.trimmed().mid(2);
        }
        if (displayCountryName && m_config->hideContinents().contains(countryName.trimmed().left(2)) && std_type != 2 && !jt65bc) {
            show_line = false;
        }
    }
    
    if (decodedText.isNonStd2() && m_config->hidefree() && std_type != 1 && !jt65bc) {
        show_line = false;
    } else if (m_config->showcq() && std_type != 1 && std_type != 2 && qAbs(rx_frq-decodedText.frequencyOffset()) >10 && !jt65bc) {
        show_line = false;
    } else if (m_config->showcq73() && std_type != 1 && std_type != 2 && !decodedText.isEnd() && qAbs(rx_frq-decodedText.frequencyOffset()) >10 && !jt65bc) {
        show_line = false;
    } else if (decodedText.isHint()) {
        servis = "*"; // hinted decode
    } else if (decodedText.isWrong()) {
        servis = "?"; // error decode
    }
    if (show_line) {
        if (actwind) {
            if (windowPopup && window != NULL) {
                window->showNormal();
				window->raise();
				QApplication::setActiveWindow(window);
			}
		}
        if (beep && !once_notified) {
            QApplication::beep();
			notified = true;
        }
    }
    if (bypassAllFilters || bypassRxfFilters) {
            show_line = true;
    }
    if (jt65bc) {
        bgColor = "white";
        txtColor = "black";
    }
    if (show_line) {
        appendText(messageText, bgColor, txtColor, std_type, servis, cntry, forceBold, strikethrough, underlined);
    }
	return notified;
}


void DisplayText::displayTransmittedText(QString text, QString modeTx, qint32 txFreq,
                                         QColor color_TxMsg, bool bFastMode)
{
    QString bg=color_TxMsg.name();
    QString t1=" @ ";
    if(modeTx=="JT65") t1=" # ";
    if(modeTx=="T10") t1=" ~ ";
    QString t2;
    t2.sprintf("%4d",txFreq);
    QString t;
    if(bFastMode) {
      t = QDateTime::currentDateTimeUtc().toString("hhmmss") + \
        "  Tx      " + t2 + t1 + text;
    } else {
      t = QDateTime::currentDateTimeUtc().toString("hhmm") + \
        "  Tx      " + t2 + t1 + text;
    }
    appendText(t,bg);
}

void DisplayText::displayQSY(QString text)
{
  QString t = QDateTime::currentDateTimeUtc().toString("hhmmss") + "            " + text;
  QString bg="hot pink";
  appendText(t,bg);
}
