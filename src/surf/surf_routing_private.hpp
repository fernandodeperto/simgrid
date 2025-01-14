/* Copyright (c) 2009-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_ROUTING_PRIVATE_H
#define _SURF_SURF_ROUTING_PRIVATE_H

#include <float.h>
#include "internal_config.h"

#include "surf_interface.hpp"
#include "xbt/base.h"
#include "xbt/dynar.h"
#include "xbt/str.h"
#include "xbt/config.h"
#include "xbt/graph.h"
#include "xbt/set.h"
#include "surf/surfxml_parse.h"

/* ************************************************************************** */
/* ******************************* NO ROUTING ******************************* */
/* Only save the AS tree, and forward calls to child ASes */
XBT_PRIVATE AS_t model_none_create(void);
XBT_PRIVATE AS_t model_none_create_sized(size_t childsize);
XBT_PRIVATE void model_none_finalize(AS_t as);
/* ************************************************************************** */
/* ***************** GENERIC PARSE FUNCTIONS (declarations) ***************** */
XBT_PRIVATE AS_t model_generic_create_sized(size_t childsize);
XBT_PRIVATE void model_generic_finalize(AS_t as);

XBT_PRIVATE int generic_parse_PU(AS_t rc, sg_routing_edge_t elm);
XBT_PRIVATE int generic_parse_AS(AS_t rc, sg_routing_edge_t elm);
XBT_PRIVATE void generic_parse_bypassroute(AS_t rc, sg_platf_route_cbarg_t e_route);

/* ************************************************************************** */
/* *************** GENERIC BUSINESS METHODS (declarations) ****************** */

XBT_PRIVATE xbt_dynar_t generic_get_onelink_routes(AS_t rc);
XBT_PRIVATE sg_platf_route_cbarg_t generic_get_bypassroute(AS_t rc,
    sg_routing_edge_t src,
    sg_routing_edge_t dst,
    double *lat);

/* ************************************************************************** */
/* ****************** GENERIC AUX FUNCTIONS (declarations) ****************** */

/* change a route containing link names into a route containing link entities.
 * If change_order is true, the links are put in reverse order in the
 * produced route */
XBT_PRIVATE sg_platf_route_cbarg_t generic_new_extended_route(e_surf_routing_hierarchy_t hierarchy,
                                   sg_platf_route_cbarg_t data, int preserve_order);
XBT_PRIVATE AS_t
generic_autonomous_system_exist(AS_t rc, char *element);
XBT_PRIVATE AS_t
generic_processing_units_exist(AS_t rc, char *element);
void generic_src_dst_check(AS_t rc, sg_routing_edge_t src,
    sg_routing_edge_t dst);

/* ************************************************************************** */
/* *************************** FLOYD ROUTING ******************************** */
XBT_PRIVATE AS_t model_floyd_create(void);  /* create structures for floyd routing model */
XBT_PRIVATE void model_floyd_end(AS_t as);      /* finalize the creation of floyd routing model */
XBT_PRIVATE void model_floyd_parse_route(AS_t rc, sg_platf_route_cbarg_t route);

/* ************************************************** */
/* **************  Cluster ROUTING   **************** */

XBT_PRIVATE As *model_cluster_create(void);      /* create structures for cluster routing model */
XBT_PRIVATE As *model_torus_cluster_create(void);
XBT_PRIVATE As *model_fat_tree_cluster_create(void);

/* ************************************************** */
/* **************  Vivaldi ROUTING   **************** */
XBT_PRIVATE AS_t model_vivaldi_create(void);      /* create structures for vivaldi routing model */
#define HOST_PEER(peername) bprintf("peer_%s", peername)
#define ROUTER_PEER(peername) bprintf("router_%s", peername)
#define LINK_PEER(peername) bprintf("link_%s", peername)

/* ************************************************************************** */
/* ********** Dijkstra & Dijkstra Cached ROUTING **************************** */
XBT_PRIVATE AS_t model_dijkstra_both_create(int cached);    /* create by calling dijkstra or dijkstracache */
XBT_PRIVATE AS_t model_dijkstra_create(void);       /* create structures for dijkstra routing model */
XBT_PRIVATE AS_t model_dijkstracache_create(void);  /* create structures for dijkstracache routing model */
XBT_PRIVATE void model_dijkstra_both_end(AS_t as);      /* finalize the creation of dijkstra routing model */
XBT_PRIVATE void model_dijkstra_both_parse_route (AS_t rc, sg_platf_route_cbarg_t route);

/* ************************************************************************** */
/* *************************** FULL ROUTING ********************************* */
XBT_PRIVATE AS_t model_full_create(void);   /* create structures for full routing model */
XBT_PRIVATE void model_full_end(AS_t as);       /* finalize the creation of full routing model */
XBT_PRIVATE void model_full_set_route(  /* Set the route and ASroute between src and dst */
    AS_t rc, sg_platf_route_cbarg_t route);
/* ************************************************************************** */
/* ************************* GRAPH EXPORTING FUNCTIONS ********************** */
XBT_PRIVATE xbt_node_t new_xbt_graph_node (xbt_graph_t graph, const char *name, xbt_dict_t nodes);
XBT_PRIVATE xbt_edge_t new_xbt_graph_edge (xbt_graph_t graph, xbt_node_t s, xbt_node_t d, xbt_dict_t edges);


#endif                          /* _SURF_SURF_ROUTING_PRIVATE_H */
