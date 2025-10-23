#include<stdio.h>
#include<sys/epoll.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<errno.h>
#include"pal_time.h"
#include"lib.h"

/*
** Some result codes which are returned.  Don't use the numbers!  Other
** error codes are defined by the platform.
*/
#define RESULT_OK                   0
#define RESULT_NO_SECONDARY_IF      1
#define RESULT_NO_SUPPORT           0x80

#define CHECK_THREAD_CONTEXT()

/* Thread types.  */
#define THREAD_READ             0
#define THREAD_WRITE            1
#define THREAD_TIMER            2
#define THREAD_EVENT            3
#define THREAD_QUEUE            4
#define THREAD_UNUSED           5
#define THREAD_READ_HIGH        6
#define THREAD_READ_PEND        7
#define THREAD_EVENT_LOW        8
#define THREAD_WRITE_HIGH       9
#define THREAD_EVENT_TPOOL      10
#define THREAD_TIMER_TPOOL      11
#define THREAD_EVENT_HIGH       12
#define THREAD_EVENT_MID_HIGH   13
#define THREAD_EVENT_MIDDLE     14

/* Macros.  */
#define THREAD_ARG(X)           ((X)->arg)
#define THREAD_FD(X)            ((X)->u.fd)
#define THREAD_VAL(X)           ((X)->u.val)
#define THREAD_TIME_VAL(X)      ((X)->u.sands)
#define THREAD_GLOB(X)          ((X)->zg)


struct lib_globals
{
  int protocol;
  u_int8_t flags;
  /* Thread master. */
  struct thread_master *master;
};

struct thread
{
  /* Linked list.  */
  struct thread *next;
  struct thread *prev;

  /* Pointer to the struct thread_master.  */
  struct thread_master *master;

  /* Pointer to the struct lib_globals. */
  struct lib_globals *zg;

 /* Event function.  */
  int (*func) (struct thread *);

  /* Event argument.  */
  void *arg;

  /* Second argument of the event to be passed along with arg & timer*/
  /*Incase arg2 is not of type int, this can be changed to union of args*/
  int arg2;

  /* Thread type.  */
  char type;

  /* Priority.  */
  char priority;
#define THREAD_PRIORITY_SUPER_HIGH   0
#define THREAD_PRIORITY_HIGH         1
#define THREAD_PRIORITY_MID_HIGH     2
#define THREAD_PRIORITY_MIDDLE       3
#define THREAD_PRIORITY_LOW          4

  /* Thread timer index.  */
  char index;

  /* Arguments.  */
  union
  {
    /* Second argument of the event.  */
    int val;

    /* File descriptor in case of read/write.  */
    int fd;

    /* Rest of time sands value.  */
    struct pal_timeval sands;
  } u;
};
/* Linked list of thread. */
struct thread_list
{
  struct thread *head;
  struct thread *tail;
  u_int32_t count;
};

struct thread_master
{
  /* Priority based queue.  */
  struct thread_list queue_super_high;
  struct thread_list queue_high;
  struct thread_list queue_mid_high;
  struct thread_list queue_middle;
  struct thread_list queue_low;

  /* Timer */
#define THREAD_TIMER_SLOT           5
#define THREAD_TIMER_SLOT_START     1
#define THREAD_TIMER_HIGH_SLOT      0
#define TIMER_EXTEND_SIZE           60
  int index;
  int current_index;
  int time_granularity;
  int extend_count;
  struct pal_timeval previous_time;
  struct thread_list timer[THREAD_TIMER_SLOT];
  struct thread_list *timer_extend;

  /* Thread to be executed.  */
  struct thread_list read_pend;
  struct thread_list read_high;
  struct thread_list read;
  struct thread_list write_high;
  struct thread_list write;
  struct thread_list event_high;
  struct thread_list event_mid_high;
  struct thread_list event_middle;
  struct thread_list event;
  struct thread_list event_low;
  struct thread_list unuse;
  //#ifdef UNLIMIT_FD
#define MAX_FD_SZ 8192
  int epfd;
  struct epoll_event epevent[MAX_FD_SZ];
//#endif /* UNLIMIT_FD */
  int max_fd;
  u_int32_t alloc;

#define MAX_HIQ_EXEC_TOKEN_DEF (30)
#define MAX_MHIQ_EXEC_TOKEN_DEF (20)
#define MAX_MIDQ_EXEC_TOKEN_DEF (10)
#define MAX_LOQ_EXEC_TOKEN_DEF (1)
#define MAX_HIQ_EXEC_TOKEN(m) (m->max_hiq_exec_token)
#define MAX_MHIQ_EXEC_TOKEN(m) (m->max_mhiq_exec_token)
#define MAX_MIDQ_EXEC_TOKEN(m) (m->max_midq_exec_token)
#define MAX_LOQ_EXEC_TOKEN(m) (m->max_loq_exec_token)


#define MAXQ_EXEC_TOKEN(m) (m->max_q_exec_token)
  u_int32_t q_exec_token;
  u_int32_t hiq_exec_token;
  u_int32_t mhiq_exec_token;
  u_int32_t midq_exec_token;
  u_int32_t loq_exec_token;

