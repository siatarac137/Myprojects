#include "hsl_server.h"

struct hsl_server hsl_server_cmd = {NULL, "hslserver-cmd"};

struct hsl_server hsl_server_async = {NULL, "hslserver-async"};

struct hsl_server hsl_server_async_cmd = {NULL, "hslserver-async-cmd"};



/* Call back function to read message. */
int
hsl_server_read_hal_msg (struct message_handler *ms, struct message_entry *me,
    struct hsl_server_entry *hse,
    pal_sock_handle_t sock)
{
  u_char *message_buf;
  int nbytes;

  message_buf = hse->recv_msg_buf;
  nbytes = readn (sock, message_buf, 100);
  if (nbytes <= 0)
    return -1;

  //printf (" Message received from the client : %s \n", message_buf);
  write(1, message_buf, 100);
  return nbytes; 
}

void *
hsl_client_hash_entry_alloc (struct hsl_server_entry *entry)
{
  return entry;
}

struct hsl_server_entry *
hsl_server_client_create (struct hsl_server *hs, u_int16_t client_id,
    pal_sock_handle_t sock, int async)
{
  struct hsl_server_entry *hse;

  hse = (struct hsl_server_entry *)malloc(sizeof (struct hsl_server_entry));
  if (hse == NULL)
    return NULL;

  memset(hse, 0, sizeof (struct hsl_server_entry));
  if(async == 1)
    {
      hse->bulk_rsp_ptr = (struct hal_async_bulk_msg *)malloc(sizeof (struct hal_async_bulk_msg));
      if (hse->bulk_rsp_ptr == NULL)
        {
          free(hse);
          return NULL;
        }
      memset(hse->bulk_rsp_ptr, 0, sizeof (struct hal_async_bulk_msg));
    }

  hse->recv_msg_buf = (char *)malloc(MESSAGE_MAX_SIZE);
  if (hse->recv_msg_buf == NULL)
    {
      if (hse->bulk_rsp_ptr)
        free(hse->bulk_rsp_ptr);
      hse->bulk_rsp_ptr = NULL;
      free(hse);
      return NULL;
    }

  hse->sockfd = sock;
  hse->client_id = client_id;

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&hse->lock, &attr);
  /* Destroy local copy of attr */
  pthread_mutexattr_destroy(&attr);

  HSL_SERVER_WR_LOCK(hs);
  HSL_SERVER_ENTRY_LOCK(hse);
  listnode_add (hs->client, hse);
  hash_get (hs->client_sock_table, hse, hsl_client_hash_entry_alloc);
  hash_get (hs->client_id_table, hse, hsl_client_hash_entry_alloc);
  HSL_SERVER_WR_UNLOCK(hs);

  return hse;
}

u_int32_t
hsl_client_sock_hash_entry_key_make (void *entry)
{
  u_int32_t key;
  struct hsl_server_entry *hse;

  hse = (struct hsl_server_entry *) entry;

  if (! hse)
    return -1;;

  key = hse->sockfd;

  return key;
}


u_int32_t
hsl_client_id_hash_entry_key_make (void *entry)
{
  u_int32_t key;
  struct hsl_server_entry *hse;

  hse = (struct hsl_server_entry *) entry;

  if (! hse)
    return -1;

  key = hse->client_id;

  return key;
}

bool
hsl_client_sock_hash_entry_cmp (void *data1, void *data2)
{
  struct hsl_server_entry *hse1 = (struct hsl_server_entry *) data1;
  struct hsl_server_entry *hse2 = (struct hsl_server_entry *) data2;

  if (!hse1 || !hse2)
    return -1;

  return (hse1->sockfd == hse2->sockfd) ? 1 : 0;
}


bool
hsl_client_id_hash_entry_cmp (void *data1, void *data2)
{
  struct hsl_server_entry *hse1 = (struct hsl_server_entry *) data1;
  struct hsl_server_entry *hse2 = (struct hsl_server_entry *) data2;

  if (!hse1 || !hse2)
    return -1;

  return (hse1->client_id == hse2->client_id) ? 1 : 0;
}

