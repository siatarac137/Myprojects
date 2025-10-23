#include "thread.h"
#include "pal_time.h"

struct pal_timeval *
thread_timer_wait (struct lib_globals *zg, struct thread_master *m, struct pal_timeval *timer_val)
{
  timer_val->tv_sec = 1;
  timer_val->tv_usec = 0;
  return timer_val;
}

int
timeval_cmp (struct pal_timeval a, struct pal_timeval b)
{
  return (a.tv_sec == b.tv_sec ?
          a.tv_usec - b.tv_usec : a.tv_sec - b.tv_sec);
}

void
pal_time_tzcurrent_monotonic (struct pal_timeval *tv,
                              struct pal_tzval *tz)
{
  struct timespec t = {0};
  int ret = RESULT_OK;

  if (tv && tz)
    gettimeofday (tv, tz);

  ret = clock_gettime(CLOCK_MONOTONIC, &t);
  if (ret != RESULT_OK)
    return;

  /* When argument is specified copy value.  */
  if (tv)
    {
      tv->tv_sec = t.tv_sec;
      tv->tv_usec = t.tv_nsec/1000;
    }
}


/* Add a new thread to the list.  */
void
thread_list_add (struct thread_list *list, struct thread *thread)
{
  thread->next = NULL;
  thread->prev = list->tail;
  if (list->tail)
    list->tail->next = thread;
  else
    list->head = thread;
  list->tail = thread;
  list->count++;
}

void
thread_enqueue_high (struct thread_master *m, struct thread *thread)
{
  thread->type = THREAD_QUEUE;
  thread->priority = THREAD_PRIORITY_HIGH;
  thread_list_add (&m->queue_high, thread);
}

void
thread_enqueue_mid_high (struct thread_master *m, struct thread *thread)
{
  thread->type = THREAD_QUEUE;
  thread->priority = THREAD_PRIORITY_MID_HIGH;
  thread_list_add (&m->queue_mid_high, thread);
}

void
thread_enqueue_middle (struct thread_master *m, struct thread *thread)
{
  thread->type = THREAD_QUEUE;
  thread->priority = THREAD_PRIORITY_MIDDLE;
  thread_list_add (&m->queue_middle, thread);
}

void
thread_enqueue_low (struct thread_master *m, struct thread *thread)
{
  thread->type = THREAD_QUEUE;
  thread->priority = THREAD_PRIORITY_LOW;
  thread_list_add (&m->queue_low, thread);
}

/* Allocate new thread master.  */
struct thread_master *
thread_master_create ()
{
  struct thread_master *tm;
  int i;

  tm = (struct thread_master *) malloc(sizeof (struct thread_master));
  tm->epfd = epoll_create (MAX_FD_SZ);

  for (i = 0; i < MAX_FD_SZ; i++)
    {
      memset (&tm->epevent[i], 0, sizeof(struct epoll_event));
    }

  tm->index = THREAD_TIMER_HIGH_SLOT + 1;
  tm->fetch_type = THREAD_FETCH_TYPE_1;
  tm->event_priority = THREAD_PRIORITY_HIGH;

  tm->max_hiq_exec_token  = MAX_HIQ_EXEC_TOKEN_DEF;
  tm->max_mhiq_exec_token = MAX_MHIQ_EXEC_TOKEN_DEF;
  tm->max_midq_exec_token = MAX_MIDQ_EXEC_TOKEN_DEF;
  tm->max_loq_exec_token  = MAX_LOQ_EXEC_TOKEN_DEF;
  tm->max_q_exec_token  = tm->max_hiq_exec_token +
                          tm->max_mhiq_exec_token +
                          tm->max_midq_exec_token +
                          tm->max_loq_exec_token;


  return tm;
}





/* Delete a thread from the list. */
struct thread *
thread_list_delete (struct thread_list *list, struct thread *thread)
{
  if (thread->next)
    thread->next->prev = thread->prev;
  else
    list->tail = thread->prev;
  if (thread->prev)
    thread->prev->next = thread->next;
  else
    list->head = thread->next;
  thread->next = thread->prev = NULL;
  list->count--;
  return thread;
}


