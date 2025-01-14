/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define __STDC_FORMAT_MACROS
#include <cinttypes>
#include <boost/unordered_set.hpp>

#include <xbt/sysdep.h>

#include "internal_config.h"
#include "mc_object_info.h"
#include "mc_safety.h"
#include "mc_liveness.h"
#include "mc_private.h"
#include "mc_smx.h"
#include "mc_dwarf.hpp"

#include "mc/Frame.hpp"
#include "mc/ObjectInformation.hpp"
#include "mc/Variable.hpp"

#ifdef HAVE_SMPI
#include "smpi/private.h"
#endif

#include "xbt/mmalloc.h"
#include "xbt/mmalloc/mmprivate.h"

#include <xbt/probes.h>

using simgrid::mc::remote;

typedef struct s_pointers_pair {
  void *p1;
  void *p2;
  bool operator==(s_pointers_pair const& x) const {
    return this->p1 == x.p1 && this->p2 == x.p2;
  }
  bool operator<(s_pointers_pair const& x) const {
    return this->p1 < x.p1 || (this->p1 == x.p1 && this->p2 < x.p2);
  }
} s_pointers_pair_t, *pointers_pair_t;

namespace boost {
  template<>
  struct hash<s_pointers_pair> {
    typedef uintptr_t result_type;
    result_type operator()(s_pointers_pair const& x) const {
      return (result_type) x.p1 ^
        ((result_type) x.p2 << 8 | (result_type) x.p2 >> (8*sizeof(uintptr_t) - 8));
    }
  };
}

struct mc_compare_state {
  boost::unordered_set<s_pointers_pair> compared_pointers;
};

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, xbt,
                                "Logging specific to mc_compare in mc");

/************************** Free functions ****************************/
/********************************************************************/

static void stack_region_free(stack_region_t s)
{
  if (s) {
    xbt_free(s);
  }
}

static void stack_region_free_voidp(void *s)
{
  stack_region_free((stack_region_t) * (void **) s);
}

static void pointers_pair_free(pointers_pair_t p)
{
  xbt_free(p);
}

static void pointers_pair_free_voidp(void *p)
{
  pointers_pair_free((pointers_pair_t) * (void **) p);
}

/************************** Snapshot comparison *******************************/
/******************************************************************************/

/** \brief Try to add a pair a compared pointers to the set of compared pointers
 *
 *  \result !=0 if the pointers were added (they were not in the set),
 *      0 otherwise (they were already in the set)
 */
static int add_compared_pointers(mc_compare_state& state, void *p1, void *p2)
{
  s_pointers_pair_t new_pair;
  new_pair.p1 = p1;
  new_pair.p2 = p2;
  return state.compared_pointers.insert(new_pair).second ? 1 : 0;
}

