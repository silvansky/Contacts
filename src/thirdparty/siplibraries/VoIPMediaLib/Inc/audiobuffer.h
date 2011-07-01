#ifndef AUDIOBUFFER_H_INCLUDED
#define AUDIOBUFFER_H_INCLUDED

#include "voipmedia_global.h"

#include <stdlib.h>

#include <QBuffer.h>

/**
* Small class for passing around buffers of audio data. Does not
* specify a format, so the responsibility is on the programmer to
* know what the datatype of the buffer really is.
*/
class VOIPMEDIA_EXPORT AudioBuffer
{
public:
  static int maxBuffSize;

public:
  /**
  * Creates an audio buffer of @param length bytes.
  */
  AudioBuffer( size_t length = 4096 );

  /**
  * Deletes the audio buffer, freeing the data.
  */
  ~AudioBuffer( void );

  /**
  * Returns a pointer to the audio data.
  */
  void *getData( void ) const { return data; }

  /**
  * Returns the size of the buffer.
  */
  size_t getSize( void ) const { return size; }

  /**
  * Resizes the buffer to size newlength. Will only allocate new memory
  * if the size is larger than what has been previously allocated.
  */
  void resize( size_t newsize );

private:
  void *data;
  size_t size;
  size_t realsize;
};


class AudioBuffer1 : QBuffer
{
public:
  /**
  * Creates an audio buffer of @param length bytes.
  */
  AudioBuffer1( size_t length = 4096 );

  /**
  * Deletes the audio buffer, freeing the data.
  */
  ~AudioBuffer1( void );

  /**
  * Returns a pointer to the audio data.
  */
  void *getData( void ) const { return data; }

  /**
  * Returns the size of the buffer.
  */
  size_t getSize( void ) const { return size; }

  /**
  * Resizes the buffer to size newlength. Will only allocate new memory
  * if the size is larger than what has been previously allocated.
  */
  void resize( size_t newsize );

private:
  void *data;
  size_t size;
  size_t realsize;
};

#endif // AUDIOBUFFER_H_INCLUDED
