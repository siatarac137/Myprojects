
/* main */

#include "prefix_or_ls_table.h"
int main(int argc, char **argv)
{
    struct ls_prefix lsp;
    int i = 0;
    struct ls_table *Lstable = NULL;
    
    if (argc < 2)
        return 0;
    
    Lstable = ls_table_init(LS_PREFIX_MIN_SIZE, DEFAULT_SLOT_SIZE);
    for (i = 1; i < argc; i++)
    {
        str2prefix_ipv4(argv[i], &lsp);
        ls_node_get(Lstable, &lsp);
        memset(&lsp, 0, sizeof(struct ls_prefix));
    }
    
    return 0;
}





















