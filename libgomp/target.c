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

/* This file contains the host part of the offloading support.  The device
   part is in target-device.c.  */

#include "libgomp.h"
#include "oacc-plugin.h"
#include "oacc-int.h"
#include "gomp-constants.h"
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>  /* For PRIu64.  */
#endif
#include <string.h>
#include <stdio.h>  /* For snprintf. */
#include <assert.h>
#include <errno.h>
#include "target-device.h"

#ifdef PLUGIN_SUPPORT
#include <dlfcn.h>
#include "plugin-suffix.h"
#endif


/* Used by omp_get_device_from_uid / omp_get_uid_from_device for the host.  */
static char *str_omp_initial_device = "OMP_INITIAL_DEVICE";


ialias_redirect (GOMP_task)


/* The whole initialization code for offloading plugins is only run one.  */
static pthread_once_t gomp_is_initialized = PTHREAD_ONCE_INIT;

/* Array of descriptors for all available devices.  */
struct gomp_device_descr *devices;

/* Number of GOMP_OFFLOAD_CAP_OPENMP_400 devices.  */
int num_devices_openmp;

attribute_hidden void
gomp_init_targets_once (void)
{
  (void) pthread_once (&gomp_is_initialized, gomp_target_init);
}

attribute_hidden int
gomp_get_num_devices (void)
{
  gomp_init_targets_once ();
  return num_devices_openmp;
}

static int
gomp_get_default_device ()
{
  gomp_init_targets_once ();
  struct gomp_task_icv *icv = gomp_icv (false);
  return icv->default_device_var;
}

attribute_hidden struct gomp_device_descr *
resolve_device (int device_id, bool remapped)
{
  /* Get number of devices and thus ensure that 'gomp_init_targets_once' was
     called, which must be done before using default_device_var.  */
  int num_devices = gomp_get_num_devices ();

  if ((remapped && device_id == GOMP_DEVICE_ICV)
      || device_id == GOMP_DEVICE_DEFAULT_OMP_61)
    device_id = gomp_get_default_device ();

  if (device_id < 0)
    {
      if (device_id == (remapped ? GOMP_DEVICE_HOST_FALLBACK
				 : omp_initial_device))
	return NULL;
      if (gomp_target_offload_var == GOMP_TARGET_OFFLOAD_MANDATORY
	  && num_devices == 0)
	gomp_fatal ("OMP_TARGET_OFFLOAD is set to MANDATORY, "
		    "but only the host device is available");
      else if (device_id == omp_invalid_device)
	gomp_fatal ("omp_invalid_device encountered");
      else if (gomp_target_offload_var == GOMP_TARGET_OFFLOAD_MANDATORY)
	gomp_fatal ("OMP_TARGET_OFFLOAD is set to MANDATORY, "
		    "but device not found");

      return NULL;
    }
  else if (device_id >= num_devices)
    {
      if (gomp_target_offload_var == GOMP_TARGET_OFFLOAD_MANDATORY
	  && device_id != num_devices)
	gomp_fatal ("OMP_TARGET_OFFLOAD is set to MANDATORY, "
		    "but device not found");

      return NULL;
    }

  return gomp_device_get (device_id);
}



attribute_hidden struct gomp_offload_icv_list *
gomp_get_offload_icv_item (int dev_num)
{
  struct gomp_offload_icv_list *l = gomp_offload_icv_list;
  while (l != NULL && l->device_num != dev_num)
    l = l->next;

  return l;
}


/* Host fallback for GOMP_target{,_ext} routines.  */

static void
gomp_target_fallback (void (*fn) (void *), void **hostaddrs,
		      struct gomp_device_descr *devicep, void **args)
{
  struct gomp_thread old_thr, *thr = gomp_thread ();

  if (gomp_target_offload_var == GOMP_TARGET_OFFLOAD_MANDATORY
      && devicep != NULL)
    gomp_fatal ("OMP_TARGET_OFFLOAD is set to MANDATORY, but device cannot "
		"be used for offloading");

  old_thr = *thr;
  memset (thr, '\0', sizeof (*thr));
  if (gomp_places_list)
    {
      thr->place = old_thr.place;
      thr->ts.place_partition_len = gomp_places_list_len;
    }
  if (args)
    while (*args)
      {
	intptr_t id = (intptr_t) *args++, val;
	if (id & GOMP_TARGET_ARG_SUBSEQUENT_PARAM)
	  val = (intptr_t) *args++;
	else
	  val = id >> GOMP_TARGET_ARG_VALUE_SHIFT;
	if ((id & GOMP_TARGET_ARG_DEVICE_MASK) != GOMP_TARGET_ARG_DEVICE_ALL)
	  continue;
	id &= GOMP_TARGET_ARG_ID_MASK;
	if (id != GOMP_TARGET_ARG_THREAD_LIMIT)
	  continue;
	val = val > INT_MAX ? INT_MAX : val;
	if (val)
	  gomp_icv (true)->thread_limit_var = val;
	break;
      }

  fn (hostaddrs);
  gomp_free_thread (thr);
  *thr = old_thr;
}

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

static inline unsigned int
clear_unsupported_flags (struct gomp_device_descr *devicep, unsigned int flags)
{
  /* If we cannot run asynchronously, simply ignore nowait.  */
  if (devicep != NULL && devicep->async_run_func == NULL)
    flags &= ~GOMP_TARGET_FLAG_NOWAIT;

  return flags;
}


/* Like GOMP_target, but KINDS is 16-bit, UNUSED is no longer present,
   and several arguments have been added:
   FLAGS is a bitmask, see GOMP_TARGET_FLAG_* in gomp-constants.h.
   DEPEND is array of dependencies, see GOMP_task for details.

   ARGS is a pointer to an array consisting of a variable number of both
   device-independent and device-specific arguments, which can take one two
   elements where the first specifies for which device it is intended, the type
   and optionally also the value.  If the value is not present in the first
   one, the whole second element the actual value.  The last element of the
   array is a single NULL.  Among the device independent can be for example
   NUM_TEAMS and THREAD_LIMIT.

   NUM_TEAMS is positive if GOMP_teams will be called in the body with
   that value, or 1 if teams construct is not present, or 0, if
   teams construct does not have num_teams clause and so the choice is
   implementation defined, and -1 if it can't be determined on the host
   what value will GOMP_teams have on the device.
   THREAD_LIMIT similarly is positive if GOMP_teams will be called in the
   body with that value, or 0, if teams construct does not have thread_limit
   clause or the teams construct is not present, or -1 if it can't be
   determined on the host what value will GOMP_teams have on the device.  */

