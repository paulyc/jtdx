// This source code file was last time modified by Igor UA3DJY on November 14th, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

// -*- Mode: C++ -*-
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#ifdef QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QList>
#include <QStringList>
#include <QAudioDeviceInfo>
#include <QScopedPointer>
#include <QDir>
#include <QProgressDialog>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QPointer>
#include <QSet>
#include <QFuture>
#include <QFutureWatcher>

#include "AudioDevice.hpp"
#include "commons.h"
#include "Radio.hpp"
#include "Modes.hpp"
#include "Configuration.hpp"
#include "WSPRBandHopping.hpp"
#include "Transceiver.hpp"
#include "DisplayManual.hpp"
#include "psk_reporter.h"
#include "logbook/logbook.h"
#include "decodedtext.h"
#include "commons.h"
#include "astro.h"

#define NUM_JT4_SYMBOLS 206                //(72+31)*2, embedded sync
#define NUM_JT65_SYMBOLS 126               //63 data + 63 sync
#define NUM_JT9_SYMBOLS 85                 //69 data + 16 sync
#define NUM_T10_SYMBOLS 85                 //69 data + 16 sync
#define NUM_WSPR_SYMBOLS 162               //(50+31)*2, embedded sync
#define NUM_ISCAT_SYMBOLS 1291             //30*11025/256
#define NUM_JTMSK_SYMBOLS 234              //(72+15+12)*2 + 3*11 sync + 3 f0-parity

#define NUM_CW_SYMBOLS 250
#define TX_SAMPLE_RATE 48000

extern int volatile itone[NUM_ISCAT_SYMBOLS];   //Audio tones for all Tx symbols
extern int volatile icw[NUM_CW_SYMBOLS];	    //Dits for CW ID

//--------------------------------------------------------------- MainWindow
namespace Ui {
  class MainWindow;
}

class QSettings;
class QNetworkAccessManager;
class QLineEdit;
class QFont;
class QHostInfo;
class EchoGraph;
class FastGraph;
class WideGraph;
class LogQSO;
class Transceiver;
class MessageAveraging;
class MessageClient;
class QTime;
class WSPRBandHopping;
class HelpTextWindow;
class WSPRNet;
class SoundOutput;
class Modulator;
class SoundInput;
class Detector;
class SampleDownloader;

class MainWindow : public QMainWindow
{
  Q_OBJECT;

public:
  using Frequency = Radio::Frequency;
  using FrequencyDelta = Radio::FrequencyDelta;
  using Mode = Modes::Mode;

