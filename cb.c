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


uint32_t charToBuffer(char *buf, uint32_t bufEnd, uint32_t bufferSize, char c){
  buf[bufEnd] = c;
  bufEnd = incrementBufEnd(bufEnd, bufferSize);
  buf[bufEnd] = '\0';
  return bufEnd;
}

uint32_t strToBuffer(char *buf, uint32_t bufEnd, uint32_t bufferSize, char *str){
  for (int i = 0; str[i] != '\0'; ++i) {
		buf[bufEnd] = str[i];
    bufEnd = incrementBufEnd(bufEnd, bufferSize);
  }
  buf[bufEnd] = '\0';
  return bufEnd;
}

static uint32_t printfToBufferHelper (char *buf, uint32_t bufEnd, uint32_t bufferSize, char *fmt, va_list va ) {


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
				bufEnd = strToBuffer(buf, bufEnd, bufferSize, bf);
				break;
			case 'd':
				i2a( va_arg( va, int ), bf );
				bufEnd = strToBuffer(buf, bufEnd, bufferSize, bf);
				break;
			case 's':
			  bufEnd = strToBuffer(buf, bufEnd, bufferSize, va_arg( va, char* ));
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

uint32_t printfToBuffer(char *buf, uint32_t bufEnd, uint32_t bufferSize, char *fmt, ... ){

	va_list va;
	va_start(va,fmt);
	bufEnd = printfToBufferHelper( buf, bufEnd, bufferSize, fmt, va );
	va_end(va);
  return bufEnd;
}