void
GOMP_target_ext (int device, void (*fn) (void *), size_t mapnum,
		 void **hostaddrs, size_t *sizes, unsigned short *kinds,
		 unsigned int flags, void **depend, void **args)
{
  struct gomp_device_descr *devicep = resolve_device (device, true);
  size_t tgt_align = 0, tgt_size = 0;
  bool fpc_done = false;

  /* Obtain the original TEAMS and THREADS values from ARGS.  */
  intptr_t orig_teams = 1, orig_threads = 0;
  size_t num_args = 0, len = 1, teams_len = 1, threads_len = 1;
  void **tmpargs = args;
  while (*tmpargs)
    {
      intptr_t id = (intptr_t) *tmpargs++, val;
      if (id & GOMP_TARGET_ARG_SUBSEQUENT_PARAM)
	{
	  val = (intptr_t) *tmpargs++;
	  len = 2;
	}
      else
	{
	  val = id >> GOMP_TARGET_ARG_VALUE_SHIFT;
	  len = 1;
	}
      num_args += len;
      if ((id & GOMP_TARGET_ARG_DEVICE_MASK) != GOMP_TARGET_ARG_DEVICE_ALL)
	continue;
      val = val > INT_MAX ? INT_MAX : val;
      if ((id & GOMP_TARGET_ARG_ID_MASK) == GOMP_TARGET_ARG_NUM_TEAMS)
	{
	  orig_teams = val;
	  teams_len = len;
	}
      else if ((id & GOMP_TARGET_ARG_ID_MASK) == GOMP_TARGET_ARG_THREAD_LIMIT)
	{
	  orig_threads = val;
	  threads_len = len;
	}
    }

  intptr_t new_teams = orig_teams, new_threads = orig_threads;
  /* ORIG_TEAMS == -2: No explicit teams construct specified.  Set to 1.
     ORIG_TEAMS == -1: TEAMS construct with NUM_TEAMS clause specified, but the
		       value could not be determined.  No change.
     ORIG_TEAMS == 0: TEAMS construct without NUM_TEAMS clause.
		      Set device-specific value.
     ORIG_TEAMS > 0: Value was already set through e.g. NUM_TEAMS clause.
		     No change.  */
  if (orig_teams == -2)
    new_teams = 1;
  else if (orig_teams == 0)
    {
      struct gomp_offload_icv_list *item = gomp_get_offload_icv_item (device);
      if (item != NULL)
	new_teams = item->icvs.nteams;
    }
  /* The device-specific teams-thread-limit is only set if (a) an explicit TEAMS
     region exists, i.e. ORIG_TEAMS > -2, and (b) THREADS was not already set by
     e.g. a THREAD_LIMIT clause.  */
  if (orig_teams > -2 && orig_threads == 0)
    {
      struct gomp_offload_icv_list *item = gomp_get_offload_icv_item (device);
      if (item != NULL)
	new_threads = item->icvs.teams_thread_limit;
    }

  /* Copy and change the arguments list only if TEAMS or THREADS need to be
     updated.  */
  void **new_args = args;
  if (orig_teams != new_teams || orig_threads != new_threads)
    {
      size_t tms_len = (orig_teams == new_teams
			? teams_len
			: (new_teams > -(1 << 15) && new_teams < (1 << 15)
			   ? 1 : 2));
      size_t ths_len = (orig_threads == new_threads
			? threads_len
			: (new_threads > -(1 << 15) && new_threads < (1 << 15)
			   ? 1 : 2));
      /* One additional item after the last arg must be NULL.  */
      size_t new_args_cnt = num_args - teams_len - threads_len + tms_len
			    + ths_len + 1;
      new_args = (void **) gomp_alloca (new_args_cnt * sizeof (void*));

      tmpargs = args;
      void **tmp_new_args = new_args;
      /* Copy all args except TEAMS and THREADS.  TEAMS and THREADS are copied
	 too if they have not been changed and skipped otherwise.  */
      while (*tmpargs)
	{
	  intptr_t id = (intptr_t) *tmpargs;
	  if (((id & GOMP_TARGET_ARG_ID_MASK) == GOMP_TARGET_ARG_NUM_TEAMS
	       && orig_teams != new_teams)
	      || ((id & GOMP_TARGET_ARG_ID_MASK) == GOMP_TARGET_ARG_THREAD_LIMIT
		  && orig_threads != new_threads))
	    {
	      tmpargs++;
	      if (id & GOMP_TARGET_ARG_SUBSEQUENT_PARAM)
		tmpargs++;
	    }
	  else
	    {
	      *tmp_new_args++ = *tmpargs++;
	      if (id & GOMP_TARGET_ARG_SUBSEQUENT_PARAM)
		*tmp_new_args++ = *tmpargs++;
	    }
	}

      /* Add the new TEAMS arg to the new args list if it has been changed.  */
      if (orig_teams != new_teams)
	{
	  intptr_t new_val = new_teams;
	  if (tms_len == 1)
	    {
	      new_val = (new_val << GOMP_TARGET_ARG_VALUE_SHIFT)
			 | GOMP_TARGET_ARG_NUM_TEAMS;
	      *tmp_new_args++ = (void *) new_val;
	    }
	  else
	    {
	      *tmp_new_args++ = (void *) (GOMP_TARGET_ARG_SUBSEQUENT_PARAM
					  | GOMP_TARGET_ARG_NUM_TEAMS);
	      *tmp_new_args++ = (void *) new_val;
	    }
	}

      /* Add the new THREADS arg to the new args list if it has been changed. */
      if (orig_threads != new_threads)
	{
	  intptr_t new_val = new_threads;
	  if (ths_len == 1)
	    {
	      new_val = (new_val << GOMP_TARGET_ARG_VALUE_SHIFT)
			 | GOMP_TARGET_ARG_THREAD_LIMIT;
	      *tmp_new_args++ = (void *) new_val;
	    }
	  else
	    {
	      *tmp_new_args++ = (void *) (GOMP_TARGET_ARG_SUBSEQUENT_PARAM
					  | GOMP_TARGET_ARG_THREAD_LIMIT);
	      *tmp_new_args++ = (void *) new_val;
	    }
	}

      *tmp_new_args = NULL;
    }

  flags = clear_unsupported_flags (devicep, flags);

  if (flags & GOMP_TARGET_FLAG_NOWAIT)
    {
      struct gomp_thread *thr = gomp_thread ();
      /* Create a team if we don't have any around, as nowait
	 target tasks make sense to run asynchronously even when
	 outside of any parallel.  */
      if (__builtin_expect (thr->ts.team == NULL, 0))
	{
	  struct gomp_team *team = gomp_new_team (1);
	  struct gomp_task *task = thr->task;
	  struct gomp_task **implicit_task = &task;
	  struct gomp_task_icv *icv = task ? &task->icv : &gomp_global_icv;
	  team->prev_ts = thr->ts;
	  thr->ts.team = team;
	  thr->ts.team_id = 0;
	  thr->ts.work_share = &team->work_shares[0];
	  thr->ts.last_work_share = NULL;
#ifdef HAVE_SYNC_BUILTINS
	  thr->ts.single_count = 0;
#endif
	  thr->ts.static_trip = 0;
	  thr->task = &team->implicit_task[0];
	  gomp_init_task (thr->task, NULL, icv);
	  while (*implicit_task
		 && (*implicit_task)->kind != GOMP_TASK_IMPLICIT)
	    implicit_task = &(*implicit_task)->parent;
	  if (*implicit_task)
	    {
	      thr->task = *implicit_task;
	      gomp_end_task ();
	      free (*implicit_task);
	      thr->task = &team->implicit_task[0];
	    }
	  else
	    pthread_setspecific (gomp_thread_destructor, thr);
	  if (implicit_task != &task)
	    {
	      *implicit_task = thr->task;
	      thr->task = task;
	    }
	}
      if (thr->ts.team
	  && !thr->task->final_task)
	{
	  gomp_create_target_task (devicep, fn, mapnum, hostaddrs,
				   sizes, kinds, flags, depend, new_args,
				   GOMP_TARGET_TASK_BEFORE_MAP);
	  return;
	}
    }

  /* If there are depend clauses, but nowait is not present
     (or we are in a final task), block the parent task until the
     dependencies are resolved and then just continue with the rest
     of the function as if it is a merged task.  */
  if (depend != NULL)
    {
      struct gomp_thread *thr = gomp_thread ();
      if (thr->task && thr->task->depend_hash)
	{
	  /* If we might need to wait, copy firstprivate now.  */
	  calculate_firstprivate_requirements (mapnum, sizes, kinds,
					       &tgt_align, &tgt_size);
	  if (tgt_align)
	    {
	      char *tgt = gomp_alloca (tgt_size + tgt_align - 1);
	      copy_firstprivate_data (tgt, mapnum, hostaddrs, sizes, kinds,
				      tgt_align, tgt_size);
	    }
	  fpc_done = true;
	  gomp_task_maybe_wait_for_dependencies (depend);
	}
    }

  void *fn_addr;
  if (devicep == NULL || !(fn_addr = gomp_device_fn_addr (devicep, fn)))
    {
      if (!fpc_done)
	{
	  calculate_firstprivate_requirements (mapnum, sizes, kinds,
					       &tgt_align, &tgt_size);
	  if (tgt_align)
	    {
	      char *tgt = gomp_alloca (tgt_size + tgt_align - 1);
	      copy_firstprivate_data (tgt, mapnum, hostaddrs, sizes, kinds,
				      tgt_align, tgt_size);
	    }
	}
      gomp_target_fallback (fn, hostaddrs, devicep, new_args);
      return;
    }

  gomp_target_ext_run (devicep, fn_addr, mapnum, hostaddrs, sizes, kinds,
		       fpc_done, tgt_align, tgt_size, new_args, device);

}