  // Multiple instances: call MainWindow() with *thekey
  explicit MainWindow(bool multiple, QSettings *, QSharedMemory *shdmem,
                      unsigned downSampleFactor, QNetworkAccessManager * network_manager,
                      QWidget *parent = 0);
  ~MainWindow();

public slots:
  void showSoundInError(const QString& errorMsg);
  void showSoundOutError(const QString& errorMsg);
  void showStatusMessage(const QString& statusMsg);
  void dataSink(qint64 frames);
  void fastSink(qint64 frames);
  void diskDat();
  void freezeDecode(int n);
  void guiUpdate();
  void doubleClickOnCall(bool shift, bool ctrl);
  void doubleClickOnCall2(bool shift, bool ctrl);
  void readFromStdout();
  void readFromStderr();
  void jt9_error(QProcess::ProcessError);
  void p1ReadFromStdout();
  void p1ReadFromStderr();
  void p1Error(QProcess::ProcessError);
  void setXIT(int n, Frequency base = 0u);
  void setFreq4(int rxFreq, int txFreq);
  void setRxFreq4(int rxFreq);
  void msgAvgDecode2();
  void fastPick(int x0, int x1, int y);

protected:
  virtual void keyPressEvent( QKeyEvent *e );
  void  closeEvent(QCloseEvent*);
  virtual bool eventFilter(QObject *object, QEvent *event);
  void mousePressEvent(QMouseEvent *event);

private slots:
  void on_tx1_editingFinished();
  void on_tx2_editingFinished();
  void on_tx3_editingFinished();
  void on_tx4_editingFinished();
  void on_tx5_currentTextChanged (QString const&);
  void on_tx6_editingFinished();
  void on_wantedCall_textChanged(const QString &arg1);
  void on_actionSettings_triggered();
  void on_monitorButton_clicked (bool);
  void on_swlButton_clicked (bool);
  void on_filterButton_clicked (bool);
  void on_AGCcButton_clicked (bool);
  void on_actionAbout_triggered();
  void on_autoButton_clicked (bool);
  void on_stopTxButton_clicked();
  void on_stopButton_clicked();
  void on_actionOnline_User_Guide_triggered();
  void on_actionLocal_User_Guide_triggered();
  void on_actionWide_Waterfall_triggered();
  void on_actionOpen_triggered();
  void on_actionOpen_next_in_directory_triggered();
  void on_actionDecode_remaining_files_in_directory_triggered();
  void on_actionDelete_all_wav_files_in_SaveDir_triggered();
  void on_actionOpen_log_directory_triggered ();
  void on_actionNone_triggered();
  void on_actionSave_all_triggered();
  void on_actionShow_messages_decoded_from_harmonics_toggled(bool checked);
  void on_actionShow_messages_with_my_callsign_in_RX_frequency_window_toggled(bool checked);
  void on_actionBypass_text_filters_on_RX_frequency_toggled(bool checked);
  void on_actionBypass_all_text_filters_toggled(bool checked);
  void on_actionEnable_main_window_popup_toggled(bool checked);
  void on_actionKeyboard_shortcuts_triggered();
  void on_actionSpecial_mouse_commands_triggered();
  void on_actionCopyright_Notice_triggered();
  void on_DecodeButton_clicked (bool);
  void decode();
  void decodeBusy(bool b);
  void on_EraseButton_clicked();
  void on_cleanButton_clicked();
  void on_txb1_clicked();
  void on_TxMinuteButton_clicked(bool checked);
  void on_dxCheckBox_stateChanged(int arg1);
  void on_rrrCheckBox_stateChanged(int arg1);
  void set_ntx(int n);
  void on_txb2_clicked();
  void on_txb3_clicked();
  void on_txb4_clicked();
  void on_txb5_clicked();
  void on_txb6_clicked();
  void on_lookupButton_clicked();
  void on_addButton_clicked();
  void on_dxCallEntry_textChanged(const QString &arg1);
  void on_dxGridEntry_textChanged(const QString &arg1);
  void on_genStdMsgsPushButton_clicked();
  void on_logQSOButton_clicked();
  void on_actionJT9_triggered();
  void on_actionT10_triggered();
  void on_actionJT65_triggered();
  void on_actionJT9_JT65_triggered();
  void on_actionJT4_triggered();
  void on_TxFreqSpinBox_valueChanged(int arg1);
  void on_actionSave_decoded_triggered();
  void on_actionQuickDecode_triggered();
  void on_actionMediumDecode_triggered();
  void on_actionDeepestDecode_triggered();
  void bumpFqso(int n);
  void on_actionErase_ALL_TXT_triggered();
  void on_actionErase_wsjtx_log_adi_triggered();
  void on_actionOpen_wsjtx_log_adi_triggered();
  void startTx2();
  void stopTx();
  void stopTx2();
  void on_pbCallCQ_clicked();
  void on_pbAnswerCaller_clicked();
  void on_pbSendRRR_clicked();
  void on_pbAnswerCQ_clicked();
  void on_pbSendReport_clicked();
  void on_pbSend73_clicked();
  void on_rbGenMsg_clicked(bool checked);
  void on_rbFreeText_clicked(bool checked);
  void on_freeTextMsg_currentTextChanged (QString const&);
  void on_rptSpinBox_valueChanged(int n);
  void killFile();
  void on_tuneButton_clicked (bool);
  void on_pbR2T_clicked();
  void on_pbT2R_clicked();
  void acceptQSO2(QDateTime const&, QString const& call, QString const& grid
                  , Frequency dial_freq, QString const& mode
                  , QString const& rpt_sent, QString const& rpt_received
                  , QString const& tx_power, QString const& comments
                  , QString const& name, QDateTime const&);
  void on_bandComboBox_currentIndexChanged (int index);
  void on_bandComboBox_activated (int index);
  void on_readFreq_clicked();
  void on_pbTxMode_clicked();
  void on_RxFreqSpinBox_valueChanged(int n);
  void on_cbTxLock_clicked(bool checked);
  void on_skipTx1_clicked(bool checked);
  void on_skipGrid_clicked(bool checked);
  void on_outAttenuation_valueChanged (int);
  void rigOpen ();
  void handle_transceiver_update (Transceiver::TransceiverState const&);
  void handle_transceiver_failure (QString const& reason);
  void on_actionAstronomical_data_toggled (bool);
  void on_actionShort_list_of_add_on_prefixes_and_suffixes_triggered();
  void band_changed (Frequency);
  void monitor (bool);
  void stop_tuning ();
  void stopHint_call3_rxfreq();
  void stopTuneATU();
  void auto_tx_mode(bool);
  void auto_button_off();
  void on_actionMessage_averaging_triggered();
  void on_actionInclude_averaging_triggered();
  void on_hintButton_clicked(bool);
  void on_disableTx73Button_clicked(bool);
  void on_AutoTxButton_clicked(bool);
  void on_AutoSeqButton_clicked(bool);
  void VHF_controls_visible(bool b);
  void VHF_features_enabled(bool b);
  void on_sbSubmode_valueChanged(int n);
  void networkError (QString const&);
  void on_cbMenus_toggled(bool b);
  void on_actionWSPR_2_triggered();
  void on_actionWSPR_15_triggered();
  void on_TxPowerComboBox_currentIndexChanged(const QString &arg1);
  void on_sbTxPercent_valueChanged(int n);
  void on_cbUploadWSPR_Spots_toggled(bool b);
  void WSPR_config(bool b);
  void uploadSpots();
  void TxAgain();
  void RxQSY();
  void uploadResponse(QString response);
  void p3ReadFromStdout();
  void p3ReadFromStderr();
  void p3Error(QProcess::ProcessError e);
  void on_WSPRfreqSpinBox_valueChanged(int n);
  void on_pbTxNext_clicked(bool b);
  void on_actionEcho_Graph_triggered();
  void on_actionEcho_triggered();
  void on_actionISCAT_triggered();
  void on_actionFast_Graph_triggered();
  void fast_decode_done();
  void on_actionSave_reference_spectrum_triggered();
  void on_sbTR_valueChanged(int index);
  void on_sbFtol_valueChanged(int index);
  void on_cbFast9_clicked(bool b);
  void on_actionJTMSK_triggered();
  void set_scheduler(QString const& band,bool mixed);

private:
  Q_SIGNAL void initializeAudioOutputStream (QAudioDeviceInfo,
      unsigned channels, unsigned msBuffered) const;
  Q_SIGNAL void stopAudioOutputStream () const;
  Q_SIGNAL void startAudioInputStream (QAudioDeviceInfo const&,
      int framesPerBuffer, AudioDevice * sink,
      unsigned downSampleFactor, AudioDevice::Channel) const;
  Q_SIGNAL void suspendAudioInputStream () const;
  Q_SIGNAL void resumeAudioInputStream () const;
  Q_SIGNAL void startDetector (AudioDevice::Channel) const;
  Q_SIGNAL void FFTSize (unsigned) const;
  Q_SIGNAL void detectorClose () const;
  Q_SIGNAL void finished () const;
  Q_SIGNAL void transmitFrequency (double) const;
  Q_SIGNAL void endTransmitMessage (bool quick = false) const;
  Q_SIGNAL void tune (bool = true) const;
  Q_SIGNAL void sendMessage (unsigned symbolsLength, double framesPerSymbol,
      double frequency, double toneSpacing,
      SoundOutput *, AudioDevice::Channel = AudioDevice::Mono,
      bool synchronize = true, bool fastMode = false, double dBSNR = 99.,
                             int TRperiod=60) const;
  Q_SIGNAL void outAttenuationChanged (qreal) const;
  Q_SIGNAL void toggleShorthand () const;

private:
  void astroUpdate ();
  void hideMenus (bool b);

