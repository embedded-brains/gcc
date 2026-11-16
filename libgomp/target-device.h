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

/* This file declares the interface between the host part of the offloading
   support in target.c and the device part in target-device.c.  */

#ifndef LIBGOMP_TARGET_DEVICE_H
#define LIBGOMP_TARGET_DEVICE_H 1

#include <string.h>

struct htab;

/* Defined in target.c.  */

/* Array of descriptors for all available devices.  */
extern struct gomp_device_descr *devices attribute_hidden;

/* Number of GOMP_OFFLOAD_CAP_OPENMP_400 devices.  */
extern int num_devices_openmp attribute_hidden;

extern struct gomp_device_descr *resolve_device (int, bool) attribute_hidden;
extern struct gomp_offload_icv_list *gomp_get_offload_icv_item (int)
  attribute_hidden;
extern void gomp_target_fallback (void (*) (void *), void **,
				  struct gomp_device_descr *, void **)
  attribute_hidden;
extern void gomp_target_data_fallback (struct gomp_device_descr *)
  attribute_hidden;

/* Helpers of the host fallback and the shared memory devices.  */

/* Calculate alignment and size requirements of a private copy of data shared
   as GOMP_MAP_FIRSTPRIVATE and store them to TGT_ALIGN and TGT_SIZE.  */

static inline void
calculate_firstprivate_requirements (size_t mapnum, size_t *sizes,
				     unsigned short *kinds, size_t *tgt_align,
				     size_t *tgt_size)
{
  size_t i;
  for (i = 0; i < mapnum; i++)
    if ((kinds[i] & 0xff) == GOMP_MAP_FIRSTPRIVATE)
      {
	size_t align = (size_t) 1 << (kinds[i] >> 8);
	if (*tgt_align < align)
	  *tgt_align = align;
	*tgt_size = (*tgt_size + align - 1) & ~(align - 1);
	*tgt_size += sizes[i];
      }
}

/* Copy data shared as GOMP_MAP_FIRSTPRIVATE to DST.  */

static inline void
copy_firstprivate_data (char *tgt, size_t mapnum, void **hostaddrs,
			size_t *sizes, unsigned short *kinds, size_t tgt_align,
			size_t tgt_size)
{
  uintptr_t al = (uintptr_t) tgt & (tgt_align - 1);
  if (al)
    tgt += tgt_align - al;
  tgt_size = 0;
  size_t i;
  for (i = 0; i < mapnum; i++)
    if ((kinds[i] & 0xff) == GOMP_MAP_FIRSTPRIVATE && hostaddrs[i] != NULL)
      {
	size_t align = (size_t) 1 << (kinds[i] >> 8);
	tgt_size = (tgt_size + align - 1) & ~(align - 1);
	memcpy (tgt + tgt_size, hostaddrs[i], sizes[i]);
	hostaddrs[i] = tgt + tgt_size;
	tgt_size = tgt_size + sizes[i];
	if (i + 1 < mapnum && (kinds[i+1] & 0xff) == GOMP_MAP_ATTACH)
	  {
	    *(*(uintptr_t**) hostaddrs[i+1] + sizes[i+1]) = (uintptr_t) hostaddrs[i];
	    ++i;
	  }
      }
}

/* Defined in target-device.c.  */

extern void gomp_target_init (void) attribute_hidden;
extern splay_tree_key gomp_map_lookup (splay_tree, splay_tree_key)
  attribute_hidden;
extern splay_tree_key gomp_map_0len_lookup (splay_tree, splay_tree_key)
  attribute_hidden;
extern struct target_mem_desc *gomp_map_vars (struct gomp_device_descr *,
					      size_t, void **, void **,
					      size_t *, void *, bool,
					      struct htab **,
					      enum gomp_map_vars_kind)
  attribute_hidden;
extern void gomp_unmap_vars (struct target_mem_desc *, bool, struct htab **)
  attribute_hidden;
extern void gomp_unmap_tgt (struct target_mem_desc *) attribute_hidden;
extern void gomp_update (struct gomp_device_descr *, size_t, void **, size_t *,
			 void *, bool) attribute_hidden;
extern void gomp_exit_data (struct gomp_device_descr *, size_t, void **,
			    size_t *, unsigned short *, struct htab **)
  attribute_hidden;
extern void *gomp_get_target_fn_addr (struct gomp_device_descr *,
				      void (*) (void *)) attribute_hidden;
extern void gomp_copy_back_icvs (struct gomp_device_descr *, int)
  attribute_hidden;
extern void gomp_free_device_memory (struct gomp_device_descr *, void *)
  attribute_hidden;
