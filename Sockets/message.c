#include<string.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<fcntl.h>
#include<sys/ioctl.h> /* ioctl */
#include <unistd.h> /* unlink */
#include"message.h"

#define SERVER_PATH "/tmp/.hsl_async_cmd"

/* Message client entry memory free.  */
void
message_entry_free (struct message_entry *me)
{
  THREAD_OFF (me->t_read);

  if (me->sock >= 0)
    close (me->sock);

  free(me);
}


/* Create message server.  */
struct message_handler *
message_server_create (struct lib_globals *zg)
{
  struct message_handler *ms;

  ms = (struct message_handler *)malloc(sizeof (struct message_handler));
  memset(ms, 0, sizeof(struct message_handler));
  ms->zg = zg;
  //ms->clist = vector_init (1);
  ms->sock = -1; /* initialize to -1, s >= 0 is valid socket value */
  //ms->remote_conn = PAL_FALSE;

  return ms;
}

/* Stop message server.  */
int
message_server_stop (struct message_handler *ms)
{
  if (ms->sock >= 0)
    {
      close (ms->sock);
      ms->sock = -1;
    }
  return 0;
}

/* Set message server type to UNIX domain.  Argument path is a path
   for UNIX domain socket server.  */
void
message_server_set_style_domain (struct message_handler *ms, char *path)
{
  message_server_stop (ms);
  ms->style = MESSAGE_STYLE_UNIX_DOMAIN;
  if (ms->path)
    free(ms->path);
  ms->path = strdup(path);
}

/* Set event callback function.  */
void
message_server_set_callback (struct message_handler *ms, int event,
                             MESSAGE_CALLBACK cb)
{
  if (event >= MESSAGE_EVENT_MAX)
    return;

  ms->callback[event] = cb;
}

/* Message client entry memory allocation.  */
struct message_entry *
message_entry_new ()
{
  struct message_entry *me;

  me = (struct message_entry *)malloc(sizeof (struct message_entry));
  memset(me, 0, sizeof(struct message_entry));

  return me;
}

/* Client is connected to server.  After accept socket, read thread is
   created to read client packet.  */
struct message_entry *
message_entry_register (struct message_handler *ms, pal_sock_handle_t sock)
{
  struct message_entry *me;

  me = message_entry_new ();
  me->sock = sock;
  me->zg = ms->zg;
  me->ms = ms;
  me->t_read = thread_add_read (ms->zg, message_server_read, me, sock);

#ifdef THREAD_AUDIT
  if(me->t_read)
    {
      me->func = me->t_read->func;
      me->t_type = me->t_read->type;
    }
#endif /*THREAD_AUDIT*/

  return me;
}


/* Message server read thread.  */
int
message_server_read (struct thread *t)
{
  struct message_entry *me;
  struct message_handler *ms;
  pal_sock_handle_t sock;
  int nbytes = 0;

  sock = THREAD_FD (t);
  me = THREAD_ARG (t);
  me->t_read = NULL;
  ms = me->ms;

  /* Call read header callback function.  */
  if (ms->callback[MESSAGE_EVENT_READ_HEADER])
    {
      nbytes = (*ms->callback[MESSAGE_EVENT_READ_HEADER]) (ms, me, sock);

      /* Disconnection event handler.  */
      if (nbytes < 0)
        {
          message_server_disconnect (ms, me, sock);
          return -1;
        }
    }

  /* Call read message body callback function.  */
  if (ms->callback[MESSAGE_EVENT_READ_MESSAGE])
    {
      nbytes = (*ms->callback[MESSAGE_EVENT_READ_MESSAGE]) (ms, me, sock);

      /* Disconnection event handler.  */
      if (nbytes < 0)
        {
          message_server_disconnect (ms, me, sock);
          return -1;
        }
    }

  /* Re-register read thread.  */
  me->t_read = thread_add_read (ms->zg, message_server_read, me, sock);

  return 0;
}


