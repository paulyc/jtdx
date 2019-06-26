// This source code file was last time modified by Igor UA3DJY on August 21st, 2017
// All changes are shown in the patch file coming together with the full JTDX source code.

#ifndef COMMONS_H
#define COMMONS_H

#define NSMAX 6827
#define NTMAX 120
#define RX_SAMPLE_RATE 12000

#ifdef __cplusplus
#include <cstdbool>
extern "C" {
#else
#include <stdbool.h>
#endif

  /*
   * This structure is shared with Fortran code, it MUST be kept in
   * sync with lib/jt9com.f90
   * int nutc;                   //UTC as integer, HHMM
   * int ntrperiod;              //TR period (seconds)
   * int nfqso;                  //User-selected QSO freq (kHz)
   * int npts8;                  //npts for c0() array
   * int nfa;                    //Low decode limit (Hz)
   * int nfSplit;                //JT65 | JT9 split frequency
   * int nfb;                    //High decode limit (Hz)
   * int ntol;                   //+/- decoding range around fQSO (Hz)
   * bool ndiskdat;              //true ==> data read from *.wav file
   * bool newdat;                //true ==> new data, must do long FFT
   */
extern struct dec_data {
  float ss[184*NSMAX];
  float savg[NSMAX];
  short int d2[NTMAX*RX_SAMPLE_RATE];
  float dd2[NTMAX*RX_SAMPLE_RATE];
  struct
  {
    char datetime[20];
    char mycall[12];
    char hiscall[12];
    char mygrid[6];
    char hisgrid[6];
    float emedelay;
    float dttol;
    int listutc[10];	
    int nutc;
    int ntrperiod;
    int nfqso;
    int npts8;
    int nfa;
    int nfSplit;
    int nfb;
    int ntol;
    int kin;
    int nzhsym;
    int ndepth;
    int ntxmode;
    int nmode;
    int minw;
    int nclearave;
    int minSync;
    int nlist;
    int nranera;
    int ntrials10;
    int ntrialsrxf10;
    int naggressive;
    int nharmonicsdepth;
    int ntopfreq65;
    int nprepass;
    int nsdecatt;
    int nlasttx;
    int ndelay;
    bool ndiskdat;
    bool newdat;
    bool nagain;
    bool nagainfil; 
    bool nswl;
    bool nfilter;
    bool nstophint;
    bool nagcc;
    bool nhint;
    bool fmaskact;
    bool showharmonics;
    } params;
} dec_data;

extern struct {
  float syellow[NSMAX];
} jt9w_;

extern struct {
  int   nclearave;
  int   nsum;
  float blue[4096];
  float red[4096];
} echocom_;

#ifdef __cplusplus
}
#endif

#endif // COMMONS_H