/* Reverse lookup (device addr -> host addr) for reverse offload.  We avoid
   keeping track of all variable handling - assuming that reverse offload occurs
   ony very rarely.  Downside is that the reverse search is slow.  */


/* Host fallback for GOMP_target_data{,_ext} routines.  */

static void
gomp_target_data_fallback (struct gomp_device_descr *devicep)
{
  struct gomp_task_icv *icv = gomp_icv (false);

  if (gomp_target_offload_var == GOMP_TARGET_OFFLOAD_MANDATORY
      && devicep != NULL)
    gomp_fatal ("OMP_TARGET_OFFLOAD is set to MANDATORY, but device cannot "
		"be used for offloading");

  if (icv->target_data)
    gomp_target_data_push_empty (icv);
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
GOMP_target_data_ext (int device, size_t mapnum, void **hostaddrs,
		      size_t *sizes, unsigned short *kinds)
{
  struct gomp_device_descr *devicep = resolve_device (device, true);

  if (devicep == NULL || !gomp_device_maps_data (devicep))
    return gomp_target_data_fallback (devicep);

  gomp_target_data_device (devicep, mapnum, hostaddrs, sizes, kinds, true);
}

void
GOMP_target_end_data (void)
{
  struct gomp_task_icv *icv = gomp_icv (false);
  if (icv->target_data)
    gomp_target_data_pop (icv);
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

void
GOMP_target_update_ext (int device, size_t mapnum, void **hostaddrs,
			size_t *sizes, unsigned short *kinds,
			unsigned int flags, void **depend)
{
  struct gomp_device_descr *devicep = resolve_device (device, true);

  /* If there are depend clauses, but nowait is not present,
     block the parent task until the dependencies are resolved
     and then just continue with the rest of the function as if it
     is a merged task.  Until we are able to schedule task during
     variable mapping or unmapping, ignore nowait if depend clauses
     are not present.  */
  if (depend != NULL)
    {
      struct gomp_thread *thr = gomp_thread ();
      if (thr->task && thr->task->depend_hash)
	{
	  if ((flags & GOMP_TARGET_FLAG_NOWAIT)
	      && thr->ts.team
	      && !thr->task->final_task)
	    {
	      if (gomp_create_target_task (devicep, (void (*) (void *)) NULL,
					   mapnum, hostaddrs, sizes, kinds,
					   flags | GOMP_TARGET_FLAG_UPDATE,
					   depend, NULL, GOMP_TARGET_TASK_DATA))
		return;
	    }
	  else
	    {
	      struct gomp_team *team = thr->ts.team;
	      /* If parallel or taskgroup has been cancelled, don't start new
		 tasks.  */
	      if (__builtin_expect (gomp_cancel_var, 0) && team)
		{
		  if (gomp_team_barrier_cancelled (&team->barrier))
		    return;
		  if (thr->task->taskgroup)
		    {
		      if (thr->task->taskgroup->cancelled)
			return;
		      if (thr->task->taskgroup->workshare
			  && thr->task->taskgroup->prev
			  && thr->task->taskgroup->prev->cancelled)
			return;
		    }
		}

	      gomp_task_maybe_wait_for_dependencies (depend);
	    }
	}
    }

  if (devicep == NULL || !gomp_device_maps_data (devicep))
    return;

  gomp_target_update_device (devicep, mapnum, hostaddrs, sizes, kinds);
}


void
GOMP_target_enter_exit_data (int device, size_t mapnum, void **hostaddrs,
			     size_t *sizes, unsigned short *kinds,
			     unsigned int flags, void **depend)
{
  struct gomp_device_descr *devicep = resolve_device (device, true);

  /* If there are depend clauses, but nowait is not present,
     block the parent task until the dependencies are resolved
     and then just continue with the rest of the function as if it
     is a merged task.  Until we are able to schedule task during
     variable mapping or unmapping, ignore nowait if depend clauses
     are not present.  */
  if (depend != NULL)
    {
      struct gomp_thread *thr = gomp_thread ();
      if (thr->task && thr->task->depend_hash)
	{
	  if ((flags & GOMP_TARGET_FLAG_NOWAIT)
	      && thr->ts.team
	      && !thr->task->final_task)
	    {
	      if (gomp_create_target_task (devicep, (void (*) (void *)) NULL,
					   mapnum, hostaddrs, sizes, kinds,
					   flags, depend, NULL,
					   GOMP_TARGET_TASK_DATA))
		return;
	    }
	  else
	    {
	      struct gomp_team *team = thr->ts.team;
	      /* If parallel or taskgroup has been cancelled, don't start new
		 tasks.  */
	      if (__builtin_expect (gomp_cancel_var, 0) && team)
		{
		  if (gomp_team_barrier_cancelled (&team->barrier))
		    return;
		  if (thr->task->taskgroup)
		    {
		      if (thr->task->taskgroup->cancelled)
			return;
		      if (thr->task->taskgroup->workshare
			  && thr->task->taskgroup->prev
			  && thr->task->taskgroup->prev->cancelled)
			return;
		    }
		}

	      gomp_task_maybe_wait_for_dependencies (depend);
	    }
	}
    }

  if (devicep == NULL || !gomp_device_maps_data (devicep))
    return;

  gomp_enter_exit_data (devicep, mapnum, hostaddrs, sizes, kinds, flags);
}

bool
gomp_target_task_fn (void *data)
{
  struct gomp_target_task *ttask = (struct gomp_target_task *) data;
  struct gomp_device_descr *devicep = ttask->devicep;

  if (ttask->fn != NULL)
    {
      void *fn_addr;
      if (devicep == NULL
	  || !(fn_addr = gomp_device_fn_addr (devicep, ttask->fn)))
	{
	  ttask->state = GOMP_TARGET_TASK_FALLBACK;
	  gomp_target_fallback (ttask->fn, ttask->hostaddrs, devicep,
				ttask->args);
	  return false;
	}

      return gomp_target_task_run (devicep, ttask, fn_addr);
    }
  else if (devicep == NULL || !gomp_device_maps_data (devicep))
    return false;

  gomp_target_task_data (devicep, ttask);
  return false;
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

/* Implement OpenMP 'teams' construct.

   Initialize upon FIRST call.  Return whether this invocation is active.
   Depending on whether NUM_TEAMS_LOW asks for more teams than are provided
   in hardware, we may need to loop multiple times; in that case make sure to
   update the team-level variable used by 'omp_get_team_num'.  */

bool
GOMP_teams4 (unsigned int num_teams_low, unsigned int num_teams_high,
	     unsigned int thread_limit, bool first)
{
  struct gomp_thread *thr = gomp_thread ();
  if (first)
    {
      if (thread_limit)
	{
	  struct gomp_task_icv *icv = gomp_icv (true);
	  icv->thread_limit_var
	    = thread_limit > INT_MAX ? UINT_MAX : thread_limit;
	}
      (void) num_teams_high;
      if (num_teams_low == 0)
	num_teams_low = 1;
      thr->num_teams = num_teams_low - 1;
      thr->team_num = 0;
    }
  else if (thr->team_num == thr->num_teams)
    return false;
  else
    ++thr->team_num;
  return true;
}

void *
omp_target_alloc (size_t size, int device_num)
{
  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  if (device_num == omp_initial_device
      || device_num == gomp_get_num_devices ())
    return malloc (size);

  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL)
    return NULL;

  return gomp_device_alloc (devicep, size);
}

void
omp_target_free (void *device_ptr, int device_num)
{
  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  if (device_num == omp_initial_device
      || device_num == gomp_get_num_devices ())
    {
      free (device_ptr);
      return;
    }

  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL || device_ptr == NULL)
    return;

  gomp_device_free (devicep, device_ptr);
}


/* Check whether corresponding storage exists on the device.
   - NULL pointer or invalid device: return 0
   - host device: return 1
   - Has corresponding storage: return 1
   - Otherwise: return 0

   Note that for GOMP_OFFLOAD_CAP_SHARED_MEM self mapping is used and
   omp_target_associate_ptr is disabled; the only corresponding storage
   exists then for declare_target with other clauses than an explicit or
   implicit 'link' clause.
   However, the link cause with shared memory does not count as mapped.  */

int
omp_target_is_present (const void *ptr, int device_num)
{
  if (ptr == NULL)
    return 0;

  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  if (device_num == omp_initial_device
      || device_num == gomp_get_num_devices ())
    return 1;

  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL)
    return 0;

  return gomp_device_is_present (devicep, ptr);
}

