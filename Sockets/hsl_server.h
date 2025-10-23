#include "lib.h"
#include "linklist.h"
#include "hash.h"
#include <pthread.h>
#include "message.h"
#include <linux/prctl.h>  /* Definition of PR_* constants */
#include <sys/prctl.h>
#include "network.h"

#define UNLIMIT_FD 1

#define HSL_CMD_PORT             3200
#define HSL_CMD_PATH             "/tmp/.hsl_cmd"
#define HSL_ASYNC_PORT           3201
#define HSL_ASYNC_PATH           "/tmp/.hsl_async"
#define HSL_ASYNC_CMD_PORT       3202
#define HSL_ASYNC_CMD_PATH       "/tmp/.hsl_async_cmd"

/*Data structure definitions for async & Bulking support*/
#define MAXIMUM_BULK_MSG_SIZE                           (16 *1024)

/* A threshold value for the queued up hal msgs */
#define HAL_ASYNC_MSG_QUEUE_THRESHOLD                   10000

struct hal_async_bulk_msg
{
  u_char buf[MAXIMUM_BULK_MSG_SIZE];
  u_int16_t offset;
  int sock_handle;
};

/* 'hsl' server structure. */
struct hsl_server
{
  /* Message handler.  */
  struct message_handler *ms;

  /* Name */
  char *name;

  /* Client list */
  struct list *client;

  /* RW lock */
  pthread_rwlock_t rwlock;

  /* client sock table */
  struct hash *client_sock_table;

  /* client id table */
  struct hash *client_id_table;
};

struct hsl_server_entry
{
  /* Socket descriptor */
  pal_sock_handle_t sockfd;

  /* Client id */
  u_int16_t client_id;

  /*thread ID*/
  u_int16_t thread_in_use;
  pthread_t thread_id;

  /*Bulk buffer pointer*/
  struct hal_async_bulk_msg *bulk_rsp_ptr;

  /* recv msg buffer */
  u_char *recv_msg_buf;

  /* lock */
  pthread_mutex_t lock;
};

extern struct hsl_server hsl_server_cmd;

extern struct hsl_server hsl_server_async;

extern struct hsl_server hsl_server_async_cmd;

#define HSL_SERVER_RD_LOCK(HS) pthread_rwlock_rdlock(&((HS)->rwlock))
#define HSL_SERVER_RD_UNLOCK(HS) pthread_rwlock_unlock(&((HS)->rwlock))
#define HSL_SERVER_WR_LOCK(HS) pthread_rwlock_wrlock(&((HS)->rwlock))
#define HSL_SERVER_WR_UNLOCK(HS) pthread_rwlock_unlock(&((HS)->rwlock))

#define HSL_SERVER_ENTRY_LOCK(HSE) pthread_mutex_lock(&((HSE)->lock))
#define HSL_SERVER_ENTRY_UNLOCK(HSE) pthread_mutex_unlock(&((HSE)->lock))



enum hal_l2_proto_sock_id
{
  /*Async events..*/
  HAL_SOCK_PROTO_NSM,
  HAL_SOCK_PROTO_RIB,
  HAL_SOCK_PROTO_MRIB,
  HAL_SOCK_PROTO_L2MRIB,
  HAL_SOCK_PROTO_LDP,
  HAL_SOCK_PROTO_SFLOW2,
  HAL_SOCK_PROTO_PTP2,
  HAL_SOCK_PROTO_PON,
  HAL_SOCK_PROTO_PON_OMCI,
  HAL_SOCK_PROTO_MACSEC,
  HAL_SOCK_PROTO_UDLD2,

