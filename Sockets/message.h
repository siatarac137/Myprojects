#include "thread.h"
#include "lib.h"

/* Generic message handling library.  Two connection type UNIX domain
   socket and TCP socket is supported.  */

#define MSG_CINDEX_SIZE                 64
typedef u_int64_t  cindex_t;

/* Message library event types.  */
#define MESSAGE_EVENT_CONNECT           0
#define MESSAGE_EVENT_DISCONNECT        1
#define MESSAGE_EVENT_READ_HEADER       2
#define MESSAGE_EVENT_READ_MESSAGE      3
#define MESSAGE_EVENT_DISCONNECT_VALIDATE 4
#define MESSAGE_EVENT_MAX               5

/* Message client socket type. */
#define MESSAGE_CLIENT_BLOCKING         0
#define MESSAGE_CLIENT_NON_BLOCKING     1

#define MESSAGE_SYNC_CLIENT_REGISTER    0
#define MESSAGE_ASYNC_CLIENT_REGISTER   1

#define MESSAGE_HDR_SIZE                14
#define MESSAGE_REGMSG_SIZE             4
#define MESSAGE_MAX_SIZE                (18 * 1024)

/* Message library callback function typedef.  */
struct message_handler;
struct message_entry;

typedef int (*MESSAGE_CALLBACK) (struct message_handler *,
                                 struct message_entry *, pal_sock_handle_t);

/* Message handler structure.  This structure is common both server
   and client side.  */
struct message_handler
{
  struct lib_globals *zg;

  /* Socket to accept or connect.  */
  pal_sock_handle_t sock;

  /* Style of the connection.  */
  int style;
#define MESSAGE_STYLE_UNIX_DOMAIN   0
#define MESSAGE_STYLE_TCP           1

  /* Indicate remote connection */
  int remote_conn;
  //struct pal_sockaddr_in4 remote_addr;

  /* Type of the connection.  Async or sync.  */
  int type;
#define MESSAGE_TYPE_ASYNC          0
#define MESSAGE_TYPE_SYNC           1
#define MESSAGE_TYPE_ASYNC_HIGH     2
#define MESSAGE_TYPE_ASYNC_ET       3

  /* Port for TCP socket.  */
  u_int16_t port;

  /* Path for UNIX domain socket.  */
  char *path;

  /* Call back handers.  */
  MESSAGE_CALLBACK callback[MESSAGE_EVENT_MAX];

  /* Connection read thread.  */
  struct thread *t_read;

  /* Connect thread. */
  struct thread *t_connect;

  /* Vector to manage client.  */
  struct list *clist;

  /* Information pointer.  */
  void *info;

  /* Status of connection. */
  int status;
#define MESSAGE_HANDLER_DISCONNECTED      0
#define MESSAGE_HANDLER_CONNECTED         1

  /* Non blocking connection. */
  bool sock_nonblock;

};


/* This structure is used at server side for managing client
   connection.  */
struct message_entry
{
  struct lib_globals *zg;

  /* Pointer to message server structure.  */
  struct message_handler *ms;

  /* Socket to client.  */
  pal_sock_handle_t sock;

  /* Information to user specific data.  */
  void *info;

  /* Read thread.  */
  struct thread *t_read;

#ifdef THREAD_AUDIT
 /* Event function.  */
  int (*func) (struct thread *);
  char t_type;
#endif /*THREAD_AUDIT*/

};


/* Client message for registration with HSL server */
struct preg_msg
{
  u_int16_t len;
  u_int16_t value;
};

/* Create message server.  */
struct message_handler *
message_server_create (struct lib_globals *zg);

int
message_server_stop (struct message_handler *ms);

void
message_server_set_style_domain (struct message_handler *ms, char *path);

void
message_server_set_callback (struct message_handler *ms, int event,
    MESSAGE_CALLBACK cb);

struct message_entry *
message_entry_new ();
struct message_entry *
message_entry_register (struct message_handler *ms, pal_sock_handle_t sock);

int
message_server_read (struct thread *t);

/* Accept client connection.  */
int
message_server_accept (struct thread *t);

pal_sock_handle_t
message_server_socket_unix (struct message_handler *mh);
int
message_server_start (struct message_handler *ms);

void
message_server_disconnect (struct message_handler *ms,
                           struct message_entry *me, pal_sock_handle_t sock);
void
message_entry_free (struct message_entry *me);

