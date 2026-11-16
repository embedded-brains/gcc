/* Copyright (C) 2013-2026 Free Software Foundation, Inc.
   Contributed by Jakub Jelinek <jakub@redhat.com>.

   This file is part of the GNU Offloading and Multi Processing Library
   (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* This file contains the legacy entry points of the offloading support.
   Current compilers call the GOMP_*_ext entry points and GOMP_teams4.  */

#include "libgomp.h"
#include "gomp-constants.h"
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "target-device.h"

/* Called when encountering a target directive.  If DEVICE
   is GOMP_DEVICE_ICV, it means use device-var ICV.  If it is
   GOMP_DEVICE_HOST_FALLBACK (or any value
   larger than last available hw device), use host fallback.
   FN is address of host code, UNUSED is part of the current ABI, but
   we're not actually using it.  HOSTADDRS, SIZES and KINDS are arrays
   with MAPNUM entries, with addresses of the host objects,
   sizes of the host objects (resp. for pointer kind pointer bias
   and assumed sizeof (void *) size) and kinds.  */

void
GOMP_target (int device, void (*fn) (void *), const void *unused,
	     size_t mapnum, void **hostaddrs, size_t *sizes,
	     unsigned char *kinds)
{
  struct gomp_device_descr *devicep = resolve_device (device, true);

  void *fn_addr;
  if (devicep == NULL
      || !(devicep->capabilities & GOMP_OFFLOAD_CAP_OPENMP_400)
      /* All shared memory devices should use the GOMP_target_ext function.  */
      || devicep->capabilities & GOMP_OFFLOAD_CAP_SHARED_MEM
      || !(fn_addr = gomp_get_target_fn_addr (devicep, fn)))
    return gomp_target_fallback (fn, hostaddrs, devicep, NULL);

  gomp_target_run (devicep, fn_addr, mapnum, hostaddrs, sizes, kinds);
}

void
GOMP_target_data (int device, const void *unused, size_t mapnum,
		  void **hostaddrs, size_t *sizes, unsigned char *kinds)
{
  struct gomp_device_descr *devicep = resolve_device (device, true);

  if (devicep == NULL
      || !(devicep->capabilities & GOMP_OFFLOAD_CAP_OPENMP_400)
      || (devicep->capabilities & GOMP_OFFLOAD_CAP_SHARED_MEM))
    return gomp_target_data_fallback (devicep);

  struct target_mem_desc *tgt
    = gomp_map_vars (devicep, mapnum, hostaddrs, NULL, sizes, kinds, false,
		     NULL, GOMP_MAP_VARS_DATA);
  struct gomp_task_icv *icv = gomp_icv (true);
  tgt->prev = icv->target_data;
  icv->target_data = tgt;
}

void
GOMP_target_update (int device, const void *unused, size_t mapnum,
		    void **hostaddrs, size_t *sizes, unsigned char *kinds)
{
  struct gomp_device_descr *devicep = resolve_device (device, true);

  if (devicep == NULL
      || !(devicep->capabilities & GOMP_OFFLOAD_CAP_OPENMP_400)
      || devicep->capabilities & GOMP_OFFLOAD_CAP_SHARED_MEM)
    return;

  gomp_update (devicep, mapnum, hostaddrs, sizes, kinds, false);
}

/* Implement OpenMP 'teams' construct, legacy entry point.  */

void
GOMP_teams (unsigned int num_teams, unsigned int thread_limit)
{
  if (thread_limit)
    {
      struct gomp_task_icv *icv = gomp_icv (true);
      icv->thread_limit_var
	= thread_limit > INT_MAX ? UINT_MAX : thread_limit;
    }
  (void) num_teams;
}