static int
omp_target_memcpy_check (int dst_device_num, int src_device_num,
			 struct gomp_device_descr **dst_devicep,
			 struct gomp_device_descr **src_devicep)
{
  if (dst_device_num == omp_default_device)
    dst_device_num = gomp_get_default_device ();
  if (src_device_num == omp_default_device)
    src_device_num = gomp_get_default_device ();

  if (dst_device_num != gomp_get_num_devices ()
      /* Above gomp_get_num_devices has to be called unconditionally.  */
      && dst_device_num != omp_initial_device)
    {
      *dst_devicep = resolve_device (dst_device_num, false);
      if (*dst_devicep == NULL)
	return EINVAL;

      if (!gomp_device_maps_data (*dst_devicep))
	*dst_devicep = NULL;
    }

  if (src_device_num != num_devices_openmp
      && src_device_num != omp_initial_device)
    {
      *src_devicep = resolve_device (src_device_num, false);
      if (*src_devicep == NULL)
	return EINVAL;

      if (!gomp_device_maps_data (*src_devicep))
	*src_devicep = NULL;
    }

  return 0;
}

static int
omp_target_memcpy_copy (void *dst, const void *src, size_t length,
			size_t dst_offset, size_t src_offset,
			struct gomp_device_descr *dst_devicep,
			struct gomp_device_descr *src_devicep)
{
  if (src_devicep == NULL && dst_devicep == NULL)
    {
      memcpy ((char *) dst + dst_offset, (char *) src + src_offset, length);
      return 0;
    }
  return gomp_device_memcpy (dst, src, length, dst_offset, src_offset,
			     dst_devicep, src_devicep);
}

