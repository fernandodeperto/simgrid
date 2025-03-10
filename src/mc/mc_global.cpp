/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cinttypes>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "mc_base.h"

#ifndef _XBT_WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif

#include "simgrid/sg_config.h"
#include "../surf/surf_private.h"
#include "../simix/smx_private.h"
#include "xbt/fifo.h"
#include "xbt/automaton.h"
#include "xbt/dict.h"
#include "mc_record.h"

#ifdef HAVE_MC
#include "mc_server.h"
#include <libunwind.h>
#include <xbt/mmalloc.h>
#include "../xbt/mmalloc/mmprivate.h"
#include "mc_object_info.h"
#include "mc_comm_pattern.h"
#include "mc_request.h"
#include "mc_safety.h"
#include "mc_memory_map.h"
#include "mc_snapshot.h"
#include "mc_liveness.h"
#include "mc_private.h"
#include "mc_unw.h"
#include "mc_smx.h"
#include "mcer_ignore.h"
#endif
#include "mc_record.h"
#include "mc_protocol.h"
#include "mc_client.h"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc,
                                "Logging specific to MC (global)");

e_mc_mode_t mc_mode;

double *mc_time = NULL;

#ifdef HAVE_MC
int user_max_depth_reached = 0;

/* MC global data structures */
mc_state_t mc_current_state = NULL;
char mc_replay_mode = FALSE;

__thread mc_comparison_times_t mc_comp_times = NULL;
__thread double mc_snapshot_comparison_time;
mc_stats_t mc_stats = NULL;
mc_global_t initial_global_state = NULL;
xbt_fifo_t mc_stack = NULL;

/* Liveness */
xbt_automaton_t _mc_property_automaton = NULL;

/* Dot output */
FILE *dot_output = NULL;
const char *colors[13];


/*******************************  Initialisation of MC *******************************/
/*********************************************************************************/

static void MC_init_dot_output()
{                               /* FIXME : more colors */

  colors[0] = "blue";
  colors[1] = "red";
  colors[2] = "green3";
  colors[3] = "goldenrod";
  colors[4] = "brown";
  colors[5] = "purple";
  colors[6] = "magenta";
  colors[7] = "turquoise4";
  colors[8] = "gray25";
  colors[9] = "forestgreen";
  colors[10] = "hotpink";
  colors[11] = "lightblue";
  colors[12] = "tan";

  dot_output = fopen(_sg_mc_dot_output_file, "w");

  if (dot_output == NULL) {
    perror("Error open dot output file");
    xbt_abort();
  }

  fprintf(dot_output,
          "digraph graphname{\n fixedsize=true; rankdir=TB; ranksep=.25; edge [fontsize=12]; node [fontsize=10, shape=circle,width=.5 ]; graph [resolution=20, fontsize=10];\n");

}

#ifdef HAVE_MC
void MC_init()
{
  mc_time = xbt_new0(double, simix_process_maxpid);

  if (_sg_mc_visited > 0 || _sg_mc_liveness  || _sg_mc_termination || mc_mode == MC_MODE_SERVER) {
    /* Those requests are handled on the client side and propagated by message
     * to the server: */

    MC_ignore_heap(mc_time, simix_process_maxpid * sizeof(double));

    smx_process_t process;
    xbt_swag_foreach(process, simix_global->process_list) {
      MC_ignore_heap(&(process->process_hookup), sizeof(process->process_hookup));
    }
  }
}

void MC_init_model_checker(pid_t pid, int socket)
{
  mc_model_checker = new simgrid::mc::ModelChecker(pid, socket);

  mc_comp_times = xbt_new0(s_mc_comparison_times_t, 1);

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  if ((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0] != '\0'))
    MC_init_dot_output();

  /* Init parmap */
  //parmap = xbt_parmap_mc_new(xbt_os_get_numcores(), XBT_PARMAP_DEFAULT);

  /* Ignore some variables from xbt/ex.h used by exception e for stacks comparison */
  MC_ignore_local_variable("e", "*");
  MC_ignore_local_variable("__ex_cleanup", "*");
  MC_ignore_local_variable("__ex_mctx_en", "*");
  MC_ignore_local_variable("__ex_mctx_me", "*");
  MC_ignore_local_variable("__xbt_ex_ctx_ptr", "*");
  MC_ignore_local_variable("_log_ev", "*");
  MC_ignore_local_variable("_throw_ctx", "*");
  MC_ignore_local_variable("ctx", "*");

  MC_ignore_local_variable("self", "simcall_BODY_mc_snapshot");
  MC_ignore_local_variable("next_cont"
    "ext", "smx_ctx_sysv_suspend_serial");
  MC_ignore_local_variable("i", "smx_ctx_sysv_suspend_serial");

  /* Ignore local variable about time used for tracing */
  MC_ignore_local_variable("start_time", "*");

  /* Static variable used for tracing */
  MCer_ignore_global_variable("counter");

  /* SIMIX */
  MCer_ignore_global_variable("smx_total_comms");
}
#endif

