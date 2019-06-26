// This source code file was last time modified by Igor UA3DJY on September 25th, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

#include "Configuration.hpp"

//
// Read me!
//
// This file defines a configuration dialog with the user. The general
// strategy is to expose agreed  configuration parameters via a custom
// interface (See  Configuration.hpp). The state exposed  through this
// public   interface  reflects   stored  or   derived  data   in  the
// Configuration::impl object.   The Configuration::impl  structure is
// an implementation of the PIMPL (a.k.a.  Cheshire Cat or compilation
// firewall) implementation hiding idiom that allows internal state to
// be completely removed from the public interface.
//
// There  is a  secondary level  of parameter  storage which  reflects
// current settings UI  state, these parameters are not  copied to the
// state   store  that   the  public   interface  exposes   until  the
// Configuration:impl::accept() operation is  successful. The accept()
// operation is  tied to the settings  OK button. The normal  and most
// convenient place to store this intermediate settings UI state is in
// the data models of the UI  controls, if that is not convenient then
// separate member variables  must be used to store that  state. It is
// important for the user experience that no publicly visible settings
// are changed  while the  settings UI are  changed i.e.  all settings
// changes   must    be   deferred   until   the    "OK"   button   is
// clicked. Conversely, all changes must  be discarded if the settings
// UI "Cancel" button is clicked.
//
// There is  a complication related  to the radio interface  since the
// this module offers  the facility to test the  radio interface. This
// test means  that the  public visibility to  the radio  being tested
// must be  changed.  To  maintain the  illusion of  deferring changes
// until they  are accepted, the  original radio related  settings are
// stored upon showing  the UI and restored if the  UI is dismissed by
// canceling.
//
// It  should be  noted that  the  settings UI  lives as  long as  the
// application client that uses it does. It is simply shown and hidden
// as it  is needed rather than  creating it on demand.  This strategy
// saves a  lot of  expensive UI  drawing at the  expense of  a little
// storage and  gives a  convenient place  to deliver  settings values
// from.
//
// Here is an overview of the high level flow of this module:
//
// 1)  On  construction the  initial  environment  is initialized  and
// initial   values  for   settings  are   read  from   the  QSettings
// database. At  this point  default values for  any new  settings are
// established by  providing a  default value  to the  QSettings value
// queries. This should be the only place where a hard coded value for
// a   settings  item   is   defined.   Any   remaining  one-time   UI
// initialization is also done. At the end of the constructor a method
// initialize_models()  is called  to  load the  UI  with the  current
// settings values.
//
// 2) When the settings UI is displayed by a client calling the exec()
// operation, only temporary state need be stored as the UI state will
// already mirror the publicly visible settings state.
//
// 3) As  the user makes  changes to  the settings UI  only validation
// need be  carried out since the  UI control data models  are used as
// the temporary store of unconfirmed settings.  As some settings will
// depend  on each  other a  validate() operation  is available,  this
// operation implements a check of any complex multi-field values.
//
// 4) If the  user discards the settings changes by  dismissing the UI
// with the  "Cancel" button;  the reject()  operation is  called. The
// reject() operation calls initialize_models()  which will revert all
// the  UI visible  state  to  the values  as  at  the initial  exec()
// operation.  No   changes  are  moved   into  the  data   fields  in
// Configuration::impl that  reflect the  settings state  published by
// the public interface (Configuration.hpp).
//
// 5) If  the user accepts the  settings changes by dismissing  the UI
// with the "OK" button; the  accept() operation is called which calls
// the validate() operation  again and, if it passes,  the fields that
// are used  to deliver  the settings  state are  updated from  the UI
// control models  or other temporary  state variables. At the  end of
// the accept()  operation, just  before hiding  the UI  and returning
// control to the caller; the new  settings values are stored into the
// settings database by a call to the write_settings() operation, thus
// ensuring that  settings changes are  saved even if  the application
// crashes or is subsequently killed.
//
// 6)  On  destruction,  which   only  happens  when  the  application
// terminates,  the settings  are saved  to the  settings database  by
// calling the  write_settings() operation. This is  largely redundant
// but is still done to save the default values of any new settings on
// an initial run.
//
// To add a new setting:
//
// 1) Update the UI with the new widget to view and change the value.
//
// 2)  Add  a member  to  Configuration::impl  to store  the  accepted
// setting state. If the setting state is dynamic; add a new signal to
// broadcast the setting value.
//
// 3) Add a  query method to the  public interface (Configuration.hpp)
// to access the  new setting value. If the settings  is dynamic; this
// step  is optional  since  value  changes will  be  broadcast via  a
// signal.
//
// 4) Add a forwarding operation to implement the new query (3) above.
//
// 5)  Add a  settings read  call to  read_settings() with  a sensible
// default value. If  the setting value is dynamic, add  a signal emit
// call to broadcast the setting value change.
//
// 6) Add  code to  initialize_models() to  load the  widget control's
// data model with the current value.
//
// 7) If there is no convenient data model field, add a data member to
// store the proposed new value. Ensure  this member has a valid value
// on exit from read_settings().
//
// 8)  Add  any  required  inter-field validation  to  the  validate()
// operation.
//
// 9) Add code to the accept()  operation to extract the setting value
// from  the  widget   control  data  model  and  load   it  into  the
// Configuration::impl  member  that  reflects  the  publicly  visible
// setting state. If  the setting value is dynamic; add  a signal emit
// call to broadcast any changed state of the setting.
//
// 10) Add  a settings  write call  to save the  setting value  to the
// settings database.
//

#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <functional>
#include <limits>
#include <cmath>

#include <QApplication>
#include <QMetaType>
#include <QList>
#include <QSettings>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QDialog>
#include <QMessageBox>
#include <QAction>
#include <QFileDialog>
#include <QDir>
#include <QTemporaryFile>
#include <QFormLayout>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QIntValidator>
#include <QThread>
#include <QTimer>
#include <QStandardPaths>
#include <QFont>
#include <QFontDialog>
#include <QColorDialog>
#include <QSerialPortInfo>
#include <QScopedPointer>
#include <QDebug>

#include "qt_helpers.hpp"
#include "MetaDataRegistry.hpp"
#include "SettingsGroup.hpp"
#include "FrequencyLineEdit.hpp"
#include "CandidateKeyFilter.hpp"
#include "ForeignKeyDelegate.hpp"
#include "TransceiverFactory.hpp"
#include "Transceiver.hpp"
#include "Bands.hpp"
#include "Modes.hpp"
#include "FrequencyList.hpp"
#include "StationList.hpp"
#include "NetworkServerLookup.hpp"

#include "pimpl_impl.hpp"

#include "ui_Configuration.h"
#include "moc_Configuration.cpp"

namespace
{
  // these undocumented flag values when stored in (Qt::UserRole - 1)
  // of a ComboBox item model index allow the item to be enabled or
  // disabled
  int const combo_box_item_enabled (32 | 1);
  int const combo_box_item_disabled (0);

//  QRegExp message_alphabet {"[- A-Za-z0-9+./?]*"};
  QRegExp message_alphabet {"[- @A-Za-z0-9+./?#<>]*"};

}


//
// Dialog to get a new Frequency item
//
class FrequencyDialog final
  : public QDialog
{
public:
  using Item = FrequencyList::Item;

  explicit FrequencyDialog (Modes * modes_model, QWidget * parent = nullptr)
    : QDialog {parent}
  {
    setWindowTitle (QApplication::applicationName () + " - " +
                    tr ("Add Frequency"));
    mode_combo_box_.setModel (modes_model);

    auto form_layout = new QFormLayout ();
    form_layout->addRow (tr ("&Mode:"), &mode_combo_box_);
    form_layout->addRow (tr ("&Frequency (MHz):"), &frequency_line_edit_);

    auto main_layout = new QVBoxLayout (this);
    main_layout->addLayout (form_layout);

    auto button_box = new QDialogButtonBox {QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
    main_layout->addWidget (button_box);

    connect (button_box, &QDialogButtonBox::accepted, this, &FrequencyDialog::accept);
    connect (button_box, &QDialogButtonBox::rejected, this, &FrequencyDialog::reject);
  }

  Item item () const
  {
    return {frequency_line_edit_.frequency (), Modes::value (mode_combo_box_.currentText ())};
  }

private:
  QComboBox mode_combo_box_;
  FrequencyLineEdit frequency_line_edit_;
};


//
// Dialog to get a new Station item
//
class StationDialog final
  : public QDialog
{
public:
  explicit StationDialog (StationList const * stations, Bands * bands, QWidget * parent = nullptr)
    : QDialog {parent}
    , filtered_bands_ {new CandidateKeyFilter {bands, stations, 0, 0}}
  {
    setWindowTitle (QApplication::applicationName () + " - " + tr ("Add Station"));

    band_.setModel (filtered_bands_.data ());
      
    auto form_layout = new QFormLayout ();
    form_layout->addRow (tr ("&Band:"), &band_);
    form_layout->addRow (tr ("&Offset (MHz):"), &delta_);
    form_layout->addRow (tr ("&Antenna:"), &description_);

    auto main_layout = new QVBoxLayout (this);
    main_layout->addLayout (form_layout);

    auto button_box = new QDialogButtonBox {QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
    main_layout->addWidget (button_box);

    connect (button_box, &QDialogButtonBox::accepted, this, &StationDialog::accept);
    connect (button_box, &QDialogButtonBox::rejected, this, &StationDialog::reject);

    if (delta_.text ().isEmpty ())
      {
        delta_.setText ("0");
      }
  }

  StationList::Station station () const
  {
    return {band_.currentText (), delta_.frequency_delta (), description_.text ()};
  }

  int exec () override
  {
    filtered_bands_->set_active_key ();
    return QDialog::exec ();
  }

private:
  QScopedPointer<CandidateKeyFilter> filtered_bands_;

  QComboBox band_;
  FrequencyDeltaLineEdit delta_;
  QLineEdit description_;
};

class RearrangableMacrosModel
  : public QStringListModel
{
public:
  Qt::ItemFlags flags (QModelIndex const& index) const override
  {
    auto flags = QStringListModel::flags (index);
    if (index.isValid ())
      {
        // disallow drop onto existing items
        flags &= ~Qt::ItemIsDropEnabled;
      }
    return flags;
  }
};


//
// Class MessageItemDelegate
//
//	Item delegate for message entry such as free text message macros.
//
class MessageItemDelegate final
  : public QStyledItemDelegate
{
public:
  explicit MessageItemDelegate (QObject * parent = nullptr)
    : QStyledItemDelegate {parent}
  {
  }

  QWidget * createEditor (QWidget * parent
                          , QStyleOptionViewItem const& /* option*/
                          , QModelIndex const& /* index */
                          ) const override
  {
    auto editor = new QLineEdit {parent};
    editor->setFrame (false);
    editor->setValidator (new QRegExpValidator {message_alphabet, editor});
    return editor;
  }
};

// Internal implementation of the Configuration class.
class Configuration::impl final
  : public QDialog
{
  Q_OBJECT;

public:
  using FrequencyDelta = Radio::FrequencyDelta;
  using port_type = Configuration::port_type;

  explicit impl (Configuration * self, QSettings * settings, QWidget * parent);
  ~impl ();

  bool have_rig ();

  void transceiver_frequency (Frequency);
  void transceiver_tx_frequency (Frequency);
  void transceiver_mode (MODE);
  void transceiver_ptt (bool);
  void sync_transceiver (bool force_signal);

  Q_SLOT int exec () override;
  Q_SLOT void accept () override;
  Q_SLOT void reject () override;
  Q_SLOT void done (int) override;

private:
  typedef QList<QAudioDeviceInfo> AudioDevices;

  void read_settings ();
  void write_settings ();

  bool load_audio_devices (QAudio::Mode, QComboBox *, QAudioDeviceInfo *);
  void update_audio_channels (QComboBox const *, int, QComboBox *, bool);

  void set_application_font (QFont const&);

  void initialize_models ();
  bool split_mode () const
  {
    return
      (WSJT_RIG_NONE_CAN_SPLIT || !rig_is_dummy_) &&
      (rig_params_.split_mode != TransceiverFactory::split_mode_none);
  }
  bool open_rig (bool force = false);
  //bool set_mode ();
  void close_rig ();
  TransceiverFactory::ParameterPack gather_rig_data ();
  void enumerate_rigs ();
  void set_rig_invariants ();
  bool validate ();
  void message_box (QString const& reason, QString const& detail = QString ());
  void fill_port_combo_box (QComboBox *);
  Frequency apply_calibration (Frequency) const;
  Frequency remove_calibration (Frequency) const;

  Q_SLOT void on_font_push_button_clicked ();
  Q_SLOT void on_decoded_text_font_push_button_clicked ();
  Q_SLOT void on_PTT_port_combo_box_activated (int);
  Q_SLOT void on_CAT_port_combo_box_activated (int);
  Q_SLOT void on_CAT_serial_baud_combo_box_currentIndexChanged (int);
  Q_SLOT void on_CAT_data_bits_button_group_buttonClicked (int);
  Q_SLOT void on_CAT_stop_bits_button_group_buttonClicked (int);
  Q_SLOT void on_CAT_handshake_button_group_buttonClicked (int);
  Q_SLOT void on_CAT_poll_interval_spin_box_valueChanged (int);
  Q_SLOT void on_split_mode_button_group_buttonClicked (int);
  Q_SLOT void on_test_CAT_push_button_clicked ();
  Q_SLOT void on_test_PTT_push_button_clicked (bool checked);
  Q_SLOT void on_force_DTR_combo_box_currentIndexChanged (int);
  Q_SLOT void on_force_RTS_combo_box_currentIndexChanged (int);
  Q_SLOT void on_rig_combo_box_currentIndexChanged (int);
  Q_SLOT void on_sound_input_combo_box_currentTextChanged (QString const&);
  Q_SLOT void on_sound_output_combo_box_currentTextChanged (QString const&);
  Q_SLOT void on_add_macro_push_button_clicked (bool = false);
  Q_SLOT void on_delete_macro_push_button_clicked (bool = false);
  Q_SLOT void on_PTT_method_button_group_buttonClicked (int);
  Q_SLOT void on_callsign_line_edit_editingFinished ();
  Q_SLOT void on_grid_line_edit_editingFinished ();
  Q_SLOT void on_add_macro_line_edit_editingFinished ();
  Q_SLOT void delete_macro ();
  void delete_selected_macros (QModelIndexList);
  Q_SLOT void on_save_path_select_push_button_clicked (bool);
  Q_SLOT void on_azel_path_select_push_button_clicked (bool);
  Q_SLOT void delete_frequencies ();
  Q_SLOT void on_reset_frequencies_push_button_clicked (bool);
  Q_SLOT void insert_frequency ();
  Q_SLOT void delete_stations ();
  Q_SLOT void insert_station ();
  Q_SLOT void handle_transceiver_update (TransceiverState const&, unsigned sequence_number);
  Q_SLOT void handle_transceiver_failure (QString const& reason);
  Q_SLOT void on_countryName_check_box_clicked(bool checked);
  Q_SLOT void on_ShowCQ_check_box_clicked(bool checked);
  Q_SLOT void on_ShowCQ73_check_box_clicked(bool checked);
  Q_SLOT void on_prompt_to_log_check_box_clicked(bool checked);
  Q_SLOT void on_autolog_check_box_clicked(bool checked);

  Q_SLOT void on_txtColor_check_box_clicked(bool checked);
  Q_SLOT void on_workedStriked_check_box_clicked(bool checked);
  Q_SLOT void on_workedUnderlined_check_box_clicked(bool checked);
  Q_SLOT void on_workedColor_check_box_clicked(bool checked);
  Q_SLOT void on_workedDontShow_check_box_clicked(bool checked);
  Q_SLOT void on_newDXCC_check_box_clicked(bool checked);
  Q_SLOT void on_newCall_check_box_clicked(bool checked);
  Q_SLOT void on_newGrid_check_box_clicked(bool checked);
  Q_SLOT void on_newDXCCBand_check_box_clicked(bool checked);
  Q_SLOT void on_newCallBand_check_box_clicked(bool checked);
  Q_SLOT void on_newGridBand_check_box_clicked(bool checked);
  Q_SLOT void on_newDXCCBandMode_check_box_clicked(bool checked);
  Q_SLOT void on_newCallBandMode_check_box_clicked(bool checked);
  Q_SLOT void on_newGridBandMode_check_box_clicked(bool checked);
  Q_SLOT void on_newPotential_check_box_clicked(bool checked);

  Q_SLOT void on_eqsluser_edit_editingFinished();
  Q_SLOT void on_eqslpasswd_edit_editingFinished();
  Q_SLOT void on_eqslnick_edit_editingFinished();

  Q_SLOT void on_pbCQmsg_clicked();
  Q_SLOT void on_pbMyCall_clicked();
  Q_SLOT void on_pbTxMsg_clicked();
  Q_SLOT void on_pbNewDXCC_clicked();
  Q_SLOT void on_pbNewDXCCBand_clicked();
  Q_SLOT void on_pbNewCall_clicked();
  Q_SLOT void on_pbNewCallBand_clicked();
  Q_SLOT void on_pbNewGrid_clicked();
  Q_SLOT void on_pbNewGridBand_clicked();
  Q_SLOT void on_pbStandardCall_clicked();
  Q_SLOT void on_pbWorkedCall_clicked();

  Q_SLOT void on_hhComboBox_1_currentIndexChanged (int);
  Q_SLOT void on_mmComboBox_1_currentIndexChanged (int);
  Q_SLOT void on_hhComboBox_2_currentIndexChanged (int);
  Q_SLOT void on_mmComboBox_2_currentIndexChanged (int);
  Q_SLOT void on_hhComboBox_3_currentIndexChanged (int);
  Q_SLOT void on_mmComboBox_3_currentIndexChanged (int);
  Q_SLOT void on_hhComboBox_4_currentIndexChanged (int);
  Q_SLOT void on_mmComboBox_4_currentIndexChanged (int);
  Q_SLOT void on_hhComboBox_5_currentIndexChanged (int);
  Q_SLOT void on_mmComboBox_5_currentIndexChanged (int);
  
  Q_SLOT void on_bandComboBox_1_currentIndexChanged (QString const&);
  Q_SLOT void on_bandComboBox_2_currentIndexChanged (QString const&);
  Q_SLOT void on_bandComboBox_3_currentIndexChanged (QString const&);
  Q_SLOT void on_bandComboBox_4_currentIndexChanged (QString const&);
  Q_SLOT void on_bandComboBox_5_currentIndexChanged (QString const&);

  // typenames used as arguments must match registered type names :(
  Q_SIGNAL void start_transceiver (unsigned seqeunce_number) const;
  Q_SIGNAL void set_transceiver (Transceiver::TransceiverState const&,
                                 unsigned sequence_number) const;
  Q_SIGNAL void stop_transceiver () const;

  Configuration * const self_;	// back pointer to public interface

  QThread * transceiver_thread_;
  TransceiverFactory transceiver_factory_;
  QList<QMetaObject::Connection> rig_connections_;

  QScopedPointer<Ui::configuration_dialog> ui_;

  QSettings * settings_;

  QDir doc_dir_;
  QDir data_dir_;
  QDir temp_dir_;
  QDir default_save_directory_;
  QDir save_directory_;
  QDir default_azel_directory_;
  QDir azel_directory_;

  QFont font_;
  QFont next_font_;

  QFont decoded_text_font_;
  QFont next_decoded_text_font_;

  bool restart_sound_input_device_;
  bool restart_sound_output_device_;

  Type2MsgGen type_2_msg_gen_;

  QStringListModel macros_;
  RearrangableMacrosModel next_macros_;
  QAction * macro_delete_action_;

  Bands bands_;
  Modes modes_;
  FrequencyList frequencies_;
  FrequencyList next_frequencies_;
  StationList stations_;
  StationList next_stations_;
  FrequencyDelta current_offset_;
  FrequencyDelta current_tx_offset_;

  QAction * frequency_delete_action_;
  QAction * frequency_insert_action_;
  FrequencyDialog * frequency_dialog_;

  QAction * station_delete_action_;
  QAction * station_insert_action_;
  StationDialog * station_dialog_;

  TransceiverFactory::ParameterPack rig_params_;
  TransceiverFactory::ParameterPack saved_rig_params_;
  TransceiverFactory::Capabilities::PortType last_port_type_;
  bool rig_is_dummy_;
  bool rig_active_;
  bool have_rig_;
  bool rig_changed_;
  TransceiverState cached_rig_state_;
  int rig_resolution_;          // see Transceiver::resolution signal
  double frequency_calibration_intercept_;
  double frequency_calibration_slope_ppm_;
  unsigned transceiver_command_number_;

  // configuration fields that we publish
  QString my_callsign_;
  QString my_grid_;
  QColor color_CQ_;
  QColor next_color_CQ_;
  QColor color_MyCall_;
  QColor next_color_MyCall_;
  QColor color_TxMsg_;
  QColor next_color_TxMsg_;
  QColor color_NewDXCC_;
  QColor next_color_NewDXCC_;
  QColor color_NewDXCCBand_;
  QColor next_color_NewDXCCBand_;
  QColor color_NewGrid_;
  QColor next_color_NewGrid_;
  QColor color_NewGridBand_;
  QColor next_color_NewGridBand_;
  QColor color_NewCall_;
  QColor next_color_NewCall_;
  QColor color_NewCallBand_;
  QColor next_color_NewCallBand_;
  QColor color_StandardCall_;
  QColor next_color_StandardCall_;
  QColor color_WorkedCall_;
  QColor next_color_WorkedCall_;
  qint32 id_interval_;
  qint32 ntrials_;
  qint32 ntrials10_;
  qint32 ntrialsrxf10_;
  qint32 npreampass_;
  qint32 aggressive_;
  qint32 eqsltimer_;
  qint32 harmonicsdepth_;
  qint32 nsingdecatt_;
  qint32 ntopfreq65_;
  qint32 nbacktocq_;
  qint32 nretransmitmsg_;
  qint32 nhalttxsamemsgrprt_;
  qint32 nhalttxsamemsg73_;
  bool fmaskact_;
  bool backtocq_;
  bool retransmitmsg_;
  bool halttxsamemsgrprt_;
  bool halttxsamemsg73_;
  bool halttxreplyother_;
  bool hidefree_;
  bool showcq_;
  bool showcq73_;
  bool id_after_73_;
  bool tx_QSY_allowed_;
  bool spot_to_psk_reporter_;
  bool prevent_spotting_false_;
  bool send_to_eqsl_;
  QString eqsl_username_;
  QString eqsl_passwd_;  
  QString eqsl_nickname_;
  bool usesched_;
  QString sched_hh_1_;
  QString sched_mm_1_;
  QString sched_band_1_;
  bool sched_mix_1_;
  QString sched_hh_2_;
  QString sched_mm_2_;
  QString sched_band_2_;
  bool sched_mix_2_;
  QString sched_hh_3_;
  QString sched_mm_3_;
  QString sched_band_3_;
  bool sched_mix_3_;
  QString sched_hh_4_;
  QString sched_mm_4_;
  QString sched_band_4_;
  bool sched_mix_4_;
  QString sched_hh_5_;
  QString sched_mm_5_;
  QString sched_band_5_;
  bool sched_mix_5_;
  bool monitor_off_at_startup_;
  bool monitor_last_used_;
  bool log_as_RTTY_;
  bool report_in_comments_;
  bool prompt_to_log_;
  bool autolog_;
  bool insert_blank_;
  bool countryName_;
  bool countryPrefix_;
  bool txtColor_;
  bool workedColor_;
  bool workedStriked_;
  bool workedUnderlined_;
  bool workedDontShow_;
  bool newDXCC_;
  bool newDXCCBand_;
  bool newDXCCBandMode_;
  bool newCall_;
  bool newCallBand_;
  bool newCallBandMode_;
  bool newGrid_;
  bool newGridBand_;
  bool newGridBandMode_;
  bool newPotential_;
  bool hideAfrica_;
  bool hideAntarctica_;
  bool hideAsia_;
  bool hideEurope_;
  bool hideOceania_;
  bool hideNAmerica_;
  bool hideSAmerica_;
  bool next_txtColor_;
  bool next_workedColor_;
  bool next_workedStriked_;
  bool next_workedUnderlined_;
  bool next_workedDontShow_;
  bool next_newDXCC_;
  bool next_newDXCCBand_;
  bool next_newDXCCBandMode_;
  bool next_newCall_;
  bool next_newCallBand_;
  bool next_newCallBandMode_;
  bool next_newGrid_;
  bool next_newGridBand_;
  bool next_newGridBandMode_;
  bool next_newPotential_;
  bool clear_DX_;
  bool clear_DX_exit_;
  bool miles_;
  bool watchdog_;
  bool TX_messages_;
  bool enable_VHF_features_;
  bool decode_at_52s_;
  bool offsetRxFreq_;
  bool beepOnMyCall_;
  bool beepOnNewDXCC_;
  bool beepOnNewGrid_;
  bool beepOnNewCall_;
  bool beepOnFirstMsg_;
  QString udp_server_name_;
  port_type udp_server_port_;
  QString tcp_server_name_;
  port_type tcp_server_port_;
  bool accept_udp_requests_;
  bool enable_tcp_connection_;
  bool udpWindowToFront_;
  bool udpWindowRestore_;
  DataMode data_mode_;
  bool pwrBandTxMemory_;
  bool pwrBandTuneMemory_;

  QAudioDeviceInfo audio_input_device_;
  bool default_audio_input_device_selected_;
  AudioDevice::Channel audio_input_channel_;
  QAudioDeviceInfo audio_output_device_;
  bool default_audio_output_device_selected_;
  AudioDevice::Channel audio_output_channel_;

  friend class Configuration;
};

#include "Configuration.moc"


// delegate to implementation class
Configuration::Configuration (QSettings * settings, QWidget * parent)
  : m_ {this, settings, parent}
{
}

Configuration::~Configuration ()
{
}

QDir Configuration::doc_dir () const {return m_->doc_dir_;}
QDir Configuration::data_dir () const {return m_->data_dir_;}
QDir Configuration::temp_dir () const {return m_->temp_dir_;}

int Configuration::exec () {return m_->exec ();}
bool Configuration::is_active () const {return m_->isVisible ();}

QAudioDeviceInfo const& Configuration::audio_input_device () const {return m_->audio_input_device_;}
AudioDevice::Channel Configuration::audio_input_channel () const {return m_->audio_input_channel_;}
QAudioDeviceInfo const& Configuration::audio_output_device () const {return m_->audio_output_device_;}
AudioDevice::Channel Configuration::audio_output_channel () const {return m_->audio_output_channel_;}
bool Configuration::restart_audio_input () const {return m_->restart_sound_input_device_;}
bool Configuration::restart_audio_output () const {return m_->restart_sound_output_device_;}
auto Configuration::type_2_msg_gen () const -> Type2MsgGen {return m_->type_2_msg_gen_;}
QString Configuration::my_callsign () const {return m_->my_callsign_;}
QString Configuration::my_grid () const {return m_->my_grid_;}
QString Configuration::hideContinents () const
{
  QString result;
  if (m_->hideAfrica_) result.append("AF");
  if (m_->hideAntarctica_) result.append("AN");
  if (m_->hideEurope_) result.append("EU");
  if (m_->hideAsia_) result.append("AS");
  if (m_->hideSAmerica_) result.append("SA");
  if (m_->hideOceania_) result.append("OC");
  if (m_->hideNAmerica_) result.append("NA");
  return result;
}
QColor Configuration::color_CQ () const {return m_->color_CQ_;}
QColor Configuration::color_MyCall () const {return m_->color_MyCall_;}
QColor Configuration::color_TxMsg () const {return m_->color_TxMsg_;}
QColor Configuration::color_NewDXCC () const {return m_->color_NewDXCC_;}
QColor Configuration::color_NewDXCCBand () const {return m_->color_NewDXCCBand_;}
QColor Configuration::color_NewGrid () const {return m_->color_NewGrid_;}
QColor Configuration::color_NewGridBand () const {return m_->color_NewGridBand_;}
QColor Configuration::color_NewCall () const {return m_->color_NewCall_;}
QColor Configuration::color_NewCallBand () const {return m_->color_NewCallBand_;}
QColor Configuration::color_StandardCall () const {return m_->color_StandardCall_;}
QColor Configuration::color_WorkedCall () const {return m_->color_WorkedCall_;}
QFont Configuration::decoded_text_font () const {return m_->decoded_text_font_;}
qint32 Configuration::id_interval () const {return m_->id_interval_;}
qint32 Configuration::ntrials() const {return m_->ntrials_;}
qint32 Configuration::ntrials10() const {return m_->ntrials10_;}
qint32 Configuration::ntrialsrxf10() const {return m_->ntrialsrxf10_;}
qint32 Configuration::npreampass() const {return m_->npreampass_;}
qint32 Configuration::aggressive() const {return m_->aggressive_;}
qint32 Configuration::eqsltimer() const {return m_->eqsltimer_;}
qint32 Configuration::harmonicsdepth() const {return m_->harmonicsdepth_;}
qint32 Configuration::nsingdecatt() const {return m_->nsingdecatt_;}
qint32 Configuration::ntopfreq65() const {return m_->ntopfreq65_;}
qint32 Configuration::nbacktocq() const {return m_->nbacktocq_;}
qint32 Configuration::nretransmitmsg() const {return m_->nretransmitmsg_;}
qint32 Configuration::nhalttxsamemsgrprt() const {return m_->nhalttxsamemsgrprt_;}
qint32 Configuration::nhalttxsamemsg73() const {return m_->nhalttxsamemsg73_;}
bool Configuration::fmaskact () const {return m_->fmaskact_;}
bool Configuration::backtocq () const {return m_->backtocq_;}
bool Configuration::retransmitmsg () const {return m_->retransmitmsg_;}
bool Configuration::halttxsamemsgrprt () const {return m_->halttxsamemsgrprt_;}
bool Configuration::halttxsamemsg73 () const {return m_->halttxsamemsg73_;}
bool Configuration::halttxreplyother () const {return m_->halttxreplyother_;}
bool Configuration::hidefree () const {return m_->hidefree_;}
bool Configuration::showcq () const {return m_->showcq_;}
bool Configuration::showcq73 () const {return m_->showcq73_;}
bool Configuration::id_after_73 () const {return m_->id_after_73_;}
bool Configuration::tx_QSY_allowed () const {return m_->tx_QSY_allowed_;}
bool Configuration::spot_to_psk_reporter () const
{
  // rig must be open and working to spot externally
  return is_transceiver_online () && m_->spot_to_psk_reporter_;
}
bool Configuration::prevent_spotting_false () const {return m_->prevent_spotting_false_;}
bool Configuration::monitor_off_at_startup () const {return m_->monitor_off_at_startup_;}
bool Configuration::monitor_last_used () const {return m_->rig_is_dummy_ || m_->monitor_last_used_;}
bool Configuration::log_as_RTTY () const {return m_->log_as_RTTY_;}
bool Configuration::send_to_eqsl () const {return m_->send_to_eqsl_;}
QString Configuration::eqsl_username () const {return m_->eqsl_username_;}
QString Configuration::eqsl_passwd () const {return m_->eqsl_passwd_;}
QString Configuration::eqsl_nickname () const {return m_->eqsl_nickname_;}
bool Configuration::usesched () const {return m_->usesched_;}
QString Configuration::sched_hh_1 () const {return m_->sched_hh_1_;}
QString Configuration::sched_mm_1 () const {return m_->sched_mm_1_;}
QString Configuration::sched_band_1 () const {return m_->sched_band_1_;}
bool Configuration::sched_mix_1 () const {return m_->sched_mix_1_;}
QString Configuration::sched_hh_2 () const {return m_->sched_hh_2_;}
QString Configuration::sched_mm_2 () const {return m_->sched_mm_2_;}
QString Configuration::sched_band_2 () const {return m_->sched_band_2_;}
bool Configuration::sched_mix_2 () const {return m_->sched_mix_2_;}
QString Configuration::sched_hh_3 () const {return m_->sched_hh_3_;}
QString Configuration::sched_mm_3 () const {return m_->sched_mm_3_;}
QString Configuration::sched_band_3 () const {return m_->sched_band_3_;}
bool Configuration::sched_mix_3 () const {return m_->sched_mix_3_;}
QString Configuration::sched_hh_4 () const {return m_->sched_hh_4_;}
QString Configuration::sched_mm_4 () const {return m_->sched_mm_4_;}
QString Configuration::sched_band_4 () const {return m_->sched_band_4_;}
bool Configuration::sched_mix_4 () const {return m_->sched_mix_4_;}
QString Configuration::sched_hh_5 () const {return m_->sched_hh_5_;}
QString Configuration::sched_mm_5 () const {return m_->sched_mm_5_;}
QString Configuration::sched_band_5 () const {return m_->sched_band_5_;}
bool Configuration::sched_mix_5 () const {return m_->sched_mix_5_;}
bool Configuration::report_in_comments () const {return m_->report_in_comments_;}
bool Configuration::prompt_to_log () const {return m_->prompt_to_log_;}
bool Configuration::autolog () const {return m_->autolog_;}
bool Configuration::insert_blank () const {return m_->insert_blank_;}
bool Configuration::countryName () const {return m_->countryName_;}
bool Configuration::countryPrefix () const {return m_->countryPrefix_;}
bool Configuration::txtColor () const {return m_->txtColor_;}
bool Configuration::workedColor () const {return m_->workedColor_;}
bool Configuration::workedStriked () const {return m_->workedStriked_;}
bool Configuration::workedUnderlined () const {return m_->workedUnderlined_;}
bool Configuration::workedDontShow () const {return m_->workedDontShow_;}
bool Configuration::newDXCC () const {return m_->newDXCC_;}
bool Configuration::newDXCCBand () const {return m_->newDXCCBand_;}
bool Configuration::newDXCCBandMode () const {return m_->newDXCCBandMode_;}
bool Configuration::newCall () const {return m_->newCall_;}
bool Configuration::newCallBand () const {return m_->newCallBand_;}
bool Configuration::newCallBandMode () const {return m_->newCallBandMode_;}
bool Configuration::newGrid () const {return m_->newGrid_;}
bool Configuration::newGridBand () const {return m_->newGridBand_;}
bool Configuration::newGridBandMode () const {return m_->newGridBandMode_;}
bool Configuration::newPotential () const {return m_->newPotential_;}
bool Configuration::clear_DX () const {return m_->clear_DX_;}
bool Configuration::clear_DX_exit () const {return m_->clear_DX_exit_;}
bool Configuration::miles () const {return m_->miles_;}
bool Configuration::watchdog () const {return m_->watchdog_;}
bool Configuration::TX_messages () const {return m_->TX_messages_;}
bool Configuration::enable_VHF_features () const {return m_->enable_VHF_features_;}
bool Configuration::decode_at_52s () const {return m_->decode_at_52s_;}
bool Configuration::offsetRxFreq () const {return m_->offsetRxFreq_;}
bool Configuration::beepOnMyCall () const {return m_->beepOnMyCall_;}
bool Configuration::beepOnNewDXCC () const {return m_->beepOnNewDXCC_;}
bool Configuration::beepOnNewGrid () const {return m_->beepOnNewGrid_;}
bool Configuration::beepOnNewCall () const {return m_->beepOnNewCall_;}
bool Configuration::beepOnFirstMsg () const {return m_->beepOnFirstMsg_;}
bool Configuration::split_mode () const {return m_->split_mode ();}
QString Configuration::udp_server_name () const {return m_->udp_server_name_;}
auto Configuration::udp_server_port () const -> port_type {return m_->udp_server_port_;}
QString Configuration::tcp_server_name () const {return m_->tcp_server_name_;}
auto Configuration::tcp_server_port () const -> port_type {return m_->tcp_server_port_;}
bool Configuration::accept_udp_requests () const {return m_->accept_udp_requests_;}
bool Configuration::enable_tcp_connection () const {return m_->enable_tcp_connection_;}
bool Configuration::udpWindowToFront () const {return m_->udpWindowToFront_;}
bool Configuration::udpWindowRestore () const {return m_->udpWindowRestore_;}
Bands * Configuration::bands () {return &m_->bands_;}
Bands const * Configuration::bands () const {return &m_->bands_;}
StationList * Configuration::stations () {return &m_->stations_;}
StationList const * Configuration::stations () const {return &m_->stations_;}
FrequencyList * Configuration::frequencies () {return &m_->frequencies_;}
FrequencyList const * Configuration::frequencies () const {return &m_->frequencies_;}
QStringListModel * Configuration::macros () {return &m_->macros_;}
QStringListModel const * Configuration::macros () const {return &m_->macros_;}
QDir Configuration::save_directory () const {return m_->save_directory_;}
QDir Configuration::azel_directory () const {return m_->azel_directory_;}
QString Configuration::rig_name () const {return m_->rig_params_.rig_name;}
bool Configuration::pwrBandTxMemory () const {return m_->pwrBandTxMemory_;}
bool Configuration::pwrBandTuneMemory () const {return m_->pwrBandTuneMemory_;}

bool Configuration::is_transceiver_online () const
{
  return m_->rig_active_;
}

bool Configuration::transceiver_online ()
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::transceiver_online: " << m_->cached_rig_state_;
#endif

  return m_->have_rig ();
}

int Configuration::transceiver_resolution () const
{
  return m_->rig_resolution_;
}

void Configuration::transceiver_offline ()
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::transceiver_offline:" << m_->cached_rig_state_;
#endif

  m_->close_rig ();
}