int
omp_target_memcpy (void *dst, const void *src, size_t length, size_t dst_offset,
		   size_t src_offset, int dst_device_num, int src_device_num)
{
  struct gomp_device_descr *dst_devicep = NULL, *src_devicep = NULL;
  int ret = omp_target_memcpy_check (dst_device_num, src_device_num,
				     &dst_devicep, &src_devicep);

  if (ret)
    return ret;

  ret = omp_target_memcpy_copy (dst, src, length, dst_offset, src_offset,
				dst_devicep, src_devicep);

  return ret;
}

typedef struct
{
  void *dst;
  const void *src;
  size_t length;
  size_t dst_offset;
  size_t src_offset;
  struct gomp_device_descr *dst_devicep;
  struct gomp_device_descr *src_devicep;
} omp_target_memcpy_data;

static void
omp_target_memcpy_async_helper (void *args)
{
  omp_target_memcpy_data *a = args;
  if (omp_target_memcpy_copy (a->dst, a->src, a->length, a->dst_offset,
			      a->src_offset, a->dst_devicep, a->src_devicep))
    gomp_fatal ("omp_target_memcpy failed");
}

int
omp_target_memcpy_async (void *dst, const void *src, size_t length,
			 size_t dst_offset, size_t src_offset,
			 int dst_device_num, int src_device_num,
			 int depobj_count, omp_depend_t *depobj_list)
{
  struct gomp_device_descr *dst_devicep = NULL, *src_devicep = NULL;
  unsigned int flags = 0;
  void *depend[depobj_count + 5];
  int i;
  int check = omp_target_memcpy_check (dst_device_num, src_device_num,
				       &dst_devicep, &src_devicep);

  omp_target_memcpy_data s = {
    .dst = dst,
    .src = src,
    .length = length,
    .dst_offset = dst_offset,
    .src_offset = src_offset,
    .dst_devicep = dst_devicep,
    .src_devicep = src_devicep
  };

  if (check)
    return check;

  if (depobj_count > 0 && depobj_list != NULL)
    {
      flags |= GOMP_TASK_FLAG_DEPEND;
      depend[0] = 0;
      depend[1] = (void *) (uintptr_t) depobj_count;
      depend[2] = depend[3] = depend[4] = 0;
      for (i = 0; i < depobj_count; ++i)
	depend[i + 5] = &depobj_list[i];
    }

  GOMP_task (omp_target_memcpy_async_helper, &s, NULL, sizeof (s),
	     __alignof__ (s), true, flags, depend, 0, NULL);

  return 0;
}

static int
omp_target_memcpy_rect_worker (void *dst, const void *src, size_t element_size,
			       int num_dims, const size_t *volume,
			       const size_t *dst_offsets,
			       const size_t *src_offsets,
			       const size_t *dst_dimensions,
			       const size_t *src_dimensions,
			       struct gomp_device_descr *dst_devicep,
			       struct gomp_device_descr *src_devicep,
			       size_t *tmp_size, void **tmp)
{
  size_t dst_slice = element_size;
  size_t src_slice = element_size;
  size_t j, dst_off, src_off, length;
  int i, ret;

  if (num_dims == 1)
    {
      if (__builtin_mul_overflow (element_size, volume[0], &length)
	  || __builtin_mul_overflow (element_size, dst_offsets[0], &dst_off)
	  || __builtin_mul_overflow (element_size, src_offsets[0], &src_off))
	return EINVAL;
      if (dst_devicep == NULL && src_devicep == NULL)
	{
	  memcpy ((char *) dst + dst_off, (const char *) src + src_off,
		  length);
	  return 0;
	}
      return gomp_device_memcpy_line (dst, src, dst_off, src_off, length,
				      dst_devicep, src_devicep, tmp_size, tmp);
    }

  /* host->device, device->host and intra device.  */
  if (dst_devicep != NULL || src_devicep != NULL)
    {
      ret = gomp_device_memcpy_rect (dst, src, element_size, num_dims, volume,
				     dst_offsets, src_offsets, dst_dimensions,
				     src_dimensions, dst_devicep, src_devicep);
      if (ret != -1)
	return ret;
    }

  for (i = 1; i < num_dims; i++)
    if (__builtin_mul_overflow (dst_slice, dst_dimensions[i], &dst_slice)
	|| __builtin_mul_overflow (src_slice, src_dimensions[i], &src_slice))
      return EINVAL;
  if (__builtin_mul_overflow (dst_slice, dst_offsets[0], &dst_off)
      || __builtin_mul_overflow (src_slice, src_offsets[0], &src_off))
    return EINVAL;
  for (j = 0; j < volume[0]; j++)
    {
      ret = omp_target_memcpy_rect_worker ((char *) dst + dst_off,
					   (const char *) src + src_off,
					   element_size, num_dims - 1,
					   volume + 1, dst_offsets + 1,
					   src_offsets + 1, dst_dimensions + 1,
					   src_dimensions + 1, dst_devicep,
					   src_devicep, tmp_size, tmp);
      if (ret)
	return ret;
      dst_off += dst_slice;
      src_off += src_slice;
    }
  return 0;
}