/* Delete top of the list and return it. */
struct thread *
thread_trim_head (struct thread_list *list)
{
  if (list->head)
    return thread_list_delete (list, list->head);
  return NULL;
}


static struct thread *
thread_new (struct lib_globals *zg)
{
  struct thread *th;
  struct thread_master *m = zg->master;

  th = (struct thread*)malloc(sizeof (struct thread));
  if (th)
    m->alloc++;

  return th;
}


static void
thread_free (struct thread *th)
{
  struct thread_master *m;

  assert (th->zg != NULL);

  m = th->zg->master;
  m->alloc--;

  free(th);
}



/* Move thread to unuse list. */
void
thread_add_unuse (struct thread_master *m, struct thread *thread)
{
  assert (m != NULL);
  assert (thread->zg != NULL);
  assert (thread->next == NULL);
  assert (thread->prev == NULL);
  assert (thread->type == THREAD_UNUSED);
  CHECK_THREAD_CONTEXT();
  thread_list_add (&m->unuse, thread);
}



/* Free all unused thread. */
void
thread_list_free (struct thread_master *m, struct thread_list *list)
{
  struct thread *t;
  struct thread *next;

  for (t = list->head; t; t = next)
    {
      next = t->next;
      thread_free (t);
      list->count--;
    }
}

void
thread_call (struct thread *thread)
{
  (*thread->func) (thread);
}

/* Fake execution of the thread with given arguemment.  */
struct thread *
thread_execute (struct lib_globals *zg,
                int (*func)(struct thread *),
                void *arg,
                int val)
{
  struct thread dummy;

#if 0
  /* Set the global VR context to PVR. */
  if (! zg->vr_in_cxt)
    zg->vr_in_cxt = ipi_vr_get_privileged(zg);
#endif /* if 0 */

  memset (&dummy, 0, sizeof (struct thread));

  dummy.type = THREAD_EVENT;
  dummy.zg = zg;
  dummy.master = NULL;
  dummy.func = func;
  dummy.arg = arg;
  dummy.u.val = val;
  thread_call (&dummy);

  return NULL;
}

void
thread_list_execute (struct lib_globals *zg, struct thread_list *list)
{
  struct thread *thread;

  thread = thread_trim_head (list);
  if (thread != NULL)
    {
      thread_execute (zg, thread->func, thread->arg, thread->u.val);
      thread->type = THREAD_UNUSED;
      thread_add_unuse (zg->master, thread);
    }
}

void
thread_list_clear (struct lib_globals *zg, struct thread_list *list)
{
  struct thread *thread;

  while ((thread = thread_trim_head (list)))
    {
      thread->type = THREAD_UNUSED;
      thread_add_unuse (zg->master, thread);
    }
}

/* Stop thread scheduler. */
void
thread_master_finish (struct lib_globals *zg, struct thread_master *m)
{
  int i;

  thread_list_free (m, &m->queue_super_high);
  thread_list_free (m, &m->queue_high);
  thread_list_free (m, &m->queue_mid_high);
  thread_list_free (m, &m->queue_middle);
  thread_list_free (m, &m->queue_low);
  thread_list_free (m, &m->read);
  thread_list_free (m, &m->read_high);
  thread_list_free (m, &m->write);
  thread_list_free (m, &m->write_high);
  for (i = 0; i < THREAD_TIMER_SLOT; i++)
    thread_list_free (m, &m->timer[i]);
  thread_list_free (m, &m->event_high);
  thread_list_free (m, &m->event_mid_high);
  thread_list_free (m, &m->event_middle);
  thread_list_free (m, &m->event);
  thread_list_free (m, &m->event_low);
  thread_list_free (m, &m->unuse);

#if 0
  if (m->timer_extend)
    lib_finish_timer_extend (m);
#endif /* if 0 */

  free(m);
}


/* Get new thread.  */
struct thread *
thread_get (struct lib_globals *zg, char type,
            int (*func) (struct thread *), void *arg)
{
  struct thread_master *m = zg->master;
  struct thread *thread;

  CHECK_THREAD_CONTEXT();

  thread = thread_new (zg);
  if (thread == NULL)
    return NULL;

  thread->type = type;
  thread->master = m;
  thread->func = func;
  thread->arg = arg;
  thread->zg = zg;

  return thread;
}


