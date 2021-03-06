// This source code file was last time modified by Igor Chernikov UA3DJY on August 20th, 2016.
// All changes are shown in the patch file coming together with the full JTDX source code.

#ifndef FASTGRAPH_H
#define FASTGRAPH_H
#include <QDialog>
#include <QScopedPointer>

namespace Ui {
  class FastGraph;
}

class QSettings;

class FastGraph : public QDialog
{
  Q_OBJECT

protected:
  void closeEvent (QCloseEvent *) override;

public:
  explicit FastGraph(QSettings *, QWidget *parent = 0);
  ~FastGraph();

  void   plotSpec();
  void   saveSettings();

signals:
  void fastPick(int x0, int x1, int y);

public slots:
  void fastPick1a(int x0, int x1, int y);

private slots:
  void on_gainSlider_valueChanged(int value);
  void on_zeroSlider_valueChanged(int value);  
  void on_greenZeroSlider_valueChanged(int value);
  void on_pbAutoLevel_clicked();

private:
  QSettings * m_settings;
  float m_ave;

  QScopedPointer<Ui::FastGraph> ui;
};

extern float fast_green[703];
extern int   fast_jh;

#endif // FASTGRAPH_H