void Configuration::transceiver_frequency (Frequency f)
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::transceiver_frequency:" << f << m_->cached_rig_state_;
#endif
  m_->transceiver_frequency (f);
}

void Configuration::transceiver_tx_frequency (Frequency f)
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::transceiver_tx_frequency:" << f << m_->cached_rig_state_;
#endif

  m_->transceiver_tx_frequency (f);
}

void Configuration::transceiver_mode (MODE mode)
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::transceiver_mode:" << mode << m_->cached_rig_state_;
#endif

  m_->transceiver_mode (mode);
}

void Configuration::transceiver_ptt (bool on)
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::transceiver_ptt:" << on << m_->cached_rig_state_;
#endif

  m_->transceiver_ptt (on);
}

void Configuration::sync_transceiver (bool force_signal, bool enforce_mode_and_split)
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::sync_transceiver: force signal:" << force_signal << "enforce_mode_and_split:" << enforce_mode_and_split << m_->cached_rig_state_;
#endif

  m_->sync_transceiver (force_signal);
  if (!enforce_mode_and_split)
    {
      m_->transceiver_tx_frequency (0);
    }
}

Configuration::impl::impl (Configuration * self, QSettings * settings, QWidget * parent)
  : QDialog {parent}
  , self_ {self}
  , ui_ {new Ui::configuration_dialog}
  , settings_ {settings}
  , doc_dir_ {QApplication::applicationDirPath ()}
  , data_dir_ {QApplication::applicationDirPath ()}
  , frequencies_ {&bands_}
  , next_frequencies_ {&bands_}
  , stations_ {&bands_}
  , next_stations_ {&bands_}
  , current_offset_ {0}
  , current_tx_offset_ {0}
  , frequency_dialog_ {new FrequencyDialog {&modes_, this}}
  , station_dialog_ {new StationDialog {&next_stations_, &bands_, this}}
  , last_port_type_ {TransceiverFactory::Capabilities::none}
  , rig_is_dummy_ {false}
  , rig_active_ {false}
  , have_rig_ {false}
  , rig_changed_ {false}
  , rig_resolution_ {0}
  , transceiver_command_number_ {0}
  , default_audio_input_device_selected_ {false}
  , default_audio_output_device_selected_ {false}
{
  ui_->setupUi (this);

#if !defined (CMAKE_BUILD)
#define WSJT_SHARE_DESTINATION "."
#define WSJT_DOC_DESTINATION "."
#define WSJT_DATA_DESTINATION "."
#endif

#if !defined (Q_OS_WIN) || QT_VERSION >= 0x050300
  auto doc_path = QStandardPaths::locate (QStandardPaths::DataLocation, WSJT_DOC_DESTINATION, QStandardPaths::LocateDirectory);
  if (doc_path.isEmpty ())
    {
      doc_dir_.cdUp ();
#if defined (Q_OS_MAC)
      doc_dir_.cdUp ();
      doc_dir_.cdUp ();
#endif
      doc_dir_.cd (WSJT_SHARE_DESTINATION);
      doc_dir_.cd (WSJT_DOC_DESTINATION);
    }
  else
    {
      doc_dir_.cd (doc_path);
    }

  auto data_path = QStandardPaths::locate (QStandardPaths::DataLocation, WSJT_DATA_DESTINATION, QStandardPaths::LocateDirectory);
  if (data_path.isEmpty ())
    {
      data_dir_.cdUp ();
#if defined (Q_OS_MAC)
      data_dir_.cdUp ();
      data_dir_.cdUp ();
#endif
      data_dir_.cd (WSJT_SHARE_DESTINATION);
      data_dir_.cd (WSJT_DATA_DESTINATION);
    }
  else
    {
      data_dir_.cd (data_path);
    }
#else
  doc_dir_.cd (WSJT_DOC_DESTINATION);
  data_dir_.cd (WSJT_DATA_DESTINATION);
#endif

  {
    // Create a temporary directory in a suitable location
    QString temp_location {QStandardPaths::writableLocation (QStandardPaths::TempLocation)};
    if (!temp_location.isEmpty ())
      {
        temp_dir_.setPath (temp_location);
      }

    bool ok {false};
    QString unique_directory {QApplication::applicationName ()};
    do
      {
        if (!temp_dir_.mkpath (unique_directory)
            || !temp_dir_.cd (unique_directory))
          {
            QMessageBox::critical (this, "JTDX", tr ("Create temporary directory error: ") + temp_dir_.absolutePath ());
            throw std::runtime_error {"Failed to create a temporary directory"};
          }
        if (!temp_dir_.isReadable () || !(ok = QTemporaryFile {temp_dir_.absoluteFilePath ("test")}.open ()))
          {
            if (QMessageBox::Cancel == QMessageBox::critical (this, "JTDX",
                                                              tr ("Create temporary directory error:\n%1\n"
                                                                  "Another application may be locking the directory").arg (temp_dir_.absolutePath ()),
                                                              QMessageBox::Retry | QMessageBox::Cancel))
              {
                throw std::runtime_error {"Failed to create a usable temporary directory"};
              }
            temp_dir_.cdUp ();  // revert to parent as this one is no good
          }
      }
    while (!ok);
  }

  {
    // Find a suitable data file location
    QDir data_dir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)};
    if (!data_dir.mkpath ("."))
      {
        QMessageBox::critical (this, "JTDX", tr ("Create data directory error: ") + data_dir.absolutePath ());
        throw std::runtime_error {"Failed to create data directory"};
      }

    // Make sure the default save directory exists
    QString save_dir {"save"};
    default_save_directory_ = data_dir;
    default_azel_directory_ = data_dir;
    if (!default_save_directory_.mkpath (save_dir) || !default_save_directory_.cd (save_dir))
      {
        QMessageBox::critical (this, "JTDX", tr ("Create Directory", "Cannot create directory \"") +
                               default_save_directory_.absoluteFilePath (save_dir) + "\".");
        throw std::runtime_error {"Failed to create save directory"};
      }

    // we now have a deafult save path that exists

    // make sure samples directory exists
    QString samples_dir {"samples"};
    if (!default_save_directory_.mkpath (samples_dir))
      {
        QMessageBox::critical (this, "JTDX", tr ("Create Directory", "Cannot create directory \"") +
                               default_save_directory_.absoluteFilePath (samples_dir) + "\".");
        throw std::runtime_error {"Failed to create save directory"};
      }

    // copy in any new sample files to the sample directory
    QDir dest_dir {default_save_directory_};
    dest_dir.cd (samples_dir);
    
    QDir source_dir {":/" + samples_dir};
    source_dir.cd (save_dir);
    source_dir.cd (samples_dir);
    auto list = source_dir.entryInfoList (QStringList {{"*.wav"}}, QDir::Files | QDir::Readable);
    Q_FOREACH (auto const& item, list)
      {
        if (!dest_dir.exists (item.fileName ()))
          {
            QFile file {item.absoluteFilePath ()};
            file.copy (dest_dir.absoluteFilePath (item.fileName ()));
          }
      }
  }

  // this must be done after the default paths above are set
  read_settings ();

  //
  // validation
  //
  ui_->callsign_line_edit->setValidator (new QRegExpValidator {QRegExp {"[A-Za-z0-9/-]+"}, this});
  ui_->grid_line_edit->setValidator (new QRegExpValidator {QRegExp {"[A-Ra-r]{2,2}[0-9]{2,2}[A-Xa-x]{0,2}"}, this});
  ui_->add_macro_line_edit->setValidator (new QRegExpValidator {message_alphabet, this});

  ui_->udp_server_port_spin_box->setMinimum (1);
  ui_->udp_server_port_spin_box->setMaximum (std::numeric_limits<port_type>::max ());
  ui_->tcp_server_port_spin_box->setMinimum (1);
  ui_->tcp_server_port_spin_box->setMaximum (std::numeric_limits<port_type>::max ());

  // Dependent checkboxes 
  ui_->countryPrefix_check_box->setChecked(countryName_ && countryPrefix_);
  ui_->countryPrefix_check_box->setEnabled(countryName_);
  ui_->Africa_check_box->setChecked (countryName_ && hideAfrica_);
  ui_->Africa_check_box->setEnabled(countryName_);
  ui_->Antarctica_check_box->setChecked (countryName_ && hideAntarctica_);
  ui_->Antarctica_check_box->setEnabled(countryName_);
  ui_->Asia_check_box->setChecked (countryName_ && hideAsia_);
  ui_->Asia_check_box->setEnabled(countryName_);
  ui_->Europe_check_box->setChecked (countryName_ && hideEurope_);
  ui_->Europe_check_box->setEnabled(countryName_);
  ui_->Oceania_check_box->setChecked (countryName_ && hideOceania_);
  ui_->Oceania_check_box->setEnabled(countryName_);
  ui_->NAmerica_check_box->setChecked (countryName_ && hideNAmerica_);
  ui_->NAmerica_check_box->setEnabled(countryName_);
  ui_->SAmerica_check_box->setChecked (countryName_ && hideSAmerica_);
  ui_->SAmerica_check_box->setEnabled(countryName_);

  ui_->workedColor_check_box->setChecked((newDXCC_ || newGrid_ || newCall_) && workedColor_);
  ui_->workedColor_check_box->setEnabled(newDXCC_ || newGrid_ || newCall_);
  ui_->workedStriked_check_box->setChecked((newDXCC_ || newGrid_ || newCall_) && !workedUnderlined_ && workedStriked_);
  ui_->workedStriked_check_box->setEnabled((newDXCC_ || newGrid_ || newCall_) && !workedUnderlined_);
  ui_->workedUnderlined_check_box->setChecked((newDXCC_ || newGrid_ || newCall_) && !workedStriked_ && workedUnderlined_);
  ui_->workedUnderlined_check_box->setEnabled((newDXCC_ || newGrid_ || newCall_) && !workedStriked_);
  ui_->workedDontShow_check_box->setChecked((newDXCC_ || newGrid_ || newCall_) && workedDontShow_);
  ui_->workedDontShow_check_box->setEnabled(newDXCC_ || newGrid_ || newCall_);

  ui_->newDXCCBand_check_box->setChecked(newDXCC_ && newDXCCBand_);
  ui_->newDXCCBand_check_box->setEnabled(newDXCC_);
  ui_->newCallBand_check_box->setChecked(newCall_ && newCallBand_);
  ui_->newCallBand_check_box->setEnabled(newCall_);
  ui_->newGridBand_check_box->setChecked(newGrid_ && newGridBand_);
  ui_->newGridBand_check_box->setEnabled(newGrid_);
  ui_->newDXCCBandMode_check_box->setChecked(newDXCC_ && newDXCCBandMode_);
  ui_->newDXCCBandMode_check_box->setEnabled(newDXCC_);
  ui_->newCallBandMode_check_box->setChecked(newCall_ && newCallBandMode_);
  ui_->newCallBandMode_check_box->setEnabled(newCall_);
  ui_->newGridBandMode_check_box->setChecked(newGrid_ && newGridBandMode_);
  ui_->newGridBandMode_check_box->setEnabled(newGrid_);
  ui_->beep_on_newDXCC_check_box->setChecked(newDXCC_ && beepOnNewDXCC_);
  ui_->beep_on_newDXCC_check_box->setEnabled(newDXCC_);
  ui_->beep_on_newCall_check_box->setChecked(newCall_ && beepOnNewCall_);
  ui_->beep_on_newCall_check_box->setEnabled(newCall_);
  ui_->beep_on_newGrid_check_box->setChecked(newGrid_ && beepOnNewGrid_);
  ui_->beep_on_newGrid_check_box->setEnabled(newGrid_);
  //
  // assign ids to radio buttons
  //
  ui_->CAT_data_bits_button_group->setId (ui_->CAT_7_bit_radio_button, TransceiverFactory::seven_data_bits);
  ui_->CAT_data_bits_button_group->setId (ui_->CAT_8_bit_radio_button, TransceiverFactory::eight_data_bits);

  ui_->CAT_stop_bits_button_group->setId (ui_->CAT_one_stop_bit_radio_button, TransceiverFactory::one_stop_bit);
  ui_->CAT_stop_bits_button_group->setId (ui_->CAT_two_stop_bit_radio_button, TransceiverFactory::two_stop_bits);

  ui_->CAT_handshake_button_group->setId (ui_->CAT_handshake_none_radio_button, TransceiverFactory::handshake_none);
  ui_->CAT_handshake_button_group->setId (ui_->CAT_handshake_xon_radio_button, TransceiverFactory::handshake_XonXoff);
  ui_->CAT_handshake_button_group->setId (ui_->CAT_handshake_hardware_radio_button, TransceiverFactory::handshake_hardware);

  ui_->PTT_method_button_group->setId (ui_->PTT_VOX_radio_button, TransceiverFactory::PTT_method_VOX);
  ui_->PTT_method_button_group->setId (ui_->PTT_CAT_radio_button, TransceiverFactory::PTT_method_CAT);
  ui_->PTT_method_button_group->setId (ui_->PTT_DTR_radio_button, TransceiverFactory::PTT_method_DTR);
  ui_->PTT_method_button_group->setId (ui_->PTT_RTS_radio_button, TransceiverFactory::PTT_method_RTS);

  ui_->TX_audio_source_button_group->setId (ui_->TX_source_mic_radio_button, TransceiverFactory::TX_audio_source_front);
  ui_->TX_audio_source_button_group->setId (ui_->TX_source_data_radio_button, TransceiverFactory::TX_audio_source_rear);

  ui_->TX_mode_button_group->setId (ui_->mode_none_radio_button, data_mode_none);
  ui_->TX_mode_button_group->setId (ui_->mode_USB_radio_button, data_mode_USB);
  ui_->TX_mode_button_group->setId (ui_->mode_data_radio_button, data_mode_data);

  ui_->split_mode_button_group->setId (ui_->split_none_radio_button, TransceiverFactory::split_mode_none);
  ui_->split_mode_button_group->setId (ui_->split_rig_radio_button, TransceiverFactory::split_mode_rig);
  ui_->split_mode_button_group->setId (ui_->split_emulate_radio_button, TransceiverFactory::split_mode_emulate);

  //
  // setup PTT port combo box drop down content
  //
  fill_port_combo_box (ui_->PTT_port_combo_box);
  ui_->PTT_port_combo_box->addItem ("CAT");

  //
  // setup hooks to keep audio channels aligned with devices
  //
  {
    using namespace std;
    using namespace std::placeholders;

    function<void (int)> cb (bind (&Configuration::impl::update_audio_channels, this, ui_->sound_input_combo_box, _1, ui_->sound_input_channel_combo_box, false));
    connect (ui_->sound_input_combo_box, static_cast<void (QComboBox::*)(int)> (&QComboBox::currentIndexChanged), cb);
    cb = bind (&Configuration::impl::update_audio_channels, this, ui_->sound_output_combo_box, _1, ui_->sound_output_channel_combo_box, true);
    connect (ui_->sound_output_combo_box, static_cast<void (QComboBox::*)(int)> (&QComboBox::currentIndexChanged), cb);
  }

  //
  // setup macros list view
  //
  ui_->macros_list_view->setModel (&next_macros_);
  ui_->macros_list_view->setItemDelegate (new MessageItemDelegate {this});

  macro_delete_action_ = new QAction {tr ("&Delete"), ui_->macros_list_view};
  ui_->macros_list_view->insertAction (nullptr, macro_delete_action_);
  connect (macro_delete_action_, &QAction::triggered, this, &Configuration::impl::delete_macro);


  //
  // setup working frequencies table model & view
  //
  frequencies_.sort (FrequencyList::frequency_column);

  ui_->frequencies_table_view->setModel (&next_frequencies_);
  ui_->frequencies_table_view->sortByColumn (FrequencyList::frequency_column, Qt::AscendingOrder);
  ui_->frequencies_table_view->setColumnHidden (FrequencyList::frequency_mhz_column, true);
  ui_->frequencies_table_view->setColumnHidden (FrequencyList::mode_frequency_mhz_column, true);

  // delegates
  auto frequencies_item_delegate = new QStyledItemDelegate {this};
  frequencies_item_delegate->setItemEditorFactory (item_editor_factory ());
  ui_->frequencies_table_view->setItemDelegate (frequencies_item_delegate);
  ui_->frequencies_table_view->setItemDelegateForColumn (FrequencyList::mode_column, new ForeignKeyDelegate {&modes_, 0, this});

  // actions
  frequency_delete_action_ = new QAction {tr ("&Delete"), ui_->frequencies_table_view};
  ui_->frequencies_table_view->insertAction (nullptr, frequency_delete_action_);
  connect (frequency_delete_action_, &QAction::triggered, this, &Configuration::impl::delete_frequencies);

  frequency_insert_action_ = new QAction {tr ("&Insert ..."), ui_->frequencies_table_view};
  ui_->frequencies_table_view->insertAction (nullptr, frequency_insert_action_);
  connect (frequency_insert_action_, &QAction::triggered, this, &Configuration::impl::insert_frequency);


  // Schedulers
  
  ui_->bandComboBox_1->setModel(&next_frequencies_);
  ui_->bandComboBox_1->setModelColumn(FrequencyList::mode_frequency_mhz_column);
  ui_->bandComboBox_2->setModel(&next_frequencies_);
  ui_->bandComboBox_2->setModelColumn(FrequencyList::mode_frequency_mhz_column);
  ui_->bandComboBox_3->setModel(&next_frequencies_);
  ui_->bandComboBox_3->setModelColumn(FrequencyList::mode_frequency_mhz_column);
  ui_->bandComboBox_4->setModel(&next_frequencies_);
  ui_->bandComboBox_4->setModelColumn(FrequencyList::mode_frequency_mhz_column);
  ui_->bandComboBox_5->setModel(&next_frequencies_);
  ui_->bandComboBox_5->setModelColumn(FrequencyList::mode_frequency_mhz_column);
  //
  // setup stations table model & view
  //
  stations_.sort (StationList::band_column);

  ui_->stations_table_view->setModel (&next_stations_);
  ui_->stations_table_view->sortByColumn (StationList::band_column, Qt::AscendingOrder);

  // delegates
  auto stations_item_delegate = new QStyledItemDelegate {this};
  stations_item_delegate->setItemEditorFactory (item_editor_factory ());
  ui_->stations_table_view->setItemDelegate (stations_item_delegate);
  ui_->stations_table_view->setItemDelegateForColumn (StationList::band_column, new ForeignKeyDelegate {&bands_, &next_stations_, 0, StationList::band_column, this});

  // actions
  station_delete_action_ = new QAction {tr ("&Delete"), ui_->stations_table_view};
  ui_->stations_table_view->insertAction (nullptr, station_delete_action_);
  connect (station_delete_action_, &QAction::triggered, this, &Configuration::impl::delete_stations);

  station_insert_action_ = new QAction {tr ("&Insert ..."), ui_->stations_table_view};
  ui_->stations_table_view->insertAction (nullptr, station_insert_action_);
  connect (station_insert_action_, &QAction::triggered, this, &Configuration::impl::insert_station);

  //
  // load combo boxes with audio setup choices
  //
  default_audio_input_device_selected_ = load_audio_devices (QAudio::AudioInput, ui_->sound_input_combo_box, &audio_input_device_);
  default_audio_output_device_selected_ = load_audio_devices (QAudio::AudioOutput, ui_->sound_output_combo_box, &audio_output_device_);

  update_audio_channels (ui_->sound_input_combo_box, ui_->sound_input_combo_box->currentIndex (), ui_->sound_input_channel_combo_box, false);
  update_audio_channels (ui_->sound_output_combo_box, ui_->sound_output_combo_box->currentIndex (), ui_->sound_output_channel_combo_box, true);

  ui_->sound_input_channel_combo_box->setCurrentIndex (audio_input_channel_);
  ui_->sound_output_channel_combo_box->setCurrentIndex (audio_output_channel_);

  restart_sound_input_device_ = false;
  restart_sound_output_device_ = false;

  enumerate_rigs ();
  initialize_models ();

  transceiver_thread_ = new QThread {this};
  transceiver_thread_->start ();

}

