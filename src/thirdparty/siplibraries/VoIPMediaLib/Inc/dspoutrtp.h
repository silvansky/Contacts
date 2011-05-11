#ifndef DSPOUTRTP_H_INCLUDED
#define DSPOUTRTP_H_INCLUDED

#include "voipmedia_global.h"

#include <qtimer.h>
#include <QFile>

//#include "../gsm/gsm.h"
//#include "../gsm/private.h"
#include "ilbc/iLBC_define.h"
#include "ilbc/iLBC_encode.h"
#include "ilbc/iLBC_decode.h"

#include <speex/speex.h>
#include <speex/speex_preprocess.h>

//#include <speex/speex_echo.h>

#include "udpmessagesocket.h"
#include "sdp.h"
#include "dspout.h"

//#include "callaudio.h"
#include "speex_jitter_buffer.h"


#include "../config.h"
#ifdef SRTP
#include "../srtp/SRTPWrapper.h"
#endif

#define GSM_ENC_BYTES     33
#define GSM_DEC_SAMPLES   160

#define ILBCNOOFWORDS   (NO_OF_BYTES/2)
#define TIME_PER_FRAME  30

struct SpeexJitter;
/**
* @short RTP implementation of DspOut.
* @author Billy Biggs <vektor@div8.net>
*
* This is the RTP implementation of DspOut.
*/
class VOIPMEDIA_EXPORT DspOutRtp : public DspOut
{
public:
  /**
  * Constructs a DspOutRtp object outputting to the given
  * hostname.
  */
  DspOutRtp( const codecType newCodec, int newCodecNum,
    const QString &hostName = QString::null, UDPMessageSocket *s = NULL, int minPort = 0, int maxPort = 0 );

  /**
  * Destructor.  Will close the device if it is open.
  */
  virtual ~DspOutRtp( void );

  bool openDevice( DeviceMode mode );
  bool writeBuffer( void );
  bool setPortNum( int newport );
  int getPortNum( void ) const { return portnum; }
  int getVideoPortNum( void ) const { return videoPortnum; }
  unsigned int readableBytes( void );
  bool readBuffer( int bytes = 0 );
  void setPayload( int payload ) { fixedrtplen = (size_t)payload; }
  bool sendStunRequest( UDPMessageSocket *socket );
  unsigned int receiveStunResponse( UDPMessageSocket *socket );
  void setStunSrv( QString newStunSrv );

  void setCodec( const codecType newCodec, int newCodecNum );
  codecType getCodec() const { return _codec; }

  // Костыль jitter-buffer. Вызывается извне при каждом тике таймера
  bool getFromJitterBuffer( QByteArray& data );

private:
  //int writeGSMBuffer( gsm Gsm_Inst,
  //  unsigned char *input_buf, unsigned char *output_buf,
  //  unsigned char *tmp_buf, unsigned char *queue,
  //  int *qlen, int size );
  //int readGSMBuffer( gsm Gsm_Inst,
  //  unsigned char *input_buf, unsigned char *output_buf,
  //  int ignore );

  int writeILBCBuffer_20( iLBC_Enc_Inst_t *Enc_Inst,
    unsigned char *input_buf, unsigned char *output_buf,
    unsigned char *tmp_buf, unsigned char *queue,
    int *qlen, int size );

  int readILBCBuffer_20( iLBC_Dec_Inst_t *Dec_Inst,
    unsigned char *input_buf, unsigned char *output_buf,
    short mode, int ignore);

  int writeILBCBuffer_30( iLBC_Enc_Inst_t *Enc_Inst,
    unsigned char *input_buf, unsigned char *output_buf,
    unsigned char *tmp_buf, unsigned char *queue,
    int *qlen, int size );

  int readILBCBuffer_30( iLBC_Dec_Inst_t *Dec_Inst,
    unsigned char *input_buf, unsigned char *output_buf,
    short mode, int ignore);

  UDPMessageSocket *socket;      // UDP Socket
  int minPort, maxPort;
  int portnum;
  int videoPortnum;
  int output_fd;                // The fd of the audio output
  unsigned char *packetbuf;     // Buffer for the packet data
  double lasttime;
  short curseq;
  unsigned char *bufunsend;

  unsigned char *inbuf;
  unsigned char *outbuf;
  unsigned char *tmpbuf;
  unsigned char *quebuf;

  //codec
  codecType _codec;
  int _codecNum;
  //gsm gsmInstEnc;
  //gsm gsmInstDec;
  int qlen;
  iLBC_Enc_Inst_t ilbcEncInst_20;
  iLBC_Dec_Inst_t ilbcDecInst_20;
  iLBC_Enc_Inst_t ilbcEncInst_30;
  iLBC_Dec_Inst_t ilbcDecInst_30;

  SpeexBits speexBits;
  //SpeexEchoState *echoState;
  SpeexPreprocessState *st;
  void *speex_enc_state;
  void *speex_dec_state;

  void *_speexJitter;
  int _speexFrameSize;
  SpeexJitter jitter;


  size_t numunsend;
  size_t fixedrtplen;
  unsigned long deb_frag;
  unsigned long deb_rtp;
  int ts;
  int ssrc;
  int ref_sec;
  int ref_usec;
  int dsize;
  bool useStun;
  QString stunSrv;
  bool destroySocket;


  QFile tempFileToRtp;


#ifdef SRTP
  SRTPWrapper* wrapper;
  bool isSRTP();
  void setSRTP(bool use);
#endif
};

#endif  // DSPOUTRTP_H_INCLUDED