/*******************************  Core of MC *******************************/
/**************************************************************************/

void MC_run()
{
  mc_mode = MC_MODE_CLIENT;
  MC_init();
  MC_client_main_loop();
}

void MC_exit(void)
{
  xbt_free(mc_time);

  MC_memory_exit();
  //xbt_abort();
}

#ifdef HAVE_MC
int MC_deadlock_check()
{
  if (mc_mode == MC_MODE_SERVER) {
    int res;
    if ((res = mc_model_checker->process().send_message(MC_MESSAGE_DEADLOCK_CHECK)))
      xbt_die("Could not check deadlock state");
    s_mc_int_message_t message;
    ssize_t s = mc_model_checker->process().receive_message(message);
    if (s == -1)
      xbt_die("Could not receive message");
    else if (s != sizeof(message) || message.type != MC_MESSAGE_DEADLOCK_CHECK_REPLY) {
      xbt_die("%s received unexpected message %s (%i, size=%i) "
        "expected MC_MESSAGE_DEADLOCK_CHECK_REPLY (%i, size=%i)",
        MC_mode_name(mc_mode),
        MC_message_type_name(message.type), (int) message.type, (int) s,
        (int) MC_MESSAGE_DEADLOCK_CHECK_REPLY, (int) sizeof(message)
        );
    }
    else
      return message.value;
  }

  int deadlock = FALSE;
  smx_process_t process;
  if (xbt_swag_size(simix_global->process_list)) {
    deadlock = TRUE;
    xbt_swag_foreach(process, simix_global->process_list) {
      if (MC_process_is_enabled(process)) {
        deadlock = FALSE;
        break;
      }
    }
  }
  return deadlock;
}
#endif

/**
 * \brief Re-executes from the state at position start all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
 * \param start Start index to begin the re-execution.
 */
void MC_replay(xbt_fifo_t stack)
{
  int value, count = 1;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item, start_item;
  mc_state_t state;
  
  XBT_DEBUG("**** Begin Replay ****");

  /* Intermediate backtracking */
  if(_sg_mc_checkpoint > 0 || _sg_mc_termination || _sg_mc_visited > 0) {
    start_item = xbt_fifo_get_first_item(stack);
    state = (mc_state_t)xbt_fifo_get_item_content(start_item);
    if(state->system_state){
      MC_restore_snapshot(state->system_state);
      if(_sg_mc_comms_determinism || _sg_mc_send_determinism) 
        MC_restore_communications_pattern(state);
      return;
    }
  }


  /* Restore the initial state */
  MC_restore_snapshot(initial_global_state->snapshot);
  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */

  start_item = xbt_fifo_get_last_item(stack);

  if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
    // int n = xbt_dynar_length(incomplete_communications_pattern);
    unsigned n = MC_smx_get_maxpid();
    assert(n == xbt_dynar_length(incomplete_communications_pattern));
    assert(n == xbt_dynar_length(initial_communications_pattern));
    for (unsigned j=0; j < n ; j++) {
      xbt_dynar_reset((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, j, xbt_dynar_t));
      xbt_dynar_get_as(initial_communications_pattern, j, mc_list_comm_pattern_t)->index_comm = 0;
    }
  }

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (item = start_item;
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    state = (mc_state_t) xbt_fifo_get_item_content(item);
    saved_req = MC_state_get_executed_request(state, &value);
    
    if (saved_req) {
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */

      const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
      req = &issuer->simcall;

      /* Debug information */
      if (XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)) {
        req_str = MC_request_to_string(req, value, MC_REQUEST_SIMIX);
        XBT_DEBUG("Replay: %s (%p)", req_str, state);
        xbt_free(req_str);
      }

      /* TODO : handle test and testany simcalls */
      e_mc_call_type_t call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        call = MC_get_call_type(req);

      MC_simcall_handle(req, value);

      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        MC_handle_comm_pattern(call, req, value, NULL, 1);

      MC_wait_for_requests();

      count++;
    }

    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;

  }

  XBT_DEBUG("**** End Replay ****");
}

