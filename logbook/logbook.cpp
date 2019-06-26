// This source code file was last time modified by Arvo ES1JA on April 9th, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

#include "logbook.h"
#include <QDebug>
#include <QFontMetrics>
#include <QStandardPaths>
#include <QDir>
#include <iostream>

namespace
{
  auto logFileName = "wsjtx_log.adi";
  auto countryFileName = "cty.dat";
}

void LogBook::init(bool displayCountyPrefix)
{
  QDir dataPath {QStandardPaths::writableLocation (QStandardPaths::DataLocation)};
  QString countryDataFilename;
  if (dataPath.exists (countryFileName))
    {
      // User override
      countryDataFilename = dataPath.absoluteFilePath (countryFileName);
    }
  else
    {
      countryDataFilename = QString {":/"} + countryFileName;
    }

  _countries.init(countryDataFilename);
  _countries.load(displayCountyPrefix);

  _log.init(dataPath.absoluteFilePath (logFileName), &_countries);
  _log.load();

  /*
    int QSOcount = _log.getCount();
    int count = _worked.getWorkedCount();
    qDebug() << QSOcount << "QSOs and" << count << "countries worked in file" << logFilename;
  */

  //    QString call = "ok1ct";
  //    QString countryName;
  //    bool callWorkedBefore,countryWorkedBefore;
  //    match(/*in*/call, /*out*/ countryName,callWorkedBefore,countryWorkedBefore);
  //    qDebug() << countryName;

}

void LogBook::matchDXCC(/*in*/const QString call,
                    /*out*/ QString &countryName,
                    bool &WorkedBefore,
                    bool &WorkedBeforeBandMode,
                    /*in*/ double dialFreq,
                    const QString mode)
{
    if (call.length() > 0) {
        QString band = ADIF::bandFromFrequency(dialFreq / 1.e6);
        if (countryName == "") {
            countryName = _countries.find(call);
        }
        if (countryName.length() > 0 && countryName != "  where?") { //  is there, do checks
            WorkedBefore = _log.matchCountry(countryName, "", "");
            if (!WorkedBefore) {
                WorkedBeforeBandMode = false;
            } else if (band != "" || mode !="") {
                WorkedBeforeBandMode = _log.matchCountry(countryName, band, mode);   
            }
        } else {
            countryName = "  where?"; //error: prefix not found
            WorkedBefore = true;
            WorkedBeforeBandMode = true;
        }
        
        // qDebug() << "Logbook:" << call << ":" << countryName << "Cty B4:" << countryWorkedBefore << "call B4:" << callWorkedBefore << "Freq B4:" << dialFreq << "Band B4:" << band << "Mode B4:" << mode;
    }
}

void LogBook::matchGrid(/*in*/const QString gridsquare,
                    /*out*/ bool &WorkedBefore,
                    bool &WorkedBeforeBandMode,
                    /*in*/ double dialFreq,
                    const QString mode)
{
    if (gridsquare.length() > 0) {
        QString band = ADIF::bandFromFrequency(dialFreq / 1.e6);
        WorkedBefore = _log.matchGrid(gridsquare.left(4), "", "");
        if (!WorkedBefore) {
            WorkedBeforeBandMode = false;
        } else if (band != "" || mode !="") {
            WorkedBeforeBandMode = _log.matchGrid(gridsquare.left(4), band, mode);
        }
        
        // qDebug() << "Logbook:" << call << ":" << countryName << "Cty B4:" << countryWorkedBefore << "call B4:" << callWorkedBefore << "Freq B4:" << dialFreq << "Band B4:" << band << "Mode B4:" << mode;
    }
}

void LogBook::matchCall(/*in*/const QString call,
                    /*out*/ QString &countryName,
                    bool &WorkedBefore,
                    bool &WorkedBeforeBandMode,
                    /*in*/ double dialFreq,
                    const QString mode)
{
    if (call.length() > 0) {
        QString band = ADIF::bandFromFrequency(dialFreq / 1.e6);
        WorkedBefore = _log.match(call, "", "");
        if (!WorkedBefore) {
            WorkedBeforeBandMode = false;
        } else if (band != "" || mode !="") {
            WorkedBeforeBandMode = _log.match(call, band, mode);
        }
        if (countryName == "") {
            countryName = _countries.find(call);
            if (countryName == "") {
              countryName = "  where?"; //error: prefix not found
            }
        }
        
        // qDebug() << "Logbook:" << call << ":" << countryName << "Cty B4:" << countryWorkedBefore << "call B4:" << callWorkedBefore << "Freq B4:" << dialFreq << "Band B4:" << band << "Mode B4:" << mode;
    }
}

void LogBook::getDXCC(/*in*/const QString call,
                    /*out*/ QString &countryName)
{
    if (call.length() > 0) {
        if (countryName == "") {
            countryName = _countries.find(call);
            if (countryName == "") {
              countryName = "  where?"; //error: prefix not found
            }
        }
        
        // qDebug() << "Logbook:" << call << ":" << countryName << "Cty B4:" << countryWorkedBefore << "call B4:" << callWorkedBefore << "Freq B4:" << dialFreq << "Band B4:" << band << "Mode B4:" << mode;
    }
}

void LogBook::addAsWorked(const QString call, const QString band, const QString mode, const QString date, const QString gridsquare)
{
    //qDebug() << "adding " << call << " as worked";
    _log.add(call,band,mode,date,gridsquare);
}

int LogBook::get_qso_count(const QString mod)
{
    return _log.getCount(mod);
}