Configuration::impl::~impl ()
{
  transceiver_thread_->quit ();
  transceiver_thread_->wait ();
  write_settings ();
  temp_dir_.removeRecursively (); // clean up temp files
}

void Configuration::impl::initialize_models ()
{
  auto pal = ui_->callsign_line_edit->palette ();
  if (my_callsign_.isEmpty ())
    {
      pal.setColor (QPalette::Base, "#ffccff");
    }
  else
    {
      pal.setColor (QPalette::Base, Qt::white);
    }
  ui_->callsign_line_edit->setPalette (pal);
  ui_->grid_line_edit->setPalette (pal);
  ui_->callsign_line_edit->setText (my_callsign_);
  ui_->grid_line_edit->setText (my_grid_);
  ui_->labTx->setStyleSheet(QString("background: %1").arg(color_TxMsg_.name()));
  next_txtColor_ = txtColor_;
  next_workedColor_ = workedColor_;
  next_workedStriked_ = workedStriked_;
  next_workedUnderlined_ = workedUnderlined_;
  next_workedDontShow_ = workedDontShow_;
  next_newDXCC_ = newDXCC_;
  next_newDXCCBand_ = newDXCCBand_;
  next_newDXCCBandMode_ = newDXCCBandMode_;
  next_newGrid_ = newGrid_;
  next_newGridBand_ = newGridBand_;
  next_newGridBandMode_ = newGridBandMode_;
  next_newCall_ = newCall_;
  next_newCallBand_ = newCallBand_;
  next_newCallBandMode_ = newCallBandMode_;
  next_newPotential_ = newPotential_;
  if (txtColor_){
    ui_->labCQ->setStyleSheet(QString("background: %1;color: #ffffff").arg(color_CQ_.name()));
    ui_->labMyCall->setStyleSheet(QString("background: %1;color: #ffffff").arg(color_MyCall_.name()));
    ui_->labStandardCall->setStyleSheet(QString("background: %1;color: #ffffff").arg(color_StandardCall_.name()));
    ui_->labNewDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewDXCC_.name(),color_CQ_.name()));
    ui_->labNewDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewDXCCBand_.name(),color_CQ_.name()));
    ui_->labNewGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewGrid_.name(),color_CQ_.name()));
    ui_->labNewGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewGridBand_.name(),color_CQ_.name()));
    ui_->labNewCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewCall_.name(),color_CQ_.name()));
    ui_->labNewCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewCallBand_.name(),color_CQ_.name()));
    if (workedColor_) {
      if (workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(color_WorkedCall_.name(),color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(color_WorkedCall_.name(),color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(color_WorkedCall_.name(),color_StandardCall_.name()));
      } else if (workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(color_WorkedCall_.name(),color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(color_WorkedCall_.name(),color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(color_WorkedCall_.name(),color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg(color_WorkedCall_.name(),color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg(color_WorkedCall_.name(),color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg(color_WorkedCall_.name(),color_StandardCall_.name()));
      }
    } else if (workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",color_StandardCall_.name()));
    } else if (workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg("white",color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg("white",color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg("white",color_StandardCall_.name()));
    }
    ui_->labNewMcDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewDXCC_.name(),color_MyCall_.name()));
    ui_->labNewMcDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewDXCCBand_.name(),color_MyCall_.name()));
    ui_->labNewMcGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewGrid_.name(),color_MyCall_.name()));
    ui_->labNewMcGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewGridBand_.name(),color_MyCall_.name()));
    ui_->labNewMcCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewCall_.name(),color_MyCall_.name()));
    ui_->labNewMcCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewCallBand_.name(),color_MyCall_.name()));
    ui_->labNewScDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewDXCC_.name(),color_StandardCall_.name()));
    ui_->labNewScDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewDXCCBand_.name(),color_StandardCall_.name()));
    ui_->labNewScGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewGrid_.name(),color_StandardCall_.name()));
    ui_->labNewScGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewGridBand_.name(),color_StandardCall_.name()));
    ui_->labNewScCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewCall_.name(),color_StandardCall_.name()));
    ui_->labNewScCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(color_NewCallBand_.name(),color_StandardCall_.name()));
  } else {
    ui_->labCQ->setStyleSheet(QString("background: #ffffff;color: %1").arg(color_CQ_.name()));
    ui_->labMyCall->setStyleSheet(QString("background: #ffffff;color: %1").arg(color_MyCall_.name()));
    ui_->labStandardCall->setStyleSheet(QString("background: #ffffff;color: %1").arg(color_StandardCall_.name()));
    ui_->labNewDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewDXCC_.name(),color_CQ_.name()));
    ui_->labNewDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewDXCCBand_.name(),color_CQ_.name()));
    ui_->labNewGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewGrid_.name(),color_CQ_.name()));
    ui_->labNewGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewGridBand_.name(),color_CQ_.name()));
    ui_->labNewCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewCall_.name(),color_CQ_.name()));
    ui_->labNewCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewCallBand_.name(),color_CQ_.name()));
    if (workedColor_) {
      if (workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(color_WorkedCall_.name(),color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(color_WorkedCall_.name(),color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(color_WorkedCall_.name(),color_StandardCall_.name()));
      } else if (workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(color_WorkedCall_.name(),color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(color_WorkedCall_.name(),color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(color_WorkedCall_.name(),color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg(color_WorkedCall_.name(),color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg(color_WorkedCall_.name(),color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg(color_WorkedCall_.name(),color_StandardCall_.name()));
      }
    } else if (workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",color_StandardCall_.name()));
    } else if (workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg("white",color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg("white",color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg("white",color_StandardCall_.name()));
    }
    ui_->labNewMcDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewDXCC_.name(),color_MyCall_.name()));
    ui_->labNewMcDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewDXCCBand_.name(),color_MyCall_.name()));
    ui_->labNewMcGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewGrid_.name(),color_MyCall_.name()));
    ui_->labNewMcGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewGridBand_.name(),color_MyCall_.name()));
    ui_->labNewMcCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewCall_.name(),color_MyCall_.name()));
    ui_->labNewMcCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewCallBand_.name(),color_MyCall_.name()));
    ui_->labNewScDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewDXCC_.name(),color_StandardCall_.name()));
    ui_->labNewScDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewDXCCBand_.name(),color_StandardCall_.name()));
    ui_->labNewScGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewGrid_.name(),color_StandardCall_.name()));
    ui_->labNewScGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewGridBand_.name(),color_StandardCall_.name()));
    ui_->labNewScCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewCall_.name(),color_StandardCall_.name()));
    ui_->labNewScCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(color_NewCallBand_.name(),color_StandardCall_.name()));
  }
  ui_->labNewDXCC->setVisible(newDXCC_);
  ui_->labNewDXCCBand->setVisible(newDXCCBandMode_ || newDXCCBand_);
  ui_->labNewGrid->setVisible(newGrid_);
  ui_->labNewGridBand->setVisible(newGridBandMode_ || newGridBand_);
  ui_->labNewCall->setVisible(newCall_);
  ui_->labNewCallBand->setVisible(newCallBandMode_ || newCallBand_);
  ui_->labWorkedCall->setVisible(newDXCC_ || newGrid_ || newCall_);
  ui_->labNewMcDXCC->setVisible(newDXCC_);
  ui_->labNewMcDXCCBand->setVisible(newDXCCBandMode_ || newDXCCBand_);
  ui_->labNewMcGrid->setVisible(newGrid_);
  ui_->labNewMcGridBand->setVisible(newGridBandMode_ || newGridBand_);
  ui_->labNewMcCall->setVisible(newCall_);
  ui_->labNewMcCallBand->setVisible(newCallBandMode_ || newCallBand_);
  ui_->labWorkedMcCall->setVisible(newDXCC_ || newGrid_ || newCall_);
  ui_->labStandardCall->setVisible(newPotential_);
//  ui_->labStandardCall->setVisible((!newDXCC_ && !newGrid_ && !newCall_) && newPotential_);
//  ui_->labCQ->setVisible(!newDXCC_ && !newGrid_ && !newCall_);
//  ui_->labMyCall->setVisible(!newDXCC_ && !newGrid_ && !newCall_);
  ui_->labNewScDXCC->setVisible(newDXCC_ && newPotential_);
  ui_->labNewScDXCCBand->setVisible((newDXCCBandMode_ || newDXCCBand_) && newPotential_);
  ui_->labNewScGrid->setVisible(newGrid_ && newPotential_);
  ui_->labNewScGridBand->setVisible((newGridBandMode_ || newGridBand_) && newPotential_);
  ui_->labNewScCall->setVisible(newCall_ && newPotential_);
  ui_->labNewScCallBand->setVisible((newCallBandMode_ || newCallBand_) && newPotential_);
  ui_->labWorkedScCall->setVisible((newDXCC_ || newGrid_ || newCall_) && newPotential_);

  ui_->CW_id_interval_spin_box->setValue (id_interval_);  
  ui_->sbNtrials->setValue (ntrials_);
  ui_->sbNtrials10->setValue (ntrials10_);
  ui_->sbNtrialsRXF10->setValue (ntrialsrxf10_);
  ui_->sbNpreampass->setValue (npreampass_);
  ui_->sbAggressive->setValue (aggressive_);
  ui_->sbEqslTimer->setValue (eqsltimer_);
  ui_->sbHarmonics->setValue (harmonicsdepth_);
  ui_->sbNsingdecatt->setValue (nsingdecatt_);
  ui_->fMask_check_box->setChecked (fmaskact_);
  ui_->backToCQ_checkBox->setChecked (backtocq_);
  ui_->retransmitMsg_checkBox->setChecked (retransmitmsg_);
  ui_->haltTxSameMsgRprt_checkBox->setChecked (halttxsamemsgrprt_);
  ui_->haltTxSameMsg73_checkBox->setChecked (halttxsamemsg73_);
  ui_->haltTxReplyOther_checkBox->setChecked (halttxreplyother_);
  ui_->HideFree_check_box->setChecked (hidefree_);
  ui_->ShowCQ_check_box->setChecked (showcq_);
  ui_->ShowCQ73_check_box->setChecked (showcq73_);
  ui_->sbTopFreq->setValue (ntopfreq65_);
  ui_->sbBackToCQ->setValue (nbacktocq_);
  ui_->sbRetransmitMsg->setValue (nretransmitmsg_);
  ui_->sbHaltTxSameMsgRprt->setValue (nhalttxsamemsgrprt_);
  ui_->sbHaltTxSameMsg73->setValue (nhalttxsamemsg73_);
  ui_->label_11->setText ("<a><img src=\":/decpasses.png\" height=\"464\" /></a>");
  ui_->label_13->setText ("<a><img src=\":/dtrange.png\" height=\"165\" /></a>");

  ui_->PTT_method_button_group->button (rig_params_.ptt_type)->setChecked (true);
  ui_->save_path_display_label->setText (save_directory_.absolutePath ());
  ui_->azel_path_display_label->setText (azel_directory_.absolutePath ());
  ui_->CW_id_after_73_check_box->setChecked (id_after_73_);
  ui_->tx_QSY_check_box->setChecked (tx_QSY_allowed_);
  ui_->psk_reporter_check_box->setChecked (spot_to_psk_reporter_);
  ui_->preventFalseUDP_check_box->setChecked (prevent_spotting_false_);
  ui_->eqsl_check_box->setEnabled (!eqsl_username_.isEmpty () && !eqsl_passwd_.isEmpty () && !eqsl_nickname_.isEmpty ());
  ui_->eqsl_check_box->setChecked ((!eqsl_username_.isEmpty () && !eqsl_passwd_.isEmpty () && !eqsl_nickname_.isEmpty ()) && send_to_eqsl_);
  ui_->eqsluser_edit->setText (eqsl_username_);
  ui_->eqslpasswd_edit->setText (eqsl_passwd_);
  ui_->eqslnick_edit->setText (eqsl_nickname_);
  ui_->UseSched_check_box->setChecked (usesched_);
  ui_->hhComboBox_1->setCurrentText (sched_hh_1_);
  ui_->mmComboBox_1->setCurrentText (sched_mm_1_);
  ui_->bandComboBox_1->setCurrentText (sched_band_1_);
  ui_->band_mix_check_box_1->setChecked (sched_mix_1_);
  ui_->hhComboBox_2->setCurrentText (sched_hh_2_);
  ui_->mmComboBox_2->setCurrentText (sched_mm_2_);
  ui_->bandComboBox_2->setCurrentText (sched_band_2_);
  ui_->band_mix_check_box_2->setChecked (sched_mix_2_);
  ui_->hhComboBox_3->setCurrentText (sched_hh_3_);
  ui_->mmComboBox_3->setCurrentText (sched_mm_3_);
  ui_->bandComboBox_3->setCurrentText (sched_band_3_);
  ui_->band_mix_check_box_3->setChecked (sched_mix_3_);
  ui_->hhComboBox_4->setCurrentText (sched_hh_4_);
  ui_->mmComboBox_4->setCurrentText (sched_mm_4_);
  ui_->bandComboBox_4->setCurrentText (sched_band_4_);
  ui_->band_mix_check_box_4->setChecked (sched_mix_4_);
  ui_->hhComboBox_5->setCurrentText (sched_hh_5_);
  ui_->mmComboBox_5->setCurrentText (sched_mm_5_);
  ui_->bandComboBox_5->setCurrentText (sched_band_5_);
  ui_->band_mix_check_box_5->setChecked (sched_mix_5_);
  ui_->monitor_off_check_box->setChecked (monitor_off_at_startup_);
  ui_->monitor_last_used_check_box->setChecked (monitor_last_used_);
  ui_->log_as_RTTY_check_box->setChecked (log_as_RTTY_);
  ui_->report_in_comments_check_box->setChecked (report_in_comments_);
  ui_->prompt_to_log_check_box->setChecked (prompt_to_log_);
  ui_->autolog_check_box->setChecked (autolog_);
  ui_->insert_blank_check_box->setChecked (insert_blank_);
  ui_->countryName_check_box->setChecked (countryName_);
  ui_->countryPrefix_check_box->setChecked (countryName_ && countryPrefix_);
  ui_->txtColor_check_box->setChecked (txtColor_);
  ui_->workedColor_check_box->setChecked (workedColor_ && (newDXCC_ || newCall_ || newGrid_));
  ui_->workedStriked_check_box->setChecked (!workedUnderlined_ && workedStriked_ && (newDXCC_ || newCall_ || newGrid_));
  ui_->workedUnderlined_check_box->setChecked (!workedStriked_ && workedUnderlined_ && (newDXCC_ || newCall_ || newGrid_));
  ui_->workedDontShow_check_box->setChecked (workedDontShow_ && (newDXCC_ || newCall_ || newGrid_));
  ui_->newDXCC_check_box->setChecked (newDXCC_);
  ui_->newDXCCBand_check_box->setChecked (newDXCCBand_ && newDXCC_);
  ui_->newDXCCBandMode_check_box->setChecked (newDXCCBandMode_ && newDXCC_);
  ui_->newCall_check_box->setChecked (newCall_);
  ui_->newCallBand_check_box->setChecked (newCallBand_ && newCall_);
  ui_->newCallBandMode_check_box->setChecked (newCallBandMode_ && newCall_);
  ui_->newGrid_check_box->setChecked (newGrid_);
  ui_->newGridBand_check_box->setChecked (newGridBand_ && newGrid_);
  ui_->newGridBandMode_check_box->setChecked (newGridBandMode_ && newGrid_);
  ui_->newPotential_check_box->setChecked (newPotential_);
  ui_->Africa_check_box->setChecked (countryName_ && hideAfrica_);
  ui_->Antarctica_check_box->setChecked (countryName_ && hideAntarctica_);
  ui_->Asia_check_box->setChecked (countryName_ && hideAsia_);
  ui_->Europe_check_box->setChecked (countryName_ && hideEurope_);
  ui_->Oceania_check_box->setChecked (countryName_ && hideOceania_);
  ui_->NAmerica_check_box->setChecked (countryName_ && hideNAmerica_);
  ui_->SAmerica_check_box->setChecked (countryName_ && hideSAmerica_);
  ui_->clear_DX_check_box->setChecked (clear_DX_);
  ui_->clear_DX_exit_check_box->setChecked (clear_DX_exit_);
  ui_->miles_check_box->setChecked (miles_);
  ui_->watchdog_check_box->setChecked (watchdog_);
  ui_->TX_messages_check_box->setChecked (TX_messages_);
  ui_->enable_VHF_features_check_box->setChecked(enable_VHF_features_);
  ui_->decode_at_52s_check_box->setChecked(decode_at_52s_);
  ui_->offset_Rx_freq_check_box->setChecked(offsetRxFreq_);
  ui_->beep_on_my_call_check_box->setChecked(beepOnMyCall_);
  ui_->beep_on_newDXCC_check_box->setChecked(beepOnNewDXCC_ && newDXCC_);
  ui_->beep_on_newGrid_check_box->setChecked(beepOnNewGrid_ && newGrid_);
  ui_->beep_on_newCall_check_box->setChecked(beepOnNewCall_ && newCall_);
  ui_->beep_on_firstMsg_check_box->setChecked(beepOnFirstMsg_);
  ui_->type_2_msg_gen_combo_box->setCurrentIndex (type_2_msg_gen_);
  ui_->rig_combo_box->setCurrentText (rig_params_.rig_name);
  ui_->TX_mode_button_group->button (data_mode_)->setChecked (true);
  ui_->split_mode_button_group->button (rig_params_.split_mode)->setChecked (true);
  ui_->CAT_serial_baud_combo_box->setCurrentText (QString::number (rig_params_.baud));
  ui_->CAT_data_bits_button_group->button (rig_params_.data_bits)->setChecked (true);
  ui_->CAT_stop_bits_button_group->button (rig_params_.stop_bits)->setChecked (true);
  ui_->CAT_handshake_button_group->button (rig_params_.handshake)->setChecked (true);
  ui_->checkBoxPwrBandTxMemory->setChecked(pwrBandTxMemory_);
  ui_->checkBoxPwrBandTuneMemory->setChecked(pwrBandTuneMemory_);  
  if (rig_params_.force_dtr)
    {
      ui_->force_DTR_combo_box->setCurrentIndex (rig_params_.dtr_high ? 1 : 2);
    }
  else
    {
      ui_->force_DTR_combo_box->setCurrentIndex (0);
    }
  if (rig_params_.force_rts)
    {
      ui_->force_RTS_combo_box->setCurrentIndex (rig_params_.rts_high ? 1 : 2);
    }
  else
    {
      ui_->force_RTS_combo_box->setCurrentIndex (0);
    }
  ui_->TX_audio_source_button_group->button (rig_params_.audio_source)->setChecked (true);
  ui_->CAT_poll_interval_spin_box->setValue (rig_params_.poll_interval);
  ui_->udp_server_line_edit->setText (udp_server_name_);
  ui_->udp_server_port_spin_box->setValue (udp_server_port_);
  ui_->tcp_server_line_edit->setText (tcp_server_name_);
  ui_->tcp_server_port_spin_box->setValue (tcp_server_port_);
  ui_->accept_udp_requests_check_box->setChecked (accept_udp_requests_);
  ui_->TCP_checkBox->setChecked (enable_tcp_connection_);
  ui_->udpWindowToFront->setChecked(udpWindowToFront_);
  ui_->udpWindowRestore->setChecked(udpWindowRestore_);
  ui_->calibration_intercept_spin_box->setValue (frequency_calibration_intercept_);
  ui_->calibration_slope_ppm_spin_box->setValue (frequency_calibration_slope_ppm_);

  if (rig_params_.ptt_port.isEmpty ())
    {
      if (ui_->PTT_port_combo_box->count ())
        {
          ui_->PTT_port_combo_box->setCurrentText (ui_->PTT_port_combo_box->itemText (0));
        }
    }
  else
    {
      ui_->PTT_port_combo_box->setCurrentText (rig_params_.ptt_port);
    }

  next_macros_.setStringList (macros_.stringList ());
  next_frequencies_.frequency_list (frequencies_.frequency_list ());
  next_stations_.station_list (stations_.station_list ());

  set_rig_invariants ();
}