void MC_replay_liveness(xbt_fifo_t stack)
{
  xbt_fifo_item_t item;
  mc_pair_t pair = NULL;
  mc_state_t state = NULL;
  smx_simcall_t req = NULL, saved_req = NULL;
  int value, depth = 1;
  char *req_str;

  XBT_DEBUG("**** Begin Replay ****");

  /* Intermediate backtracking */
  if(_sg_mc_checkpoint > 0) {
    item = xbt_fifo_get_first_item(stack);
    pair = (mc_pair_t) xbt_fifo_get_item_content(item);
    if(pair->graph_state->system_state){
      MC_restore_snapshot(pair->graph_state->system_state);
      return;
    }
  }

  /* Restore the initial state */
  MC_restore_snapshot(initial_global_state->snapshot);

    /* Traverse the stack from the initial state and re-execute the transitions */
    for (item = xbt_fifo_get_last_item(stack);
         item != xbt_fifo_get_first_item(stack);
         item = xbt_fifo_get_prev_item(item)) {

      pair = (mc_pair_t) xbt_fifo_get_item_content(item);

      state = (mc_state_t) pair->graph_state;

      if (pair->exploration_started) {

        saved_req = MC_state_get_executed_request(state, &value);

        if (saved_req != NULL) {
          /* because we got a copy of the executed request, we have to fetch the
             real one, pointed by the request field of the issuer process */
          const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
          req = &issuer->simcall;

          /* Debug information */
          if (XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)) {
            req_str = MC_request_to_string(req, value, MC_REQUEST_SIMIX);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }

        }

        MC_simcall_handle(req, value);
        MC_wait_for_requests();
      }

      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;

      depth++;
      
    }

  XBT_DEBUG("**** End Replay ****");
}

/**
 * \brief Dumps the contents of a model-checker's stack and shows the actual
 *        execution trace
 * \param stack The stack to dump
 */
void MC_dump_stack_safety(xbt_fifo_t stack)
{
  MC_show_stack_safety(stack);
  
  mc_state_t state;

  while ((state = (mc_state_t) xbt_fifo_pop(stack)) != NULL)
    MC_state_delete(state, !state->in_visited_states ? 1 : 0);
}