static int compare_areas_with_type(struct mc_compare_state& state,
                                   int process_index,
                                   void* real_area1, mc_snapshot_t snapshot1, mc_mem_region_t region1,
                                   void* real_area2, mc_snapshot_t snapshot2, mc_mem_region_t region2,
                                   simgrid::mc::Type* type, int pointer_level)
{
  simgrid::mc::Process* process = &mc_model_checker->process();

  simgrid::mc::Type* subtype;
  simgrid::mc::Type* subsubtype;
  int elm_size, i, res;

  top:
  switch (type->type) {
  case DW_TAG_unspecified_type:
    return 1;

  case DW_TAG_base_type:
  case DW_TAG_enumeration_type:
  case DW_TAG_union_type:
  {
    return MC_snapshot_region_memcmp(
      real_area1, region1, real_area2, region2,
      type->byte_size) != 0;
  }
  case DW_TAG_typedef:
  case DW_TAG_volatile_type:
  case DW_TAG_const_type:
    // Poor man's TCO:
    type = type->subtype;
    goto top;
  case DW_TAG_array_type:
    subtype = type->subtype;
    switch (subtype->type) {
    case DW_TAG_unspecified_type:
      return 1;

    case DW_TAG_base_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
    case DW_TAG_structure_type:
    case DW_TAG_class_type:
    case DW_TAG_union_type:
      if (subtype->full_type)
        subtype = subtype->full_type;
      elm_size = subtype->byte_size;
      break;
    case DW_TAG_const_type:
    case DW_TAG_typedef:
    case DW_TAG_volatile_type:
      subsubtype = subtype->subtype;
      if (subsubtype->full_type)
        subsubtype = subsubtype->full_type;
      elm_size = subsubtype->byte_size;
      break;
    default:
      return 0;
      break;
    }
    for (i = 0; i < type->element_count; i++) {
      size_t off = i * elm_size;
      res = compare_areas_with_type(state, process_index,
            (char*) real_area1 + off, snapshot1, region1,
            (char*) real_area2 + off, snapshot2, region2,
            type->subtype, pointer_level);
      if (res == 1)
        return res;
    }
    break;
  case DW_TAG_pointer_type:
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:
  {
    void* addr_pointed1 = MC_region_read_pointer(region1, real_area1);
    void* addr_pointed2 = MC_region_read_pointer(region2, real_area2);

    if (type->subtype && type->subtype->type == DW_TAG_subroutine_type) {
      return (addr_pointed1 != addr_pointed2);
    } else {

      if (addr_pointed1 == NULL && addr_pointed2 == NULL)
        return 0;
      if (addr_pointed1 == NULL || addr_pointed2 == NULL)
        return 1;
      if (!add_compared_pointers(state, addr_pointed1, addr_pointed2))
        return 0;

      pointer_level++;

      // Some cases are not handled here:
      // * the pointers lead to different areas (one to the heap, the other to the RW segment ...);
      // * a pointer leads to the read-only segment of the current object;
      // * a pointer lead to a different ELF object.

      if (addr_pointed1 > process->heap_address
          && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1)) {
        if (!
            (addr_pointed2 > process->heap_address
             && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2)))
          return 1;
        // The pointers are both in the heap:
        return compare_heap_area(process_index, addr_pointed1, addr_pointed2, snapshot1,
                                 snapshot2, NULL, type->subtype, pointer_level);
      }

      // The pointers are both in the current object R/W segment:
      else if (region1->contain(simgrid::mc::remote(addr_pointed1))) {
        if (!region2->contain(simgrid::mc::remote(addr_pointed2)))
          return 1;
        if (!type->type_id)
          return (addr_pointed1 != addr_pointed2);
        else {
          return compare_areas_with_type(state, process_index,
                                         addr_pointed1, snapshot1, region1,
                                         addr_pointed2, snapshot2, region2,
                                         type->subtype, pointer_level);
        }
      }

      // TODO, We do not handle very well the case where
      // it belongs to a different (non-heap) region from the current one.

      else {
        return (addr_pointed1 != addr_pointed2);
      }
    }
    break;
  }
  case DW_TAG_structure_type:
  case DW_TAG_class_type:
    for(simgrid::mc::Member& member : type->members) {
      void *member1 =
        mc_member_resolve(real_area1, type, &member, snapshot1, process_index);
      void *member2 =
        mc_member_resolve(real_area2, type, &member, snapshot2, process_index);
      mc_mem_region_t subregion1 = mc_get_region_hinted(member1, snapshot1, process_index, region1);
      mc_mem_region_t subregion2 = mc_get_region_hinted(member2, snapshot2, process_index, region2);
      res =
          compare_areas_with_type(state, process_index,
                                  member1, snapshot1, subregion1,
                                  member2, snapshot2, subregion2,
                                  member.type, pointer_level);
      if (res == 1)
        return res;
    }
    break;
  case DW_TAG_subroutine_type:
    return -1;
    break;
  default:
    XBT_VERB("Unknown case : %d", type->type);
    break;
  }

  return 0;
}

static int compare_global_variables(simgrid::mc::ObjectInformation* object_info,
                                    int process_index,
                                    mc_mem_region_t r1,
                                    mc_mem_region_t r2, mc_snapshot_t snapshot1,
                                    mc_snapshot_t snapshot2)
{
  xbt_assert(r1 && r2, "Missing region.");

#ifdef HAVE_SMPI
  if (r1->storage_type() == simgrid::mc::StorageType::Privatized) {
    xbt_assert(process_index >= 0);
    if (r2->storage_type() != simgrid::mc::StorageType::Privatized) {
      return 1;
    }

    size_t process_count = MC_smpi_process_count();
    xbt_assert(process_count == r1->privatized_data().size()
      && process_count == r2->privatized_data().size());

    // Compare the global variables separately for each simulates process:
    for (size_t process_index = 0; process_index < process_count; process_index++) {
      int is_diff = compare_global_variables(object_info, process_index,
        &r1->privatized_data()[process_index],
        &r2->privatized_data()[process_index],
        snapshot1, snapshot2);
      if (is_diff) return 1;
    }
    return 0;
  }
#else
  xbt_assert(r1->storage_type() != simgrid::mc::StorageType::Privatized);
#endif
  xbt_assert(r2->storage_type() != simgrid::mc::StorageType::Privatized);

  struct mc_compare_state state;

  std::vector<simgrid::mc::Variable>& variables = object_info->global_variables;

  for (simgrid::mc::Variable& current_var : variables) {

    // If the variable is not in this object, skip it:
    // We do not expect to find a pointer to something which is not reachable
    // by the global variables.
    if ((char *) current_var.address < (char *) object_info->start_rw
        || (char *) current_var.address > (char *) object_info->end_rw)
      continue;

    simgrid::mc::Type* bvariable_type = current_var.type;
    int res =
        compare_areas_with_type(state, process_index,
                                (char *) current_var.address, snapshot1, r1,
                                (char *) current_var.address, snapshot2, r2,
                                bvariable_type, 0);
    if (res == 1) {
      XBT_TRACE3(mc, global_diff, -1, -1, current_var->name);
      XBT_VERB("Global variable %s (%p) is different between snapshots",
               current_var.name.c_str(),
               (char *) current_var.address);
      return 1;
    }

  }

  return 0;

}

