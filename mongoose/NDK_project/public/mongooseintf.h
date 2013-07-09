#ifndef MONGOOSEINTF_HEADER_INCLUDED
#define  MONGOOSEINTF_HEADER_INCLUDED

// #define USE_LUA // Optional - delete to miss out LUA

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int start_mongoose(char *argv[]);
int stop_mongoose();
char *sdup(const char *str);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
