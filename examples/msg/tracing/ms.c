/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>tracing/ms.c</b> This is a master/slave program where the master creates
 * tasks, send them to the slaves. For each task received, the slave executes
 * it and then destroys it. This program uses several tracing functions that
 * enable the tracing of categorized resource utilization, the use of trace marks,
 * and user variables associated to the hosts of the platform file.
 * You might want to run this program with the following parameters:
 * --cfg=tracing/categorized:yes
 * --cfg=tracing/uncategorized:yes
 * --cfg=viva/categorized:viva_cat.plist
 * --cfg=viva/uncategorized:viva_uncat.plist
 * (See \ref tracing_tracing_options for details)
 */

#include <stdio.h>
#include "simgrid/msg.h"
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);

/** Emitter function  */
int master(int argc, char *argv[])
{
  long number_of_tasks = atol(argv[1]);
  double task_comp_size = atof(argv[2]);
  double task_comm_size = atof(argv[3]);
  long slaves_count = atol(argv[4]);

  //setting the variable "is_master" (previously declared) to value 1
  TRACE_host_variable_set(MSG_host_get_name(MSG_host_self()), "is_master", 1);

  TRACE_mark("msmark", "start_send_tasks");
  int i;
  for (i = 0; i < number_of_tasks; i++) {
    msg_task_t task = NULL;
    task = MSG_task_create("task", task_comp_size, task_comm_size, NULL);

    //setting the variable "task_creation" to value i
    TRACE_host_variable_set(MSG_host_get_name(MSG_host_self()), "task_creation", i);

    //setting the category of task to "compute"
    //the category of a task must be defined before it is sent or executed
    MSG_task_set_category(task, "compute");
    MSG_task_send(task, "master_mailbox");
  }
  TRACE_mark("msmark", "finish_send_tasks");

  for (i = 0; i < slaves_count; i++) {
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_set_category(finalize, "finalize");
    MSG_task_send(finalize, "master_mailbox");
  }

  return 0;
}

/** Receiver function  */
int slave(int argc, char *argv[])
{
  msg_task_t task = NULL;

  TRACE_host_variable_set(MSG_host_get_name(MSG_host_self()), "is_slave", 1);
  TRACE_host_variable_set(MSG_host_get_name(MSG_host_self()),
                          "task_computation",
                          0);
  while (1) {
    MSG_task_receive(&(task), "master_mailbox");

    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }
    //adding the value returned by MSG_task_get_compute_duration(task)
    //to the variable "task_computation"
    TRACE_host_variable_add(MSG_host_get_name(MSG_host_self()),
                            "task_computation",
                            MSG_task_get_flops_amount(task));
    MSG_task_execute(task);
    MSG_task_destroy(task);
    task = NULL;
  }
  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    exit(1);
  }

  char *platform_file = argv[1];
  char *deployment_file = argv[2];
  MSG_create_environment(platform_file);

  //declaring user variables
  TRACE_host_variable_declare("is_slave");
  TRACE_host_variable_declare("is_master");
  TRACE_host_variable_declare("task_creation");
  TRACE_host_variable_declare("task_computation");

  //declaring user markers and values
  TRACE_declare_mark("msmark");
  TRACE_declare_mark_value ("msmark", "start_send_tasks");
  TRACE_declare_mark_value ("msmark", "finish_send_tasks");

  //declaring user categories with RGB colors (values from 0 to 1)
  TRACE_category_with_color ("compute", "1 0 0");  //compute is red
  TRACE_category_with_color ("finalize", "0 1 0"); //finalize is green
  //categories without user-defined colors receive
  //random colors generated by the tracing system
  TRACE_category ("request");
  TRACE_category_with_color ("report", NULL);

  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  MSG_launch_application(deployment_file);

  MSG_main();

  unsigned int cursor;
  xbt_dynar_t categories = TRACE_get_categories ();
  if (categories){
    XBT_INFO ("Declared tracing categories:");
    char *category;
    xbt_dynar_foreach (categories, cursor, category){
      XBT_INFO ("%s", category);
    }
    xbt_dynar_free (&categories);
  }

  xbt_dynar_t marks = TRACE_get_marks ();
  if (marks){
    XBT_INFO ("Declared marks:");
    char *mark;
    xbt_dynar_foreach (marks, cursor, mark){
      XBT_INFO ("%s", mark);
    }
    xbt_dynar_free (&marks);
  }

  return 0;
}                               /* end_of_main */