const char *
hsl_get_client_name (enum hal_l2_proto_sock_id id)
{
  return NULL;  
}

int
hsl_get_domain_server_path (struct hsl_server *is, char **path)
{
  int ret = 0;

#if 0
  if (is == &hsl_server_cmd)
    *path = HSL_CMD_PATH;
  else if (is == &hsl_server_async)
    *path = HSL_ASYNC_PATH;
  else 
#endif /* if 0 */
    if (is == &hsl_server_async_cmd)
      *path = HSL_ASYNC_CMD_PATH;
    else
      ret = -1;

  return ret;
}

struct hsl_server_entry *
hsl_server_client_lookup_by_fd (struct hsl_server *hs, pal_sock_handle_t sock)
{
  struct hsl_server_entry *hse, key;

  if (! hs || ! hs->client_sock_table)
    return NULL;

  key.sockfd = sock;

  HSL_SERVER_RD_LOCK(hs);
  hse = hash_lookup (hs->client_sock_table, &key);
  if (hse)
    {
      HSL_SERVER_ENTRY_LOCK(hse);
      HSL_SERVER_RD_UNLOCK(hs);
      return hse;
    }
  HSL_SERVER_RD_UNLOCK(hs);

  return NULL;
}

struct hsl_server_entry *
hsl_server_client_lookup_by_id (struct hsl_server *hs, u_int16_t client_id)
{
  struct hsl_server_entry *hse, key;

  if (! hs || ! hs->client_id_table)
    return NULL;

  key.client_id = client_id;

  HSL_SERVER_RD_LOCK(hs);
  hse = hash_lookup (hs->client_id_table, &key);
  if (hse)
    {
      HSL_SERVER_ENTRY_LOCK(hse);
      HSL_SERVER_RD_UNLOCK(hs);
      return hse;
    }
  HSL_SERVER_RD_UNLOCK(hs);

  return NULL;
}

/* Call back function to read message. */
int
hsl_read (struct message_handler *ms, struct message_entry *me,
    pal_sock_handle_t sock)
{
  int nbytes;
  int async = 0;
  struct hsl_server *hs;
  struct preg_msg *preg;
  struct hsl_server_entry *hse;
  u_char message_buf[MESSAGE_REGMSG_SIZE];  /* sizeof (struct preg_msg) */

  hs = ms->info;

  hse = hsl_server_client_lookup_by_fd (hs, sock);

  /* This is the first message received from the client */
  if (hse == NULL)
    {
      nbytes = readn (sock, message_buf, MESSAGE_REGMSG_SIZE);

      if (nbytes <= 0)
        return -1;

      /*Allocate the Buulk Rsp memory only for async clients..*/
      if ((struct hsl_server *)ms->info == &hsl_server_async_cmd)
        async = 1;

      preg = (struct preg_msg *) message_buf;
      hse = hsl_server_client_lookup_by_id (hs,preg->value);

#if 1
      if(hse == NULL)
        hse = hsl_server_client_create (hs, preg->value, sock, async);
      else
        {
#if 0
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
              "Server Entry not found in client_sock_table but found in"
              " client_id_table. Disconnecting sock[%d] Client ID[%d]\n",
              sock, hse->client_id);
#endif /* if 0*/
          HSL_SERVER_ENTRY_UNLOCK(hse);
          return -1;
        }
#endif /* if 0 */
      hse->thread_in_use = 0;
      if ((struct hsl_server *)ms->info == &hsl_server_cmd ||
          (struct hsl_server *)ms->info == &hsl_server_async_cmd)
        {
          hse->thread_id = pthread_self();
          hse->thread_in_use = 1;
          prctl(PR_SET_NAME, hsl_get_client_name (preg->value), 0, 0, 0);
        }
      HSL_SERVER_ENTRY_UNLOCK(hse);
      return 1;
    }

#if 1
  switch (hse->client_id)
    {
      /* Everything else (msgs) */
    case HAL_SOCK_PROTO_NSM_ASYNC:
    default:
      nbytes = hsl_server_read_hal_msg (ms, me, hse, sock);
      break;
    }