static int
omp_target_memcpy_rect_check (void *dst, const void *src, int dst_device_num,
			      int src_device_num,
			      struct gomp_device_descr **dst_devicep,
			      struct gomp_device_descr **src_devicep)
{
  if (!dst && !src)
    return INT_MAX;

  int ret = omp_target_memcpy_check (dst_device_num, src_device_num,
				     dst_devicep, src_devicep);
  if (ret)
    return ret;

  return 0;
}

static int
omp_target_memcpy_rect_copy (void *dst, const void *src,
			     size_t element_size, int num_dims,
			     const size_t *volume, const size_t *dst_offsets,
			     const size_t *src_offsets,
			     const size_t *dst_dimensions,
			     const size_t *src_dimensions,
			     struct gomp_device_descr *dst_devicep,
			     struct gomp_device_descr *src_devicep)
{
  size_t tmp_size = 0;
  void *tmp = NULL;
  bool with_device = dst_devicep != NULL || src_devicep != NULL;

  if (with_device)
    gomp_device_lock_pair (dst_devicep, src_devicep);
  int ret = omp_target_memcpy_rect_worker (dst, src, element_size, num_dims,
					   volume, dst_offsets, src_offsets,
					   dst_dimensions, src_dimensions,
					   dst_devicep, src_devicep,
					   &tmp_size, &tmp);
  if (with_device)
    gomp_device_unlock_pair (dst_devicep, src_devicep);
  if (tmp)
    free (tmp);

  return ret;
}

int
omp_target_memcpy_rect (void *dst, const void *src, size_t element_size,
			int num_dims, const size_t *volume,
			const size_t *dst_offsets,
			const size_t *src_offsets,
			const size_t *dst_dimensions,
			const size_t *src_dimensions,
			int dst_device_num, int src_device_num)
{
  struct gomp_device_descr *dst_devicep = NULL, *src_devicep = NULL;

  int check = omp_target_memcpy_rect_check (dst, src, dst_device_num,
					    src_device_num, &dst_devicep,
					    &src_devicep);

  if (check)
    return check;

  int ret = omp_target_memcpy_rect_copy (dst, src, element_size, num_dims,
					 volume, dst_offsets, src_offsets,
					 dst_dimensions, src_dimensions,
					 dst_devicep, src_devicep);

  return ret;
}

typedef struct
{
  void *dst;
  const void *src;
  size_t element_size;
  const size_t *volume;
  const size_t *dst_offsets;
  const size_t *src_offsets;
  const size_t *dst_dimensions;
  const size_t *src_dimensions;
  struct gomp_device_descr *dst_devicep;
  struct gomp_device_descr *src_devicep;
  int num_dims;
} omp_target_memcpy_rect_data;

static void
omp_target_memcpy_rect_async_helper (void *args)
{
  omp_target_memcpy_rect_data *a = args;
  int ret = omp_target_memcpy_rect_copy (a->dst, a->src, a->element_size,
					 a->num_dims, a->volume, a->dst_offsets,
					 a->src_offsets, a->dst_dimensions,
					 a->src_dimensions, a->dst_devicep,
					 a->src_devicep);
  if (ret)
    gomp_fatal ("omp_target_memcpy_rect failed");
}

int
omp_target_memcpy_rect_async (void *dst, const void *src, size_t element_size,
			      int num_dims, const size_t *volume,
			      const size_t *dst_offsets,
			      const size_t *src_offsets,
			      const size_t *dst_dimensions,
			      const size_t *src_dimensions,
			      int dst_device_num, int src_device_num,
			      int depobj_count, omp_depend_t *depobj_list)
{
  struct gomp_device_descr *dst_devicep = NULL, *src_devicep = NULL;
  unsigned flags = 0;
  int check = omp_target_memcpy_rect_check (dst, src, dst_device_num,
					    src_device_num, &dst_devicep,
					    &src_devicep);
  void *depend[depobj_count + 5];
  int i;

  omp_target_memcpy_rect_data s = {
    .dst = dst,
    .src = src,
    .element_size = element_size,
    .num_dims = num_dims,
    .volume = volume,
    .dst_offsets = dst_offsets,
    .src_offsets = src_offsets,
    .dst_dimensions = dst_dimensions,
    .src_dimensions = src_dimensions,
    .dst_devicep = dst_devicep,
    .src_devicep = src_devicep
  };

  if (check)
    return check;

  if (depobj_count > 0 && depobj_list != NULL)
    {
      flags |= GOMP_TASK_FLAG_DEPEND;
      depend[0] = 0;
      depend[1] = (void *) (uintptr_t) depobj_count;
      depend[2] = depend[3] = depend[4] = 0;
      for (i = 0; i < depobj_count; ++i)
	depend[i + 5] = &depobj_list[i];
    }

  GOMP_task (omp_target_memcpy_rect_async_helper, &s, NULL, sizeof (s),
	     __alignof__ (s), true, flags, depend, 0, NULL);

  return 0;
}

static void
omp_target_memset_int (void *ptr, int val, size_t count,
		       struct gomp_device_descr *devicep)
{
  if (__builtin_expect (count == 0, 0))
    return;
  if (devicep == NULL)
    {
      memset (ptr, val, count);
      return;
    }

  gomp_device_memset (devicep, ptr, val, count);
}

void*
omp_target_memset (void *ptr, int val, size_t count, int device_num)
{
  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  struct gomp_device_descr *devicep;
  if (device_num == omp_initial_device
      || device_num == gomp_get_num_devices ()
      || (devicep = resolve_device (device_num, false)) == NULL
      || !gomp_device_maps_data (devicep))
    devicep = NULL;

  omp_target_memset_int (ptr, val, count, devicep);
  return ptr;
}