static int compare_local_variables(int process_index,
                                   mc_snapshot_t snapshot1,
                                   mc_snapshot_t snapshot2,
                                   mc_snapshot_stack_t stack1,
                                   mc_snapshot_stack_t stack2)
{
  struct mc_compare_state state;

  if (stack1->local_variables.size() != stack2->local_variables.size()) {
    XBT_VERB("Different number of local variables");
    return 1;
  } else {
    unsigned int cursor = 0;
    local_variable_t current_var1, current_var2;
    int res;
    while (cursor < stack1->local_variables.size()) {
      current_var1 = &stack1->local_variables[cursor];
      current_var2 = &stack1->local_variables[cursor];
      if (current_var1->name != current_var2->name
          || current_var1->subprogram != current_var1->subprogram
          || current_var1->ip != current_var2->ip) {
        // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
        XBT_VERB
            ("Different name of variable (%s - %s) "
             "or frame (%s - %s) or ip (%lu - %lu)",
             current_var1->name.c_str(),
             current_var2->name.c_str(),
             current_var1->subprogram->name.c_str(),
             current_var2->subprogram->name.c_str(),
             current_var1->ip, current_var2->ip);
        return 1;
      }
      // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram

        simgrid::mc::Type* subtype = current_var1->type;
        res =
            compare_areas_with_type(state, process_index,
                                    current_var1->address, snapshot1, mc_get_snapshot_region(current_var1->address, snapshot1, process_index),
                                    current_var2->address, snapshot2, mc_get_snapshot_region(current_var2->address, snapshot2, process_index),
                                    subtype, 0);

      if (res == 1) {
        // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
        XBT_TRACE3(mc, local_diff, -1, -1, current_var1->name);
        XBT_VERB
            ("Local variable %s (%p - %p) in frame %s "
             "is different between snapshots",
             current_var1->name.c_str(),
             current_var1->address,
             current_var2->address,
             current_var1->subprogram->name.c_str());
        return res;
      }
      cursor++;
    }
    return 0;
  }
}