void Configuration::impl::done (int r)
{
  // do this here since window is still on screen at this point
  SettingsGroup g {settings_, "Configuration"};
  settings_->setValue ("window/geometry", saveGeometry ());

  QDialog::done (r);
}

void Configuration::impl::read_settings ()
{
  SettingsGroup g {settings_, "Configuration"};
  restoreGeometry (settings_->value ("window/geometry").toByteArray ());

  my_callsign_ = settings_->value ("MyCall", "").toString ();
  my_grid_ = settings_->value ("MyGrid", "").toString ();
  next_color_CQ_ = color_CQ_ = settings_->value("colorCQ","#000000").toString();
  next_color_MyCall_ = color_MyCall_ = settings_->value("colorMyCall","#c00000").toString();
  next_color_StandardCall_ = color_StandardCall_ = settings_->value("colorStandardCall","#808080").toString();
  next_color_TxMsg_ = color_TxMsg_ = settings_->value("colorTxMsg","#ffff00").toString();
  next_color_NewDXCC_ = color_NewDXCC_ = settings_->value("colorNewDXCC","#c000c0").toString();
  next_color_NewDXCCBand_ = color_NewDXCCBand_ = settings_->value("colorNewDXCCBand","#ff00ff").toString();
  next_color_NewGrid_ = color_NewGrid_ = settings_->value("colorNewGrid","#00c0c0").toString();
  next_color_NewGridBand_ = color_NewGridBand_ = settings_->value("colorNewGridBand","#00ffff").toString();
  next_color_NewCall_ = color_NewCall_ = settings_->value("colorNewCall","#ffaaff").toString();
  next_color_NewCallBand_ = color_NewCallBand_ = settings_->value("colorNewCallBand","#aaffff").toString();
  next_color_WorkedCall_ = color_WorkedCall_ = settings_->value("colorWorkedCall","#00ff00").toString();

  if (next_font_.fromString (settings_->value ("Font", QGuiApplication::font ().toString ()).toString ())
      && next_font_ != font_)
    {
      font_ = next_font_;
      set_application_font (font_);
    }
  else
    {
      next_font_ = font_;
    }

  if (next_decoded_text_font_.fromString (settings_->value ("DecodedTextFont", "Courier, 10").toString ())
      && next_decoded_text_font_ != decoded_text_font_)
    {
      decoded_text_font_ = next_decoded_text_font_;
      Q_EMIT self_->decoded_text_font_changed (decoded_text_font_);
    }
  else
    {
      next_decoded_text_font_ = decoded_text_font_;
    }

  id_interval_ = settings_->value ("IDint", 0).toInt ();
  ntrials_ = settings_->value ("nTrials", 3).toInt ();
  ntrials10_ = settings_->value ("nTrialsT10", 1).toInt ();
  ntrialsrxf10_ = settings_->value ("nTrialsRxFreqT10", 1).toInt ();
  npreampass_ = settings_->value ("nPreampass", 4).toInt ();
  aggressive_ = settings_->value ("Aggressive", 1).toInt ();
  eqsltimer_ = settings_->value ("eQSLtimer", 10).toInt ();
  harmonicsdepth_ = settings_->value ("HarmonicsDecodingDepth", 0).toInt ();
  ntopfreq65_ = settings_->value ("TopFrequencyJT65", 2700).toInt ();
  nbacktocq_ = settings_->value ("SeqBackToCqMsgCounter", 2).toInt ();
  nretransmitmsg_ = settings_->value ("SeqRetransmitMsgCounter", 2).toInt ();
  nhalttxsamemsgrprt_ = settings_->value ("SeqHaltTxSameMsgRprtCounter", 4).toInt ();
  nhalttxsamemsg73_ = settings_->value ("SeqHaltTxSameMsg73Counter", 4).toInt ();
  nsingdecatt_ = settings_->value ("nSingleDecodeAttempts", 1).toInt ();
  fmaskact_ = settings_->value ("FMaskDecoding", false).toBool ();
  backtocq_ = settings_->value ("SeqBackToCqMsg", false).toBool ();
  retransmitmsg_ = settings_->value ("SeqRetransmitMsg", false).toBool ();
  halttxsamemsgrprt_ = settings_->value ("SeqHaltTxSameMsgRprt", false).toBool ();
  halttxsamemsg73_ = settings_->value ("SeqHaltTxSameMsg73", false).toBool ();
  halttxreplyother_ = settings_->value ("SeqHaltTxReplyOther", true).toBool ();
  hidefree_ = settings_->value ("HideFreeMsgs", false).toBool ();
  showcq_ = settings_->value ("ShowCQMsgs", false).toBool ();
  showcq73_ = settings_->value ("ShowCQ73Msgs", false).toBool ();
  hideAfrica_ = settings_->value ("hideAfrica", false).toBool ();
  hideAntarctica_ = settings_->value ("hideAntarctica", false).toBool ();
  hideAsia_ = settings_->value ("hideAsia", false).toBool ();
  hideEurope_ = settings_->value ("hideEurope", false).toBool ();
  hideOceania_ = settings_->value ("hideOceania", false).toBool ();
  hideNAmerica_ = settings_->value ("hideNAmerica", false).toBool ();
  hideSAmerica_ = settings_->value ("hideSAmerica", false).toBool ();
  save_directory_ = settings_->value ("SaveDir", default_save_directory_.absolutePath ()).toString ();
  azel_directory_ = settings_->value ("AzElDir", default_azel_directory_.absolutePath ()).toString ();

  {
    //
    // retrieve audio input device
    //
    auto saved_name = settings_->value ("SoundInName").toString ();

    // deal with special Windows default audio devices
    auto default_device = QAudioDeviceInfo::defaultInputDevice ();
    if (saved_name == default_device.deviceName ())
      {
        audio_input_device_ = default_device;
        default_audio_input_device_selected_ = true;
      }
    else
      {
        default_audio_input_device_selected_ = false;
        Q_FOREACH (auto const& p, QAudioDeviceInfo::availableDevices (QAudio::AudioInput)) // available audio input devices
          {
            if (p.deviceName () == saved_name)
              {
                audio_input_device_ = p;
              }
          }
      }
  }

  {
    //
    // retrieve audio output device
    //
    auto saved_name = settings_->value("SoundOutName").toString();

    // deal with special Windows default audio devices
    auto default_device = QAudioDeviceInfo::defaultOutputDevice ();
    if (saved_name == default_device.deviceName ())
      {
        audio_output_device_ = default_device;
        default_audio_output_device_selected_ = true;
      }
    else
      {
        default_audio_output_device_selected_ = false;
        Q_FOREACH (auto const& p, QAudioDeviceInfo::availableDevices (QAudio::AudioOutput)) // available audio output devices
          {
            if (p.deviceName () == saved_name)
              {
                audio_output_device_ = p;
              }
          }
      }
  }

  // retrieve audio channel info
  audio_input_channel_ = AudioDevice::fromString (settings_->value ("AudioInputChannel", "Mono").toString ());
  audio_output_channel_ = AudioDevice::fromString (settings_->value ("AudioOutputChannel", "Mono").toString ());

  type_2_msg_gen_ = settings_->value ("Type2MsgGen", QVariant::fromValue (Configuration::type_2_msg_3_full)).value<Configuration::Type2MsgGen> ();

  monitor_off_at_startup_ = settings_->value ("MonitorOFF", false).toBool ();
  monitor_last_used_ = settings_->value ("MonitorLastUsed", false).toBool ();
  spot_to_psk_reporter_ = settings_->value ("PSKReporter", false).toBool ();
  prevent_spotting_false_ = settings_->value ("preventFalseUDPspots", true).toBool ();
  send_to_eqsl_ = settings_->value ("EQSLSend", false).toBool ();
  eqsl_username_ = settings_->value ("EQSLUser", "").toString ();
  eqsl_passwd_ = settings_->value ("EQSLPasswd", "").toString ();
  eqsl_nickname_ = settings_->value ("EQSLNick", "").toString ();
  usesched_ = settings_->value ("UseSchedBands", false).toBool ();
  sched_hh_1_ = settings_->value ("Sched_hh_1", "").toString ();
  sched_mm_1_ = settings_->value ("Sched_mm_1", "").toString ();
  sched_band_1_ = settings_->value ("Sched_band_1", "").toString ();
  sched_mix_1_ = settings_->value ("Sched_mix_1", false).toBool ();
  sched_hh_2_ = settings_->value ("Sched_hh_2", "").toString ();
  sched_mm_2_ = settings_->value ("Sched_mm_2", "").toString ();
  sched_band_2_ = settings_->value ("Sched_band_2", "").toString ();
  sched_mix_2_ = settings_->value ("Sched_mix_2", false).toBool ();
  sched_hh_3_ = settings_->value ("Sched_hh_3", "").toString ();
  sched_mm_3_ = settings_->value ("Sched_mm_3", "").toString ();
  sched_band_3_ = settings_->value ("Sched_band_3", "").toString ();
  sched_mix_3_ = settings_->value ("Sched_mix_3", false).toBool ();
  sched_hh_4_ = settings_->value ("Sched_hh_4", "").toString ();
  sched_mm_4_ = settings_->value ("Sched_mm_4", "").toString ();
  sched_band_4_ = settings_->value ("Sched_band_4", "").toString ();
  sched_mix_4_ = settings_->value ("Sched_mix_4", false).toBool ();
  sched_hh_5_ = settings_->value ("Sched_hh_5", "").toString ();
  sched_mm_5_ = settings_->value ("Sched_mm_5", "").toString ();
  sched_band_5_ = settings_->value ("Sched_band_5", "").toString ();
  sched_mix_5_ = settings_->value ("Sched_mix_5", false).toBool ();
  id_after_73_ = settings_->value ("After73", false).toBool ();
  tx_QSY_allowed_ = settings_->value ("TxQSYAllowed", false).toBool ();

  macros_.setStringList (settings_->value ("Macros", QStringList {"@ TNX 73"}).toStringList ());

  if (settings_->contains ("FrequenciesForModes"))
    {
      auto const& v = settings_->value ("FrequenciesForModes");
      if (v.isValid ())
        {
          frequencies_.frequency_list (v.value<FrequencyList::FrequencyItems> ());
        }
      else
        {
          frequencies_.reset_to_defaults ();
        }
    }
  else
    {
      frequencies_.reset_to_defaults ();
    }

  stations_.station_list (settings_->value ("stations").value<StationList::Stations> ());

  log_as_RTTY_ = settings_->value ("toRTTY", false).toBool ();
  report_in_comments_ = settings_->value("dBtoComments", false).toBool ();
  rig_params_.rig_name = settings_->value ("Rig", TransceiverFactory::basic_transceiver_name_).toString ();
  rig_is_dummy_ = TransceiverFactory::basic_transceiver_name_ == rig_params_.rig_name;
  rig_params_.network_port = settings_->value ("CATNetworkPort").toString ();
  rig_params_.usb_port = settings_->value ("CATUSBPort").toString ();
  rig_params_.serial_port = settings_->value ("CATSerialPort").toString ();
  rig_params_.baud = settings_->value ("CATSerialRate", 4800).toInt ();
  rig_params_.data_bits = settings_->value ("CATDataBits", QVariant::fromValue (TransceiverFactory::eight_data_bits)).value<TransceiverFactory::DataBits> ();
  rig_params_.stop_bits = settings_->value ("CATStopBits", QVariant::fromValue (TransceiverFactory::two_stop_bits)).value<TransceiverFactory::StopBits> ();
  rig_params_.handshake = settings_->value ("CATHandshake", QVariant::fromValue (TransceiverFactory::handshake_none)).value<TransceiverFactory::Handshake> ();
  rig_params_.force_dtr = settings_->value ("CATForceDTR", false).toBool ();
  rig_params_.dtr_high = settings_->value ("DTR", false).toBool ();
  rig_params_.force_rts = settings_->value ("CATForceRTS", false).toBool ();
  rig_params_.rts_high = settings_->value ("RTS", false).toBool ();
  rig_params_.ptt_type = settings_->value ("PTTMethod", QVariant::fromValue (TransceiverFactory::PTT_method_VOX)).value<TransceiverFactory::PTTMethod> ();
  rig_params_.audio_source = settings_->value ("TXAudioSource", QVariant::fromValue (TransceiverFactory::TX_audio_source_front)).value<TransceiverFactory::TXAudioSource> ();
  rig_params_.ptt_port = settings_->value ("PTTport").toString ();
  data_mode_ = settings_->value ("DataMode", QVariant::fromValue (data_mode_none)).value<Configuration::DataMode> ();
  prompt_to_log_ = settings_->value ("PromptToLog", false).toBool ();
  autolog_ = settings_->value ("AutoQSOLogging", false).toBool ();
  insert_blank_ = settings_->value ("InsertBlank", false).toBool ();
  countryName_ = settings_->value ("countryName", true).toBool ();
  countryPrefix_ = settings_->value ("countryPrefix", false).toBool ();
  next_txtColor_ = txtColor_ = settings_->value ("txtColor", false).toBool ();
  next_workedColor_ = workedColor_ = settings_->value ("workedColor", true).toBool ();
  next_workedStriked_ = workedStriked_ = settings_->value ("workedStriked", false).toBool ();
  next_workedUnderlined_ = workedUnderlined_ = settings_->value ("workedUnderlined", true).toBool ();
  next_workedDontShow_ = workedDontShow_ = settings_->value ("workedDontShow", false).toBool ();
  next_newDXCC_ = newDXCC_ = settings_->value ("newDXCC", true).toBool ();
  next_newDXCCBand_ = newDXCCBand_ = settings_->value ("newDXCCBand", false).toBool ();
  next_newDXCCBandMode_ = newDXCCBandMode_ = settings_->value ("newDXCCBandMode", false).toBool ();
  next_newGrid_ = newGrid_ = settings_->value ("newGrid", false).toBool ();
  next_newGridBand_ = newGridBand_ = settings_->value ("newGridBand", false).toBool ();
  next_newGridBandMode_ = newGridBandMode_ = settings_->value ("newGridBandMode", false).toBool ();
  next_newCall_ = newCall_ = settings_->value ("newCall", true).toBool ();
  next_newCallBand_ = newCallBand_ = settings_->value ("newCallBand", false).toBool ();
  next_newCallBandMode_ = newCallBandMode_ = settings_->value ("newCallBandMode", false).toBool ();
  next_newPotential_ = newPotential_ = settings_->value ("newPotential", false).toBool ();
  clear_DX_ = settings_->value ("ClearCallGrid", false).toBool ();
  clear_DX_exit_ = settings_->value ("ClearCallGridExit", false).toBool ();
  miles_ = settings_->value ("Miles", false).toBool ();
  watchdog_ = settings_->value ("Runaway", false).toBool ();
  TX_messages_ = settings_->value ("Tx2QSO", true).toBool ();
  enable_VHF_features_ = settings_->value("VHFUHF",false).toBool ();
  decode_at_52s_ = settings_->value("Decode52",false).toBool ();
  offsetRxFreq_ = settings_->value("OffsetRx",false).toBool();
  beepOnMyCall_ = settings_->value("BeepOnMyCall", false).toBool();
  beepOnNewDXCC_ = settings_->value("BeepOnNewDXCC", false).toBool();
  beepOnNewGrid_ = settings_->value("BeepOnNewGrid", false).toBool();
  beepOnNewCall_ = settings_->value("BeepOnNewCall", false).toBool();
  beepOnFirstMsg_ = settings_->value("BeepOnFirstMsg", false).toBool();
  rig_params_.poll_interval = settings_->value ("Polling", 0).toInt ();
  rig_params_.split_mode = settings_->value ("SplitMode", QVariant::fromValue (TransceiverFactory::split_mode_none)).value<TransceiverFactory::SplitMode> ();
  udp_server_name_ = settings_->value ("UDPServer", "127.0.0.1").toString ();
  udp_server_port_ = settings_->value ("UDPServerPort", 2237).toUInt ();
  tcp_server_name_ = settings_->value ("TCPServer", "127.0.0.1").toString ();
  tcp_server_port_ = settings_->value ("TCPServerPort", 52001).toUInt ();
  accept_udp_requests_ = settings_->value ("AcceptUDPRequests", false).toBool ();
  enable_tcp_connection_ = settings_->value ("EnableTCPConnection", false).toBool ();
  udpWindowToFront_ = settings_->value ("udpWindowToFront",false).toBool ();
  udpWindowRestore_ = settings_->value ("udpWindowRestore",false).toBool ();
  frequency_calibration_intercept_ = settings_->value ("CalibrationIntercept", 0.).toDouble ();
  frequency_calibration_slope_ppm_ = settings_->value ("CalibrationSlopePPM", 0.).toDouble ();
  pwrBandTxMemory_ = settings_->value("pwrBandTxMemory",false).toBool ();
  pwrBandTuneMemory_ = settings_->value("pwrBandTuneMemory",false).toBool ();  
}