void MC_show_stack_safety(xbt_fifo_t stack)
{
  int value;
  mc_state_t state;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;

  for (item = xbt_fifo_get_last_item(stack);
       item; item = xbt_fifo_get_prev_item(item)) {
    state = (mc_state_t)xbt_fifo_get_item_content(item);
    req = MC_state_get_executed_request(state, &value);
    if (req) {
      req_str = MC_request_to_string(req, value, MC_REQUEST_EXECUTED);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }
}

void MC_show_deadlock(smx_simcall_t req)
{
  /*char *req_str = NULL; */
  XBT_INFO("**************************");
  XBT_INFO("*** DEAD-LOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Locked request:");
  /*req_str = MC_request_to_string(req);
     XBT_INFO("%s", req_str);
     xbt_free(req_str); */
  XBT_INFO("Counter-example execution trace:");
  MC_dump_stack_safety(mc_stack);
  MC_print_statistics(mc_stats);
}

void MC_show_non_termination(void){
  XBT_INFO("******************************************");
  XBT_INFO("*** NON-PROGRESSIVE CYCLE DETECTED ***");
  XBT_INFO("******************************************");
  XBT_INFO("Counter-example execution trace:");
  MC_dump_stack_safety(mc_stack);
  MC_print_statistics(mc_stats);
}


void MC_show_stack_liveness(xbt_fifo_t stack)
{
  int value;
  mc_pair_t pair;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;

  for (item = xbt_fifo_get_last_item(stack);
       item; item = xbt_fifo_get_prev_item(item)) {
    pair = (mc_pair_t) xbt_fifo_get_item_content(item);
    req = MC_state_get_executed_request(pair->graph_state, &value);
    if (req && req->call != SIMCALL_NONE) {
      req_str = MC_request_to_string(req, value, MC_REQUEST_EXECUTED);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }
}


void MC_dump_stack_liveness(xbt_fifo_t stack)
{
  mc_pair_t pair;
  while ((pair = (mc_pair_t) xbt_fifo_pop(stack)) != NULL)
    MC_pair_delete(pair);
}

void MC_print_statistics(mc_stats_t stats)
{
  if(_sg_mc_comms_determinism) {
    if (!initial_global_state->recv_deterministic && initial_global_state->send_deterministic){
      XBT_INFO("******************************************************");
      XBT_INFO("**** Only-send-deterministic communication pattern ****");
      XBT_INFO("******************************************************");
      XBT_INFO("%s", initial_global_state->recv_diff);
    }else if(!initial_global_state->send_deterministic && initial_global_state->recv_deterministic) {
      XBT_INFO("******************************************************");
      XBT_INFO("**** Only-recv-deterministic communication pattern ****");
      XBT_INFO("******************************************************");
      XBT_INFO("%s", initial_global_state->send_diff);
    }
  }

  if (stats->expanded_pairs == 0) {
    XBT_INFO("Expanded states = %lu", stats->expanded_states);
    XBT_INFO("Visited states = %lu", stats->visited_states);
  } else {
    XBT_INFO("Expanded pairs = %lu", stats->expanded_pairs);
    XBT_INFO("Visited pairs = %lu", stats->visited_pairs);
  }
  XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  if ((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0] != '\0')) {
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if (initial_global_state != NULL && (_sg_mc_comms_determinism || _sg_mc_send_determinism)) {
    XBT_INFO("Send-deterministic : %s", !initial_global_state->send_deterministic ? "No" : "Yes");
    if (_sg_mc_comms_determinism)
      XBT_INFO("Recv-deterministic : %s", !initial_global_state->recv_deterministic ? "No" : "Yes");
  }
  if (getenv("SIMGRID_MC_SYSTEM_STATISTICS"))
    system("free");
}

void MC_automaton_load(const char *file)
{
  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();

  xbt_automaton_load(_mc_property_automaton, file);
}

// TODO, fix cross-process access (this function is not used)
void MC_dump_stacks(FILE* file)
{
  int nstack = 0;
  stack_region_t current_stack;
  unsigned cursor;
  xbt_dynar_foreach(stacks_areas, cursor, current_stack) {
    unw_context_t * context = (unw_context_t *)current_stack->context;
    fprintf(file, "Stack %i:\n", nstack);

    int nframe = 0;
    char buffer[100];
    unw_cursor_t c;
    unw_init_local (&c, context);
    unw_word_t off;
    do {
      const char * name = !unw_get_proc_name(&c, buffer, 100, &off) ? buffer : "?";
#if defined(__x86_64__)
      unw_word_t rip = 0;
      unw_word_t rsp = 0;
      unw_get_reg(&c, UNW_X86_64_RIP, &rip);
      unw_get_reg(&c, UNW_X86_64_RSP, &rsp);
      fprintf(file, "  %i: %s (RIP=0x%" PRIx64 " RSP=0x%" PRIx64 ")\n",
        nframe, name, (std::uint64_t) rip, (std::uint64_t) rsp);
#else
      fprintf(file, "  %i: %s\n", nframe, name);
#endif
      ++nframe;
    } while(unw_step(&c));

    ++nstack;
  }
}
#endif

double MC_process_clock_get(smx_process_t process)
{
  if (mc_time) {
    if (process != NULL)
      return mc_time[process->pid];
    else
      return -1;
  } else {
    return 0;
  }
}

void MC_process_clock_add(smx_process_t process, double amount)
{
  mc_time[process->pid] += amount;
}

#ifdef HAVE_MC
void MC_report_assertion_error(void)
{
  XBT_INFO("**************************");
  XBT_INFO("*** PROPERTY NOT VALID ***");
  XBT_INFO("**************************");
  XBT_INFO("Counter-example execution trace:");
  MC_record_dump_path(mc_stack);
  MC_dump_stack_safety(mc_stack);
  MC_print_statistics(mc_stats);
}

void MC_invalidate_cache(void)
{
  if (mc_model_checker)
    mc_model_checker->process().cache_flags = 0;
}
#endif

}