  u_int32_t max_q_exec_token;
  u_int32_t max_hiq_exec_token;
  u_int32_t max_mhiq_exec_token;
  u_int32_t max_midq_exec_token;
  u_int32_t max_loq_exec_token;


#define THREAD_FETCH_TYPE_1 0
#define THREAD_FETCH_TYPE_2 1
  u_int32_t fetch_type;
  char event_priority;
};

#define THREAD_OFF(thread) \
  do { \
    if (thread) \
      { \
        thread_cancel (thread); \
        thread = NULL; \
      } \
  } while (0)

#define THREAD_READ_OFF(thread)   THREAD_OFF(thread)
#define THREAD_WRITE_OFF(thread)  THREAD_OFF(thread)
#define THREAD_TIMER_OFF(thread)  THREAD_OFF(thread)

struct pal_timeval *
thread_timer_wait (struct lib_globals *zg, struct thread_master *m, struct pal_timeval *timer_val);

 int
 timeval_cmp (struct pal_timeval a, struct pal_timeval b);

  void
 pal_time_tzcurrent_monotonic (struct pal_timeval *tv,
                               struct pal_tzval *tz);

 /* Add a new thread to the list.  */
void
thread_list_add (struct thread_list *list, struct thread *thread);

 void
 thread_enqueue_high (struct thread_master *m, struct thread *thread);

  void
 thread_enqueue_high (struct thread_master *m, struct thread *thread);

 void
thread_enqueue_middle (struct thread_master *m, struct thread *thread);

void
thread_enqueue_low (struct thread_master *m, struct thread *thread);

 /* Allocate new thread master.  */
 struct thread_master *
 thread_master_create ();

 /* Delete a thread from the list. */
struct thread *
thread_list_delete (struct thread_list *list, struct thread *thread);

/* Delete top of the list and return it. */
struct thread *
thread_trim_head (struct thread_list *list);


 static struct thread *
 thread_new (struct lib_globals *zg);

  static void
 thread_free (struct thread *th);

 /* Move thread to unuse list. */
void
thread_add_unuse (struct thread_master *m, struct thread *thread);

/* Free all unused thread. */
void
thread_list_free (struct thread_master *m, struct thread_list *list);


void
thread_call (struct thread *thread);

/* Fake execution of the thread with given arguemment.  */
struct thread *
thread_execute (struct lib_globals *zg,
                int (*func)(struct thread *),
                void *arg,
                int val);

void
thread_list_execute (struct lib_globals *zg, struct thread_list *list);


 void
 thread_list_clear (struct lib_globals *zg, struct thread_list *list);

 /* Stop thread scheduler. */
void
thread_master_finish (struct lib_globals *zg, struct thread_master *m);

/* Get new thread.  */
struct thread *
thread_get (struct lib_globals *zg, char type,
            int (*func) (struct thread *), void *arg);


 /* Keep track of the maximum file descriptor for read/write. */
 static void
 thread_update_max_fd (struct lib_globals *zg, struct thread_master *m, int fd);

 int
epoll_add_fd_update (struct lib_globals *zg, struct thread_master *m,
                     int fd, struct thread *thread, u_int32_t events );


void
epoll_delete_fd_update (struct thread *thread, u_int32_t events );


 /* Add new read thread. */
struct thread *
thread_add_read (struct lib_globals *zg,
    int (*func) (struct thread *), void *arg, int fd);

 /* Add new write thread. */
struct thread *
thread_add_write (struct lib_globals *zg,
                 int (*func) (struct thread *), void *arg, int fd);

 /* Add simple event thread. */
 struct thread *
 thread_add_event (struct lib_globals *zg,
                   int (*func) (struct thread *), void *arg, int val);


  /* Cancel thread from scheduler. */
 void
 thread_cancel (struct thread *thread);


 struct thread *
thread_run (struct thread_master *m, struct thread *thread,
            struct thread *fetch);


int
thread_process_fd (struct thread_master *m, struct thread_list *list,
                           struct epoll_event *ep, int nfd, int read);



struct thread *
thread_fetch3 (struct lib_globals *zg, struct thread *fetch,
               int (*check_termination_cb)(void));