  QDir m_dataDir;
  QString m_revision;
  bool m_multiple;
  QSettings * m_settings;

  QScopedPointer<Ui::MainWindow> ui;

  // other windows
  Configuration m_config;
  WSPRBandHopping m_WSPR_band_hopping;
  bool m_WSPR_tx_next;
  QMessageBox m_rigErrorMessageBox;
  QScopedPointer<SampleDownloader> m_sampleDownloader;

  QScopedPointer<WideGraph> m_wideGraph;
  QScopedPointer<EchoGraph> m_echoGraph;
  QScopedPointer<FastGraph> m_fastGraph;
  QScopedPointer<LogQSO> m_logDlg;
  QScopedPointer<Astro> m_astroWidget;
  QScopedPointer<HelpTextWindow> m_shortcuts;
  QScopedPointer<HelpTextWindow> m_prefixes;
  QScopedPointer<HelpTextWindow> m_mouseCmnds;
  QScopedPointer<MessageAveraging> m_msgAvgWidget;

  Transceiver::TransceiverState m_rigState;
  Frequency  m_lastDialFreq;
  QString m_lastBand;
  Frequency  m_callingFrequency;
  Frequency  m_dialFreqRxWSPR;  // best guess at WSPR QRG

  Detector * m_detector;
  unsigned m_FFTSize;
  SoundInput * m_soundInput;
  Modulator * m_modulator;
  SoundOutput * m_soundOutput;
  QThread m_audioThread;