int snapshot_compare(void *state1, void *state2)
{
  simgrid::mc::Process* process = &mc_model_checker->process();

  mc_snapshot_t s1, s2;
  int num1, num2;

  if (_sg_mc_liveness) {        /* Liveness MC */
    s1 = ((mc_visited_pair_t) state1)->graph_state->system_state;
    s2 = ((mc_visited_pair_t) state2)->graph_state->system_state;
    num1 = ((mc_visited_pair_t) state1)->num;
    num2 = ((mc_visited_pair_t) state2)->num;
  }else if (_sg_mc_termination) { /* Non-progressive cycle MC */
    s1 = ((mc_state_t) state1)->system_state;
    s2 = ((mc_state_t) state2)->system_state;
    num1 = ((mc_state_t) state1)->num;
    num2 = ((mc_state_t) state2)->num;
  } else {                      /* Safety or comm determinism MC */
    s1 = ((mc_visited_state_t) state1)->system_state;
    s2 = ((mc_visited_state_t) state2)->system_state;
    num1 = ((mc_visited_state_t) state1)->num;
    num2 = ((mc_visited_state_t) state2)->num;
  }

  int errors = 0;
  int res_init;

  xbt_os_timer_t global_timer = xbt_os_timer_new();
  xbt_os_timer_t timer = xbt_os_timer_new();

  xbt_os_walltimer_start(global_timer);

#ifdef MC_DEBUG
  xbt_os_walltimer_start(timer);
#endif

  int hash_result = 0;
  if (_sg_mc_hash) {
    hash_result = (s1->hash != s2->hash);
    if (hash_result) {
      XBT_TRACE2(mc, hash_diff, num1, num2);
      XBT_VERB("(%d - %d) Different hash : 0x%" PRIx64 "--0x%" PRIx64, num1,
               num2, s1->hash, s2->hash);
#ifndef MC_DEBUG
      return 1;
#endif
    } else {
      XBT_VERB("(%d - %d) Same hash : 0x%" PRIx64, num1, num2, s1->hash);
    }
  }

  /* Compare enabled processes */
  if (s1->enabled_processes != s2->enabled_processes) {
      //XBT_TRACE3(mc, state_diff, num1, num2, "Different enabled processes");
      XBT_VERB("(%d - %d) Different enabled processes", num1, num2);
      // return 1; ??
  }

  unsigned long i = 0;
  size_t size_used1, size_used2;
  int is_diff = 0;

  /* Compare size of stacks */
  while (i < s1->stacks.size()) {
    size_used1 = s1->stack_sizes[i];
    size_used2 = s2->stack_sizes[i];
    if (size_used1 != size_used2) {
#ifdef MC_DEBUG
      if (is_diff == 0) {
        xbt_os_walltimer_stop(timer);
        mc_comp_times->stacks_sizes_comparison_time =
            xbt_os_timer_elapsed(timer);
      }
      XBT_DEBUG("(%d - %d) Different size used in stacks : %zu - %zu", num1,
                num2, size_used1, size_used2);
      errors++;
      is_diff = 1;
#else
#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different size used in stacks : %zu - %zu", num1,
               num2, size_used1, size_used2);
#endif
      XBT_TRACE3(mc, state_diff, num1, num2, "Different stack size");

      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
#endif
    }
    i++;
  }

#ifdef MC_DEBUG
  if (is_diff == 0)
    xbt_os_walltimer_stop(timer);
  xbt_os_walltimer_start(timer);
#endif

  /* Init heap information used in heap comparison algorithm */
  xbt_mheap_t heap1 = (xbt_mheap_t)s1->read_bytes(
    alloca(sizeof(struct mdesc)), sizeof(struct mdesc),
    remote(process->heap_address),
    simgrid::mc::ProcessIndexMissing, simgrid::mc::AddressSpace::Lazy);
  xbt_mheap_t heap2 = (xbt_mheap_t)s2->read_bytes(
    alloca(sizeof(struct mdesc)), sizeof(struct mdesc),
    remote(process->heap_address),
    simgrid::mc::ProcessIndexMissing, simgrid::mc::AddressSpace::Lazy);
  res_init = init_heap_information(heap1, heap2, &s1->to_ignore, &s2->to_ignore);
  if (res_init == -1) {
#ifdef MC_DEBUG
    XBT_DEBUG("(%d - %d) Different heap information", num1, num2);
    errors++;
#else
#ifdef MC_VERBOSE
    XBT_TRACE3(mc, state_diff, num1, num2, "Different heap information");
    XBT_VERB("(%d - %d) Different heap information", num1, num2);
#endif

    xbt_os_walltimer_stop(global_timer);
    mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
    xbt_os_timer_free(global_timer);

    return 1;
#endif
  }
#ifdef MC_DEBUG
  xbt_os_walltimer_start(timer);
#endif

  /* Stacks comparison */
  unsigned cursor = 0;
  int diff_local = 0;
#ifdef MC_DEBUG
  is_diff = 0;