/* Keep track of the maximum file descriptor for read/write. */
static void
thread_update_max_fd (struct lib_globals *zg, struct thread_master *m, int fd)
{
  if (m && m->max_fd < fd)
    m->max_fd = fd;
}

int
epoll_add_fd_update (struct lib_globals *zg, struct thread_master *m,
                     int fd, struct thread *thread, u_int32_t events )
{
  int op;
  int ret = 0;
  struct epoll_event *ev;

  if (fd >= MAX_FD_SZ)
    return 1;

  ev = &m->epevent[fd];
  if (ev->events == 0)
    {
      ev->data.fd = fd;
      ev->events |= events;
      op = EPOLL_CTL_ADD;
    }
  else
    {
      ev->events |= events;
      op = EPOLL_CTL_MOD;
    }

  if (epoll_ctl (m->epfd, op, fd, ev) < 0)
    {
      thread->type = THREAD_UNUSED;
      thread_add_unuse (zg->master, thread);
      ret = 1;
    }
  return ret;
}


void
epoll_delete_fd_update (struct thread *thread, u_int32_t events )
{
  struct epoll_event *ev;

  if (thread->u.fd >= MAX_FD_SZ)
    return;

  ev = &thread->master->epevent[thread->u.fd];
  ev->events &= ~events;
  if ((ev->events) == 0)
    {
      epoll_ctl (thread->master->epfd, EPOLL_CTL_DEL, thread->u.fd, ev);
      ev->data.fd = 0;
      ev->events = 0;
    }
  else
    epoll_ctl (thread->master->epfd, EPOLL_CTL_MOD, thread->u.fd, ev);
}


/* Add new read thread. */
struct thread *
thread_add_read (struct lib_globals *zg,
                 int (*func) (struct thread *), void *arg, int fd)
{
  struct thread *thread;
  struct thread_master *m = zg->master;

  assert (m != NULL);

  if (fd < 0)
    return NULL;

  if (fd >= MAX_FD_SZ)
    return NULL;

  thread = thread_get (zg, THREAD_READ, func, arg);
  if (thread == NULL)
    return NULL;

  thread_update_max_fd (zg, m, fd);
  if (epoll_add_fd_update (zg, m, fd, thread, EPOLLIN))
   return NULL;
  thread->u.fd = fd;
  thread_list_add (&m->read, thread);

  return thread;
}


/* Add new write thread. */
struct thread *
thread_add_write (struct lib_globals *zg,
                 int (*func) (struct thread *), void *arg, int fd)
{
  struct thread_master *m = zg->master;
  struct thread *thread;

  assert (m != NULL);

  if (fd < 0 || fd >= MAX_FD_SZ)
    return NULL;

  thread = thread_get (zg, THREAD_WRITE, func, arg);
  if (thread == NULL)
    return NULL;

  thread_update_max_fd (zg, m, fd);
  if (epoll_add_fd_update (zg, m, fd, thread, EPOLLOUT))
    return NULL;

  thread->u.fd = fd;
  thread_list_add (&m->write, thread);

  return thread;
}


/* Add simple event thread. */
struct thread *
thread_add_event (struct lib_globals *zg,
                  int (*func) (struct thread *), void *arg, int val)
{
  struct thread *thread;
  struct thread_master *m = zg->master;

  assert (m != NULL);

  thread = thread_get (zg, THREAD_EVENT, func, arg);
  if (thread == NULL)
    return NULL;

  thread->u.val = val;
  thread_list_add (&m->event, thread);

  return thread;
}

