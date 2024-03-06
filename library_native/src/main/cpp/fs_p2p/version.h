#ifndef VERSION_H
#define VERSION_H

//#define MAJOR 1
//#define MINOR 1
//#define PATCH 1

#define VERSION_MAJOR(V) ((V & 0xff0000) >> 16)
#define VERSION_MINOR(V) ((V & 0xff00) >> 8)
#define VERSION_PATCH(V) (V & 0xff)

#define VERSION_CHK(major, minor, patch) \
    (((major&0xff)<<16) | ((minor&0xff)<<8) | (patch&0xff))

#define VERSION VERSION_CHK(MAJOR, MINOR, PATCH)

/*! Stringify \a x. */
#define _TOSTR(x)   #x
/*! Stringify \a x, perform macro expansion. */
#define TOSTR(x)  _TOSTR(x)


/* the following are compile time version */
/* C++11 requires a space between literal and identifier */
#define VERSION_STR        TOSTR(MAJOR) "." TOSTR(MINOR) "." TOSTR(PATCH)

#endif // VERSION_H
