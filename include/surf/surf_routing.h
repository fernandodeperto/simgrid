/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_ROUTING_H
#define _SURF_SURF_ROUTING_H

#include "xbt/lib.h"
#include "simgrid/platf.h"

SG_BEGIN_DECL()

XBT_PUBLIC_DATA(xbt_lib_t) host_lib;
XBT_PUBLIC_DATA(int) SURF_HOST_LEVEL;     //Surf workstation level
XBT_PUBLIC_DATA(int) SIMIX_STORAGE_LEVEL; //Simix storage level
XBT_PUBLIC_DATA(int) SD_STORAGE_LEVEL;    //Simdag storage level
XBT_PUBLIC_DATA(int) COORD_HOST_LEVEL;    //Coordinates level
XBT_PUBLIC_DATA(int) NS3_HOST_LEVEL;      //host node for ns3

XBT_PUBLIC_DATA(xbt_lib_t) as_router_lib;
XBT_PUBLIC_DATA(int) ROUTING_ASR_LEVEL;  //Routing level
XBT_PUBLIC_DATA(int) COORD_ASR_LEVEL;  //Coordinates level
XBT_PUBLIC_DATA(int) NS3_ASR_LEVEL;    //host node for ns3
XBT_PUBLIC_DATA(int) ROUTING_PROP_ASR_LEVEL; //Properties for AS and router

XBT_PUBLIC_DATA(xbt_lib_t) storage_lib;
XBT_PUBLIC_DATA(int) ROUTING_STORAGE_LEVEL;        //Routing storage level
XBT_PUBLIC_DATA(int) ROUTING_STORAGE_HOST_LEVEL;
XBT_PUBLIC_DATA(int) SURF_STORAGE_LEVEL;  // Surf storage level
XBT_PUBLIC_DATA(xbt_lib_t) file_lib;
XBT_PUBLIC_DATA(xbt_lib_t) storage_type_lib;
XBT_PUBLIC_DATA(int) ROUTING_STORAGE_TYPE_LEVEL;   //Routing storage_type level

/* The callbacks to register for the routing to work */
void routing_AS_begin(sg_platf_AS_cbarg_t AS);
void routing_AS_end(sg_platf_AS_cbarg_t AS);

void routing_cluster_add_backbone(void* bb);

SG_END_DECL()

#endif                          /* _SURF_SURF_H */
