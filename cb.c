#include "cb.h"
#include "rpi.h"
#include "util.h"
#include <stdarg.h>

uint32_t incrementBufEnd(uint32_t bufEnd, uint32_t bufSize){
  if(bufEnd+1==bufSize){
    return 0;
  }
  return bufEnd+1;
}

uint32_t decrementBufEnd(uint32_t bufEnd, uint32_t bufSize){
  if(bufEnd==0){
    return bufSize-1;
  }
  return bufEnd-1;
}


uint32_t charToBuffer(size_t line, char *buf, uint32_t bufEnd, char c){
  uint32_t bufferSize;
  switch(line){
    case CONSOLE: bufferSize = OUTPUT_BUFFER_SIZE; break;
    case MARKLIN: bufferSize = TRAIN_BUFFER_SIZE; break;
    default: return 0;
  }
  buf[bufEnd] = c;
  bufEnd = incrementBufEnd(bufEnd, bufferSize);
  buf[bufEnd] = '\0';
  return bufEnd;
}

uint32_t strToBuffer(size_t line, char *buf, uint32_t bufEnd, char *str){
  uint32_t bufferSize;
  switch(line){
    case CONSOLE: bufferSize = OUTPUT_BUFFER_SIZE; break;
    case MARKLIN: bufferSize = TRAIN_BUFFER_SIZE; break;
    default: return 0;
  }
  for (int i = 0; str[i] != '\0'; ++i) {
		buf[bufEnd] = str[i];
    bufEnd = incrementBufEnd(bufEnd, bufferSize);
  }
  buf[bufEnd] = '\0';
  return bufEnd;
}

static uint32_t printfToBufferHelper (size_t line, char *buf, uint32_t bufEnd, char *fmt, va_list va ) {

  uint32_t bufferSize;
  switch(line){
    case CONSOLE: bufferSize = OUTPUT_BUFFER_SIZE; break;
    case MARKLIN: bufferSize = TRAIN_BUFFER_SIZE; break;
    default: return 0;
  }

	char bf[12];
	char ch;

  while ( ( ch = *(fmt++) ) ) {
		if ( ch != '%' ){
			buf[bufEnd] = ch;
      bufEnd = incrementBufEnd(bufEnd, bufferSize);
    }
		else {
			ch = *(fmt++);
			switch( ch ) {
			case 'u':
				ui2a( va_arg( va, unsigned int ), 10, bf );
				bufEnd = strToBuffer(line, buf, bufEnd, bf);
				break;
			case 'd':
				i2a( va_arg( va, int ), bf );
				bufEnd = strToBuffer(line, buf, bufEnd, bf);
				break;
			case 's':
			  bufEnd = strToBuffer(line, buf, bufEnd, va_arg( va, char* ));
				break;
			case '%':
			  buf[bufEnd] = ch;
        bufEnd = incrementBufEnd(bufEnd, bufferSize);
				break;
      case '\0':
        buf[bufEnd] = '\0';
        return bufEnd;
			}
		}
	}
  buf[bufEnd] = '\0';
  return bufEnd;
}

uint32_t printfToBuffer(size_t line, char *buf, uint32_t bufEnd, char *fmt, ... ){

	va_list va;
	va_start(va,fmt);
	bufEnd = printfToBufferHelper( line, buf, bufEnd, fmt, va );
	va_end(va);
  return bufEnd;
}
