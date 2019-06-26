// This source code file was last time modified by Arvo ES1JA on December 1st, 2016
// All changes are shown in the patch file coming together with the full JTDX source code.

/*
 * From an ADIF file and cty.dat, get a call's DXCC entity and its worked before status
 * VK3ACF July 2013
 */

#ifndef LOGBOOK_H
#define LOGBOOK_H


#include <QString>
#include <QFont>

#include "countrydat.h"
#include "adif.h"

class QDir;

class LogBook
{
public:
    void init(bool displayCountyPrefix);
    void matchDXCC(/*in*/ const QString call,
              /*out*/ QString &countryName,
                      bool &WorkedBefore,
                      bool &WorkedBeforeBandMode,
               /*in*/ double dialFreq=0,
                      const QString mode="");
    void matchGrid(/*in*/ const QString gridsquare,
              /*out*/ bool &WorkedBefore,
                      bool &WorkedBeforeBandMode,
               /*in*/ double dialFreq=0,
                      const QString mode="");
    void matchCall(/*in*/ const QString call,
              /*out*/ QString &countryName,
                      bool &WorkedBefore,
                      bool &WorkedBeforeBandMode,
               /*in*/ double dialFreq=0,
                      const QString mode="");
    void getDXCC(/*in*/ const QString call,
              /*out*/ QString &countryName);
    void addAsWorked(const QString call, const QString band, const QString mode, const QString date, const QString gridsquare);
    int get_qso_count(const QString mod="");
private:
   CountryDat _countries;
   ADIF _log;
};

#endif // LOGBOOK_H