extern const char *gomp_get_uid_for_device (struct gomp_device_descr *, int)
  attribute_hidden;
extern void gomp_target_run (struct gomp_device_descr *, void *, size_t,
			     void **, size_t *, unsigned char *)
  attribute_hidden;
extern void gomp_target_ext_run (struct gomp_device_descr *, void *, size_t,
				 void **, size_t *, unsigned short *, bool,
				 size_t, size_t, void **, int)
  attribute_hidden;
extern void gomp_enter_exit_data (struct gomp_device_descr *, size_t, void **,
				  size_t *, unsigned short *, unsigned int)
  attribute_hidden;
extern bool gomp_target_task_run (struct gomp_device_descr *,
				  struct gomp_target_task *, void *)
  attribute_hidden;
extern void gomp_target_task_data (struct gomp_device_descr *,
				   struct gomp_target_task *)
  attribute_hidden;
extern bool gomp_device_maps_data (struct gomp_device_descr *)
  attribute_hidden;
extern void *gomp_device_fn_addr (struct gomp_device_descr *,
				  void (*) (void *)) attribute_hidden;
extern struct gomp_device_descr *gomp_device_get (int) attribute_hidden;
extern void gomp_target_data_push_empty (struct gomp_task_icv *)
  attribute_hidden;
extern void gomp_target_data_device (struct gomp_device_descr *, size_t,
				     void **, size_t *, void *, bool)
  attribute_hidden;
extern void gomp_target_data_pop (struct gomp_task_icv *) attribute_hidden;
extern void gomp_target_update_device (struct gomp_device_descr *, size_t,
				       void **, size_t *, unsigned short *)
  attribute_hidden;
extern void *gomp_device_alloc (struct gomp_device_descr *, size_t)
  attribute_hidden;
extern void gomp_device_free (struct gomp_device_descr *, void *)
  attribute_hidden;
extern int gomp_device_is_present (struct gomp_device_descr *, const void *)
  attribute_hidden;
extern int gomp_device_memcpy (void *, const void *, size_t, size_t, size_t,
			       struct gomp_device_descr *,
			       struct gomp_device_descr *) attribute_hidden;
extern int gomp_device_memcpy_line (void *, const void *, size_t, size_t,
				    size_t, struct gomp_device_descr *,
				    struct gomp_device_descr *, size_t *,
				    void **) attribute_hidden;
extern int gomp_device_memcpy_rect (void *, const void *, size_t, int,
				    const size_t *, const size_t *,
				    const size_t *, const size_t *,
				    const size_t *, struct gomp_device_descr *,
				    struct gomp_device_descr *)
  attribute_hidden;
extern void gomp_device_lock_pair (struct gomp_device_descr *,
				   struct gomp_device_descr *)
  attribute_hidden;
extern void gomp_device_unlock_pair (struct gomp_device_descr *,
				     struct gomp_device_descr *)
  attribute_hidden;
extern void gomp_device_memset (struct gomp_device_descr *, void *, int,
				size_t) attribute_hidden;
extern int gomp_device_associate_ptr (struct gomp_device_descr *,
				      const void *, const void *, size_t,
				      size_t) attribute_hidden;
extern int gomp_device_disassociate_ptr (struct gomp_device_descr *,
					 const void *) attribute_hidden;
extern void *gomp_device_get_mapped_ptr (struct gomp_device_descr *,
					 const void *) attribute_hidden;
extern int gomp_device_is_accessible (struct gomp_device_descr *,
				      const void *, size_t) attribute_hidden;
extern omp_intptr_t gomp_device_get_interop_int (struct gomp_device_descr *,
					struct interop_obj_t *,
					omp_interop_property_t,
					omp_interop_rc_t *) attribute_hidden;
extern void * gomp_device_get_interop_ptr (struct gomp_device_descr *,
					struct interop_obj_t *,
					omp_interop_property_t,
					omp_interop_rc_t *) attribute_hidden;
extern const char * gomp_device_get_interop_str (struct gomp_device_descr *,
					struct interop_obj_t *,
					omp_interop_property_t,
					omp_interop_rc_t *) attribute_hidden;
extern const char *gomp_device_get_interop_type_desc
  (struct gomp_device_descr *, struct interop_obj_t *, omp_interop_property_t)
  attribute_hidden;
extern void gomp_device_interop (struct gomp_device_descr *,
				 struct interop_obj_t *,
				 enum gomp_interop_flag) attribute_hidden;
extern struct interop_obj_t *gomp_device_interop_init
  (struct gomp_device_descr *, bool, const char *) attribute_hidden;
extern int gomp_device_from_uid (const char *) attribute_hidden;

#endif /* LIBGOMP_TARGET_DEVICE_H */
