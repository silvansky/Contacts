#ifndef G711_H_INCLUDED
#define G711_H_INCLUDED

unsigned char linear2ulaw( int sample );
short ulaw2linear( unsigned char ulawbyte );

unsigned char linear2pcma( int sample );
int pcma2linear( unsigned char pcmabyte );

#endif // G711_H_INCLUDED