  qint64  m_msErase;
  qint64  m_secBandChanged;
  qint64  m_freqMoon;
  Frequency m_freqNominal;
  Frequency m_freqTxNominal;
  Astro::Correction m_astroCorrection;

  double  m_s6;
  double  m_tRemaining;

  float   m_DTtol;
  float   m_t0;
  float   m_t1;
  float   m_t0Pick;
  float   m_t1Pick;

  qint32  m_waterfallAvg;
  qint32  m_ntx;
  qint32  m_addtx;
  qint32  m_nlasttx;
  qint32  m_timeout;
  qint32  m_XIT;
  qint32  m_setftx;
  qint32  m_ndepth;
  qint32  m_sec0;
  qint32  m_RxLog;
  qint32  m_nutc0;
  qint32  m_ntr;
  qint32  m_tx;
  qint32  m_hsym;
  qint32  m_TRperiod;
  qint32  m_nsps;
  qint32  m_hsymStop;
  qint32  m_inGain;
  qint32  m_ncw;
  qint32  m_secID;
  qint32  m_repeatMsg;
  qint32  m_watchdogLimit;
  qint32  m_nSubMode;
  qint32  m_nclearave;
  qint32  m_dBm;
  qint32  m_pctx;
  qint32  m_nseq;
  qint32  m_nWSPRdecodes;
  qint32  m_k0;
  qint32  m_kdone;
  qint32  m_nPick;
  qint32  m_TRindex;
  qint32  m_FtolIndex;
  qint32  m_Ftol;
  qint32  m_TRperiodFast;
  qint32  m_nTx73;
  qint32  m_norprtrcvdCQ;
  qint32  m_norprtrcvdRCQ;

