/* { dg-do compile } */
/* { dg-options "-O2" } */

struct item
{
  int event;
  void *data;
};

void produce (struct item *, int);

void
caller (void *return_address, int event, void *data)
{
  struct item items[3];

  items[0].event = 1;
  items[0].data = return_address;
  items[1].event = 2;
  items[1].data = __builtin_return_address (0);
  items[2].event = event;
  items[2].data = data;
  produce (items, 3);
}

/* The return address of the current frame is the link register on entry.
   The prologue stores the link register to the frame, so a load of the
   return address from the frame may precede that store.  */
/* { dg-final { scan-assembler-not "l.lwz\\s+r\[0-9\]+,\\s*-4\\(r2\\)" } } */