void Configuration::impl::write_settings ()
{
  SettingsGroup g {settings_, "Configuration"};

  settings_->setValue ("MyCall", my_callsign_);
  settings_->setValue ("MyGrid", my_grid_);
  settings_->setValue("colorCQ",color_CQ_);
  settings_->setValue("colorMyCall",color_MyCall_);
  settings_->setValue("colorTxMsg",color_TxMsg_);
  settings_->setValue("colorNewDXCC",color_NewDXCC_);
  settings_->setValue("colorNewDXCCBand",color_NewDXCCBand_);
  settings_->setValue("colorNewGrid",color_NewGrid_);
  settings_->setValue("colorNewGridBand",color_NewGridBand_);
  settings_->setValue("colorNewCall",color_NewCall_);
  settings_->setValue("colorNewCallBand",color_NewCallBand_);
  settings_->setValue("colorStandardCall",color_StandardCall_);
  settings_->setValue("colorWorkedCall",color_WorkedCall_);
  settings_->setValue ("Font", font_.toString ());
  settings_->setValue ("DecodedTextFont", decoded_text_font_.toString ());
  settings_->setValue ("IDint", id_interval_);
  settings_->setValue ("nTrials", ntrials_);
  settings_->setValue ("nTrialsT10", ntrials10_);
  settings_->setValue ("nTrialsRxFreqT10", ntrialsrxf10_);
  settings_->setValue ("nPreampass", npreampass_);
  settings_->setValue ("Aggressive", aggressive_);
  settings_->setValue ("eQSLtimer", eqsltimer_);
  settings_->setValue ("HarmonicsDecodingDepth", harmonicsdepth_);
  settings_->setValue ("TopFrequencyJT65", ntopfreq65_);
  settings_->setValue ("SeqBackToCqMsgCounter", nbacktocq_);
  settings_->setValue ("SeqRetransmitMsgCounter", nretransmitmsg_);
  settings_->setValue ("SeqHaltTxSameMsgRprtCounter", nhalttxsamemsgrprt_);
  settings_->setValue ("SeqHaltTxSameMsg73Counter", nhalttxsamemsg73_);
  settings_->setValue ("FMaskDecoding", fmaskact_);
  settings_->setValue ("SeqBackToCqMsg", backtocq_);
  settings_->setValue ("SeqRetransmitMsg", retransmitmsg_);
  settings_->setValue ("SeqHaltTxSameMsgRprt", halttxsamemsgrprt_);
  settings_->setValue ("SeqHaltTxSameMsg73", halttxsamemsg73_);
  settings_->setValue ("SeqHaltTxReplyOther", halttxreplyother_);
  settings_->setValue ("HideFreeMsgs", hidefree_);
  settings_->setValue ("ShowCQMsgs", showcq_);
  settings_->setValue ("ShowCQ73Msgs", showcq73_);
  settings_->setValue ("hideAfrica", hideAfrica_);
  settings_->setValue ("hideAntarctica", hideAntarctica_);
  settings_->setValue ("hideAsia", hideAsia_);
  settings_->setValue ("hideEurope", hideEurope_);
  settings_->setValue ("hideOceania", hideOceania_);
  settings_->setValue ("hideNAmerica", hideNAmerica_);
  settings_->setValue ("hideSAmerica", hideSAmerica_);
  settings_->setValue ("nSingleDecodeAttempts", nsingdecatt_);
  settings_->setValue ("PTTMethod", QVariant::fromValue (rig_params_.ptt_type));
  settings_->setValue ("PTTport", rig_params_.ptt_port);
  settings_->setValue ("SaveDir", save_directory_.absolutePath ());
  settings_->setValue ("AzElDir", azel_directory_.absolutePath ());

  if (default_audio_input_device_selected_)
    {
      settings_->setValue ("SoundInName", QAudioDeviceInfo::defaultInputDevice ().deviceName ());
    }
  else
    {
      settings_->setValue ("SoundInName", audio_input_device_.deviceName ());
    }

  if (default_audio_output_device_selected_)
    {
      settings_->setValue ("SoundOutName", QAudioDeviceInfo::defaultOutputDevice ().deviceName ());
    }
  else
    {
      settings_->setValue ("SoundOutName", audio_output_device_.deviceName ());
    }

  settings_->setValue ("AudioInputChannel", AudioDevice::toString (audio_input_channel_));
  settings_->setValue ("AudioOutputChannel", AudioDevice::toString (audio_output_channel_));
  settings_->setValue ("Type2MsgGen", QVariant::fromValue (type_2_msg_gen_));
  settings_->setValue ("MonitorOFF", monitor_off_at_startup_);
  settings_->setValue ("MonitorLastUsed", monitor_last_used_);
  settings_->setValue ("PSKReporter", spot_to_psk_reporter_);
  settings_->setValue ("preventFalseUDPspots", prevent_spotting_false_);
  settings_->setValue ("EQSLSend", send_to_eqsl_);
  settings_->setValue ("EQSLUser", eqsl_username_);
  settings_->setValue ("EQSLPasswd", eqsl_passwd_);
  settings_->setValue ("EQSLNick", eqsl_nickname_);
  settings_->setValue ("UseSchedBands", usesched_);
  settings_->setValue ("Sched_hh_1", sched_hh_1_);
  settings_->setValue ("Sched_mm_1", sched_mm_1_);
  settings_->setValue ("Sched_band_1", sched_band_1_);
  settings_->setValue ("Sched_mix_1", sched_mix_1_);
  settings_->setValue ("Sched_hh_2", sched_hh_2_);
  settings_->setValue ("Sched_mm_2", sched_mm_2_);
  settings_->setValue ("Sched_band_2", sched_band_2_);
  settings_->setValue ("Sched_mix_2", sched_mix_2_);
  settings_->setValue ("Sched_hh_3", sched_hh_3_);
  settings_->setValue ("Sched_mm_3", sched_mm_3_);
  settings_->setValue ("Sched_band_3", sched_band_3_);
  settings_->setValue ("Sched_mix_3", sched_mix_3_);
  settings_->setValue ("Sched_hh_4", sched_hh_4_);
  settings_->setValue ("Sched_mm_4", sched_mm_4_);
  settings_->setValue ("Sched_band_4", sched_band_4_);
  settings_->setValue ("Sched_mix_4", sched_mix_4_);
  settings_->setValue ("Sched_hh_5", sched_hh_5_);
  settings_->setValue ("Sched_mm_5", sched_mm_5_);
  settings_->setValue ("Sched_band_5", sched_band_5_);
  settings_->setValue ("Sched_mix_5", sched_mix_5_);
  settings_->setValue ("After73", id_after_73_);
  settings_->setValue ("TxQSYAllowed", tx_QSY_allowed_);
  settings_->setValue ("Macros", macros_.stringList ());
  settings_->setValue ("FrequenciesForModes", QVariant::fromValue (frequencies_.frequency_list ()));
  settings_->setValue ("stations", QVariant::fromValue (stations_.station_list ()));
  settings_->setValue ("toRTTY", log_as_RTTY_);
  settings_->setValue ("dBtoComments", report_in_comments_);
  settings_->setValue ("Rig", rig_params_.rig_name);
  settings_->setValue ("CATNetworkPort", rig_params_.network_port);
  settings_->setValue ("CATUSBPort", rig_params_.usb_port);
  settings_->setValue ("CATSerialPort", rig_params_.serial_port);
  settings_->setValue ("CATSerialRate", rig_params_.baud);
  settings_->setValue ("CATDataBits", QVariant::fromValue (rig_params_.data_bits));
  settings_->setValue ("CATStopBits", QVariant::fromValue (rig_params_.stop_bits));
  settings_->setValue ("CATHandshake", QVariant::fromValue (rig_params_.handshake));
  settings_->setValue ("DataMode", QVariant::fromValue (data_mode_));
  settings_->setValue ("PromptToLog", prompt_to_log_);
  settings_->setValue ("AutoQSOLogging", autolog_);
  settings_->setValue ("InsertBlank", insert_blank_);
  settings_->setValue ("countryName", countryName_);
  settings_->setValue ("countryPrefix", countryPrefix_);
  settings_->setValue ("txtColor", txtColor_);
  settings_->setValue ("workedColor", workedColor_);
  settings_->setValue ("workedStriked", workedStriked_);
  settings_->setValue ("workedUnderlined", workedUnderlined_);
  settings_->setValue ("workedDontShow", workedDontShow_);
  settings_->setValue ("newDXCC", newDXCC_);
  settings_->setValue ("newDXCCBand", newDXCCBand_);
  settings_->setValue ("newDXCCBandMode", newDXCCBandMode_);
  settings_->setValue ("newCall", newCall_);
  settings_->setValue ("newCallBand", newCallBand_);
  settings_->setValue ("newCallBandMode", newCallBandMode_);
  settings_->setValue ("newGrid", newGrid_);
  settings_->setValue ("newGridBand", newGridBand_);
  settings_->setValue ("newGridBandMode", newGridBandMode_);
  settings_->setValue ("newPotential", newPotential_);
  settings_->setValue ("ClearCallGrid", clear_DX_);
  settings_->setValue ("ClearCallGridExit", clear_DX_exit_);
  settings_->setValue ("Miles", miles_);
  settings_->setValue ("Runaway", watchdog_);
  settings_->setValue ("Tx2QSO", TX_messages_);
  settings_->setValue ("CATForceDTR", rig_params_.force_dtr);
  settings_->setValue ("DTR", rig_params_.dtr_high);
  settings_->setValue ("CATForceRTS", rig_params_.force_rts);
  settings_->setValue ("RTS", rig_params_.rts_high);
  settings_->setValue ("TXAudioSource", QVariant::fromValue (rig_params_.audio_source));
  settings_->setValue ("Polling", rig_params_.poll_interval);
  settings_->setValue ("SplitMode", QVariant::fromValue (rig_params_.split_mode));
  settings_->setValue ("VHFUHF", enable_VHF_features_);
  settings_->setValue ("Decode52", decode_at_52s_);
  settings_->setValue ("OffsetRx", offsetRxFreq_);
  settings_->setValue ("BeepOnMyCall", beepOnMyCall_);
  settings_->setValue ("BeepOnNewDXCC", beepOnNewDXCC_);
  settings_->setValue ("BeepOnNewGrid", beepOnNewGrid_);
  settings_->setValue ("BeepOnNewCall", beepOnNewCall_);
  settings_->setValue ("BeepOnFirstMsg", beepOnFirstMsg_);
  settings_->setValue ("UDPServer", udp_server_name_);
  settings_->setValue ("UDPServerPort", udp_server_port_);
  settings_->setValue ("TCPServer", tcp_server_name_);
  settings_->setValue ("TCPServerPort", tcp_server_port_);
  settings_->setValue ("AcceptUDPRequests", accept_udp_requests_);
  settings_->setValue ("EnableTCPConnection", enable_tcp_connection_);
  settings_->setValue ("udpWindowToFront", udpWindowToFront_);
  settings_->setValue ("udpWindowRestore", udpWindowRestore_);
  settings_->setValue ("CalibrationIntercept", frequency_calibration_intercept_);
  settings_->setValue ("CalibrationSlopePPM", frequency_calibration_slope_ppm_);
  settings_->setValue ("pwrBandTxMemory", pwrBandTxMemory_);
  settings_->setValue ("pwrBandTuneMemory", pwrBandTuneMemory_);  
}

void Configuration::impl::set_rig_invariants ()
{
  auto const& rig = ui_->rig_combo_box->currentText ();
  auto const& ptt_port = ui_->PTT_port_combo_box->currentText ();
  auto ptt_method = static_cast<TransceiverFactory::PTTMethod> (ui_->PTT_method_button_group->checkedId ());

  auto CAT_PTT_enabled = transceiver_factory_.has_CAT_PTT (rig);
  auto CAT_indirect_serial_PTT = transceiver_factory_.has_CAT_indirect_serial_PTT (rig);
  auto asynchronous_CAT = transceiver_factory_.has_asynchronous_CAT (rig);
  auto is_hw_handshake = ui_->CAT_handshake_group_box->isEnabled ()
    && TransceiverFactory::handshake_hardware == static_cast<TransceiverFactory::Handshake> (ui_->CAT_handshake_button_group->checkedId ());

  ui_->test_CAT_push_button->setStyleSheet ({});

  ui_->CAT_poll_interval_label->setEnabled (!asynchronous_CAT);
  ui_->CAT_poll_interval_spin_box->setEnabled (!asynchronous_CAT);

  auto port_type = transceiver_factory_.CAT_port_type (rig);

  bool is_serial_CAT (TransceiverFactory::Capabilities::serial == port_type);
  auto const& cat_port = ui_->CAT_port_combo_box->currentText ();

  // only enable CAT option if transceiver has CAT PTT
  ui_->PTT_CAT_radio_button->setEnabled (CAT_PTT_enabled);

  auto enable_ptt_port = TransceiverFactory::PTT_method_CAT != ptt_method && TransceiverFactory::PTT_method_VOX != ptt_method;
  ui_->PTT_port_combo_box->setEnabled (enable_ptt_port);
  ui_->PTT_port_label->setEnabled (enable_ptt_port);

  if (CAT_indirect_serial_PTT)
    {
      ui_->PTT_port_combo_box->setItemData (ui_->PTT_port_combo_box->findText ("CAT")
                                            , combo_box_item_enabled, Qt::UserRole - 1);
    }
  else
    {
      ui_->PTT_port_combo_box->setItemData (ui_->PTT_port_combo_box->findText ("CAT")
                                            , combo_box_item_disabled, Qt::UserRole - 1);
      if ("CAT" == ui_->PTT_port_combo_box->currentText () && ui_->PTT_port_combo_box->currentIndex () > 0)
        {
          ui_->PTT_port_combo_box->setCurrentIndex (ui_->PTT_port_combo_box->currentIndex () - 1);
        }
    }
  ui_->PTT_RTS_radio_button->setEnabled (!(is_serial_CAT && ptt_port == cat_port && is_hw_handshake));

  if (TransceiverFactory::basic_transceiver_name_ == rig)
    {
      // makes no sense with rig as "None"
      ui_->monitor_last_used_check_box->setEnabled (false);

      ui_->CAT_control_group_box->setEnabled (false);
      ui_->test_CAT_push_button->setEnabled (false);
      ui_->test_PTT_push_button->setEnabled (TransceiverFactory::PTT_method_DTR == ptt_method
                                             || TransceiverFactory::PTT_method_RTS == ptt_method);
      ui_->TX_audio_source_group_box->setEnabled (false);
    }
  else
    {
      ui_->monitor_last_used_check_box->setEnabled (true);
      ui_->CAT_control_group_box->setEnabled (true);
      ui_->test_CAT_push_button->setEnabled (true);
      ui_->test_PTT_push_button->setEnabled (false);
      ui_->TX_audio_source_group_box->setEnabled (transceiver_factory_.has_CAT_PTT_mic_data (rig) && TransceiverFactory::PTT_method_CAT == ptt_method);
      if (port_type != last_port_type_)
        {
          last_port_type_ = port_type;
          switch (port_type)
            {
            case TransceiverFactory::Capabilities::serial:
              fill_port_combo_box (ui_->CAT_port_combo_box);
              ui_->CAT_port_combo_box->setCurrentText (rig_params_.serial_port);
              if (ui_->CAT_port_combo_box->currentText ().isEmpty () && ui_->CAT_port_combo_box->count ())
                {
                  ui_->CAT_port_combo_box->setCurrentText (ui_->CAT_port_combo_box->itemText (0));
                }
              ui_->CAT_port_label->setText (tr ("Serial Port:"));
              ui_->CAT_port_combo_box->setToolTip (tr ("Serial port used for CAT control"));
              ui_->CAT_port_combo_box->setEnabled (true);
              break;

            case TransceiverFactory::Capabilities::network:
              ui_->CAT_port_combo_box->clear ();
              ui_->CAT_port_combo_box->setCurrentText (rig_params_.network_port);
              ui_->CAT_port_label->setText (tr ("Network Server:"));
              ui_->CAT_port_combo_box->setToolTip (tr ("Optional hostname and port of network service.\n"
                                                       "Leave blank for a sensible default on this machine.\n"
                                                       "Formats:\n"
                                                       "\thostname:port\n"
                                                       "\tIPv4-address:port\n"
                                                       "\t[IPv6-address]:port"));
              ui_->CAT_port_combo_box->setEnabled (true);
              break;

            case TransceiverFactory::Capabilities::usb:
              ui_->CAT_port_combo_box->clear ();
              ui_->CAT_port_combo_box->setCurrentText (rig_params_.usb_port);
              ui_->CAT_port_label->setText (tr ("USB Device:"));
              ui_->CAT_port_combo_box->setToolTip (tr ("Optional device identification.\n"
                                                       "Leave blank for a sensible default for the rig.\n"
                                                       "Format:\n"
                                                       "\t[VID[:PID[:VENDOR[:PRODUCT]]]]"));
              ui_->CAT_port_combo_box->setEnabled (true);
              break;

            default:
              ui_->CAT_port_combo_box->clear ();
              ui_->CAT_port_combo_box->setEnabled (false);
              break;
            }
        }
      ui_->CAT_serial_port_parameters_group_box->setEnabled (is_serial_CAT);
      ui_->force_DTR_combo_box->setEnabled (is_serial_CAT
                                            && (cat_port != ptt_port
                                                || !ui_->PTT_DTR_radio_button->isEnabled ()
                                                || !ui_->PTT_DTR_radio_button->isChecked ()));
      ui_->force_RTS_combo_box->setEnabled (is_serial_CAT
                                            && !is_hw_handshake
                                            && (cat_port != ptt_port
                                                || !ui_->PTT_RTS_radio_button->isEnabled ()
                                                || !ui_->PTT_RTS_radio_button->isChecked ()));
    }
  ui_->mode_group_box->setEnabled (WSJT_RIG_NONE_CAN_SPLIT
                                   || TransceiverFactory::basic_transceiver_name_ != rig);
  ui_->split_operation_group_box->setEnabled (WSJT_RIG_NONE_CAN_SPLIT
                                              || TransceiverFactory::basic_transceiver_name_ != rig);
}

bool Configuration::impl::validate ()
{
  if (ui_->sound_input_combo_box->currentIndex () < 0
      && !QAudioDeviceInfo::availableDevices (QAudio::AudioInput).empty ())
    {
      message_box (tr ("Invalid audio input device"));
      return false;
    }

  if (ui_->sound_output_combo_box->currentIndex () < 0
      && !QAudioDeviceInfo::availableDevices (QAudio::AudioOutput).empty ())
    {
      message_box (tr ("Invalid audio output device"));
      return false;
    }

  if (!ui_->PTT_method_button_group->checkedButton ()->isEnabled ())
    {
      message_box (tr ("Invalid PTT method"));
      return false;
    }

  auto ptt_method = static_cast<TransceiverFactory::PTTMethod> (ui_->PTT_method_button_group->checkedId ());
  auto ptt_port = ui_->PTT_port_combo_box->currentText ();
  if ((TransceiverFactory::PTT_method_DTR == ptt_method || TransceiverFactory::PTT_method_RTS == ptt_method)
      && (ptt_port.isEmpty ()
          || combo_box_item_disabled == ui_->PTT_port_combo_box->itemData (ui_->PTT_port_combo_box->findText (ptt_port), Qt::UserRole - 1)))
    {
      message_box (tr ("Invalid PTT port"));
      return false;
    }

  return true;
}

int Configuration::impl::exec ()
{
  // macros can be modified in the main window
  next_macros_.setStringList (macros_.stringList ());

  have_rig_ = rig_active_;	// record that we started with a rig open
  saved_rig_params_ = rig_params_; // used to detect changes that
                                   // require the Transceiver to be
                                   // re-opened
  rig_changed_ = false;

  initialize_models ();
  return QDialog::exec();
}

TransceiverFactory::ParameterPack Configuration::impl::gather_rig_data ()
{
  TransceiverFactory::ParameterPack result;
  result.rig_name = ui_->rig_combo_box->currentText ();

  switch (transceiver_factory_.CAT_port_type (result.rig_name))
    {
    case TransceiverFactory::Capabilities::network:
      result.network_port = ui_->CAT_port_combo_box->currentText ();
      result.usb_port = rig_params_.usb_port;
      result.serial_port = rig_params_.serial_port;
      break;

    case TransceiverFactory::Capabilities::usb:
      result.usb_port = ui_->CAT_port_combo_box->currentText ();
      result.network_port = rig_params_.network_port;
      result.serial_port = rig_params_.serial_port;
      break;

    default:
      result.serial_port = ui_->CAT_port_combo_box->currentText ();
      result.network_port = rig_params_.network_port;
      result.usb_port = rig_params_.usb_port;
      break;
    }

  result.baud = ui_->CAT_serial_baud_combo_box->currentText ().toInt ();
  result.data_bits = static_cast<TransceiverFactory::DataBits> (ui_->CAT_data_bits_button_group->checkedId ());
  result.stop_bits = static_cast<TransceiverFactory::StopBits> (ui_->CAT_stop_bits_button_group->checkedId ());
  result.handshake = static_cast<TransceiverFactory::Handshake> (ui_->CAT_handshake_button_group->checkedId ());
  result.force_dtr = ui_->force_DTR_combo_box->isEnabled () && ui_->force_DTR_combo_box->currentIndex () > 0;
  result.dtr_high = ui_->force_DTR_combo_box->isEnabled () && 1 == ui_->force_DTR_combo_box->currentIndex ();
  result.force_rts = ui_->force_RTS_combo_box->isEnabled () && ui_->force_RTS_combo_box->currentIndex () > 0;
  result.rts_high = ui_->force_RTS_combo_box->isEnabled () && 1 == ui_->force_RTS_combo_box->currentIndex ();
  result.poll_interval = ui_->CAT_poll_interval_spin_box->value ();
  result.ptt_type = static_cast<TransceiverFactory::PTTMethod> (ui_->PTT_method_button_group->checkedId ());
  result.ptt_port = ui_->PTT_port_combo_box->currentText ();
  result.audio_source = static_cast<TransceiverFactory::TXAudioSource> (ui_->TX_audio_source_button_group->checkedId ());
  result.split_mode = static_cast<TransceiverFactory::SplitMode> (ui_->split_mode_button_group->checkedId ());
  return result;
}

