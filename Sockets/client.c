#include "hsl_server.h"
#include <sys/un.h>

#define HAL_SOCK_PROTO_NSM_ASYNC 100;
int
client()
{
  pal_sock_handle_t sock;
  struct sockaddr_un addr;
  int len;
  struct preg_msg preg;

  sock = socket (AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0);
  if (sock < 0)
    return -1;

  /* Prepare accept socket. */
  memset (&addr, 0, sizeof (struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy (addr.sun_path, HSL_ASYNC_CMD_PATH, strlen (HSL_ASYNC_CMD_PATH));
  addr.sun_path[strlen(HSL_ASYNC_CMD_PATH)] = '\0';
#ifdef HAVE_SUN_LEN
  len = addr.sun_len = SUN_LEN (&addr);
#else
  len = sizeof (addr.sun_family) + strlen (addr.sun_path);
#endif /* HAVE_SUN_LEN */

  /* Bind socket. */
  if (connect (sock, (struct sockaddr *) &addr, len) < 0)
    {
      //zlog_warn (mh->zg, "bind socket error: %s", pal_strerror (errno));
      close (sock);
      return -1;
    }

  preg.len  = MESSAGE_REGMSG_SIZE;
  preg.value = HAL_SOCK_PROTO_NSM_ASYNC;
  write(sock, &preg, MESSAGE_REGMSG_SIZE);

  char buf[100]= {0};

  while(read(0, buf, sizeof(buf)))
    {
      if (strncmp(buf, "end", 3) == 0)
        {
          /* close the connection */
          close(sock);
          return 0;
        }

      write(sock, &buf, sizeof(buf));
      memset(buf, 0, sizeof(buf));
    }

  return 0;
}

int 
main(int argc, char **argv)
{
  client();
  return 0;
}