typedef struct
{
  void *ptr;
  size_t count;
  struct gomp_device_descr *devicep;
  int val;
} omp_target_memset_data;

static void
omp_target_memset_async_helper (void *args)
{
  omp_target_memset_data *a = args;
  omp_target_memset_int (a->ptr, a->val, a->count, a->devicep);
}

void*
omp_target_memset_async (void *ptr, int val, size_t count, int device_num,
			 int depobj_count, omp_depend_t *depobj_list)
{
  void *depend[depobj_count + 5];
  struct gomp_device_descr *devicep;
  unsigned flags = 0;
  int i;

  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  if (device_num == omp_initial_device
      || device_num == gomp_get_num_devices ()
      || (devicep = resolve_device (device_num, false)) == NULL
      || !gomp_device_maps_data (devicep))
    devicep = NULL;

  omp_target_memset_data s = {.ptr = ptr, .val = val, .count = count,
			      .devicep = devicep};
  if (depobj_count > 0 && depobj_list != NULL)
    {
      flags |= GOMP_TASK_FLAG_DEPEND;
      depend[0] = 0;
      depend[1] = (void *) (uintptr_t) depobj_count;
      depend[2] = depend[3] = depend[4] = 0;
      for (i = 0; i < depobj_count; ++i)
	depend[i + 5] = &depobj_list[i];
    }

  GOMP_task (omp_target_memset_async_helper, &s, NULL, sizeof (s),
	     __alignof__ (s), true, flags, depend, 0, NULL);
  return ptr;
}

int
omp_target_associate_ptr (const void *host_ptr, const void *device_ptr,
			  size_t size, size_t device_offset, int device_num)
{
  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  if (device_num == omp_initial_device
      || device_num == gomp_get_num_devices ())
    return EINVAL;

  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL)
    return EINVAL;

  return gomp_device_associate_ptr (devicep, host_ptr, device_ptr, size,
				    device_offset);
}

int
omp_target_disassociate_ptr (const void *ptr, int device_num)
{
  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL)
    return EINVAL;

  return gomp_device_disassociate_ptr (devicep, ptr);
}

void *
omp_get_mapped_ptr (const void *ptr, int device_num)
{
  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  if (ptr == NULL
      || device_num == omp_initial_device
      || device_num == omp_get_initial_device ())
    return (void *) ptr;

  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL)
    return NULL;

  return gomp_device_get_mapped_ptr (devicep, ptr);
}

int
omp_target_is_accessible (const void *ptr, size_t size, int device_num)
{
  if (ptr == NULL || size == 0)
    return false;

  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  if (device_num == omp_initial_device
      || device_num == gomp_get_num_devices ())
    return true;

  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL)
    return false;

  return gomp_device_is_accessible (devicep, ptr, size);
}

int
omp_pause_resource (omp_pause_resource_t kind, int device_num)
{
  (void) kind;
  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();
  if (device_num == omp_initial_device
      || device_num == gomp_get_num_devices ())
    return gomp_pause_host ();

  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL)
    return -1;

  /* Do nothing for target devices for now.  */
  return 0;
}

int
omp_pause_resource_all (omp_pause_resource_t kind)
{
  (void) kind;
  if (gomp_pause_host ())
    return -1;
  /* Do nothing for target devices for now.  */
  return 0;
}

ialias (omp_pause_resource)
ialias (omp_pause_resource_all)

int
omp_get_num_interop_properties (const omp_interop_t interop
				__attribute__ ((unused)))
{
  /* Zero implementation defined. In total:
     omp_get_num_interop_properties () - omp_ipr_first.  */
  return 0;
}

omp_intptr_t
omp_get_interop_int (const omp_interop_t interop,
		     omp_interop_property_t property_id,
		     omp_interop_rc_t *ret_code)
{
  struct interop_obj_t *obj = (struct interop_obj_t *) interop;
  struct gomp_device_descr *devicep;

  if (property_id < omp_ipr_first || property_id >= 0)
    {
      if (ret_code)
	*ret_code = omp_irc_out_of_range;
      return 0;
    }
  if (obj == NULL
      || (devicep = resolve_device (obj->device_num, false)) == NULL)
    {
      if (ret_code)
	*ret_code = omp_irc_empty;  /* Assume omp_interop_none.  */
      return 0;
    }
  return gomp_device_get_interop_int (devicep, obj, property_id, ret_code);
}

void *
omp_get_interop_ptr (const omp_interop_t interop,
		     omp_interop_property_t property_id,
		     omp_interop_rc_t *ret_code)
{
  struct interop_obj_t *obj = (struct interop_obj_t *) interop;
  struct gomp_device_descr *devicep;

  if (property_id < omp_ipr_first || property_id >= 0)
    {
      if (ret_code)
	*ret_code = omp_irc_out_of_range;
      return 0;
    }
  if (obj == NULL
      || (devicep = resolve_device (obj->device_num, false)) == NULL)
    {
      if (ret_code)
	*ret_code = omp_irc_empty;  /* Assume omp_interop_none.  */
      return 0;
    }
  return gomp_device_get_interop_ptr (devicep, obj, property_id, ret_code);
}

const char *
omp_get_interop_str (const omp_interop_t interop,
		     omp_interop_property_t property_id,
		     omp_interop_rc_t *ret_code)
{
  struct interop_obj_t *obj = (struct interop_obj_t *) interop;
  struct gomp_device_descr *devicep;

  if (property_id < omp_ipr_first || property_id >= 0)
    {
      if (ret_code)
	*ret_code = omp_irc_out_of_range;
      return 0;
    }
  if (obj == NULL
      || (devicep = resolve_device (obj->device_num, false)) == NULL)
    {
      if (ret_code)
	*ret_code = omp_irc_empty;  /* Assume omp_interop_none.  */
      return 0;
    }
  return gomp_device_get_interop_str (devicep, obj, property_id, ret_code);
}

const char *
omp_get_interop_name (const omp_interop_t interop __attribute__ ((unused)),
		      omp_interop_property_t property_id)
{
  static const char *prop_string[0 - omp_ipr_first]
    = {"fr_id", "fr_name", "vendor", "vendor_name", "device_num", "platform",
       "device", "device_context", "targetsync"};
  if (property_id < omp_ipr_first || property_id >= 0)
    return NULL;
  return prop_string[omp_ipr_fr_id - property_id];
}

