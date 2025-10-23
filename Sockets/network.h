#include"lib.h"

s_int32_t readn (pal_sock_handle_t, u_char *, s_int32_t);
s_int32_t writen (pal_sock_handle_t, u_char *, s_int32_t);
s_int32_t readn2 (pal_sock_handle_t, u_char *, s_int32_t);
s_int32_t writen2 (pal_sock_handle_t, u_char *, s_int32_t, s_int32_t *);
s_int32_t
writen_with_priority (pal_sock_handle_t fd, u_char *ptr, s_int32_t nbytes,
                      bool is_low_priority);