/* Cancel thread from scheduler. */
void
thread_cancel (struct thread *thread)
{
  switch (thread->type)
    {
      case THREAD_READ:
        epoll_delete_fd_update (thread, EPOLLIN);
        thread_list_delete (&thread->master->read, thread);
        break;
      case THREAD_READ_HIGH:
        epoll_delete_fd_update (thread, EPOLLIN);
        thread_list_delete (&thread->master->read_high, thread);
        break;
      case THREAD_WRITE:
        epoll_delete_fd_update (thread, EPOLLOUT);
        thread_list_delete (&thread->master->write, thread);
        break;
      case THREAD_WRITE_HIGH:
         epoll_delete_fd_update (thread, EPOLLOUT);
        thread_list_delete (&thread->master->write_high, thread);
        break;
      case THREAD_TIMER:
        thread_list_delete (&thread->master->timer[(int)thread->index], thread);
        break;
      case THREAD_EVENT:
        thread_list_delete (&thread->master->event, thread);
        break;
      case THREAD_READ_PEND:
        thread_list_delete (&thread->master->read_pend, thread);
        break;
      case THREAD_EVENT_HIGH:
        thread_list_delete (&thread->master->event_high, thread);
        break;
      case THREAD_EVENT_MID_HIGH:
        thread_list_delete (&thread->master->event_mid_high, thread);
        break;
      case THREAD_EVENT_MIDDLE:
        thread_list_delete (&thread->master->event_middle, thread);
        break;
      case THREAD_EVENT_LOW:
        thread_list_delete (&thread->master->event_low, thread);
        break;
      case THREAD_QUEUE:
        switch (thread->priority)
          {
            case THREAD_PRIORITY_SUPER_HIGH:
              thread_list_delete (&thread->master->queue_super_high, thread);
              break;
            case THREAD_PRIORITY_HIGH:
              thread_list_delete (&thread->master->queue_high, thread);
              break;
            case THREAD_PRIORITY_MID_HIGH:
              thread_list_delete (&thread->master->queue_mid_high, thread);
              break;
            case THREAD_PRIORITY_MIDDLE:
              thread_list_delete (&thread->master->queue_middle, thread);
              break;
            case THREAD_PRIORITY_LOW:
              thread_list_delete (&thread->master->queue_low, thread);
              break;
          }
        break;
      default:
        break;
    }
  thread->type = THREAD_UNUSED;
  thread_add_unuse (thread->master, thread);
}

struct thread *
thread_run (struct thread_master *m, struct thread *thread,
            struct thread *fetch)
{
  *fetch = *thread;
  thread->type = THREAD_UNUSED;
  thread_add_unuse (m, thread);
  return fetch;
}

int
thread_process_fd (struct thread_master *m, struct thread_list *list,
                           struct epoll_event *ep, int nfd, int read)
{
  struct thread *thread;
  struct thread *next;
  int ready = 0;
  int i;

  for (thread = list->head; thread; thread = next)
    {
      next = thread->next;
      for (i = 0; i < nfd; i++)
        {
          if ((ep[i].events & EPOLLERR)
              && (ep[i].data.fd == THREAD_FD (thread))
              && thread->zg->protocol == IPI_PROTO_ISIS)
            {
              epoll_delete_fd_update (thread, EPOLLERR);
              thread_list_delete (list, thread);
              thread_enqueue_middle (m, thread);
              ready++;
              break;
            }

          if (read)
            {
              if ((ep[i].events & EPOLLIN)
                  && (ep[i].data.fd == THREAD_FD (thread)))
                {
                  epoll_delete_fd_update (thread, EPOLLIN);
                  thread_list_delete (list, thread);
                  thread_enqueue_middle (m, thread);
                  ready++;
                  break;
                }
            }
          else
            {
              if ((ep[i].events & EPOLLOUT)
                  && (ep[i].data.fd == THREAD_FD (thread)))
                {
                  epoll_delete_fd_update (thread, EPOLLOUT);
                  thread_list_delete (list, thread);
                  thread_enqueue_middle (m, thread);
                  ready++;
                  break;
                }
            }
        }
    }
  return ready;
}