const char *
omp_get_interop_type_desc (const omp_interop_t interop,
			   omp_interop_property_t property_id)
{
  struct interop_obj_t *obj = (struct interop_obj_t *) interop;
  struct gomp_device_descr *devicep;

  if (property_id > omp_ipr_fr_id || property_id < omp_ipr_first)
    return NULL;
  if (obj == NULL
      || (devicep = resolve_device (obj->device_num, false)) == NULL)
    return NULL;
  return gomp_device_get_interop_type_desc (devicep, obj, property_id);
}

const char *
omp_get_interop_rc_desc (const omp_interop_t interop __attribute__ ((unused)),
			 omp_interop_rc_t ret_code)
{
  static const char *rc_strings[omp_irc_no_value - omp_irc_other]
    = {"no meaningful value available",
       "successful",
       "provided interoperability object is equal to omp_interop_none",
       "property ID is out of range",
       "property type is integer; use omp_get_interop_int",
       "property type is pointer; use omp_get_interop_ptr",
       "property type is string; use omp_get_interop_str"};

  /* omp_irc_other is returned by device-side omp_get_interop_{int,ptr,str};
     on the host it is not used, hence, return NULL here.   */
  if (ret_code > omp_irc_no_value || ret_code <= omp_irc_other)
    return NULL;
  return rc_strings[omp_irc_no_value - ret_code];
}

ialias (omp_get_num_interop_properties)
ialias (omp_get_interop_int)
ialias (omp_get_interop_ptr)
ialias (omp_get_interop_str)
ialias (omp_get_interop_name)
ialias (omp_get_interop_type_desc)
ialias (omp_get_interop_rc_desc)

struct interop_data_t
{
  int device_num, n_init, n_use, n_destroy;
  struct interop_obj_t ***init;
  struct interop_obj_t **use;
  struct interop_obj_t ***destroy;
  const int *target_targetsync;
  const char **prefer_type;
};

static void
gomp_interop_internal (void *data)
{
  struct interop_data_t *args = (struct interop_data_t *) data;
  struct gomp_device_descr *devicep;

  /* Destroy objects to free resources.  */
  for (int i = 0; i < args->n_destroy; i++)
    {
      struct interop_obj_t **obj = args->destroy[i];
      if (*obj == NULL /* omp_interop_none */)
	continue;
      devicep = resolve_device ((*obj)->device_num, false);
      if (devicep != NULL)
	gomp_device_interop (devicep, *obj, gomp_interop_flag_destroy);
      free (*obj);
      *obj = NULL;
    }

  /* Init streams next to give 'use' more time for completion.  */
  if (args->n_init)
    {
      devicep = resolve_device (args->device_num, false);
      for (int i = 0; i < args->n_init; i++)
	{
	  struct interop_obj_t **obj = args->init[i];
	  bool targetsync
	    = (args->target_targetsync[i] & GOMP_INTEROP_TARGETSYNC);
	  const char *prefer_type
	    = (args->prefer_type ? args->prefer_type[i] : NULL);
	  if (devicep == NULL)
	    {
	      *obj = NULL;
	      continue;
	    }
	  *obj = gomp_device_interop_init (devicep, targetsync, prefer_type);
	}
    }

  for (int i = 0; i < args->n_use; i++)
    {
      struct interop_obj_t *obj = args->use[i];
      if (obj == NULL)
	continue;
      devicep = resolve_device (obj->device_num, false);
      if (devicep != NULL)
	gomp_device_interop (devicep, obj, gomp_interop_flag_use);
    }
}

/* Process the OpenMP interop directive. 'init' and 'destroy' take an array
   of 'omp_interop_t *', 'use' an array of 'omp_interop_t', where
   'omp_interop_t' is internally 'struct interop_obj_t *';
   'flags' is used for the 'nowait' clause.  */

void
GOMP_interop (int device_num, int n_init, struct interop_obj_t ***init,
	      const int *target_targetsync, const char **prefer_type, int n_use,
	      struct interop_obj_t **use, int n_destroy,
	      struct interop_obj_t ***destroy, unsigned int flags,
	      void **depend)
{
  struct interop_data_t args;
  args.device_num = device_num;
  args.n_init = n_init;
  args.n_use = n_use;
  args.n_destroy = n_destroy;
  args.init = init;
  args.target_targetsync = target_targetsync;
  args.prefer_type = prefer_type;
  args.use = use;
  args.destroy = destroy;

  /* No need to create a task for 'init' as that should be fast. */
  bool use_task = false;
  if (flags & GOMP_INTEROP_FLAG_NOWAIT)
    {
      for (int i = 0; i < n_use && !use_task; i++)
	if (args.use[i])
	  use_task |= args.use[i]->stream != NULL;
      for (int i = 0; i < n_destroy && !use_task; i++)
	if (*args.destroy[i])
	  use_task |= (*args.destroy[i])->stream != NULL;
    }

  if (use_task)
    GOMP_task (gomp_interop_internal, &args, NULL, sizeof (args),
	       __alignof__ (args), true, depend ? GOMP_TASK_FLAG_DEPEND : 0,
	       depend, 0, NULL);
  else
    {
      gomp_interop_internal (&args);
      if (depend)
	GOMP_taskwait_depend (depend);
    }
}


const char *
omp_get_uid_from_device (int device_num)
{
  if (device_num == omp_default_device)
    device_num = gomp_get_default_device ();

  if (device_num < omp_initial_device || device_num > gomp_get_num_devices ())
    return NULL;

  if (device_num == omp_initial_device || device_num == gomp_get_num_devices ())
    return str_omp_initial_device;

  struct gomp_device_descr *devicep = resolve_device (device_num, false);
  if (devicep == NULL)
    return NULL;
  return gomp_get_uid_for_device (devicep, device_num);
}

int
omp_get_device_from_uid (const char *uid)
{
  if (uid == NULL)
    return omp_invalid_device;
  if (strcmp (uid, str_omp_initial_device) == 0)
    return omp_initial_device;
  return gomp_device_from_uid (uid);
}

ialias (omp_get_uid_from_device)
ialias (omp_get_device_from_uid)