void Configuration::impl::accept ()
{
  // Called when OK button is clicked.

  if (!validate ())
    {
      return;			// not accepting
    }

  // extract all rig related configuration parameters into temporary
  // structure for checking if the rig needs re-opening without
  // actually updating our live state
  auto temp_rig_params = gather_rig_data ();

  // open_rig() uses values from models so we use it to validate the
  // Transceiver settings before agreeing to accept the configuration
  if (temp_rig_params != rig_params_ && !open_rig ())
    {
      return;			// not accepting
    }

  QDialog::accept();            // do this before accessing custom
                                // models so that any changes in
                                // delegates in views get flushed to
                                // the underlying models before we
                                // access them

  sync_transceiver (true);	// force an update

  //
  // from here on we are bound to accept the new configuration
  // parameters so extract values from models and make them live
  //

  if (next_font_ != font_)
    {
      font_ = next_font_;
      set_application_font (font_);
    }

  if (next_decoded_text_font_ != decoded_text_font_)
    {
      decoded_text_font_ = next_decoded_text_font_;
      Q_EMIT self_->decoded_text_font_changed (decoded_text_font_);
    }

  color_CQ_ = next_color_CQ_;
  color_MyCall_ = next_color_MyCall_;
  color_TxMsg_ = next_color_TxMsg_;
  color_NewDXCC_ = next_color_NewDXCC_;
  color_NewDXCCBand_ = next_color_NewDXCCBand_;
  color_NewGrid_ = next_color_NewGrid_;
  color_NewGridBand_ = next_color_NewGridBand_;
  color_NewCall_ = next_color_NewCall_;
  color_NewCallBand_ = next_color_NewCallBand_;
  color_StandardCall_ = next_color_StandardCall_;
  color_WorkedCall_ = next_color_WorkedCall_;

  rig_params_ = temp_rig_params; // now we can go live with the rig
                                 // related configuration parameters
  rig_is_dummy_ = TransceiverFactory::basic_transceiver_name_ == rig_params_.rig_name;

  // Check to see whether SoundInThread must be restarted,
  // and save user parameters.
  {
    auto const& device_name = ui_->sound_input_combo_box->currentText ();
    if (device_name != audio_input_device_.deviceName ())
      {
        auto const& default_device = QAudioDeviceInfo::defaultInputDevice ();
        if (device_name == default_device.deviceName ())
          {
            audio_input_device_ = default_device;
          }
        else
          {
            bool found {false};
            Q_FOREACH (auto const& d, QAudioDeviceInfo::availableDevices (QAudio::AudioInput))
              {
                if (device_name == d.deviceName ())
                  {
                    audio_input_device_ = d;
                    found = true;
                  }
              }
            if (!found)
              {
                audio_input_device_ = default_device;
              }
          }
        restart_sound_input_device_ = true;
      }
  }

  {
    auto const& device_name = ui_->sound_output_combo_box->currentText ();
    if (device_name != audio_output_device_.deviceName ())
      {
        auto const& default_device = QAudioDeviceInfo::defaultOutputDevice ();
        if (device_name == default_device.deviceName ())
          {
            audio_output_device_ = default_device;
          }
        else
          {
            bool found {false};
            Q_FOREACH (auto const& d, QAudioDeviceInfo::availableDevices (QAudio::AudioOutput))
              {
                if (device_name == d.deviceName ())
                  {
                    audio_output_device_ = d;
                    found = true;
                  }
              }
            if (!found)
              {
                audio_output_device_ = default_device;
              }
          }
        restart_sound_output_device_ = true;
      }
  }

  if (audio_input_channel_ != static_cast<AudioDevice::Channel> (ui_->sound_input_channel_combo_box->currentIndex ()))
    {
      audio_input_channel_ = static_cast<AudioDevice::Channel> (ui_->sound_input_channel_combo_box->currentIndex ());
      restart_sound_input_device_ = true;
    }
  Q_ASSERT (audio_input_channel_ <= AudioDevice::Right);

  if (audio_output_channel_ != static_cast<AudioDevice::Channel> (ui_->sound_output_channel_combo_box->currentIndex ()))
    {
      audio_output_channel_ = static_cast<AudioDevice::Channel> (ui_->sound_output_channel_combo_box->currentIndex ());
      restart_sound_output_device_ = true;
    }
  Q_ASSERT (audio_output_channel_ <= AudioDevice::Both);

  my_callsign_ = ui_->callsign_line_edit->text ();
  my_grid_ = ui_->grid_line_edit->text ();
  spot_to_psk_reporter_ = ui_->psk_reporter_check_box->isChecked ();
  prevent_spotting_false_ = ui_->preventFalseUDP_check_box->isChecked ();
  send_to_eqsl_ = ui_->eqsl_check_box->isChecked ();
  eqsl_username_ = ui_->eqsluser_edit->text ();
  eqsl_passwd_ = ui_->eqslpasswd_edit->text ();
  eqsl_nickname_ = ui_->eqslnick_edit->text ();
  usesched_ = ui_->UseSched_check_box->isChecked ();
  sched_hh_1_ = ui_->hhComboBox_1->currentText ();
  sched_mm_1_ = ui_->mmComboBox_1->currentText ();
  sched_band_1_ = ui_->bandComboBox_1->currentText ();
  sched_mix_1_ = ui_->band_mix_check_box_1->isChecked ();
  sched_hh_2_ = ui_->hhComboBox_2->currentText ();
  sched_mm_2_ = ui_->mmComboBox_2->currentText ();
  sched_band_2_ = ui_->bandComboBox_2->currentText ();
  sched_mix_2_ = ui_->band_mix_check_box_2->isChecked ();
  sched_hh_3_ = ui_->hhComboBox_3->currentText ();
  sched_mm_3_ = ui_->mmComboBox_3->currentText ();
  sched_band_3_ = ui_->bandComboBox_3->currentText ();
  sched_mix_3_ = ui_->band_mix_check_box_3->isChecked ();
  sched_hh_4_ = ui_->hhComboBox_4->currentText ();
  sched_mm_4_ = ui_->mmComboBox_4->currentText ();
  sched_band_4_ = ui_->bandComboBox_4->currentText ();
  sched_mix_4_ = ui_->band_mix_check_box_4->isChecked ();
  sched_hh_5_ = ui_->hhComboBox_5->currentText ();
  sched_mm_5_ = ui_->mmComboBox_5->currentText ();
  sched_band_5_ = ui_->bandComboBox_5->currentText ();
  sched_mix_5_ = ui_->band_mix_check_box_5->isChecked ();
  id_interval_ = ui_->CW_id_interval_spin_box->value ();
  ntrials_ = ui_->sbNtrials->value ();
  ntrials10_ = ui_->sbNtrials10->value ();
  ntrialsrxf10_ = ui_->sbNtrialsRXF10->value ();
  npreampass_= ui_->sbNpreampass->value ();
  aggressive_= ui_->sbAggressive->value ();
  eqsltimer_= ui_->sbEqslTimer->value ();
  harmonicsdepth_= ui_->sbHarmonics->value ();
  nsingdecatt_ = ui_->sbNsingdecatt->value ();
  ntopfreq65_ = ui_->sbTopFreq->value ();
  nbacktocq_ = ui_->sbBackToCQ->value ();
  nretransmitmsg_ = ui_->sbRetransmitMsg->value ();
  nhalttxsamemsgrprt_ = ui_->sbHaltTxSameMsgRprt->value ();
  nhalttxsamemsg73_ = ui_->sbHaltTxSameMsg73->value ();
  fmaskact_ = ui_->fMask_check_box->isChecked ();
  backtocq_ = ui_->backToCQ_checkBox->isChecked ();
  retransmitmsg_ = ui_->retransmitMsg_checkBox->isChecked ();
  halttxsamemsgrprt_ = ui_->haltTxSameMsgRprt_checkBox->isChecked ();
  halttxsamemsg73_ = ui_->haltTxSameMsg73_checkBox->isChecked ();
  halttxreplyother_ = ui_->haltTxReplyOther_checkBox->isChecked ();
  hidefree_ = ui_->HideFree_check_box->isChecked ();
  showcq_ = ui_->ShowCQ_check_box->isChecked ();
  showcq73_ = ui_->ShowCQ73_check_box->isChecked ();
  id_after_73_ = ui_->CW_id_after_73_check_box->isChecked ();
  tx_QSY_allowed_ = ui_->tx_QSY_check_box->isChecked ();
  monitor_off_at_startup_ = ui_->monitor_off_check_box->isChecked ();
  monitor_last_used_ = ui_->monitor_last_used_check_box->isChecked ();
  type_2_msg_gen_ = static_cast<Type2MsgGen> (ui_->type_2_msg_gen_combo_box->currentIndex ());
  log_as_RTTY_ = ui_->log_as_RTTY_check_box->isChecked ();
  report_in_comments_ = ui_->report_in_comments_check_box->isChecked ();
  prompt_to_log_ = ui_->prompt_to_log_check_box->isChecked ();
  autolog_ = ui_->autolog_check_box->isChecked ();
  insert_blank_ = ui_->insert_blank_check_box->isChecked ();
  countryName_ = ui_->countryName_check_box->isChecked ();
  countryPrefix_ = ui_->countryPrefix_check_box->isChecked ();
  txtColor_ = ui_->txtColor_check_box->isChecked ();
  workedColor_ = ui_->workedColor_check_box->isChecked ();
  workedStriked_ = ui_->workedStriked_check_box->isChecked ();
  workedUnderlined_ = ui_->workedUnderlined_check_box->isChecked ();
  workedDontShow_ = ui_->workedDontShow_check_box->isChecked ();
  newDXCC_ = ui_->newDXCC_check_box->isChecked ();
  newDXCCBand_ = ui_->newDXCCBand_check_box->isChecked ();
  newDXCCBandMode_ = ui_->newDXCCBandMode_check_box->isChecked ();
  newCall_ = ui_->newCall_check_box->isChecked ();
  newCallBand_ = ui_->newCallBand_check_box->isChecked ();
  newCallBandMode_ = ui_->newCallBandMode_check_box->isChecked ();
  newGrid_ = ui_->newGrid_check_box->isChecked ();
  newGridBand_ = ui_->newGridBand_check_box->isChecked ();
  newGridBandMode_ = ui_->newGridBandMode_check_box->isChecked ();
  newPotential_ = ui_->newPotential_check_box->isChecked ();
  hideAfrica_= ui_->Africa_check_box->isChecked ();
  hideAntarctica_= ui_->Antarctica_check_box->isChecked ();
  hideAsia_= ui_->Asia_check_box->isChecked ();
  hideEurope_= ui_->Europe_check_box->isChecked ();
  hideOceania_= ui_->Oceania_check_box->isChecked ();
  hideNAmerica_= ui_->NAmerica_check_box->isChecked ();
  hideSAmerica_= ui_->SAmerica_check_box->isChecked ();
  clear_DX_ = ui_->clear_DX_check_box->isChecked ();
  clear_DX_exit_ = ui_->clear_DX_exit_check_box->isChecked ();
  miles_ = ui_->miles_check_box->isChecked ();
  watchdog_ = ui_->watchdog_check_box->isChecked ();
  TX_messages_ = ui_->TX_messages_check_box->isChecked ();
  data_mode_ = static_cast<DataMode> (ui_->TX_mode_button_group->checkedId ());
  save_directory_ = ui_->save_path_display_label->text ();
  azel_directory_ = ui_->azel_path_display_label->text ();
  enable_VHF_features_ = ui_->enable_VHF_features_check_box->isChecked ();
  decode_at_52s_ = ui_->decode_at_52s_check_box->isChecked ();

  offsetRxFreq_ = ui_->offset_Rx_freq_check_box->isChecked();
  beepOnMyCall_ = ui_->beep_on_my_call_check_box->isChecked();
  beepOnNewDXCC_ = ui_->beep_on_newDXCC_check_box->isChecked();
  beepOnNewGrid_ = ui_->beep_on_newGrid_check_box->isChecked();
  beepOnNewCall_ = ui_->beep_on_newCall_check_box->isChecked();
  beepOnFirstMsg_ = ui_->beep_on_firstMsg_check_box->isChecked();
  frequency_calibration_intercept_ = ui_->calibration_intercept_spin_box->value ();
  frequency_calibration_slope_ppm_ = ui_->calibration_slope_ppm_spin_box->value ();
  pwrBandTxMemory_ = ui_->checkBoxPwrBandTxMemory->isChecked ();
  pwrBandTuneMemory_ = ui_->checkBoxPwrBandTuneMemory->isChecked ();  

  auto new_server = ui_->udp_server_line_edit->text ();
  if (new_server != udp_server_name_)
    {
      udp_server_name_ = new_server;
      Q_EMIT self_->udp_server_changed (new_server);
    }

  auto new_port = ui_->udp_server_port_spin_box->value ();
  if (new_port != udp_server_port_)
    {
      udp_server_port_ = new_port;
      Q_EMIT self_->udp_server_port_changed (new_port);
    }

  auto new_tcpserver = ui_->tcp_server_line_edit->text ();
  if (new_tcpserver != tcp_server_name_)
    {
      tcp_server_name_ = new_tcpserver;
      Q_EMIT self_->tcp_server_changed (new_tcpserver);
    }

  auto new_tcpport = ui_->tcp_server_port_spin_box->value ();
  if (new_tcpport != tcp_server_port_)
    {
      tcp_server_port_ = new_tcpport;
      Q_EMIT self_->tcp_server_port_changed (new_tcpport);
    }

  accept_udp_requests_ = ui_->accept_udp_requests_check_box->isChecked ();
  enable_tcp_connection_ = ui_->TCP_checkBox->isChecked ();
  udpWindowToFront_ = ui_->udpWindowToFront->isChecked ();
  udpWindowRestore_ = ui_->udpWindowRestore->isChecked ();

  if (macros_.stringList () != next_macros_.stringList ())
    {
      macros_.setStringList (next_macros_.stringList ());
    }

  if (frequencies_.frequency_list () != next_frequencies_.frequency_list ())
    {
      frequencies_.frequency_list (next_frequencies_.frequency_list ());
      frequencies_.sort (FrequencyList::frequency_column);
    }

  if (stations_.station_list () != next_stations_.station_list ())
    {
      stations_.station_list(next_stations_.station_list ());
      stations_.sort (StationList::band_column);
    }
 
  write_settings ();		// make visible to all
}

void Configuration::impl::reject ()
{
  initialize_models ();		// reverts to settings as at exec ()

  // check if the Transceiver instance changed, in which case we need
  // to re open any prior Transceiver type
  if (rig_changed_)
    {
      if (have_rig_)
        {
          // we have to do this since the rig has been opened since we
          // were exec'ed even though it might fail
          open_rig ();
        }
      else
        {
          close_rig ();
        }
    }

  QDialog::reject ();
}

void Configuration::impl::message_box (QString const& reason, QString const& detail)
{
  QMessageBox mb;
  mb.setText (reason);
  if (!detail.isEmpty ())
    {
      mb.setDetailedText (detail);
    }
  mb.setStandardButtons (QMessageBox::Ok);
  mb.setDefaultButton (QMessageBox::Ok);
  mb.setIcon (QMessageBox::Critical);
  mb.exec ();
}

void Configuration::impl::on_eqsluser_edit_editingFinished()
{
  ui_->eqsl_check_box->setEnabled (!ui_->eqsluser_edit->text ().isEmpty () && !ui_->eqslpasswd_edit->text ().isEmpty () && !ui_->eqslnick_edit->text ().isEmpty ());  
  ui_->eqsl_check_box->setChecked ((!ui_->eqsluser_edit->text ().isEmpty () && !ui_->eqslpasswd_edit->text ().isEmpty () && !ui_->eqslnick_edit->text ().isEmpty ()) && send_to_eqsl_);  
}

void Configuration::impl::on_eqslpasswd_edit_editingFinished()
{
  ui_->eqsl_check_box->setEnabled (!ui_->eqsluser_edit->text ().isEmpty () && !ui_->eqslpasswd_edit->text ().isEmpty () && !ui_->eqslnick_edit->text ().isEmpty ());  
  ui_->eqsl_check_box->setChecked ((!ui_->eqsluser_edit->text ().isEmpty () && !ui_->eqslpasswd_edit->text ().isEmpty () && !ui_->eqslnick_edit->text ().isEmpty ()) && send_to_eqsl_);  
}

void Configuration::impl::on_eqslnick_edit_editingFinished()
{
  ui_->eqsl_check_box->setEnabled (!ui_->eqsluser_edit->text ().isEmpty () && !ui_->eqslpasswd_edit->text ().isEmpty () && !ui_->eqslnick_edit->text ().isEmpty ());  
  ui_->eqsl_check_box->setChecked ((!ui_->eqsluser_edit->text ().isEmpty () && !ui_->eqslpasswd_edit->text ().isEmpty () && !ui_->eqslnick_edit->text ().isEmpty ()) && send_to_eqsl_);  
}


void Configuration::impl::on_font_push_button_clicked ()
{
  next_font_ = QFontDialog::getFont (0, next_font_, this);
}

void Configuration::impl::on_countryName_check_box_clicked(bool checked)
{
  ui_->countryPrefix_check_box->setChecked(checked && countryPrefix_);
  ui_->countryPrefix_check_box->setEnabled(checked);
  ui_->Africa_check_box->setChecked (checked && hideAfrica_);
  ui_->Africa_check_box->setEnabled(checked);
  ui_->Antarctica_check_box->setChecked (checked && hideAntarctica_);
  ui_->Antarctica_check_box->setEnabled(checked);
  ui_->Asia_check_box->setChecked (checked && hideAsia_);
  ui_->Asia_check_box->setEnabled(checked);
  ui_->Europe_check_box->setChecked (checked && hideEurope_);
  ui_->Europe_check_box->setEnabled(checked);
  ui_->Oceania_check_box->setChecked (checked && hideOceania_);
  ui_->Oceania_check_box->setEnabled(checked);
  ui_->NAmerica_check_box->setChecked (checked && hideNAmerica_);
  ui_->NAmerica_check_box->setEnabled(checked);
  ui_->SAmerica_check_box->setChecked (checked && hideSAmerica_);
  ui_->SAmerica_check_box->setEnabled(checked);
}

void Configuration::impl::on_ShowCQ_check_box_clicked(bool checked)
{
  if(checked)
  {
     ui_->ShowCQ73_check_box->setChecked(!checked);
  }
}

void Configuration::impl::on_ShowCQ73_check_box_clicked(bool checked)
{
  if(checked)
  {
  ui_->ShowCQ_check_box->setChecked(!checked);
  }
}

void Configuration::impl::on_prompt_to_log_check_box_clicked(bool checked)
{
  if(checked)
  {
     ui_->autolog_check_box->setChecked(!checked);
  }
}

void Configuration::impl::on_autolog_check_box_clicked(bool checked)
{
  if(checked)
  {
  ui_->prompt_to_log_check_box->setChecked(!checked);
  }
}

void Configuration::impl::on_txtColor_check_box_clicked(bool checked)
{
  next_txtColor_ = checked;
  if (next_txtColor_){
    ui_->labCQ->setStyleSheet(QString("background: %1;color: #ffffff").arg(next_color_CQ_.name()));
    ui_->labMyCall->setStyleSheet(QString("background: %1;color: #ffffff").arg(next_color_MyCall_.name()));
    ui_->labStandardCall->setStyleSheet(QString("background: %1;color: #ffffff").arg(next_color_StandardCall_.name()));
    ui_->labNewDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_CQ_.name()));
    ui_->labNewDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_CQ_.name()));
    ui_->labNewGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_CQ_.name()));
    ui_->labNewGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_CQ_.name()));
    ui_->labNewCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_CQ_.name()));
    ui_->labNewCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_CQ_.name()));
    if (next_workedColor_) {
      if (next_workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else if (next_workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      }
    } else if (next_workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
    } else if (next_workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_StandardCall_.name()));
    }
    ui_->labNewMcDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_MyCall_.name()));
    ui_->labNewMcDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_MyCall_.name()));
    ui_->labNewMcGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_MyCall_.name()));
    ui_->labNewMcGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_MyCall_.name()));
    ui_->labNewMcCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_MyCall_.name()));
    ui_->labNewMcCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_MyCall_.name()));
    ui_->labNewScDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_StandardCall_.name()));
    ui_->labNewScDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_StandardCall_.name()));
    ui_->labNewScGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_StandardCall_.name()));
    ui_->labNewScGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_StandardCall_.name()));
    ui_->labNewScCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_StandardCall_.name()));
    ui_->labNewScCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_StandardCall_.name()));
  } else {
    ui_->labCQ->setStyleSheet(QString("background: #ffffff;color: %1").arg(next_color_CQ_.name()));
    ui_->labMyCall->setStyleSheet(QString("background: #ffffff;color: %1").arg(next_color_MyCall_.name()));
    ui_->labStandardCall->setStyleSheet(QString("background: #ffffff;color: %1").arg(next_color_StandardCall_.name()));
    ui_->labNewDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_CQ_.name()));
    ui_->labNewDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_CQ_.name()));
    ui_->labNewGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_CQ_.name()));
    ui_->labNewGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_CQ_.name()));
    ui_->labNewCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_CQ_.name()));
    ui_->labNewCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_CQ_.name()));
    if (next_workedColor_) {
      if (next_workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else if (next_workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      }
    } else if (next_workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
    } else if (next_workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_StandardCall_.name()));
    }
    ui_->labNewMcDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_MyCall_.name()));
    ui_->labNewMcDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_MyCall_.name()));
    ui_->labNewMcGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_MyCall_.name()));
    ui_->labNewMcGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_MyCall_.name()));
    ui_->labNewMcCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_MyCall_.name()));
    ui_->labNewMcCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_MyCall_.name()));
    ui_->labNewScDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_StandardCall_.name()));
    ui_->labNewScDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_StandardCall_.name()));
    ui_->labNewScGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_StandardCall_.name()));
    ui_->labNewScGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_StandardCall_.name()));
    ui_->labNewScCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_StandardCall_.name()));
    ui_->labNewScCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_StandardCall_.name()));
  }
}

void Configuration::impl::on_workedColor_check_box_clicked(bool checked)
{
  next_workedColor_ = checked;
  if (next_txtColor_) {
    if (next_workedColor_) {
      if (next_workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else if (next_workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      }
    } else if (next_workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
    } else if (next_workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_StandardCall_.name()));
    }
  } else {
    if (next_workedColor_) {
      if (next_workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else if (next_workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      }
    } else if (next_workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
    } else if (next_workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_StandardCall_.name()));
    }
  }
}

void Configuration::impl::on_workedDontShow_check_box_clicked(bool checked)
{
  next_workedDontShow_ = checked;
}

void Configuration::impl::on_workedStriked_check_box_clicked(bool checked)
{
  next_workedStriked_ = checked;
  ui_->workedUnderlined_check_box->setChecked(!checked && workedUnderlined_);
  ui_->workedUnderlined_check_box->setEnabled(!checked);
  if (next_txtColor_) {
    if (next_workedColor_) {
      if (next_workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else if (next_workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      }
    } else if (next_workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
    } else if (next_workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_StandardCall_.name()));
    }
  } else {
    if (next_workedColor_) {
      if (next_workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else if (next_workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      }
    } else if (next_workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
    } else if (next_workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_StandardCall_.name()));
    }
  }
}

void Configuration::impl::on_workedUnderlined_check_box_clicked(bool checked)
{
  next_workedUnderlined_ = checked;
  ui_->workedStriked_check_box->setChecked(!checked && workedStriked_);
  ui_->workedStriked_check_box->setEnabled(!checked);
  if (next_txtColor_) {
    if (next_workedColor_) {
      if (next_workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else if (next_workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      }
    } else if (next_workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
    } else if (next_workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_StandardCall_.name()));
    }
  } else {
    if (next_workedColor_) {
      if (next_workedStriked_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else if (next_workedUnderlined_) {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
        ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
        ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
      }
    } else if (next_workedStriked_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
    } else if (next_workedUnderlined_) {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
    } else {
      ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_CQ_.name()));
      ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_MyCall_.name()));
      ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_StandardCall_.name()));
    }
  }
}

void Configuration::impl::on_newPotential_check_box_clicked(bool checked)
{
  next_newPotential_ = checked;
  ui_->labStandardCall->setVisible(checked);
  ui_->labNewScDXCC->setVisible(next_newDXCC_ && checked);
  ui_->labNewScDXCCBand->setVisible((next_newDXCCBandMode_ || next_newDXCCBand_) && checked);
  ui_->labNewScGrid->setVisible(next_newGrid_ && checked);
  ui_->labNewScGridBand->setVisible((next_newGridBandMode_ || next_newGridBand_) && checked);
  ui_->labNewScCall->setVisible(next_newCall_ && checked);
  ui_->labNewScCallBand->setVisible((next_newCallBandMode_ || next_newCallBand_) && checked);
  ui_->labWorkedScCall->setVisible((next_newDXCC_ || next_newGrid_ || next_newCall_) && checked);
}

void Configuration::impl::on_newDXCC_check_box_clicked(bool checked)
{
  next_newDXCC_ = checked;
  ui_->newDXCCBand_check_box->setChecked(checked && newDXCCBand_);
  next_newDXCCBand_ = checked && newDXCCBand_;
  ui_->newDXCCBand_check_box->setEnabled(checked);
  ui_->beep_on_newDXCC_check_box->setChecked(checked && beepOnNewDXCC_);
  ui_->beep_on_newDXCC_check_box->setEnabled(checked);
  ui_->newDXCCBandMode_check_box->setChecked(checked && newDXCCBandMode_);
  next_newDXCCBandMode_ = checked && newDXCCBandMode_;
  ui_->newDXCCBandMode_check_box->setEnabled(checked);
  ui_->workedColor_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_workedColor_);
  ui_->workedColor_check_box->setEnabled(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->workedStriked_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedUnderlined_ && next_workedStriked_);
  ui_->workedStriked_check_box->setEnabled((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedUnderlined_);
  ui_->workedUnderlined_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedStriked_ && next_workedUnderlined_);
  ui_->workedUnderlined_check_box->setEnabled((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedStriked_);
  ui_->workedDontShow_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_workedDontShow_);
  ui_->workedDontShow_check_box->setEnabled(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labNewDXCC->setVisible(next_newDXCC_);
  ui_->labNewMcDXCC->setVisible(next_newDXCC_);
  ui_->labNewScDXCC->setVisible(next_newDXCC_ && next_newPotential_);
  ui_->labNewDXCCBand->setVisible(next_newDXCCBand_ || next_newDXCCBandMode_);
  ui_->labNewMcDXCCBand->setVisible(next_newDXCCBand_ || next_newDXCCBandMode_);
  ui_->labNewScDXCCBand->setVisible((next_newDXCCBand_ || next_newDXCCBandMode_) && next_newPotential_);
  ui_->labWorkedCall->setVisible(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labWorkedMcCall->setVisible(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labWorkedScCall->setVisible((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_newPotential_);
}

void Configuration::impl::on_newCall_check_box_clicked(bool checked)
{
  next_newCall_ = checked;
  ui_->newCallBand_check_box->setChecked(checked && newCallBand_);
  next_newCallBand_ = checked && newCallBand_;
  ui_->newCallBand_check_box->setEnabled(checked);
  ui_->beep_on_newCall_check_box->setChecked(checked && beepOnNewCall_);
  ui_->beep_on_newCall_check_box->setEnabled(checked);
  ui_->newCallBandMode_check_box->setChecked(checked && newCallBandMode_);
  next_newCallBandMode_ = checked && newCallBandMode_;
  ui_->newCallBandMode_check_box->setEnabled(checked);
  ui_->workedColor_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_workedColor_);
  ui_->workedColor_check_box->setEnabled(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->workedStriked_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedUnderlined_ && next_workedStriked_);
  ui_->workedStriked_check_box->setEnabled((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedUnderlined_);
  ui_->workedUnderlined_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedStriked_ && next_workedUnderlined_);
  ui_->workedUnderlined_check_box->setEnabled((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedStriked_);
  ui_->workedDontShow_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_workedDontShow_);
  ui_->workedDontShow_check_box->setEnabled(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labNewCall->setVisible(next_newCall_);
  ui_->labNewMcCall->setVisible(next_newCall_);
  ui_->labNewScCall->setVisible(next_newCall_ && next_newPotential_);
  ui_->labNewCallBand->setVisible(next_newCallBand_ || next_newCallBandMode_);
  ui_->labNewMcCallBand->setVisible(next_newCallBand_ || next_newCallBandMode_);
  ui_->labNewScCallBand->setVisible((next_newCallBand_ || next_newCallBandMode_) && next_newPotential_);
  ui_->labWorkedCall->setVisible(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labWorkedMcCall->setVisible(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labWorkedScCall->setVisible((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_newPotential_);
}

void Configuration::impl::on_newGrid_check_box_clicked(bool checked)
{
  next_newGrid_ = checked;
  ui_->newGridBand_check_box->setChecked(checked && newGridBand_);
  next_newGridBand_ = checked && newGridBand_;
  ui_->newGridBand_check_box->setEnabled(checked);
  ui_->beep_on_newGrid_check_box->setChecked(checked && beepOnNewGrid_);
  ui_->beep_on_newGrid_check_box->setEnabled(checked);
  ui_->newGridBandMode_check_box->setChecked(checked && newGridBandMode_);
  next_newGridBandMode_ = checked && newGridBandMode_;
  ui_->newGridBandMode_check_box->setEnabled(checked);
  ui_->workedColor_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_workedColor_);
  ui_->workedColor_check_box->setEnabled(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->workedStriked_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedUnderlined_ && next_workedStriked_);
  ui_->workedStriked_check_box->setEnabled((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedUnderlined_);
  ui_->workedUnderlined_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedStriked_ && next_workedUnderlined_);
  ui_->workedUnderlined_check_box->setEnabled((next_newDXCC_ || next_newGrid_ || next_newCall_) && !next_workedStriked_);
  ui_->workedDontShow_check_box->setChecked((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_workedDontShow_);
  ui_->workedDontShow_check_box->setEnabled(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labNewGrid->setVisible(next_newGrid_);
  ui_->labNewMcGrid->setVisible(next_newGrid_);
  ui_->labNewScGrid->setVisible(next_newGrid_ && next_newPotential_);
  ui_->labNewGridBand->setVisible(next_newGridBand_ || next_newGridBandMode_);
  ui_->labNewMcGridBand->setVisible(next_newGridBand_ || next_newGridBandMode_);
  ui_->labNewScGridBand->setVisible((next_newGridBand_ || next_newGridBandMode_) && next_newPotential_);
  ui_->labWorkedCall->setVisible(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labWorkedMcCall->setVisible(next_newDXCC_ || next_newGrid_ || next_newCall_);
  ui_->labWorkedScCall->setVisible((next_newDXCC_ || next_newGrid_ || next_newCall_) && next_newPotential_);
}

void Configuration::impl::on_newDXCCBand_check_box_clicked(bool checked)
{
  next_newDXCCBand_ = checked;
  ui_->labNewDXCCBand->setVisible(next_newDXCCBand_ || next_newDXCCBandMode_);
  ui_->labNewMcDXCCBand->setVisible(next_newDXCCBand_ || next_newDXCCBandMode_);
  ui_->labNewScDXCCBand->setVisible((next_newDXCCBand_ || next_newDXCCBandMode_) && next_newPotential_);
}

void Configuration::impl::on_newCallBand_check_box_clicked(bool checked)
{
  next_newCallBand_ = checked;
  ui_->labNewCallBand->setVisible(next_newCallBand_ || next_newCallBandMode_);
  ui_->labNewMcCallBand->setVisible(next_newCallBand_ || next_newCallBandMode_);
  ui_->labNewScCallBand->setVisible((next_newCallBand_ || next_newCallBandMode_) && next_newPotential_);
}

void Configuration::impl::on_newGridBand_check_box_clicked(bool checked)
{
  next_newGridBand_ = checked;
  ui_->labNewGridBand->setVisible(next_newGridBand_ || next_newGridBandMode_);
  ui_->labNewMcGridBand->setVisible(next_newGridBand_ || next_newGridBandMode_);
  ui_->labNewScGridBand->setVisible((next_newGridBand_ || next_newGridBandMode_) && next_newPotential_);
}

void Configuration::impl::on_newDXCCBandMode_check_box_clicked(bool checked)
{
  next_newDXCCBandMode_ = checked;
  ui_->labNewDXCCBand->setVisible(next_newDXCCBand_ || next_newDXCCBandMode_);
  ui_->labNewMcDXCCBand->setVisible(next_newDXCCBand_ || next_newDXCCBandMode_);
  ui_->labNewScDXCCBand->setVisible((next_newDXCCBand_ || next_newDXCCBandMode_) && next_newPotential_);
}

void Configuration::impl::on_newCallBandMode_check_box_clicked(bool checked)
{
  next_newCallBandMode_ = checked;
  ui_->labNewCallBand->setVisible(next_newCallBand_ || next_newCallBandMode_);
  ui_->labNewMcCallBand->setVisible(next_newCallBand_ || next_newCallBandMode_);
  ui_->labNewScCallBand->setVisible((next_newCallBand_ || next_newCallBandMode_) && next_newPotential_);
}

void Configuration::impl::on_newGridBandMode_check_box_clicked(bool checked)
{
  next_newGridBandMode_ = checked;
  ui_->labNewGridBand->setVisible(next_newGridBand_ || next_newGridBandMode_);
  ui_->labNewMcGridBand->setVisible(next_newGridBand_ || next_newGridBandMode_);
  ui_->labNewScGridBand->setVisible((next_newGridBand_ || next_newGridBandMode_) && next_newPotential_);
}

void Configuration::impl::on_pbCQmsg_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_CQ_, this, "CQ Messages Color");
  if (new_color.isValid ())
    {
      next_color_CQ_ = new_color;
      if (next_txtColor_) {
        ui_->labCQ->setStyleSheet(QString("background: %1;color: #ffffff").arg(next_color_CQ_.name()));
        ui_->labNewDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_CQ_.name()));
        ui_->labNewDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_CQ_.name()));
        ui_->labNewGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_CQ_.name()));
        ui_->labNewGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_CQ_.name()));
        ui_->labNewCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_CQ_.name()));
        ui_->labNewCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_CQ_.name()));
        if (next_workedColor_) {
          if (next_workedStriked_) {
            ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
          } else if (next_workedUnderlined_) {
            ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
          } else {
            ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
          }
        } else if (next_workedStriked_) {
          ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_CQ_.name()));
        } else if (next_workedUnderlined_) {
          ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_CQ_.name()));
        } else {
          ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_CQ_.name()));
        }
      } else {      
        ui_->labCQ->setStyleSheet(QString("background: #ffffff;color: %1").arg(next_color_CQ_.name()));
        ui_->labNewDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_CQ_.name()));
        ui_->labNewDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_CQ_.name()));
        ui_->labNewGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_CQ_.name()));
        ui_->labNewGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_CQ_.name()));
        ui_->labNewCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_CQ_.name()));
        ui_->labNewCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_CQ_.name()));
        if (next_workedColor_) {
          if (next_workedStriked_) {
            ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
          } else if (next_workedUnderlined_) {
            ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
          } else {
            ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
          }
        } else if (next_workedStriked_) {
          ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_CQ_.name()));
        } else if (next_workedUnderlined_) {
          ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_CQ_.name()));
        } else {
          ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_CQ_.name()));
        }
      }
    }
}

