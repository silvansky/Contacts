/*
* codec.h
*
* RFC 1890 enums
* 
*/

#ifndef _CODEC_H_
#define _CODEC_H_

#include "voipmedia_global.h"

enum VOIPMEDIA_EXPORT codecNum
{
  PCMU            =0 , 
  _1016           =1 , 
  G721            =2 , 
  GSM             =3 , 
  unassigned_4    =4 , 
  DVI4_8          =5 , 
  DVI4_16         =6 , 
  LPC             =7 , 
  PCMA            =8 , 
  G722            =9 , 
  L16_2c          =10, 
  L16_1c          =11, 
  unassigned_12   =12, 
  unassigned_13   =13, 
  MPA             =14, 
  G728            =15, 
  unassigned_16_24=16,
  CelB            =25, 
  JPEG            =26, 
  unassigned_27   =27, 
  nv              =28, 
  unassigned_29   =29, 
  unassigned_30   =30, 
  H261            =31, 
  MPV             =32, 
  MP2T            =33, 
  unassigned_34_71=34, 
  reserved_72_76  =72, 
  unassigned_77_95=77,
  dynamic_96_127  =96,
  Speex           =97, // ПОПОВ Добавил speex и iLBC
  iLBC            =98
};




#endif // _CODEC_H_