struct thread *
thread_fetch3 (struct lib_globals *zg, struct thread *fetch, 
               int (*check_termination_cb)(void))
{
  struct thread_master *m = zg->master;
  int num;
  struct thread *thread;
  struct thread *next;
  struct epoll_event events[MAX_FD_SZ];
  struct pal_timeval timer_now;
  struct pal_timeval timer_val;
  struct pal_timeval *timer_wait;
  struct pal_timeval timer_nowait;
  int i;
  int ret1 = 0,ret2 = 0,ret3 = 0,ret4 = 0;

#if 0
#ifdef RTOS_DEFAULT_WAIT_TIME
  /* 1 sec might not be optimized */
  timer_nowait.tv_sec = 1;
  timer_nowait.tv_usec = 0;
#else
  timer_nowait.tv_sec = 0;
  timer_nowait.tv_usec = 0;
#endif /* RTOS_DEFAULT_WAIT_TIME */
#endif /* if 0*/

  timer_nowait.tv_sec = 0;
  timer_nowait.tv_usec = 0;

  /* Set the global VR context to PVR. */
//  zg->vr_in_cxt = ipi_vr_get_privileged(zg);

  while (!check_termination_cb || !check_termination_cb())
    {
      /* Pending read is exception. */
      if ((thread = thread_trim_head (&m->read_pend)) != NULL)
        return thread_run (m, thread, fetch);

      /* Check ready queue.  */
      if ((thread = thread_trim_head (&m->queue_high)) != NULL)
        return thread_run (m, thread, fetch);

      if ((thread = thread_trim_head (&m->queue_middle)) != NULL)
        return thread_run (m, thread, fetch);

      if ((thread = thread_trim_head (&m->queue_low)) != NULL)
        return thread_run (m, thread, fetch);

      /* Check all of available events.  */

      /* Check events.  */
      while ((thread = thread_trim_head (&m->event)) != NULL)
        thread_enqueue_high (m, thread);

      /* Check timer.  */
      pal_time_tzcurrent_monotonic (&timer_now, NULL);

#if 0              
#ifdef THREAD_AUDIT
      thread_audit_run(zg, &timer_now);
#endif /*THREAD_AUDIT*/
#endif /* if 0 */

      for (i = 0; i < THREAD_TIMER_SLOT; i++)
        for (thread = m->timer[i].head; thread; thread = next)
          {
            next = thread->next;
            if (timeval_cmp (timer_now, thread->u.sands) >= 0)
              {
                thread_list_delete (&m->timer[i], thread);
                thread_enqueue_middle (m, thread);

              }
#if 0
#ifndef TIMER_NO_SORT
            else
              break;
#endif /* TIMER_NO_SORT */
#endif
          }

#if 0
      /* Structure copy.  */
      readfd = m->readfd;
      writefd = m->writefd;
      exceptfd = m->exceptfd;
#endif /* if 0 */

      /* Check any thing to be execute.  */
      if (m->queue_high.head || m->queue_middle.head || m->queue_low.head)
        timer_wait = &timer_nowait;
      else
        timer_wait = thread_timer_wait (zg, m, &timer_val);

          /* First check for sockets.  Return immediately.  */
            num = epoll_wait (m->epfd, events, m->max_fd + 1,
                    timer_wait ? (timer_wait->tv_sec ? timer_wait->tv_sec * 1000:
                        timer_wait->tv_usec == 0 ? 0:
                        timer_wait->tv_usec/1000 ? timer_wait->tv_usec/1000: 1) : -1);

          /* Error handling.  */
          if (num < 0)
            {
              if (errno == EINTR)
                continue;
              return NULL;
            }

          /* File descriptor is readable/writable.  */
          if (num > 0)
            {
              /* High priority read thead. */
              ret1 = thread_process_fd (m, &m->read_high, events, num, 1);

              /* High Priority Write thead. */
              ret2 = thread_process_fd (m, &m->write_high, events, num, 0);

              /* Normal priority read thead. */
              ret3 = thread_process_fd (m, &m->read, events, num, 1);

              /* Write thead. */
              ret4 = thread_process_fd (m, &m->write, events, num, 0);
              if (zg->protocol == IPI_PROTO_ISIS)
                {
                  if (ret1 || ret2 || ret3 || ret4)
                    continue;
                }
            }

      /* Low priority events. */
      if ((thread = thread_trim_head (&m->event_low)) != NULL)
        thread_enqueue_low (m, thread);
    }
   /* Termination callback says it is time */
   return NULL;
}

/* Call the thread.  */