#endif
  mc_snapshot_stack_t stack1, stack2;
  while (cursor < s1->stacks.size()) {
    stack1 = &s1->stacks[cursor];
    stack2 = &s1->stacks[cursor];

    if (stack1->process_index != stack2->process_index) {
      diff_local = 1;
      XBT_DEBUG("(%d - %d) Stacks with different process index (%i vs %i)", num1, num2,
        stack1->process_index, stack2->process_index);
    }
    else diff_local =
        compare_local_variables(stack1->process_index, s1, s2, stack1, stack2);
    if (diff_local > 0) {
      XBT_TRACE3(mc, state_diff, num1, num2, "Different local variables");
#ifdef MC_DEBUG
      if (is_diff == 0) {
        xbt_os_walltimer_stop(timer);
        mc_comp_times->stacks_comparison_time = xbt_os_timer_elapsed(timer);
      }
      XBT_DEBUG("(%d - %d) Different local variables between stacks %d", num1,
                num2, cursor + 1);
      errors++;
      is_diff = 1;
#else

#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different local variables between stacks %d", num1,
               num2, cursor + 1);
#endif

      reset_heap_information();
      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
#endif
    }
    cursor++;
  }

  size_t regions_count = s1->snapshot_regions.size();
  // TODO, raise a difference instead?
  xbt_assert(regions_count == s2->snapshot_regions.size());

  mc_comp_times->global_variables_comparison_time = 0;

  for (size_t k = 0; k != regions_count; ++k) {
    mc_mem_region_t region1 = s1->snapshot_regions[k].get();
    mc_mem_region_t region2 = s2->snapshot_regions[k].get();

    // Preconditions:
    if (region1->region_type() != simgrid::mc::RegionType::Data)
      continue;

    xbt_assert(region1->region_type() == region2->region_type());
    xbt_assert(region1->object_info() == region2->object_info());
    xbt_assert(region1->object_info());

    std::string const& name = region1->object_info()->file_name;

#ifdef MC_DEBUG
    if (is_diff == 0)
      xbt_os_walltimer_stop(timer);
    xbt_os_walltimer_start(timer);
#endif

    /* Compare global variables */
    is_diff =
      compare_global_variables(region1->object_info(  ), simgrid::mc::AddressSpace::Normal,
        region1, region2,
        s1, s2);

    if (is_diff != 0) {
      XBT_TRACE3(mc, state_diff, num1, num2, "Different global variables");
#ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
      mc_comp_times->global_variables_comparison_time
        += xbt_os_timer_elapsed(timer);
      XBT_DEBUG("(%d - %d) Different global variables in %s",
        num1, num2, name.c_str());
      errors++;
#else
#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different global variables in %s",
        num1, num2, name.c_str());
#endif

      reset_heap_information();
      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
#endif
    }
  }

#ifdef MC_DEBUG
  xbt_os_walltimer_start(timer);
#endif

  /* Compare heap */
  if (mmalloc_compare_heap(s1, s2) > 0) {
    XBT_TRACE3(mc, state_diff, num1, num2, "Different heap");

#ifdef MC_DEBUG
    xbt_os_walltimer_stop(timer);
    mc_comp_times->heap_comparison_time = xbt_os_timer_elapsed(timer);
    XBT_DEBUG("(%d - %d) Different heap (mmalloc_compare)", num1, num2);
    errors++;
#else

#ifdef MC_VERBOSE
    XBT_VERB("(%d - %d) Different heap (mmalloc_compare)", num1, num2);
#endif

    reset_heap_information();
    xbt_os_walltimer_stop(timer);
    xbt_os_timer_free(timer);
    xbt_os_walltimer_stop(global_timer);
    mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
    xbt_os_timer_free(global_timer);

    return 1;
#endif
  } else {
#ifdef MC_DEBUG
    xbt_os_walltimer_stop(timer);
#endif
  }

  reset_heap_information();

  xbt_os_walltimer_stop(timer);
  xbt_os_timer_free(timer);

#ifdef MC_VERBOSE
  xbt_os_walltimer_stop(global_timer);
  mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
#endif

  xbt_os_timer_free(global_timer);

#ifdef MC_DEBUG
  print_comparison_times();
#endif

#ifdef MC_VERBOSE
  if (errors || hash_result)
    XBT_VERB("(%d - %d) Difference found", num1, num2);
  else
    XBT_VERB("(%d - %d) No difference found", num1, num2);
#endif

#if defined(MC_DEBUG) && defined(MC_VERBOSE)
  if (_sg_mc_hash) {
    // * false positive SHOULD be avoided.
    // * There MUST not be any false negative.

    XBT_VERB("(%d - %d) State equality hash test is %s %s", num1, num2,
             (hash_result != 0) == (errors != 0) ? "true" : "false",
             !hash_result ? "positive" : "negative");
  }
#endif

  return errors > 0 || hash_result;

}

/***************************** Statistics *****************************/
/*******************************************************************/

void print_comparison_times()
{
  XBT_DEBUG("*** Comparison times ***");
  XBT_DEBUG("- Nb processes : %f", mc_comp_times->nb_processes_comparison_time);
  XBT_DEBUG("- Nb bytes used : %f", mc_comp_times->bytes_used_comparison_time);
  XBT_DEBUG("- Stacks sizes : %f", mc_comp_times->stacks_sizes_comparison_time);
  XBT_DEBUG("- GLobal variables : %f", mc_comp_times->global_variables_comparison_time);
  XBT_DEBUG("- Heap : %f", mc_comp_times->heap_comparison_time);
  XBT_DEBUG("- Stacks : %f", mc_comp_times->stacks_comparison_time);
}

}
