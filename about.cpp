// This source code file was last time modified by Igor UA3DJY on November 15th, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

#include "about.h"

#include <QCoreApplication>
#include <QString>

#include "revision_utils.hpp"

#include "ui_about.h"
#include "moc_about.cpp"

CAboutDlg::CAboutDlg(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::CAboutDlg)
{
  ui->setupUi(this);

  ui->labelTxt->setText ("<html><h2>"
                         + QString {"JTDX v18.0.0.133 HF software"}.simplified ()
                         + "</h2>\n\n"
                         "It is modified WSJT-X software forked from WSJT-X r6462. <br>"
                         "JTDX supports JT9 and JT65a digital modes for HF <br>"
                         "amateur radio communication, focused on DXing <br>"
                         "and being shaped by community of DXers. <br>"
						 "&copy; 2016-2017 by Igor Chernikov, UA3DJY.<br>"
                         "It is created with contributions from<br>"
                         "ES1JA, G7OED, MM0HVU, RA4UDC, UA3ALE, US-E-12, VE3NEA and <br>"
                         "LY3BG family: Vytas and Rimas Kudelis.<br><br>"
                         "&copy; 2001-2017 by Joe Taylor, K1JT, with grateful <br>"
                         "acknowledgment for contributions from AC6SL, AE4JY, <br>"
                         "DJ0OT, G3WDG, G4KLA, G4WJS, IV3NWV, IW3RAB, K3WYC, K9AN, <br>"
                         "KA6MAL, KA9Q, KB1ZMX, KD6EKQ, KI7MT, KK1D, ND0B, PY2SDR, <br>"
                         "VE1SKY, VK3ACF, VK4BDJ, VK7MO, W4TI, W4TV, and W9MDB.<br>"
						 "<br><br>"
						 "JTDX is licensed under the terms of Version3<br>"
						 "of the GNU General Public License(GPL)<br>"
                         "<a href=\"https://www.gnu.org/licenses/gpl-3.0.txt\">"
                         "https://www.gnu.org/licenses/gpl-3.0.txt</a>");
}

CAboutDlg::~CAboutDlg()
{
}