void Configuration::impl::on_pbMyCall_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_MyCall_, this, "My Call Messages Color");
  if (new_color.isValid ())
    {
      next_color_MyCall_ = new_color;
      if (next_txtColor_) {
        ui_->labMyCall->setStyleSheet(QString("background: %1;color: #ffffff").arg(next_color_MyCall_.name()));
        ui_->labNewMcDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_MyCall_.name()));
        ui_->labNewMcDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_MyCall_.name()));
        ui_->labNewMcGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_MyCall_.name()));
        ui_->labNewMcGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_MyCall_.name()));
        ui_->labNewMcCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_MyCall_.name()));
        ui_->labNewMcCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_MyCall_.name()));
        if (next_workedColor_) {
          if (next_workedStriked_) {
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
          } else if (next_workedUnderlined_) {
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
          } else {
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
          }
        } else if (next_workedStriked_) {
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
        } else if (next_workedUnderlined_) {
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_MyCall_.name()));
        } else {
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_MyCall_.name()));
        }
      } else {      
        ui_->labMyCall->setStyleSheet(QString("background: #ffffff;color: %1").arg(next_color_MyCall_.name()));
        ui_->labNewMcDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_MyCall_.name()));
        ui_->labNewMcDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_MyCall_.name()));
        ui_->labNewMcGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_MyCall_.name()));
        ui_->labNewMcGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_MyCall_.name()));
        ui_->labNewMcCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_MyCall_.name()));
        ui_->labNewMcCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_MyCall_.name()));
        if (next_workedColor_) {
          if (next_workedStriked_) {
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
          } else if (next_workedUnderlined_) {
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
          } else {
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
          }
        } else if (next_workedStriked_) {
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
        } else if (next_workedUnderlined_) {
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_MyCall_.name()));
        } else {
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_MyCall_.name()));
        }
      }
    }
}

void Configuration::impl::on_pbStandardCall_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_StandardCall_, this, "Standard Messages Color");
  if (new_color.isValid ())
    {
      next_color_StandardCall_ = new_color;
      if (next_txtColor_) {
        ui_->labStandardCall->setStyleSheet(QString("background: %1;color: #ffffff").arg(next_color_StandardCall_.name()));
        ui_->labNewScDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_StandardCall_.name()));
        ui_->labNewScDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_StandardCall_.name()));
        ui_->labNewScGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_StandardCall_.name()));
        ui_->labNewScGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_StandardCall_.name()));
        ui_->labNewScCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_StandardCall_.name()));
        ui_->labNewScCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_StandardCall_.name()));
        if (next_workedColor_) {
          if (next_workedStriked_) {
            ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          } else if (next_workedUnderlined_) {
            ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          } else {
            ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          }
        } else if (next_workedStriked_) {
          ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
        } else if (next_workedUnderlined_) {
          ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
        } else {
          ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_StandardCall_.name()));
        }
      } else {
        ui_->labStandardCall->setStyleSheet(QString("background: #ffffff;color: %1").arg(next_color_StandardCall_.name()));
        ui_->labNewScDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_StandardCall_.name()));
        ui_->labNewScDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_StandardCall_.name()));
        ui_->labNewScGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_StandardCall_.name()));
        ui_->labNewScGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_StandardCall_.name()));
        ui_->labNewScCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_StandardCall_.name()));
        ui_->labNewScCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_StandardCall_.name()));
        if (next_workedColor_) {
          if (next_workedStriked_) {
            ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          } else if (next_workedUnderlined_) {
            ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          } else {
            ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          }
        } else if (next_workedStriked_) {
          ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
        } else if (next_workedUnderlined_) {
          ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
        } else {
          ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_StandardCall_.name()));
        }
      }
    }
}

void Configuration::impl::on_pbTxMsg_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_TxMsg_, this, "Tx Messages Color");
  if (new_color.isValid ())
    {
      next_color_TxMsg_ = new_color;
      ui_->labTx->setStyleSheet(QString("background: %1").arg(next_color_TxMsg_.name()));
    }
}

void Configuration::impl::on_pbNewDXCC_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_NewDXCC_, this, "New DXCC Messages Color");
  if (new_color.isValid ())
    {
      next_color_NewDXCC_ = new_color;
      if (next_txtColor_) {
        ui_->labNewDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_CQ_.name()));
        ui_->labNewMcDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_MyCall_.name()));
        ui_->labNewScDXCC->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCC_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labNewDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_CQ_.name()));
        ui_->labNewMcDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_MyCall_.name()));
        ui_->labNewScDXCC->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCC_.name(),next_color_StandardCall_.name()));
      }
    }
}

void Configuration::impl::on_pbNewDXCCBand_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_NewDXCCBand_, this, "New DXCC on Band/Mode Messages Color");
  if (new_color.isValid ())
    {
      next_color_NewDXCCBand_ = new_color;
      if (next_txtColor_) {
        ui_->labNewDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_CQ_.name()));
        ui_->labNewMcDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_MyCall_.name()));
        ui_->labNewScDXCCBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewDXCCBand_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labNewDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_CQ_.name()));
        ui_->labNewMcDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_MyCall_.name()));
        ui_->labNewScDXCCBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewDXCCBand_.name(),next_color_StandardCall_.name()));
      }
    }
}

void Configuration::impl::on_pbNewGrid_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_NewGrid_, this, "New Grid Messages Color");
  if (new_color.isValid ())
    {
      next_color_NewGrid_ = new_color;
      if (next_txtColor_) {
        ui_->labNewGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_CQ_.name()));
        ui_->labNewMcGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_MyCall_.name()));
        ui_->labNewScGrid->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGrid_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labNewGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_CQ_.name()));
        ui_->labNewMcGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_MyCall_.name()));
        ui_->labNewScGrid->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGrid_.name(),next_color_StandardCall_.name()));
      }
    }
}

void Configuration::impl::on_pbNewGridBand_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_NewGridBand_, this, "New Grid on Band/Mode Messages Color");
  if (new_color.isValid ())
    {
      next_color_NewGridBand_ = new_color;
      if (next_txtColor_) {
        ui_->labNewGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_CQ_.name()));
        ui_->labNewMcGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_MyCall_.name()));
        ui_->labNewScGridBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewGridBand_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labNewGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_CQ_.name()));
        ui_->labNewMcGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_MyCall_.name()));
        ui_->labNewScGridBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewGridBand_.name(),next_color_StandardCall_.name()));
      }
    }
}

void Configuration::impl::on_pbNewCall_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_NewCall_, this, "New Call Messages Color");
  if (new_color.isValid ())
    {
      next_color_NewCall_ = new_color;
      if (next_txtColor_) {
        ui_->labNewCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_CQ_.name()));
        ui_->labNewMcCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_MyCall_.name()));
        ui_->labNewScCall->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCall_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labNewCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_CQ_.name()));
        ui_->labNewMcCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_MyCall_.name()));
        ui_->labNewScCall->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCall_.name(),next_color_StandardCall_.name()));
      }
    }
}

void Configuration::impl::on_pbNewCallBand_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_NewCallBand_, this, "New Call on Band/Mode Messages Color");
  if (new_color.isValid ())
    {
      next_color_NewCallBand_ = new_color;
      if (next_txtColor_) {
        ui_->labNewCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_CQ_.name()));
        ui_->labNewMcCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_MyCall_.name()));
        ui_->labNewScCallBand->setStyleSheet(QString("font-weight: bold;background: %2;color: %1").arg(next_color_NewCallBand_.name(),next_color_StandardCall_.name()));
      } else {
        ui_->labNewCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_CQ_.name()));
        ui_->labNewMcCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_MyCall_.name()));
        ui_->labNewScCallBand->setStyleSheet(QString("font-weight: bold;background: %1;color: %2").arg(next_color_NewCallBand_.name(),next_color_StandardCall_.name()));
      }
    }
}

void Configuration::impl::on_pbWorkedCall_clicked()
{
  auto new_color = QColorDialog::getColor(next_color_WorkedCall_, this, "Worked Call Color");
  if (new_color.isValid ())
    {
      next_color_WorkedCall_ = new_color;
      if (next_txtColor_) {
        if (next_workedColor_) {
          if (next_workedStriked_) {
            ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
            ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          } else if (next_workedUnderlined_) {
            ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
            ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          } else {
            ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
            ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          }
        } else if (next_workedStriked_) {
          ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_CQ_.name()));
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
          ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
        } else if (next_workedUnderlined_) {
          ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_CQ_.name()));
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_MyCall_.name()));
          ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
        } else {
          ui_->labWorkedCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_CQ_.name()));
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_MyCall_.name()));
          ui_->labWorkedScCall->setStyleSheet(QString("background: %2;color: %1").arg("white",next_color_StandardCall_.name()));
        }
      } else {
        if (next_workedColor_) {
          if (next_workedStriked_) {
            ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
            ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          } else if (next_workedUnderlined_) {
            ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
            ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          } else {
            ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_CQ_.name()));
            ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_MyCall_.name()));
            ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg(next_color_WorkedCall_.name(),next_color_StandardCall_.name()));
          }
        } else if (next_workedStriked_) {
          ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_CQ_.name()));
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_MyCall_.name()));
          ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: line-through").arg("white",next_color_StandardCall_.name()));
        } else if (next_workedUnderlined_) {
          ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_CQ_.name()));
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_MyCall_.name()));
          ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2;text-decoration: underline").arg("white",next_color_StandardCall_.name()));
        } else {
          ui_->labWorkedCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_CQ_.name()));
          ui_->labWorkedMcCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_MyCall_.name()));
          ui_->labWorkedScCall->setStyleSheet(QString("background: %1;color: %2").arg("white",next_color_StandardCall_.name()));
        }
      }
    }
}

void Configuration::impl::on_decoded_text_font_push_button_clicked ()
{
  next_decoded_text_font_ = QFontDialog::getFont (0, decoded_text_font_ , this
                                                  , tr ("JTDX Decoded Text Font Chooser")
#if QT_VERSION >= 0x050201
                                                  , QFontDialog::MonospacedFonts
#endif
                                                  );
}

void Configuration::impl::on_PTT_port_combo_box_activated (int /* index */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_CAT_port_combo_box_activated (int /* index */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_CAT_serial_baud_combo_box_currentIndexChanged (int /* index */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_CAT_handshake_button_group_buttonClicked (int /* id */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_rig_combo_box_currentIndexChanged (int /* index */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_CAT_data_bits_button_group_buttonClicked (int /* id */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_CAT_stop_bits_button_group_buttonClicked (int /* id */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_CAT_poll_interval_spin_box_valueChanged (int /* value */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_split_mode_button_group_buttonClicked (int /* id */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_test_CAT_push_button_clicked ()
{
  if (!validate ())
    {
      return;
    }

  ui_->test_CAT_push_button->setStyleSheet ({});
  if (open_rig (true))
    {
      //Q_EMIT sync (true);
    }

  set_rig_invariants ();
}

void Configuration::impl::on_test_PTT_push_button_clicked (bool checked)
{
  ui_->test_PTT_push_button->setChecked (!checked); // let status
                                                    // update check us
  if (!validate ())
    {
      return;
    }

  if (open_rig ())
    {
      Q_EMIT self_->transceiver_ptt (checked);
    }
}

void Configuration::impl::on_force_DTR_combo_box_currentIndexChanged (int /* index */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_force_RTS_combo_box_currentIndexChanged (int /* index */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_PTT_method_button_group_buttonClicked (int /* id */)
{
  set_rig_invariants ();
}

void Configuration::impl::on_callsign_line_edit_editingFinished ()
{
  ui_->callsign_line_edit->setText (ui_->callsign_line_edit->text ().toUpper ());
}

void Configuration::impl::on_grid_line_edit_editingFinished ()
{
  auto text = ui_->grid_line_edit->text ();
  ui_->grid_line_edit->setText (text.left (4).toUpper () + text.mid (4).toLower ());
}

void Configuration::impl::on_sound_input_combo_box_currentTextChanged (QString const& text)
{
  default_audio_input_device_selected_ = QAudioDeviceInfo::defaultInputDevice ().deviceName () == text;
}

void Configuration::impl::on_sound_output_combo_box_currentTextChanged (QString const& text)
{
  default_audio_output_device_selected_ = QAudioDeviceInfo::defaultOutputDevice ().deviceName () == text;
}

void Configuration::impl::on_hhComboBox_1_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->mmComboBox_1->setEnabled(true);
     ui_->mmComboBox_1->setCurrentText (sched_mm_1_);
  } else {
     ui_->mmComboBox_1->setEnabled(false);
     ui_->mmComboBox_1->setCurrentIndex(0);
  }
//  printf("hh1 changed %d\n",index);
}

void Configuration::impl::on_mmComboBox_1_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->bandComboBox_1->setEnabled(true);
     ui_->bandComboBox_1->setCurrentText (sched_band_1_);
  } else {
     ui_->bandComboBox_1->setEnabled(false);
     ui_->bandComboBox_1->setCurrentIndex(-1);
  }
//  printf("mm1 changed %d\n",index);
}

void Configuration::impl::on_bandComboBox_1_currentIndexChanged(QString const& text)
{
  if (text != ""){
      ui_->bandComboBox_1->setCurrentText(text);
      ui_->hhComboBox_2->setEnabled(true);
      ui_->hhComboBox_2->setCurrentText (sched_hh_2_);
      ui_->UseSched_check_box->setEnabled(true);
      ui_->UseSched_check_box->setChecked (usesched_);
      if (text.right(4) == "JT65") {
        ui_->band_mix_check_box_1->setEnabled(true);
        ui_->band_mix_check_box_1->setChecked (sched_mix_1_);
      } else {
        ui_->band_mix_check_box_1->setEnabled(false);
        ui_->band_mix_check_box_1->setChecked (false);
      }
  } else if (ui_->bandComboBox_1->isEnabled()) {
      ui_->bandComboBox_1->setCurrentText(sched_band_1_);
      if (!sched_band_1_.isEmpty ()){
        ui_->hhComboBox_2->setEnabled(true);
        ui_->hhComboBox_2->setCurrentText (sched_hh_2_);
        ui_->UseSched_check_box->setEnabled(true);
        ui_->UseSched_check_box->setChecked (usesched_);
        if (sched_band_1_.right(4) == "JT65") {
          ui_->band_mix_check_box_1->setEnabled(true);
          ui_->band_mix_check_box_1->setChecked (sched_mix_1_);
        } else {
          ui_->band_mix_check_box_1->setEnabled(false);
          ui_->band_mix_check_box_1->setChecked (false);
        }
      } else {
        ui_->hhComboBox_2->setEnabled(false);
        ui_->hhComboBox_2->setCurrentIndex(0);
        ui_->UseSched_check_box->setEnabled(false);
        ui_->UseSched_check_box->setChecked (false);
        ui_->band_mix_check_box_1->setEnabled(false);
        ui_->band_mix_check_box_1->setChecked (false);
      }
  } 
  else {
      ui_->bandComboBox_1->setCurrentText("");
      ui_->hhComboBox_2->setEnabled(false);
      ui_->hhComboBox_2->setCurrentIndex(0);
      ui_->UseSched_check_box->setEnabled(false);
      ui_->UseSched_check_box->setChecked (false);
      ui_->band_mix_check_box_1->setEnabled(false);
      ui_->band_mix_check_box_1->setChecked (false);
  }
//  printf("band1 changed %s:%s\n",text.toStdString().c_str(),ui_->bandComboBox_1->currentText().toStdString().c_str());
}

void Configuration::impl::on_hhComboBox_2_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->mmComboBox_2->setEnabled(true);
     ui_->mmComboBox_2->setCurrentText (sched_mm_2_);
  } else {
     ui_->mmComboBox_2->setEnabled(false);
     ui_->mmComboBox_2->setCurrentIndex(0);
  }
//  printf("hh2 changed %d\n",index);
}

void Configuration::impl::on_mmComboBox_2_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->bandComboBox_2->setEnabled(true);
     ui_->bandComboBox_2->setCurrentText (sched_band_2_);
  } else {
     ui_->bandComboBox_2->setEnabled(false);
     ui_->bandComboBox_2->setCurrentIndex(-1);
  }
//  printf("mm2 changed %d\n",index);
}

void Configuration::impl::on_bandComboBox_2_currentIndexChanged(QString const& text)
{
  if (text != ""){
      ui_->bandComboBox_2->setCurrentText(text);
      ui_->hhComboBox_3->setEnabled(true);
      ui_->hhComboBox_3->setCurrentText (sched_hh_3_);
      if (text.right(4) == "JT65") {
        ui_->band_mix_check_box_2->setEnabled(true);
        ui_->band_mix_check_box_2->setChecked (sched_mix_2_);
      } else {
        ui_->band_mix_check_box_2->setEnabled(false);
        ui_->band_mix_check_box_2->setChecked (false);
      }
  } else if (ui_->bandComboBox_2->isEnabled()) {
      ui_->bandComboBox_2->setCurrentText(sched_band_2_);
      if (!sched_band_2_.isEmpty ()){
        ui_->hhComboBox_3->setEnabled(true);
        ui_->hhComboBox_3->setCurrentText (sched_hh_3_);
        if (sched_band_2_.right(4) == "JT65") {
          ui_->band_mix_check_box_2->setEnabled(true);
          ui_->band_mix_check_box_2->setChecked (sched_mix_2_);
        } else {
          ui_->band_mix_check_box_2->setEnabled(false);
          ui_->band_mix_check_box_2->setChecked (false);
        }
      } else {
        ui_->hhComboBox_3->setEnabled(false);
        ui_->hhComboBox_3->setCurrentIndex(0);
        ui_->band_mix_check_box_2->setEnabled(false);
        ui_->band_mix_check_box_2->setChecked (false);
      }
  } 
  else {
      ui_->bandComboBox_2->setCurrentText("");
      ui_->hhComboBox_3->setEnabled(false);
      ui_->hhComboBox_3->setCurrentIndex(0);
      ui_->band_mix_check_box_2->setEnabled(false);
      ui_->band_mix_check_box_2->setChecked (false);
  }
//  printf("band2 changed %s:%s\n",text.toStdString().c_str(),ui_->bandComboBox_2->currentText().toStdString().c_str());
}

void Configuration::impl::on_hhComboBox_3_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->mmComboBox_3->setEnabled(true);
     ui_->mmComboBox_3->setCurrentText (sched_mm_3_);
  } else {
     ui_->mmComboBox_3->setEnabled(false);
     ui_->mmComboBox_3->setCurrentIndex(0);
  }
//  printf("hh3 changed %d\n",index);
}

void Configuration::impl::on_mmComboBox_3_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->bandComboBox_3->setEnabled(true);
     ui_->bandComboBox_3->setCurrentText (sched_band_3_);
  } else {
     ui_->bandComboBox_3->setEnabled(false);
     ui_->bandComboBox_3->setCurrentIndex(-1);
  }
//  printf("mm3 changed %d\n",index);
}