#endif /* if 0 */
  HSL_SERVER_ENTRY_UNLOCK(hse);

  if (nbytes < 0)
    return -1;

  return 1;
}


/*Client specific thread Handler..*/
void *
hsl_client_thread(void *arg)
{
  int nbytes = 0;
  int num = 0;
  struct message_entry *me;
  struct message_handler *ms;
  pal_sock_handle_t sock;
#ifdef UNLIMIT_FD
#define MAX_EVENTS       1
  struct epoll_event events[MAX_EVENTS];
  struct epoll_event ev;
  int epfd;
#else
  pal_sock_set_t fds;
#endif /*UNLIMIT_FD*/

  me = (struct message_entry *)arg;
  ms = me->ms;
  sock = me->sock;

#ifdef UNLIMIT_FD
  /*Will monitor only one socket*/
  epfd = epoll_create (1);

  if (epfd == -1)
    pthread_exit(NULL);

  ev.events = EPOLLIN;
  ev.data.fd = sock;

  if (epoll_ctl (epfd, EPOLL_CTL_ADD, sock, &ev) == -1)
    {
      close (epfd);
      pthread_exit(NULL);
    }
#else
  FD_ZERO(&fds);
  PAL_SOCK_HANDLESET_SET(sock, &fds);
#endif /*UNLIMIT_FD*/

  while(1)
    {
#ifdef UNLIMIT_FD
      num = epoll_wait (epfd, events, MAX_EVENTS, -1);
#else
      num = pal_sock_select(PAL_SOCKSET_SIZE, &fds, NULL, NULL, NULL);
#endif /*UNLIMIT_FD*/

      /* Error handling.  */
      if (num < 0)
        {
          if (errno == EINTR)
            continue;
          /*Release "me" structure..*/
          message_server_disconnect (ms, me, sock);
#ifdef UNLIMIT_FD
          epoll_ctl (epfd, EPOLL_CTL_DEL, sock, NULL);
          close (epfd);
#endif /*UNLIMIT_FD*/

          /*Exit from the thread*/
          pthread_exit(NULL);
        }

      /* check anything is pending to read from the socket  */
      if (num > 0)
        {
          nbytes = hsl_read(ms, me, sock);

          /* Disconnection event handler.  */
          if (nbytes < 0)
            {
              /*Release "me" structure..*/
              message_server_disconnect (ms, me, sock);
#ifdef UNLIMIT_FD
              epoll_ctl (epfd, EPOLL_CTL_DEL, sock, NULL);
              close (epfd);
#endif /*UNLIMIT_FD*/

              /*Exist from the thread*/
              pthread_exit(NULL);
            }
        }
    }
#ifdef UNLIMIT_FD
  epoll_ctl (epfd, EPOLL_CTL_DEL, sock, NULL);
  close (epfd);
#endif /*UNLIMIT_FD*/

  pthread_exit(NULL);
}



/* Client connect to 'hsl' server. */
int
hsl_server_connect (struct message_handler *ms, struct message_entry *me,
    pal_sock_handle_t sock)
{
  int rc;
  pthread_t thread;

#ifdef DEBUG
  zlog_debug (hsl_zg, "%s: Connection recv\n",
      ((struct hsl_server *)ms->info)->name);
#endif

  /*
   * for the sync & async command processing create separate thread
   * for each client
   */
  if ((struct hsl_server *)ms->info == &hsl_server_cmd ||
      (struct hsl_server *)ms->info == &hsl_server_async_cmd)
    {
      /*create separate thread for each client*/
      rc = pthread_create(&thread, NULL, hsl_client_thread, (void *)me);
      if (rc)
        {
#if 0
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
              "Error in spawning client thread \n");
          HSL_FN_EXIT (STATUS_ERROR);
#endif /* if 0 */
          return -1;
        }
      /*
       * Cancel the native thread.Bcoz we are moving the context
       * to new thread
       */
      thread_cancel(me->t_read);
      me->t_read = NULL;
    }
  return 0;
}

