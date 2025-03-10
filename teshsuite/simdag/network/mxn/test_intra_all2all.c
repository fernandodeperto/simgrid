/* Latency tests                                                            */

/* Copyright (c) 2007, 2009-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "simgrid/simdag.h"

/*
 * intra communication test
 * All2All
 * 
 * send 1 byte from all to all
 * + 2 secs latency
 * should be 8  (platform_4p_1switch.xml)
 * 
 */

int main(int argc, char **argv)
{
  double time;
  SD_task_t task;
  xbt_dynar_t ret;

  double communication_amount[] = { 0.0, 1.0, 1.0, 1.0,
    1.0, 0.0, 1.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 0.0,
  };

  double no_cost[] = { 0.0, 0.0, 0.0, 0.0 };


  /***************************************/

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  task = SD_task_create("All2all task", NULL, 1.0);

  SD_task_schedule(task, 4, SD_workstation_get_list(), no_cost,
                   communication_amount, -1.0);

  ret = SD_simulate(-1.0);
  xbt_dynar_free(&ret);

  time = SD_get_clock();

  printf("%g\n", time);
  fflush(stdout);

  SD_task_destroy(task);

  SD_exit();

  return 0;
}