  /*L2 sockets..*/
  HAL_SOCK_PROTO_MSTP,
  HAL_SOCK_PROTO_MSTP_LINK_ASYNC,
  HAL_SOCK_PROTO_MCCP_MSTP,
  HAL_SOCK_PROTO_SPB,
  HAL_SOCK_PROTO_LACP,
  HAL_SOCK_PROTO_IGMP_SNOOP,
  HAL_SOCK_PROTO_MCCP_IGMP,
  HAL_SOCK_PROTO_MLD_SNOOP,
  HAL_SOCK_PROTO_GARP,
  HAL_SOCK_PROTO_8021X,
  HAL_SOCK_PROTO_CFM,
  HAL_SOCK_PROTO_CFM_PKT,
  HAL_SOCK_PROTO_EFM,
  HAL_SOCK_PROTO_LLDP,
  HAL_SOCK_PROTO_ELMI,
  HAL_SOCK_PROTO_PTP,
  HAL_SOCK_PROTO_SFLOW,
  HAL_SOCK_PROTO_SFLOW_CNTR,
  HAL_SOCK_PROTO_NDD,
  HAL_SOCK_PROTO_OAM,
  HAL_SOCK_PROTO_OAM_PKT,
  HAL_SOCK_PROTO_VRRP,
  HAL_SOCK_PROTO_UDLD,
  HAL_SOCK_PROTO_TRILL,
  HAL_SOCK_PROTO_OFL_PKT,
  HAL_SOCK_PROTO_DHCP_SNOOP,
  HAL_SOCK_PROTO_LAG,
  HAL_SOCK_PROTO_CMM,
  HAL_SOCK_PROTO_SYNCE,
  HAL_SOCK_PROTO_SYNCE_PKT,
  HAL_SOCK_PROTO_NVO_VXLAN,
  HAL_SOCK_PROTO_NVO_MPLS,
  HAL_SOCK_PROTO_NVO_SRV6,
  HAL_SOCK_PROTO_MLAG,
  HAL_SOCK_PROTO_RSVP_PKT,
  HAL_SOCK_PROTO_LBD,

  /*Sync command sockets..*/
  HAL_SOCK_PROTO_NSM_SYNC = 50,
  HAL_SOCK_PROTO_RIB_SYNC,
  HAL_SOCK_PROTO_MRIB_SYNC,
  HAL_SOCK_PROTO_L2MRIB_SYNC,
  HAL_SOCK_PROTO_LDP_SYNC,
  HAL_SOCK_PROTO_NDD_SYNC,
  HAL_SOCK_PROTO_OAM_SYNC,
  HAL_SOCK_PROTO_VRRP_SYNC,
  HAL_SOCK_PROTO_UDLD_SYNC,
  HAL_SOCK_PROTO_VPORT_SYNC,
  HAL_SOCK_PROTO_CFM_SYNC,
  HAL_SOCK_PROTO_LAG_SYNC,
  HAL_SOCK_PROTO_CMM_SYNC,
  HAL_SOCK_PROTO_SFLOW_SYNC,
  HAL_SOCK_PROTO_SYNCE_SYNC,
  HAL_SOCK_PROTO_PTP_SYNC,
  HAL_SOCK_PROTO_PON_SYNC,
  HAL_SOCK_PROTO_MACSEC_SYNC,

  /*ASync command sockets..*/
  HAL_SOCK_PROTO_NSM_ASYNC = 100,
  HAL_SOCK_PROTO_RIB_ASYNC,
  HAL_SOCK_PROTO_MRIB_ASYNC,
  HAL_SOCK_PROTO_L2MRIB_ASYNC,
  HAL_SOCK_PROTO_LDP_ASYNC,
  HAL_SOCK_PROTO_NDD_ASYNC,
  HAL_SOCK_PROTO_OAM_ASYNC,
  HAL_SOCK_PROTO_VRRP_ASYNC,
  HAL_SOCK_PROTO_UDLD_ASYNC,
  HAL_SOCK_PROTO_CFM_ASYNC,
  HAL_SOCK_PROTO_LAG_ASYNC,
  HAL_SOCK_PROTO_CMM_ASYNC,
  HAL_SOCK_PROTO_SFLOW_ASYNC,
  HAL_SOCK_PROTO_SYNCE_ASYNC,
  HAL_SOCK_PROTO_PTP_ASYNC,
  HAL_SOCK_PROTO_PON_ASYNC,
  HAL_SOCK_PROTO_MACSEC_ASYNC,
  HAL_SOCK_PROTO_RSVP_ASYNC,

  HAL_SOCK_PROTO_MAX,
};

int
hsl_get_domain_server_path (struct hsl_server *is, char **path);

struct hsl_server_entry *
hsl_server_client_lookup_by_fd (struct hsl_server *hs, pal_sock_handle_t sock);

struct hsl_server_entry *
hsl_server_client_lookup_by_id (struct hsl_server *hs, u_int16_t client_id);

int
hsl_read (struct message_handler *ms, struct message_entry *me,
          pal_sock_handle_t sock);

/*Client specific thread Handler..*/
void *
hsl_client_thread(void *arg);


/* Client connect to 'hsl' server. */
int
hsl_server_connect (struct message_handler *ms, struct message_entry *me,
                      pal_sock_handle_t sock);

/* Client connect to 'hsl' server. */
int
hsl_server_connect (struct message_handler *ms, struct message_entry *me,
                      pal_sock_handle_t sock);

 /* Initialize 'hsl' server. */
 int
 hsl_server_init (struct lib_globals *zg, struct hsl_server *is);