int
hsl_server_client_delete (struct hsl_server *hs, pal_sock_handle_t sock)
{
  struct listnode *node;
  struct hsl_server_entry *hse;

  HSL_SERVER_WR_LOCK(hs);

  LIST_LOOP(hs->client, hse, node)
    {
      if (hse->sockfd != sock)
        continue;

      //     hsl_server_handle_client_down (hse->client_id);

      HSL_SERVER_ENTRY_LOCK(hse);
      if(hse->bulk_rsp_ptr != NULL)
        {
          free(hse->bulk_rsp_ptr);
          hse->bulk_rsp_ptr = NULL;
        }

      if (hse->recv_msg_buf != NULL)
        {
          free(hse->recv_msg_buf);
          hse->recv_msg_buf = NULL;
        }

#if 0
      /*Thread was not spawned for the clients which connected async socket*/
      if(hse->thread_id)
        hsl_thlist_delete_thread(hse->thread_id);
#endif /* if 0 */

      listnode_delete (hs->client, hse);
      hash_release (hs->client_sock_table, hse);
      hash_release (hs->client_id_table, hse);

      HSL_SERVER_ENTRY_UNLOCK(hse);
      pthread_mutex_destroy(&hse->lock);
      free(hse);
      break;
    }
  HSL_SERVER_WR_UNLOCK(hs);

  return RESULT_OK;
}


/* Client disconnect from NSM server.  This function should not be
   called directly.  Message library call this.  */
int
hsl_server_disconnect (struct message_handler *ms, struct message_entry *me,
    pal_sock_handle_t sock)
{
  int ret;
  struct hsl_server *hs;

  hs = ms->info;

  ret = hsl_server_client_delete (hs, sock);

#if 0
#ifdef DEBUG
  zlog_debug (hsl_zg, "%s: Disconnect recv\n",
      ((struct hsl_server *)ms->info)->name);
#endif
  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
      "HSL server disconnect [%s] sock[%d]\n",
      ((struct hsl_server *)ms->info)->name, sock);
#endif 
  return ret;
}



/* Initialize 'hsl' server. */
int
hsl_server_init (struct lib_globals *zg, struct hsl_server *is)
{
  struct message_handler *ms = NULL;
  int ret;

  pthread_rwlock_init(&is->rwlock, NULL);

  /* Get server path or port */
#ifdef HAVE_TCP_MESSAGE
  u_int16_t port;

  ret = hsl_get_tcp_server_port (is, &port);
#else
  char *path;

  ret = hsl_get_domain_server_path (is, &path);
#endif /* HAVE_TCP_MESSAGE */

  if (ret < 0)
    return ret;

  /* Create message server. */
  ms = message_server_create (zg);
  if (ms == NULL)
    return -1;

  /* Set server type. */
#ifdef HAVE_TCP_MESSAGE
  message_server_set_style_tcp (ms, port);
#else
  message_server_set_style_domain (ms, path);
#endif /* HAVE_TCP_MESSAGE */

  /* Set call back functions.  */
  message_server_set_callback (ms, MESSAGE_EVENT_CONNECT,
      hsl_server_connect);
  message_server_set_callback (ms, MESSAGE_EVENT_DISCONNECT,
      hsl_server_disconnect);
  message_server_set_callback (ms, MESSAGE_EVENT_READ_MESSAGE,
      hsl_read);

  /* Link each other. */
  ms->info = is;
  is->ms = ms;

  /* Start 'hsl' server. */
  ret = message_server_start (ms);

  if (ret == RESULT_OK)
    {
      /* Initialize the client list */
      is->client = list_new();

      is->client_sock_table = hash_create_size (HASHTABSIZE,
          hsl_client_sock_hash_entry_key_make,
          hsl_client_sock_hash_entry_cmp);
      is->client_id_table = hash_create_size (HASHTABSIZE,
          hsl_client_id_hash_entry_key_make,
          hsl_client_id_hash_entry_cmp);
    }

  return ret;
}