  bool    m_btxok;		//True if OK to transmit
  bool    m_diskData;
  bool    m_loopall;
  bool    m_decoderBusy;
  bool    m_txFirst;
  bool    m_cqdx;
  bool    m_rrr;
  bool    m_auto;
  bool    m_restart;
  bool    m_startAnother;
  bool    m_saveDecoded;
  bool    m_saveAll;
  bool    m_showHarmonics;
  bool	  m_showMyCallMsgRxWindow;
  bool    m_bypassRxfFilters;
  bool    m_bypassAllFilters;
  bool    m_windowPopup;
  bool    m_widebandDecode;
  bool    m_call3Modified;
  bool    m_dataAvailable;
  bool    m_killAll;
  bool    m_bDecoded;
  bool    m_noSuffix;
  bool    m_blankLine;
  bool    m_notified;
  bool    m_labUTCmin;
  bool    m_decodedText2;
  bool    m_freeText;
  bool    m_sentFirst73;
  int     m_currentMessageType;
  QString m_currentMessage;
  int     m_lastMessageType;
  QString m_lastMessageSent;
  bool    m_lockTxFreq;
  bool    m_skipTx1;
  bool    m_swl;
  bool    m_filter;
  bool    m_agcc;
  bool    m_hint;
  bool    m_disable_TX_on_73;
  bool    m_quick_call;
  bool    m_autoseq;
  bool    m_uploadSpots;
  bool    m_uploading;
  bool    m_txNext;
  bool    m_grid6;
  bool    m_tuneup;
  bool    m_bTxTime;
  bool    m_rxDone;
  bool    m_bSimplex; // not using split even if it is available
  bool    m_bEchoTxOK;
  bool    m_bTransmittedEcho;
  bool    m_bEchoTxed;
  bool    m_bFastMode;
  bool    m_bFast9;
  bool    m_bFastDecodeCalled;
  bool    m_bDoubleClickAfterCQnnn;
  bool	  m_logqso73;
  bool	  m_firstdecmycall;
  bool    m_mycalldecoded;
  bool	  m_nonstdrcvd;
  QString m_rightbutton;
  QString m_repliedCQ;
  QString m_dxbcallTxHalted;
  QString m_currentQSOcallsign;
  enum
    {
      CALLING,
      REPLYING,
      REPORT,
      ROGER_REPORT,
      ROGERS,
      SIGNOFF
    }
    m_QSOProgress;
  enum
    {
	  NONE,
      CALLINGTX,
      REPLYINGTX,
      REPORTTX,
      ROGER_REPORTTX,
      ROGERSTX,
      SIGNOFFTX
    }
    m_QSOProgressTX;
	QStringList m_qsohistory;
  float   m_pctZap;

  char    m_msg[100][80];

  // labels in status bar
  QLabel * tx_status_label;
  QLabel * mode_label;
  QLabel * last_tx_label;
  QLabel * auto_tx_label;
  QProgressBar* progressBar;
  QLabel * date_label;
  QLabel * qso_count;

  QMessageBox msgBox0;

  QFuture<void> m_wav_future;
  QFutureWatcher<void> m_wav_future_watcher;
  QFutureWatcher<void> watcher3;

  QProcess proc_jtdxjt9;
  QProcess p1;
  QProcess p3;

  WSPRNet *wsprNet;

  QTimer m_guiTimer;
  QTimer ptt1Timer;                 //StartTx delay
  QTimer stophintTimer;             //Stops propagation non-CQ CALL3 based Hint decoded messages
  QTimer ptt0Timer;                 //StopTx delay
  QTimer logQSOTimer;
  QTimer killFileTimer;
  QTimer tuneButtonTimer;
  QTimer cqButtonTimer;
  QTimer autoButtonTimer;
  QTimer tx73ButtonTimer;
  QTimer logClearDXTimer;
  QTimer dxbcallTxHaltedClearTimer;
  QTimer uploadTimer;
  QTimer tuneATU_Timer;
  QTimer TxAgainTimer;
  QTimer RxQSYTimer;

