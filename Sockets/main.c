#include "hsl_server.h"
#include <signal.h>

volatile sig_atomic_t isis_had_termination_signal = 0;

static int
check_termination (void)
{
  return isis_had_termination_signal;
}

int
hsl_server_stop()
{

}

int
main(int argc, char **argv)
{
   /* Malloc the master */
   struct lib_globals *zg = NULL;
   struct thread thread;

   zg = (struct lib_globals *)malloc(sizeof(struct lib_globals));
   memset(zg, 0, sizeof(struct lib_globals));

   zg->protocol = IPI_PROTO_ISIS;

   /* Malloc thread master */
   zg->master = thread_master_create();

   /* Initialize the hsl_server */
   hsl_server_init (zg, &hsl_server_async_cmd);

   while(thread_fetch3 (zg, &thread, check_termination))
     thread_call(&thread);
   
   return 0;
}
