#include "util.h"

// ascii digit to integer
int a2d( char ch ) {
	if( ch >= '0' && ch <= '9' ) return ch - '0';
	if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	return -1;
}

// unsigned int to ascii string
unsigned int ui2a( unsigned int num, unsigned int base, char *bf ) {
	int n = 0;
	int dgt;
	unsigned int d = 1;
	unsigned int digits = 1;
	while( (num / d) >= base ){ 
		d *= base;
		digits += 1;
	}
	while( d != 0 ) {
		dgt = num / d;
		num %= d;
		d /= base;
		if( n || dgt > 0 || d == 0 ) {
			*bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
			++n;
		}
	}
	*bf = 0;
	return digits;
}

// signed int to ascii string
unsigned int i2a( int num, char *bf ) {
	if( num < 0 ) {
		num = -num;
		*bf++ = '-';
	}
	return ui2a( num, 10, bf );
}

// memory

// define our own memset to avoid SIMD instructions emitted from the compiler
void *memset(void *s, int c, size_t n) {
  for (char* it = (char*)s; n > 0; --n) *it++ = c;
  return s;
}

// define our own memcpy to avoid SIMD instructions emitted from the compiler
void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
  char* sit = (char*)src;
  char* cdest = (char*)dest;
  for (size_t i = 0; i < n; ++i) *(cdest++) = *(sit++);
  return dest;
}


// string

void str_cpy(char* dest, char* src){
	int index = 0;
	while(src[index]!='\0'){
		*(dest++) = *(src++);
	}
}

int str_len(const char *str){
    int len = 0;
    while(*str!='\0'){
        len += 1;
        str += 1;
    }
    return len;
}

int str_equal(const char *c1, const char *c2){
    while(1){
        if(*c1=='\0' && *c2=='\0'){
            return 1;
        }else if(*c1=='\0'){
            return 0;
        }else if(*c2=='\0'){
            return 0;
        }else if(*c1!=*c2){
            return 0;
        }else{
            c1 += 1;
            c2 += 1;
        }
    }
}

unsigned int isInt(char c){
  if(c>='0' && c<='9'){
    return 1;
  }
  return 0;
}

unsigned int getArgumentTwoDigitNumber(char *src){
  if(isInt(src[0])&&(src[1]==' '||src[1]=='\0')){
    return a2d(src[0]);
  }else if(isInt(src[0])&&isInt(src[1])&&(src[2]==' '||src[2]=='\0')){
    return a2d(src[0])*10+a2d(src[1]);
  }
  //invalid value cuz only result should be 2 digits
  return 1000;
}
unsigned int getArgumentThreeDigitNumber(char *src){
  if(isInt(src[0])&&(src[1]==' '||src[1]=='\0')){
    return a2d(src[0]);
  }else if(isInt(src[0])&&isInt(src[1])&&(src[2]==' '||src[2]=='\0')){
    return a2d(src[0])*10+a2d(src[1]);
  }else if(isInt(src[0])&&isInt(src[1])&&isInt(src[2])&&(src[3]==' '||src[3]=='\0')){
    return a2d(src[0])*100+a2d(src[1])*10+a2d(src[2]);
  }
  //invalid value cuz only result should be 2 digits
  return 1000;
}