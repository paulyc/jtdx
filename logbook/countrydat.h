// This source code file was last time modified by Arvo ES1JA on February 22nd, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

/*
 * Reads cty.dat file
 * Establishes a map between prefixes and their country names
 * VK3ACF July 2013
 */


#ifndef __COUNTRYDAT_H
#define __COUNTRYDAT_H


#include <QString>
#include <QStringList>
#include <QHash>


class CountryDat
{
public:
  void init(const QString filename);
  void load(bool displayCountyPrefix);
  QString find(const QString prefix); // return country name or ""
   
private:
  QString _extractName(const QString line);
  QString _extractMasterPrefix(const QString line);
  QString _extractContinent(const QString line);
  void _removeBrackets(QString &line, const QString a, const QString b);
  QStringList _extractPrefix(QString &line, bool &more);

  QString _filename;
  QHash<QString, QString> _data;
};

#endif