/* Accept client connection.  */
int
message_server_accept (struct thread *t)
{
  struct message_handler *ms;
  struct message_entry *me;
  pal_sock_handle_t sock;
  pal_sock_handle_t csock;
#ifdef HAVE_TCP_MESSAGE
  struct pal_sockaddr_in4 addr;
#else /* HAVE_TCP_MESSAGE */
  struct sockaddr_un addr;
#endif /* HAVE_TCP_MESSAGE */
  int len;
  int ret;

  ms = THREAD_ARG (t);
  ms->t_read = NULL;
  sock = THREAD_FD (t);
  len = sizeof (addr);

  /* Re-register accept thread.  */
  ms->t_read = thread_add_read (ms->zg, message_server_accept, ms, sock);

  /* Accept and get client socket.  */
  csock = accept (sock, (struct sockaddr *) &addr, &len);
  if (csock < 0)
    {
      //zlog_warn (ms->zg, "accept socket error: %s", pal_strerror (errno));
      return -1;
    }

  /* Make socket non-blocking.  */
//  pal_sock_set_nonblocking (csock, PAL_TRUE);
 
#if 1
  int val;
  int arg;

  /* make accpet non-bloacking */
  val = fcntl(csock, F_GETFL, 0);/* manipulate file descriptor */
  if (PAL_SOCK_ERROR != val)
    {
      arg = val | O_NONBLOCK;
      ret = fcntl(csock, F_SETFL, arg);
#if 0
      if (ret == EBADF
          || ret == EFAULT
          || ret == EINVAL
          || ret == ENOTTY
          || ret == ENOTTY)
#endif /* if 0 */
        if (ret < 0)
          {
            printf("error in ioctl %d\n", __LINE__);
            return -1;
          }
    }
#endif /* if 0 */

  /* Register client to client list.  */
  me = message_entry_register (ms, csock);

  /* Call back function. */
  if (ms->callback[MESSAGE_EVENT_CONNECT])
    {
      ret = (*ms->callback[MESSAGE_EVENT_CONNECT]) (ms, me, csock);
      if (ret < 0)
        {
          message_entry_free (me);
          return 0;
        }

      if (me->info)
        {
          //vector_set (ms->clist, me->info);
        }
    }
  else
    message_entry_free (me);

  return 0;
}

pal_sock_handle_t
message_server_socket_unix (struct message_handler *mh)
{
  pal_sock_handle_t sock;
  struct sockaddr_un addr;
  int len;

  /* Create UNIX domain socket.  */
  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    {
      //zlog_warn (mh->zg, "socket create error: %s", pal_strerror (errno));
      return -1;
    }

  /* Unlink. */
  unlink (mh->path);

  /* Prepare accept socket. */
  memset (&addr, 0, sizeof (struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy (addr.sun_path, mh->path, strlen (mh->path));
  addr.sun_path[strlen(mh->path)] = '\0';
#ifdef HAVE_SUN_LEN
  len = addr.sun_len = SUN_LEN (&addr);
#else
  len = sizeof (addr.sun_family) + strlen (addr.sun_path);
#endif /* HAVE_SUN_LEN */

  /* Bind socket. */
  if (bind (sock, (struct sockaddr *) &addr, len) < 0)
    {
      //zlog_warn (mh->zg, "bind socket error: %s", pal_strerror (errno));
      close (sock);
      return -1;
    }

  return sock;
}

/* Create server socket according to specified server type.  Then wait
   a connection from client.  */
int
message_server_start (struct message_handler *ms)
{
#ifdef HAVE_TCP_MESSAGE
  ms->sock = message_server_socket_tcp (ms);
#else /* HAVE_TCP_MESSAGE */
  if (ms->style == MESSAGE_STYLE_UNIX_DOMAIN)
    ms->sock = message_server_socket_unix (ms);
#if 0
  else if (ms->style == MESSAGE_STYLE_TCP)
    ms->sock = message_server_socket_tcp (ms);
#endif /* 0 */
  else
    return -1;
#endif /* HAVE_TCP_MESSAGE */

  if (ms->sock == -1)
    return -1;

  /* Listen to the socket.  */
  /* Increased maximum simulataneous connections to be received by accept socket */
  if (listen (ms->sock, 20) < 0)
    {
     // zlog_warn (ms->zg, "listen socket error: %s", pal_strerror (errno));
      close (ms->sock);
      ms->sock = -1;
      return -1;
    }

  /* Register read thread.  */
  ms->t_read = thread_add_read (ms->zg, message_server_accept, ms, ms->sock);

  return 0;
}


/* Message client disconnected.  */
void
message_server_disconnect (struct message_handler *ms,
                           struct message_entry *me, pal_sock_handle_t sock)
{
  /* Cancel read thread.  */
  THREAD_OFF (me->t_read);

#ifdef HAVE_CMLD
  int ret = RESULT_ERROR;
  /* For cmld (where we have multiple threads),
     need to check if we can free now or not */
  if (ms->callback[MESSAGE_EVENT_DISCONNECT_VALIDATE])
  {
    ret = (*ms->callback[MESSAGE_EVENT_DISCONNECT_VALIDATE]) (ms, me, sock);
    if (ret == -1)
    {
      /* Return for now, Will free later */
      return;
    }
  }
#endif /* HAVE_CMLD */

  /* Close socket.  */
  close (sock);
  me->sock = -1;

  /* Call callback function if registered.  */
  if (ms->callback[MESSAGE_EVENT_DISCONNECT])
    (*ms->callback[MESSAGE_EVENT_DISCONNECT]) (ms, me, sock);

  message_entry_free (me);
}

