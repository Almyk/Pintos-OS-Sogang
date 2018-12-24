#include "vm/swap.h"
#include "lib/kernel/bitmap.h"

static struct list free_swap_list; // list of free swap slots
static struct bitmap free_swap_bitmap; // bitmap of free swap slots

void
swap_init()
{
} 
