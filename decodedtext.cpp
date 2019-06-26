// This source code file was last time modified by Arvo ES1JA on October 16th, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

#include <QStringList>
#include <QDebug>
#include "decodedtext.h"
#include <QRegularExpression>

QString DecodedText::CQersCall()
{
// JUKKA bug
//  QRegularExpression callsign_re {R"(\s(CQ|DE|QRZ)(\s?DX|\s([A-Z]{2}|\d{3}))?\s(?<callsign>[A-Z0-9/]{2,})(\s[A-R]{2}[0-9]{2})?)"};
  QRegularExpression callsign_re {R"(\s(CQ|DE|QRZ)(\s?DX|\s([A-Z]{2}|\d{3}))?\s(?<callsign>[2-9]{0,1}[A-Z]{1,2}[0-9]{1,4}[A-Z]{1,6})(\s[A-R]{2}[0-9]{2})?)"};
  return callsign_re.match (_string).captured ("callsign");
}

/*QString DecodedText::CQersCall()
{
    // extract the CQer's call   TODO: does this work with all call formats?
  int s1 {0};
  int position;
  
  if ((position = _string.indexOf (_cqLongerRe)) == column_qsoText-1)
    {
      s1 = 7 + position;
    }
  else if ((position = _string.indexOf (" CQ ")) == column_qsoText-1)
    {
      s1 = 4 + position;
      if(_string.mid(s1,3).toInt() > 0 and _string.mid(s1,3).toInt() <= 999) s1 += 4;
    }
  else if ((position = _string.indexOf (" CQ_DX ")) == column_qsoText-1)
    {
      s1 = 7 + position;
    }
  else if ((position = _string.indexOf (" CQDX ")) == column_qsoText-1)
    {
      s1 = 6 + position;
    }
  else if ((position = _string.indexOf (" DE ")) == column_qsoText-1)
    {
      s1 = 4 + position;
    }
  else if ((position = _string.indexOf (" QRZ ")) == column_qsoText-1)
    {
      s1 = 5 + position;
    }
  else if ((position = _string.indexOf (" QRZ DX ")) == column_qsoText-1)
    {
      s1 = 8 + position;
    }
  else {
    return _string.mid (s1, 0);
  }
  auto s2 = _string.indexOf (" ", s1);
  if (s2-s1 == 1){
      s1=s2+1;
      s2 = _string.indexOf (" ", s1);
  }
  if (_callRe.match(_string.mid (s1, s2 - s1)).hasMatch() && _string.mid (s1, s2 - s1).contains("TU73") == 0) {
      return _string.mid (s1, s2 - s1);
  } else {
      return _string.mid (s1, 0);
  }

}

*/



bool DecodedText::isHint()
{
	return _string.mid(43,1) == "*";
}

bool DecodedText::isWrong()
{
	return _string.mid(43,1) == "?";
}

bool DecodedText::isNonStd1()
{
	return _string.mid(43,1) == ",";
}

bool DecodedText::isNonStd2()
{
	return _string.mid(43,1) == ".";
}

bool DecodedText::isEnd()
{
	return _string.indexOf("RRR") >= 0 || _string.indexOf("RR73") >= 0 || _string.indexOf(" 73") >= 0;
}

bool DecodedText::isJT65()
{
    return _string.indexOf("#") == column_mode;
}

bool DecodedText::isJT9()
{
    return _string.indexOf("@") == column_mode;
}

bool DecodedText::isTX()
{
    int i = _string.indexOf("Tx");
    return (i >= 0 && i < 15); // TODO guessing those numbers. Does Tx ever move?
}

int DecodedText::frequencyOffset()
{
    return _string.mid(column_freq,4).toInt();
}

int DecodedText::snr()
{
  int i1=_string.indexOf(" ")+1;
  return _string.mid(i1,3).toInt();
}

float DecodedText::dt()
{
  return _string.mid(column_dt,5).toFloat();
}

