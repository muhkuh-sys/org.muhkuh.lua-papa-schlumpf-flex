
#ifndef __defines__
#define __defines__

/*
************************************************************
*   Macros Definitions
************************************************************
*/

typedef unsigned char  UINT8;
typedef signed   char  INT8;
typedef unsigned short UINT16;
typedef unsigned short WORD;
typedef signed   short INT16;
typedef signed   short SHORT;
typedef signed   long  LONG;
typedef signed   int   INT;
typedef unsigned int   UINT;
typedef unsigned long  DWORD;
typedef unsigned long  ULONG;
typedef unsigned long  UINT32;
typedef signed   long  INT32;
typedef unsigned int   BOOLEAN;

#ifndef FALSE
        #define FALSE (0)
#endif
  
#ifndef TRUE
        #define TRUE  (1)
#endif
  
#ifndef NULL
        #define NULL  0
#endif  


#define ARRAYSIZE(a)  (sizeof(a) / sizeof(a[0]))
#define POKE(addr, val) (*(volatile unsigned int *)(addr) = (val))
#define POKE_AND(addr, val) (*(volatile unsigned int *)(addr) &= (val))
#define POKE_OR(addr, val) (*(volatile unsigned int *)(addr) |= (val))
#define PEEK(addr) (*(volatile unsigned int *)(addr))

#ifndef NULL
        #define NULL  0
#endif  

#endif  // __defines__