  QString m_path;
//  QString m_pbswl_style;
//  QString m_pbfilter_style;
  QString m_baseCall;
  QString m_hisCall;
  QString m_hisCallOld;
  QString m_hisGrid;
  QString m_hisGridOld;
  QString m_wantedCall;
  QString m_appDir;
  QString m_palette;
  QString m_dateTime;
  QString m_mode;
  QString m_oldmode;
  QString m_modeTx;
  QString m_fname;
  QString m_rpt;
  QString m_rptSent;
  QString m_rptRcvd;
  QString m_qsoStart;
  QString m_qsoStop;
  QString m_cmnd;
  QString m_msgSent0;
  QString m_fileToKill;
  QString m_fileToSave;
  QString m_calls;

  QSet<QString> m_pfx;
  QSet<QString> m_sfx;

  QDateTime m_dateTimeQSOOn;

  QSharedMemory *mem_jtdxjt9;
  LogBook m_logBook;
  DecodedText m_QSOText;
  unsigned m_msAudioOutputBuffered;
  unsigned m_framesAudioInputBuffered;
  unsigned m_downSampleFactor;
  QThread::Priority m_audioThreadPriority;
  bool m_bandEdited;
  bool m_splitMode;
  bool m_monitoring;
  bool m_transmitting;
  bool m_tune;
  bool m_block_pwr_tooltip;
  bool m_PwrBandSetOK;
  Frequency m_lastMonitoredFrequency;
  double m_toneSpacing;
  int m_firstDecode;
  QProgressDialog m_optimizingProgress;
  QTimer m_heartbeat;
  MessageClient * m_messageClient;
  PSK_Reporter *psk_Reporter;
  DisplayManual m_manual;
  QHash<QString, QVariant> m_pwrBandTxMemory; // Remembers power level by band
  QHash<QString, QVariant> m_pwrBandTuneMemory; // Remembers power level by band for tuning

  //---------------------------------------------------- private functions
  void readSettings();
  void setDecodedTextFont (QFont const&);
  void writeSettings();
  void createStatusBar();
  void updateStatusBar();
  void msgBox(QString t);
  void genStdMsgs(QString rpt);
  void clearDX ();
  void logClearDX ();
  void dxbcallTxHaltedClear ();
  void lookup();
  void ba2msg(QByteArray ba, char* message);
  void msgtype(QString t, QLineEdit* tx);
  void stub();
  void statusChanged();
  bool gridOK(QString g);
  bool gridRR73(QString g);
  bool reportRCVD(QStringList msg);
  bool rReportRCVD(QStringList msg);
  bool shortList(QString callsign);
  void transmit (double snr = 99.);
  void rigFailure (QString const& reason, QString const& detail);
  void pskSetLocal ();
  void displayDialFrequency ();
  void transmitDisplay (bool);
  void processMessage(QString const& messages, qint32 position, bool ctrl);
  void replyToCQ (QTime, qint32 snr, float delta_time, quint32 delta_frequency, QString const& mode, QString const& message_text);
  void replayDecodes ();
  void postDecode (bool is_new, QString const& message);
  void postWSPRDecode (bool is_new, QStringList message_parts);
  void enable_DXCC_entity ();
  void switch_mode (Mode);
  void WSPR_scheduling ();
  void setRig ();
  void WSPR_history(Frequency dialFreq, int ndecodes);
  QString WSPR_hhmm(int n);
  void fast_config(bool b);
  void save_wave_file (QString const& name, short const * data, int seconds) const;
  void read_wav_file (QString const& fname);
};

extern int killbyname(const char* progName);
extern void getDev(int* numDevices,char hostAPI_DeviceName[][50],
                   int minChan[], int maxChan[],
                   int minSpeed[], int maxSpeed[]);
extern int next_tx_state(int pctx);

#endif // MAINWINDOW_H