/*
2343 -11  0.8 1259 # YV6BFE F6GUU R-08
2343 -19  0.3  718 # VE6WQ SQ2NIJ -14
2343  -7  0.3  815 # KK4DSD W7VP -16
2343 -13  0.1 3627 @ CT1FBK IK5YZT R+02

0605  Tx      1259 # CQ VK3ACF QF22
*/

// find and extract any report. Returns true if this is a standard message
bool DecodedText::report(QString const& myBaseCall, QString const& dxBaseCall, /*mod*/QString& report)
{
    QString msg=_string.mid(column_qsoText).trimmed();
    if(msg.length() < 1) return false;
    msg = msg.remove (_repRe);
    int i1=msg.indexOf('\r');
    if (i1>0)
      msg=msg.left (i1-1);
    bool b = stdmsg_ ((msg + "                      ").toLatin1().constData(),22);  // stdmsg is a fortran routine that packs the text, unpacks it and compares the result

    QStringList w=msg.split(" ",QString::SkipEmptyParts);
    if(w.size ()
       && b && (w[0] == myBaseCall
             || w[0].endsWith ("/" + myBaseCall)
             || w[0].startsWith (myBaseCall + "/"))
             && (dxBaseCall.isEmpty () || (w.size () > 1 
                 && (w[1] == dxBaseCall
                     || w[1].endsWith ("/" + dxBaseCall)
                     || w[1].startsWith (dxBaseCall + "/")))))
    {
        QString tt="";
        if(w.size() > 2) tt=w[2];
        bool ok;
        i1=tt.toInt(&ok);
        if (ok and i1>=-50 and i1<50)
        {
            report = tt;
        }
        else
        {
            if (tt.mid(0,1)=="R")
            {
                i1=tt.mid(1).toInt(&ok);
                if(ok and i1>=-50 and i1<50)
                {
                    report = tt.mid(1);
                }
            }
        }
    }
    return b;
}

// get the first text word, usually the call
QString DecodedText::call()
{
  auto call = _string;
  call = call.replace (_cqLongerRe, " CQ_\\1 ").mid (column_qsoText);
  int i = call.indexOf(" ");
  if (_callRe.match(call.mid(0,i)).hasMatch() && _string.mid (0, i).contains("TU73") == 0 && _string.mid (0, i).contains("73GL") == 0) {
      return call.mid(0,i);
  } else {
      return call.mid(0,0);
  }
}

// get the second word, most likely the de call and the third word, most likely grid
void DecodedText::deCallAndGrid(/*out*/QString& call, QString& grid)
{
  auto msg = _string;
  msg = msg.replace (_cqLongerRe, " CQ_\\1 ").mid (column_qsoText);
  int i1 = msg.indexOf(" ");
  call = msg.mid(i1+1);
  int i2 = call.indexOf(" ");
  if (_gridRe.match(call.mid(i2+1,4)).hasMatch() && call.mid(i2+1,4) != "RR73") {
    grid = call.mid(i2+1,4);
  }
  call = call.mid(0,i2).replace(">","");
  if (!_callRe.match(call).hasMatch() ||  call.contains("TU73") > 0 || call.contains("73GL") > 0) {
      call = "";
  }
}

int DecodedText::timeInSeconds()
{
    return 60*_string.mid(column_time,2).toInt() + _string.mid(2,2).toInt();
}

/*
2343 -11  0.8 1259 # YV6BFE F6GUU R-08
2343 -19  0.3  718 # VE6WQ SQ2NIJ -14
2343  -7  0.3  815 # KK4DSD W7VP -16
2343 -13  0.1 3627 @ CT1FBK IK5YZT R+02

0605  Tx      1259 # CQ VK3ACF QF22
*/

QString DecodedText::report()  // returns a string of the SNR field with a leading + or - followed by two digits
{
    int sr = snr();
    if (sr<-50)
        sr = -50;
    else
        if (sr > 49)
            sr = 49;

    QString rpt;
    rpt.sprintf("%d",qAbs(sr));
    if (sr > 9)
        rpt = "+" + rpt;
    else
        if (sr >= 0)
            rpt = "+0" + rpt;
        else
            if (sr >= -9)
                rpt = "-0" + rpt;
            else
                rpt = "-" + rpt;
    return rpt;
}
