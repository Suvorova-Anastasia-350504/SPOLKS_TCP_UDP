#ifdef _WIN32
#define Socket SOCKET
#define TIME_STRUCT DWORD
#endif

#ifdef linux
#define Socket int
#define TIME_STRUCT timeval
#endif