void Configuration::impl::on_bandComboBox_3_currentIndexChanged(QString const& text)
{
  if (text != ""){
      ui_->bandComboBox_3->setCurrentText(text);
      ui_->hhComboBox_4->setEnabled(true);
      ui_->hhComboBox_4->setCurrentText (sched_hh_4_);
      if (text.right(4) == "JT65") {
        ui_->band_mix_check_box_3->setEnabled(true);
        ui_->band_mix_check_box_3->setChecked (sched_mix_3_);
      } else {
        ui_->band_mix_check_box_3->setEnabled(false);
        ui_->band_mix_check_box_3->setChecked (false);
      }
  } else if (ui_->bandComboBox_3->isEnabled()) {
      ui_->bandComboBox_3->setCurrentText(sched_band_3_);
      if (!sched_band_3_.isEmpty ()){
        ui_->hhComboBox_4->setEnabled(true);
        ui_->hhComboBox_4->setCurrentText (sched_hh_4_);
        if (sched_band_3_.right(4) == "JT65") {
          ui_->band_mix_check_box_3->setEnabled(true);
          ui_->band_mix_check_box_3->setChecked (sched_mix_3_);
        } else {
          ui_->band_mix_check_box_3->setEnabled(false);
          ui_->band_mix_check_box_3->setChecked (false);
        }
      } else {
        ui_->hhComboBox_4->setEnabled(false);
        ui_->hhComboBox_4->setCurrentIndex(0);
        ui_->band_mix_check_box_3->setEnabled(false);
        ui_->band_mix_check_box_3->setChecked (false);
      }
  } 
  else {
      ui_->bandComboBox_3->setCurrentText("");
      ui_->hhComboBox_4->setEnabled(false);
      ui_->hhComboBox_4->setCurrentIndex(0);
      ui_->band_mix_check_box_3->setEnabled(false);
      ui_->band_mix_check_box_3->setChecked (false);
  }
//  printf("band3 changed %s:%s\n",text.toStdString().c_str(),ui_->bandComboBox_3->currentText().toStdString().c_str());
}

void Configuration::impl::on_hhComboBox_4_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->mmComboBox_4->setEnabled(true);
     ui_->mmComboBox_4->setCurrentText (sched_mm_4_);
  } else {
     ui_->mmComboBox_4->setEnabled(false);
     ui_->mmComboBox_4->setCurrentIndex(0);
  }
//  printf("hh4 changed %d\n",index);
}

void Configuration::impl::on_mmComboBox_4_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->bandComboBox_4->setEnabled(true);
     ui_->bandComboBox_4->setCurrentText (sched_band_4_);
  } else {
     ui_->bandComboBox_4->setEnabled(false);
     ui_->bandComboBox_4->setCurrentIndex(-1);
  }
//  printf("mm4 changed %d\n",index);
}

void Configuration::impl::on_bandComboBox_4_currentIndexChanged(QString const& text)
{
  if (text != ""){
      ui_->bandComboBox_4->setCurrentText(text);
      ui_->hhComboBox_5->setEnabled(true);
      ui_->hhComboBox_5->setCurrentText (sched_hh_5_);
      if (text.right(4) == "JT65") {
        ui_->band_mix_check_box_4->setEnabled(true);
        ui_->band_mix_check_box_4->setChecked (sched_mix_4_);
      } else {
        ui_->band_mix_check_box_4->setEnabled(false);
        ui_->band_mix_check_box_4->setChecked (false);
      }
  } else if (ui_->bandComboBox_4->isEnabled()) {
      ui_->bandComboBox_4->setCurrentText(sched_band_4_);
      if (!sched_band_4_.isEmpty ()){
        ui_->hhComboBox_5->setEnabled(true);
        ui_->hhComboBox_5->setCurrentText (sched_hh_5_);
        if (sched_band_4_.right(4) == "JT65") {
          ui_->band_mix_check_box_4->setEnabled(true);
          ui_->band_mix_check_box_4->setChecked (sched_mix_4_);
        } else {
          ui_->band_mix_check_box_4->setEnabled(false);
          ui_->band_mix_check_box_4->setChecked (false);
        }
      } else {
        ui_->hhComboBox_5->setEnabled(false);
        ui_->hhComboBox_5->setCurrentIndex(0);
        ui_->band_mix_check_box_4->setEnabled(false);
        ui_->band_mix_check_box_4->setChecked (false);
      }
  } 
  else {
      ui_->bandComboBox_4->setCurrentText("");
      ui_->hhComboBox_5->setEnabled(false);
      ui_->hhComboBox_5->setCurrentIndex(0);
      ui_->band_mix_check_box_4->setEnabled(false);
      ui_->band_mix_check_box_4->setChecked (false);
  }
//  printf("band4 changed %s:%s\n",text.toStdString().c_str(),ui_->bandComboBox_4->currentText().toStdString().c_str());
}

void Configuration::impl::on_hhComboBox_5_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->mmComboBox_5->setEnabled(true);
     ui_->mmComboBox_5->setCurrentText (sched_mm_5_);
  } else {
     ui_->mmComboBox_5->setEnabled(false);
     ui_->mmComboBox_5->setCurrentIndex(0);
  }
//  printf("hh5 changed %d\n",index);
}

void Configuration::impl::on_mmComboBox_5_currentIndexChanged(int index)
{
  if (index > 0) {
     ui_->bandComboBox_5->setEnabled(true);
     ui_->bandComboBox_5->setCurrentText (sched_band_5_);
  } else {
     ui_->bandComboBox_5->setEnabled(false);
     ui_->bandComboBox_5->setCurrentIndex(-1);
  }
//  printf("mm5 changed %d\n",index);
}

void Configuration::impl::on_bandComboBox_5_currentIndexChanged(QString const& text)
{
  if (text != ""){
      ui_->bandComboBox_5->setCurrentText(text);
      if (text.right(4) == "JT65") {
        ui_->band_mix_check_box_5->setEnabled(true);
        ui_->band_mix_check_box_5->setChecked (sched_mix_5_);
      } else {
        ui_->band_mix_check_box_5->setEnabled(false);
        ui_->band_mix_check_box_5->setChecked (false);
      }
  } else if (ui_->bandComboBox_5->isEnabled()) {
      ui_->bandComboBox_5->setCurrentText(sched_band_5_);
      if (!sched_band_5_.isEmpty ()){
        if (sched_band_5_.right(4) == "JT65") {
          ui_->band_mix_check_box_5->setEnabled(true);
          ui_->band_mix_check_box_5->setChecked (sched_mix_5_);
        } else {
          ui_->band_mix_check_box_5->setEnabled(false);
          ui_->band_mix_check_box_5->setChecked (false);
        }
      } else {
        ui_->band_mix_check_box_5->setEnabled(false);
        ui_->band_mix_check_box_5->setChecked (false);
      }
  } 
  else {
      ui_->bandComboBox_5->setCurrentText("");
      ui_->band_mix_check_box_5->setEnabled(false);
      ui_->band_mix_check_box_5->setChecked (false);
  }
//  printf("band5 changed %s:%s\n",text.toStdString().c_str(),ui_->bandComboBox_5->currentText().toStdString().c_str());
}

void Configuration::impl::on_add_macro_line_edit_editingFinished ()
{
  ui_->add_macro_line_edit->setText (ui_->add_macro_line_edit->text ().toUpper ());
}

void Configuration::impl::on_delete_macro_push_button_clicked (bool /* checked */)
{
  auto selection_model = ui_->macros_list_view->selectionModel ();
  if (selection_model->hasSelection ())
    {
      // delete all selected items
      delete_selected_macros (selection_model->selectedRows ());
    }
}

void Configuration::impl::delete_macro ()
{
  auto selection_model = ui_->macros_list_view->selectionModel ();
  if (!selection_model->hasSelection ())
    {
      // delete item under cursor if any
      auto index = selection_model->currentIndex ();
      if (index.isValid ())
        {
          next_macros_.removeRow (index.row ());
        }
    }
  else
    {
      // delete the whole selection
      delete_selected_macros (selection_model->selectedRows ());
    }
}

void Configuration::impl::delete_selected_macros (QModelIndexList selected_rows)
{
  // sort in reverse row order so that we can delete without changing
  // indices underneath us
  qSort (selected_rows.begin (), selected_rows.end (), [] (QModelIndex const& lhs, QModelIndex const& rhs)
         {
           return rhs.row () < lhs.row (); // reverse row ordering
         });

  // now delete them
  Q_FOREACH (auto index, selected_rows)
    {
      next_macros_.removeRow (index.row ());
    }
}

void Configuration::impl::on_add_macro_push_button_clicked (bool /* checked */)
{
  if (next_macros_.insertRow (next_macros_.rowCount ()))
    {
      auto index = next_macros_.index (next_macros_.rowCount () - 1);
      ui_->macros_list_view->setCurrentIndex (index);
      next_macros_.setData (index, ui_->add_macro_line_edit->text ());
      ui_->add_macro_line_edit->clear ();
    }
}

void Configuration::impl::delete_frequencies ()
{
  auto selection_model = ui_->frequencies_table_view->selectionModel ();
  selection_model->select (selection_model->selection (), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
  next_frequencies_.removeDisjointRows (selection_model->selectedRows ());
  ui_->frequencies_table_view->resizeColumnToContents (FrequencyList::mode_column);
}

void Configuration::impl::on_reset_frequencies_push_button_clicked (bool /* checked */)
{
  if (QMessageBox::Yes == QMessageBox::question (this, tr ("Reset Working Frequencies")
                                                 , tr ("Are you sure you want to discard your current "
                                                       "working frequencies and replace them with default "
                                                       "ones?")))
    {
      next_frequencies_.reset_to_defaults ();
    }
}

void Configuration::impl::insert_frequency ()
{
  if (QDialog::Accepted == frequency_dialog_->exec ())
    {
      ui_->frequencies_table_view->setCurrentIndex (next_frequencies_.add (frequency_dialog_->item ()));
      ui_->frequencies_table_view->resizeColumnToContents (FrequencyList::mode_column);
    }
}

void Configuration::impl::delete_stations ()
{
  auto selection_model = ui_->stations_table_view->selectionModel ();
  selection_model->select (selection_model->selection (), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
  next_stations_.removeDisjointRows (selection_model->selectedRows ());
  ui_->stations_table_view->resizeColumnToContents (StationList::band_column);
  ui_->stations_table_view->resizeColumnToContents (StationList::offset_column);
}

void Configuration::impl::insert_station ()
{
  if (QDialog::Accepted == station_dialog_->exec ())
    {
      ui_->stations_table_view->setCurrentIndex (next_stations_.add (station_dialog_->station ()));
      ui_->stations_table_view->resizeColumnToContents (StationList::band_column);
      ui_->stations_table_view->resizeColumnToContents (StationList::offset_column);
    }
}

void Configuration::impl::on_save_path_select_push_button_clicked (bool /* checked */)
{
  QFileDialog fd {this, tr ("Save Directory"), ui_->save_path_display_label->text ()};
  fd.setFileMode (QFileDialog::Directory);
  fd.setOption (QFileDialog::ShowDirsOnly);
  if (fd.exec ())
    {
      if (fd.selectedFiles ().size ())
        {
          ui_->save_path_display_label->setText (fd.selectedFiles ().at (0));
        }
    }
}

void Configuration::impl::on_azel_path_select_push_button_clicked (bool /* checked */)
{
  QFileDialog fd {this, tr ("AzEl Directory"), ui_->azel_path_display_label->text ()};
  fd.setFileMode (QFileDialog::Directory);
  fd.setOption (QFileDialog::ShowDirsOnly);
  if (fd.exec ()) {
    if (fd.selectedFiles ().size ()) {
      ui_->azel_path_display_label->setText(fd.selectedFiles().at(0));
    }
  }
}

bool Configuration::impl::have_rig ()
{
  if (!open_rig ())
    {
      QMessageBox::critical (this, "JTDX", tr ("Failed to open connection to rig"));
    }
  return rig_active_;
}

bool Configuration::impl::open_rig (bool force)
{
  auto result = false;

  auto const rig_data = gather_rig_data ();
  if (force || !rig_active_ || rig_data != saved_rig_params_)
    {
      try
        {
          close_rig ();

          // create a new Transceiver object
          auto rig = transceiver_factory_.create (rig_data, transceiver_thread_);
          cached_rig_state_ = Transceiver::TransceiverState {};

          // hook up Configuration transceiver control signals to Transceiver slots
          //
          // these connections cross the thread boundary
          rig_connections_ << connect (this, &Configuration::impl::set_transceiver,
                                       rig.get (), &Transceiver::set);

          // hook up Transceiver signals to Configuration signals
          //
          // these connections cross the thread boundary
          rig_connections_ << connect (rig.get (), &Transceiver::resolution, this, [=] (int resolution) {
              rig_resolution_ = resolution;
            });
          rig_connections_ << connect (rig.get (), &Transceiver::update, this, &Configuration::impl::handle_transceiver_update);
          rig_connections_ << connect (rig.get (), &Transceiver::failure, this, &Configuration::impl::handle_transceiver_failure);

          // setup thread safe startup and close down semantics
          rig_connections_ << connect (this, &Configuration::impl::start_transceiver, rig.get (), &Transceiver::start);
          rig_connections_ << connect (this, &Configuration::impl::stop_transceiver, rig.get (), &Transceiver::stop);

          auto p = rig.release ();	// take ownership

          // schedule destruction on thread quit
          connect (transceiver_thread_, &QThread::finished, p, &QObject::deleteLater);

          // schedule eventual destruction for non-closing situations
          //
          // must   be   queued    connection   to   avoid   premature
          // self-immolation  since finished  signal  is  going to  be
          // emitted from  the object that  will get destroyed  in its
          // own  stop  slot  i.e.  a   same  thread  signal  to  slot
          // connection which by  default will be reduced  to a method
          // function call.
          connect (p, &Transceiver::finished, p, &Transceiver::deleteLater, Qt::QueuedConnection);

          ui_->test_CAT_push_button->setStyleSheet ({});
          rig_active_ = true;
          Q_EMIT start_transceiver (++transceiver_command_number_); // start rig on its thread
          result = true;
        }
      catch (std::exception const& e)
        {
          handle_transceiver_failure (e.what ());
        }

      saved_rig_params_ = rig_data;
      rig_changed_ = true;
    }
  else
    {
      result = true;
    }
  return result;
}

void Configuration::impl::transceiver_frequency (Frequency f)
{
  Transceiver::MODE mode {Transceiver::UNK};
  switch (data_mode_)
    {
    case data_mode_USB: mode = Transceiver::USB; break;
    case data_mode_data: mode = Transceiver::DIG_U; break;
    case data_mode_none: break;
    }

  cached_rig_state_.online (true); // we want the rig online
  cached_rig_state_.mode (mode);

  // apply any offset & calibration
  // we store the offset here for use in feedback from the rig, we
  // cannot absolutely determine if the offset should apply but by
  // simply picking an offset when the Rx frequency is set and
  // sticking to it we get sane behaviour
  current_offset_ = stations_.offset (f);
  cached_rig_state_.frequency (apply_calibration (f + current_offset_));

  Q_EMIT set_transceiver (cached_rig_state_, ++transceiver_command_number_);
}

void Configuration::impl::transceiver_tx_frequency (Frequency f)
{
  Q_ASSERT (!f || split_mode ());
  if (split_mode ())
    {
      Transceiver::MODE mode {Transceiver::UNK};
      switch (data_mode_)
        {
        case data_mode_USB: mode = Transceiver::USB; break;
        case data_mode_data: mode = Transceiver::DIG_U; break;
        case data_mode_none: break;
        }
      cached_rig_state_.online (true); // we want the rig online
      cached_rig_state_.mode (mode);
      cached_rig_state_.split (f);
      cached_rig_state_.tx_frequency (f);

      // lookup offset for tx and apply calibration
      if (f)
        {
          // apply and offset and calibration
          // we store the offset here for use in feedback from the
          // rig, we cannot absolutely determine if the offset should
          // apply but by simply picking an offset when the Rx
          // frequency is set and sticking to it we get sane behaviour
          current_tx_offset_ = stations_.offset (f);
          cached_rig_state_.tx_frequency (apply_calibration (f + current_tx_offset_));
        }

      Q_EMIT set_transceiver (cached_rig_state_, ++transceiver_command_number_);
    }
}

void Configuration::impl::transceiver_mode (MODE m)
{
  cached_rig_state_.online (true); // we want the rig online
  cached_rig_state_.mode (m);
  Q_EMIT set_transceiver (cached_rig_state_, ++transceiver_command_number_);
}

void Configuration::impl::transceiver_ptt (bool on)
{
  cached_rig_state_.online (true); // we want the rig online
  cached_rig_state_.ptt (on);
  Q_EMIT set_transceiver (cached_rig_state_, ++transceiver_command_number_);
}

void Configuration::impl::sync_transceiver (bool /*force_signal*/)
{
  // pass this on as cache must be ignored
  // Q_EMIT sync (force_signal);
}

void Configuration::impl::handle_transceiver_update (TransceiverState const& state,
                                                     unsigned sequence_number)
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::handle_transceiver_update: Transceiver State #:" << sequence_number << state;
#endif

  // only follow rig on some information, ignore other stuff
  cached_rig_state_.online (state.online ());
  cached_rig_state_.frequency (state.frequency ());
  cached_rig_state_.split (state.split ());

  if (state.online ())
    {
      ui_->test_PTT_push_button->setChecked (state.ptt ());

      if (isVisible ())
        {
          ui_->test_CAT_push_button->setStyleSheet ("QPushButton {background-color: green;}");

          auto const& rig = ui_->rig_combo_box->currentText ();
          auto ptt_method = static_cast<TransceiverFactory::PTTMethod> (ui_->PTT_method_button_group->checkedId ());
          auto CAT_PTT_enabled = transceiver_factory_.has_CAT_PTT (rig);
          ui_->test_PTT_push_button->setEnabled ((TransceiverFactory::PTT_method_CAT == ptt_method && CAT_PTT_enabled)
                                                 || TransceiverFactory::PTT_method_DTR == ptt_method
                                                 || TransceiverFactory::PTT_method_RTS == ptt_method);
        }
    }
  else
    {
      close_rig ();
    }

  // pass on to clients if current command is processed
  if (sequence_number == transceiver_command_number_)
    {
      TransceiverState reported_state {state};
      // take off calibration & offset
      reported_state.frequency (remove_calibration (reported_state.frequency ()) - current_offset_);

      if (reported_state.tx_frequency ())
        {
          // take off calibration & offset
          reported_state.tx_frequency (remove_calibration (reported_state.tx_frequency ()) - current_tx_offset_);
        }

      Q_EMIT self_->transceiver_update (reported_state);
    }
}

void Configuration::impl::handle_transceiver_failure (QString const& reason)
{
#if WSJT_TRACE_CAT
  qDebug () << "Configuration::handle_transceiver_failure: reason:" << reason;
#endif

  close_rig ();
  ui_->test_PTT_push_button->setChecked (false);

  if (isVisible ())
    {
      message_box (tr ("Rig failure"), reason);
    }
  else
    {
      // pass on if our dialog isn't active
      Q_EMIT self_->transceiver_failure (reason);
    }
}

void Configuration::impl::close_rig ()
{
  ui_->test_PTT_push_button->setEnabled (false);

  // revert to no rig configured
  if (rig_active_)
    {
      ui_->test_CAT_push_button->setStyleSheet ("QPushButton {background-color: red;}");
      Q_EMIT stop_transceiver ();
      for (auto const& connection: rig_connections_)
        {
          disconnect (connection);
        }
      rig_connections_.clear ();
      rig_active_ = false;
    }
}

// load the available audio devices into the selection combo box and
// select the default device if the current device isn't set or isn't
// available
bool Configuration::impl::load_audio_devices (QAudio::Mode mode, QComboBox * combo_box, QAudioDeviceInfo * device)
{
  using std::copy;
  using std::back_inserter;

  bool result {false};

  combo_box->clear ();

  int current_index = -1;
  int default_index = -1;

  int extra_items {0};

  auto const& default_device = (mode == QAudio::AudioInput ? QAudioDeviceInfo::defaultInputDevice () : QAudioDeviceInfo::defaultOutputDevice ());

  // deal with special default audio devices on Windows
  if ("Default Input Device" == default_device.deviceName ()
      || "Default Output Device" == default_device.deviceName ())
    {
      default_index = 0;

      QList<QVariant> channel_counts;
      auto scc = default_device.supportedChannelCounts ();
      copy (scc.cbegin (), scc.cend (), back_inserter (channel_counts));

      combo_box->addItem (default_device.deviceName (), channel_counts);
      ++extra_items;
      if (default_device == *device)
        {
          current_index = 0;
          result = true;
        }
    }

  Q_FOREACH (auto const& p, QAudioDeviceInfo::availableDevices (mode))
    {
      // convert supported channel counts into something we can store in the item model
      QList<QVariant> channel_counts;
      auto scc = p.supportedChannelCounts ();
      copy (scc.cbegin (), scc.cend (), back_inserter (channel_counts));

      combo_box->addItem (p.deviceName (), channel_counts);
      if (p == *device)
        {
          current_index = combo_box->count () - 1;
        }
      else if (p == default_device)
        {
          default_index = combo_box->count () - 1;
        }
    }
  if (current_index < 0)	// not found - use default
    {
      *device = default_device;
      result = true;
      current_index = default_index;
    }
  combo_box->setCurrentIndex (current_index);

  return result;
}

// enable only the channels that are supported by the selected audio device
void Configuration::impl::update_audio_channels (QComboBox const * source_combo_box, int index, QComboBox * combo_box, bool allow_both)
{
  // disable all items
  for (int i (0); i < combo_box->count (); ++i)
    {
      combo_box->setItemData (i, combo_box_item_disabled, Qt::UserRole - 1);
    }

  Q_FOREACH (QVariant const& v, source_combo_box->itemData (index).toList ())
    {
      // enable valid options
      int n {v.toInt ()};
      if (2 == n)
        {
          combo_box->setItemData (AudioDevice::Left, combo_box_item_enabled, Qt::UserRole - 1);
          combo_box->setItemData (AudioDevice::Right, combo_box_item_enabled, Qt::UserRole - 1);
          if (allow_both)
            {
              combo_box->setItemData (AudioDevice::Both, combo_box_item_enabled, Qt::UserRole - 1);
            }
        }
      else if (1 == n)
        {
          combo_box->setItemData (AudioDevice::Mono, combo_box_item_enabled, Qt::UserRole - 1);
        }
    }
}

void Configuration::impl::set_application_font (QFont const& font)
{
  qApp->setStyleSheet (qApp->styleSheet () + "* {" + font_as_stylesheet (font) + '}');
  for (auto& widget : qApp->topLevelWidgets ())
    {
      widget->updateGeometry ();
    }
}

// load all the supported rig names into the selection combo box
void Configuration::impl::enumerate_rigs ()
{
  ui_->rig_combo_box->clear ();

  auto rigs = transceiver_factory_.supported_transceivers ();

  for (auto r = rigs.cbegin (); r != rigs.cend (); ++r)
    {
      if ("None" == r.key ())
        {
          // put None first
          ui_->rig_combo_box->insertItem (0, r.key (), r.value ().model_number_);
        }
      else
        {
          ui_->rig_combo_box->addItem (r.key (), r.value ().model_number_);
        }
    }

  ui_->rig_combo_box->setCurrentText (rig_params_.rig_name);
}

void Configuration::impl::fill_port_combo_box (QComboBox * cb)
{
  auto current_text = cb->currentText ();
  cb->clear ();
  Q_FOREACH (auto const& p, QSerialPortInfo::availablePorts ())
    {
      if (!p.portName ().contains ( "NULL" )) // virtual serial port pairs
        {
          // remove possibly confusing Windows device path (OK because
          // it gets added back by Hamlib)
          cb->addItem (p.systemLocation ().remove (QRegularExpression {R"(^\\\\\.\\)"}));
        }
    }
  cb->addItem("USB");
  cb->setEditText (current_text);
}

auto Configuration::impl::apply_calibration (Frequency f) const -> Frequency
{
  return std::llround (frequency_calibration_intercept_
                       + (1. + frequency_calibration_slope_ppm_ / 1.e6) * f);
}

auto Configuration::impl::remove_calibration (Frequency f) const -> Frequency
{
  return std::llround ((f - frequency_calibration_intercept_)
                       / (1. + frequency_calibration_slope_ppm_ / 1.e6));
}

#if !defined (QT_NO_DEBUG_STREAM)
ENUM_QDEBUG_OPS_IMPL (Configuration, DataMode);
ENUM_QDEBUG_OPS_IMPL (Configuration, Type2MsgGen);
#endif

ENUM_QDATASTREAM_OPS_IMPL (Configuration, DataMode);
ENUM_QDATASTREAM_OPS_IMPL (Configuration, Type2MsgGen);

ENUM_CONVERSION_OPS_IMPL (Configuration, DataMode);
ENUM_CONVERSION_OPS_IMPL (Configuration, Type2MsgGen);
