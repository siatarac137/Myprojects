#include"network.h"
#include <errno.h>

/* Maximun no. of message send retry in case of socket send buffer full
   In this way this will wait till 25 microseconds.
*/
#define MAX_SOCK_MSG_SEND_RETRY 25

/* Read nbytes from fd and store into ptr. */
s_int32_t
readn (pal_sock_handle_t fd, u_char *ptr, s_int32_t nbytes)
{
  s_int32_t nleft;
  s_int32_t nread;

  nleft = nbytes;

  while (nleft > 0) 
    {
      nread = read (fd, ptr, nleft);

      if (nread < 0)
        {
          switch (errno)
            {
            case EINTR:
            case EAGAIN:
            case EINPROGRESS:
#ifndef PNE_VERSION_6_9
#if (EWOULDBLOCK != EAGAIN)
            case EWOULDBLOCK:
#endif /* (EWOULDBLOCK != EAGAIN) */
#endif  
              usleep (1);
              continue;
            }

          return (nread);
        }
      else
        if (nread == 0) 
          break;

      nleft -= nread;
      ptr += nread;
    }

  return nbytes - nleft;
}  

/* Write nbytes from ptr to fd. */
s_int32_t
writen (pal_sock_handle_t fd, u_char *ptr, s_int32_t nbytes)
{
  s_int32_t nleft;
  s_int32_t nwritten;

  nleft = nbytes;

  while (nleft > 0) 
    {
      nwritten = write (fd, ptr, nleft);
      
      if (nwritten <= 0) 
        {
          /* Signal happened before we could write */
          switch (errno)
            {
            case EINTR:
            case EAGAIN:
            case EINPROGRESS:
#if (EWOULDBLOCK != EAGAIN)
            case EWOULDBLOCK:
#endif /* (EWOULDBLOCK != EAGAIN) */
              usleep (1);
              continue;
            case EPIPE:
            case EBADF:
              return errno;
            }
          
          return (nwritten);
        }
      nleft -= nwritten;
      ptr += nwritten;
    }

  return nbytes - nleft;
}

/* Read nbytes from fd and store into ptr. Return when NO DATA. */
s_int32_t
readn2 (pal_sock_handle_t fd, u_char *ptr, s_int32_t nbytes)
{
  s_int32_t nleft;
  s_int32_t nread;

  nleft = nbytes;

  while (nleft > 0)
    {
      nread = read (fd, ptr, nleft);

      if (nread < 0)
        {
          switch (errno)
            {
            case EINTR:
            case EINPROGRESS:
              usleep (1);
              continue;
            case EAGAIN:
#if (EWOULDBLOCK != EAGAIN)
            case EWOULDBLOCK:
#endif /* (EWOULDBLOCK != EAGAIN) */
              return -1;
            }

          return -1;
        }
      else
        if (nread == 0)
          break;

      nleft -= nread;
      ptr += nread;
    }

  return nbytes - nleft;
}


/* Write nbytes from ptr to fd. */
s_int32_t
writen2 (pal_sock_handle_t fd, u_char *ptr, s_int32_t nbytes, s_int32_t *written)
{
  s_int32_t nleft;
  s_int32_t nwritten;

  *written = 0;
  nleft = nbytes;

  while (nleft > 0)
    {
      nwritten = write (fd, ptr, nleft);

      if (nwritten <= 0)
        {
          /* Signal happened before we could write */
          switch (errno)
            {
            case EINTR:
            case EINPROGRESS:
              usleep (1);
              continue;
            case EAGAIN:
#if (EWOULDBLOCK != EAGAIN)
            case EWOULDBLOCK:
#endif /* (EWOULDBLOCK != EAGAIN) */
              return -1;
            }

          return (nwritten);
        }

      *written += nwritten;
      nleft -= nwritten;
      ptr += nwritten;
    }

  return nbytes - nleft;
}

/*--------------------------------------------------------------
Function Name : writen_with_priority
Description   : API to write input data to input socket.
                This API might drop message in case low piroity
Input         : pal_sock_handle_t fd
                u_char *ptr
                s_int32_t nbytes
                bool is_low_priority
Output        : NoOfBytesWritten/Error(<0)
----------------------------------------------------------------*/
s_int32_t
writen_with_priority (pal_sock_handle_t fd, u_char *ptr, s_int32_t nbytes,
                      bool is_low_priority)
{
  s_int32_t nleft;
  s_int32_t nwritten;
  bool part_sent = 0;
  u_int32_t retry_cnt = 0;

  nleft = nbytes;

  while (nleft > 0)
    {
      nwritten = write (fd, ptr, nleft);

      if (nwritten <= 0)
        {
          /* Signal happened before we could write */
          switch (errno)
            {
            case EINTR:
            case EAGAIN:
            case EINPROGRESS:
#if (EWOULDBLOCK != EAGAIN)
            case EWOULDBLOCK:
#endif /* (EWOULDBLOCK != EAGAIN) */

              /*For low priority message, If max no of send retry reached,
                drop the message*/
              if (is_low_priority && !part_sent
                  && (retry_cnt >= MAX_SOCK_MSG_SEND_RETRY))
                {
                  return -1;
                }

              /*This will call usleep (1) i.e. wait for 1 microsecond*/
              usleep (1);
              retry_cnt++;
              continue;
            case EPIPE:
            case EBADF:
              return errno;
            }

          return (nwritten);
        }
      nleft -= nwritten;
      ptr += nwritten;
      part_sent = 1;
      retry_cnt = 0;
    }

  return nbytes - nleft;
}

