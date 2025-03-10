/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "vm_hl13.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_vm);

void surf_vm_model_init_HL13(void){
  if (surf_cpu_model_vm) {
    surf_vm_model = new VMHL13Model();
    Model *model = surf_vm_model;

    xbt_dynar_push(model_list, &model);
    xbt_dynar_push(model_list_invoke, &model);
  }
}

/*********
 * Model *
 *********/

VMHL13Model::VMHL13Model() : VMModel() {}

void VMHL13Model::updateActionsState(double /*now*/, double /*delta*/) {}

/* ind means ''indirect'' that this is a reference on the whole dict_elm
 * structure (i.e not on the surf_resource_private infos) */

VM *VMHL13Model::createVM(const char *name, surf_resource_t host_PM)
{
  VMHL13 *ws = new VMHL13(this, name, NULL, host_PM);

  xbt_lib_set(host_lib, name, SURF_HOST_LEVEL, ws);

  /* TODO:
   * - check how network requests are scheduled between distinct processes competing for the same card.
   */
  return ws;
}

static inline double get_solved_value(CpuAction *cpu_action)
{
  return cpu_action->getVariable()->value;
}

/* In the real world, processes on the guest operating system will be somewhat
 * degraded due to virtualization overhead. The total CPU share that these
 * processes get is smaller than that of the VM process gets on a host
 * operating system. */
// const double virt_overhead = 0.95;
const double virt_overhead = 1;

double VMHL13Model::shareResources(double now)
{
  /* TODO: udpate action's cost with the total cost of processes on the VM. */


  /* 0. Make sure that we already calculated the resource share at the physical
   * machine layer. */
  {
    XBT_ATTRIB_UNUSED Model *ws_model = surf_host_model;
    XBT_ATTRIB_UNUSED Model *vm_ws_model = surf_vm_model;
    XBT_ATTRIB_UNUSED unsigned int index_of_pm_ws_model = xbt_dynar_search(model_list_invoke, &ws_model);
    XBT_ATTRIB_UNUSED unsigned int index_of_vm_ws_model = xbt_dynar_search(model_list_invoke, &vm_ws_model);
    xbt_assert((index_of_pm_ws_model < index_of_vm_ws_model), "Cannot assume surf_host_model comes before");

    /* Another option is that we call sub_ws->share_resource() here. The
     * share_resource() function has no side-effect. We can call it here to
     * ensure that. */
  }


  /* 1. Now we know how many resource should be assigned to each virtual
   * machine. We update constraints of the virtual machine layer.
   *
   *
   * If we have two virtual machine (VM1 and VM2) on a physical machine (PM1).
   *     X1 + X2 = C       (Equation 1)
   * where
   *    the resource share of VM1: X1
   *    the resource share of VM2: X2
   *    the capacity of PM1: C
   *
   * Then, if we have two process (P1 and P2) on VM1.
   *     X1_1 + X1_2 = X1  (Equation 2)
   * where
   *    the resource share of P1: X1_1
   *    the resource share of P2: X1_2
   *    the capacity of VM1: X1
   *
   * Equation 1 was solved in the physical machine layer.
   * Equation 2 is solved in the virtual machine layer (here).
   * X1 must be passed to the virtual machine laye as a constraint value.
   *
   **/

  /* iterate for all virtual machines */
  for (VMModel::vm_list_t::iterator iter =
         VMModel::ws_vms.begin();
       iter !=  VMModel::ws_vms.end(); ++iter) {

    VM *ws_vm = &*iter;
    Cpu *cpu = ws_vm->p_cpu;
    xbt_assert(cpu, "cpu-less host");

    double solved_value = get_solved_value(ws_vm->p_action);
    XBT_DEBUG("assign %f to vm %s @ pm %s", solved_value,
        ws_vm->getName(), ws_vm->p_subWs->getName());

    // TODO: check lmm_update_constraint_bound() works fine instead of the below manual substitution.
    // cpu_cas01->constraint->bound = solved_value;
    xbt_assert(cpu->getModel() == surf_cpu_model_vm);
    lmm_system_t vcpu_system = cpu->getModel()->getMaxminSystem();
    lmm_update_constraint_bound(vcpu_system, cpu->getConstraint(), virt_overhead * solved_value);
  }


  /* 2. Calculate resource share at the virtual machine layer. */
  adjustWeightOfDummyCpuActions();

  double min_by_cpu = surf_cpu_model_vm->shareResources(now);
  double min_by_net = surf_network_model->shareResourcesIsIdempotent() ? surf_network_model->shareResources(now) : -1;
  // Fixme: take storage into account once it's implemented
  double min_by_sto = -1;

  XBT_DEBUG("model %p, %s min_by_cpu %f, %s min_by_net %f, %s min_by_sto %f",
      this, typeid(surf_cpu_model_pm ).name(), min_by_cpu,
	        typeid(surf_network_model).name(), min_by_net,
            typeid(surf_storage_model).name(), min_by_sto);

  double ret = max(max(min_by_cpu, min_by_net), min_by_sto);
  if (min_by_cpu >= 0.0 && min_by_cpu < ret)
	ret = min_by_cpu;
  if (min_by_net >= 0.0 && min_by_net < ret)
	ret = min_by_net;
  if (min_by_sto >= 0.0 && min_by_sto < ret)
	ret = min_by_sto;

  /* FIXME: 3. do we have to re-initialize our cpu_action object? */
#if 0
  /* iterate for all virtual machines */
  for (VMModel::vm_list_t::iterator iter =
         VMModel::ws_vms.begin();
       iter !=  VMModel::ws_vms.end(); ++iter) {

    {
#if 0
      VM2013 *ws_vm2013 = static_cast<VM2013Ptr>(&*iter);
      XBT_INFO("cost %f remains %f start %f finish %f", ws_vm2013->cpu_action->cost,
          ws_vm2013->cpu_action->remains,
          ws_vm2013->cpu_action->start,
          ws_vm2013->cpu_action->finish
          );
#endif
#if 0
      void *ind_sub_host = xbt_lib_get_elm_or_null(host_lib, ws_vm2013->sub_ws->generic_resource.getName);
      surf_cpu_model_pm->action_unref(ws_vm2013->cpu_action);
      /* FIXME: this means busy loop? */
      // ws_vm2013->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_sub_host, GUESTOS_NOISE);
      ws_vm2013->cpu_action = surf_cpu_model_pm->extension.cpu.execute(ind_sub_host, 0);
#endif

    }
  }
#endif


  return ret;
}

Action *VMHL13Model::executeParallelTask(int host_nb,
                                         sg_host_t *host_list,
										 double *flops_amount,
										 double *bytes_amount,
										 double rate){
#define cost_or_zero(array,pos) ((array)?(array)[pos]:0.0)
  if ((host_nb == 1)
      && (cost_or_zero(bytes_amount, 0) == 0.0))
    return surf_host_execute(host_list[0], flops_amount[0]);
  else if ((host_nb == 1)
           && (cost_or_zero(flops_amount, 0) == 0.0))
    return surf_network_model_communicate(surf_network_model, host_list[0], host_list[0],bytes_amount[0], rate);
  else if ((host_nb == 2)
             && (cost_or_zero(flops_amount, 0) == 0.0)
             && (cost_or_zero(flops_amount, 1) == 0.0)) {
    int i,nb = 0;
    double value = 0.0;

    for (i = 0; i < host_nb * host_nb; i++) {
      if (cost_or_zero(bytes_amount, i) > 0.0) {
        nb++;
        value = cost_or_zero(bytes_amount, i);
      }
    }
    if (nb == 1)
      return surf_network_model_communicate(surf_network_model, host_list[0], host_list[1], value, rate);
  }
#undef cost_or_zero

  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks for more than 2 hosts. */
  return NULL;
}

/************
 * Resource *
 ************/

VMHL13::VMHL13(VMModel *model, const char* name, xbt_dict_t props,
		                                   surf_resource_t host_PM)
 : VM(model, name, props, NULL, NULL)
{
  Host *sub_ws = static_cast<Host*>(surf_host_resource_priv(host_PM));

  /* Currently, we assume a VM has no storage. */
  p_storage = NULL;

  /* Currently, a VM uses the network resource of its physical host. In
   * host_lib, this network resource object is referred from two different keys.
   * When deregistering the reference that points the network resource object
   * from the VM name, we have to make sure that the system does not call the
   * free callback for the network resource object. The network resource object
   * is still used by the physical machine. */
  sg_host_t sg_sub_ws = sg_host_by_name_or_create(sub_ws->getName());
  p_netElm = new RoutingEdgeWrapper(sg_host_edge(sg_sub_ws));
  sg_host_edge_set(sg_host_by_name_or_create(name), p_netElm);

  p_subWs = sub_ws;
  p_currentState = SURF_VM_STATE_CREATED;

  // //// CPU  RELATED STUFF ////
  // Roughly, create a vcpu resource by using the values of the sub_cpu one.
  CpuCas01 *sub_cpu = static_cast<CpuCas01*>(sg_host_surfcpu(host_PM));

  p_cpu = surf_cpu_model_vm->createCpu(name, // name
      sub_cpu->getPowerPeakList(),        // host->power_peak,
      sub_cpu->getPState(),
      1,                          // host->power_scale,
      NULL,                       // host->power_trace,
      1,                          // host->core_amount,
      SURF_RESOURCE_ON,           // host->initial_state,
      NULL,                       // host->state_trace,
      NULL);                       // host->properties,

  /* We create cpu_action corresponding to a VM process on the host operating system. */
  /* FIXME: TODO: we have to periodically input GUESTOS_NOISE to the system? how ? */
  // vm_ws->cpu_action = surf_cpu_model_pm->extension.cpu.execute(host_PM, GUESTOS_NOISE);
  p_action = sub_cpu->execute(0);

  XBT_INFO("Create VM(%s)@PM(%s) with %ld mounted disks", name, sub_ws->getName(), xbt_dynar_length(p_storage));
}

/*
 * A physical host does not disappear in the current SimGrid code, but a VM may
 * disappear during a simulation.
 */
VMHL13::~VMHL13()
{
  /* Free the cpu_action of the VM. */
  XBT_ATTRIB_UNUSED int ret = p_action->unref();
  xbt_assert(ret == 1, "Bug: some resource still remains");
}

void VMHL13::updateState(tmgr_trace_event_t /*event_type*/, double /*value*/, double /*date*/) {
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
}

bool VMHL13::isUsed() {
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
  return -1;
}

e_surf_resource_state_t VMHL13::getState()
{
  return (e_surf_resource_state_t) p_currentState;
}

void VMHL13::setState(e_surf_resource_state_t state)
{
  p_currentState = (e_surf_vm_state_t) state;
}

void VMHL13::suspend()
{
  p_action->suspend();
  p_currentState = SURF_VM_STATE_SUSPENDED;
}

void VMHL13::resume()
{
  p_action->resume();
  p_currentState = SURF_VM_STATE_RUNNING;
}

void VMHL13::save()
{
  p_currentState = SURF_VM_STATE_SAVING;

  /* FIXME: do something here */
  p_action->suspend();
  p_currentState = SURF_VM_STATE_SAVED;
}

void VMHL13::restore()
{
  p_currentState = SURF_VM_STATE_RESTORING;

  /* FIXME: do something here */
  p_action->resume();
  p_currentState = SURF_VM_STATE_RUNNING;
}

/*
 * Update the physical host of the given VM
 */
void VMHL13::migrate(surf_resource_t ind_dst_pm)
{
   /* ind_dst_pm equals to smx_host_t */
   Host *ws_dst = static_cast<Host*>(surf_host_resource_priv(ind_dst_pm));
   const char *vm_name = getName();
   const char *pm_name_src = p_subWs->getName();
   const char *pm_name_dst = ws_dst->getName();

   xbt_assert(ws_dst);

   /* update net_elm with that of the destination physical host */
   RoutingEdge *old_net_elm = p_netElm;
   RoutingEdge *new_net_elm = new RoutingEdgeWrapper(sg_host_edge(sg_host_by_name(pm_name_dst)));
   xbt_assert(new_net_elm);

   /* Unregister the current net_elm from host_lib. Do not call the free callback. */
   sg_host_edge_destroy(sg_host_by_name(vm_name), 0);

   /* Then, resister the new one. */
   p_netElm = new_net_elm;
   sg_host_edge_set(sg_host_by_name(vm_name), p_netElm);

   p_subWs = ws_dst;

   /* Update vcpu's action for the new pm */
   {
#if 0
     XBT_INFO("cpu_action->remains %g", p_action->remains);
     XBT_INFO("cost %f remains %f start %f finish %f", p_action->cost,
         p_action->remains,
         p_action->start,
         p_action->finish
         );
     XBT_INFO("cpu_action state %d", surf_action_get_state(p_action));
#endif

     /* create a cpu action bound to the pm model at the destination. */
     CpuAction *new_cpu_action = static_cast<CpuAction*>(
    		                            static_cast<Cpu*>(sg_host_surfcpu(ind_dst_pm))->execute(0));

     e_surf_action_state_t state = p_action->getState();
     if (state != SURF_ACTION_DONE)
       XBT_CRITICAL("FIXME: may need a proper handling, %d", state);
     if (p_action->getRemainsNoUpdate() > 0)
       XBT_CRITICAL("FIXME: need copy the state(?), %f", p_action->getRemainsNoUpdate());

     /* keep the bound value of the cpu action of the VM. */
     double old_bound = p_action->getBound();
     if (old_bound != 0) {
       XBT_DEBUG("migrate VM(%s): set bound (%f) at %s", vm_name, old_bound, pm_name_dst);
       new_cpu_action->setBound(old_bound);
     }

     XBT_ATTRIB_UNUSED int ret = p_action->unref();
     xbt_assert(ret == 1, "Bug: some resource still remains");

     p_action = new_cpu_action;
   }

   XBT_DEBUG("migrate VM(%s): change net_elm (%p to %p)", vm_name, old_net_elm, new_net_elm);
   XBT_DEBUG("migrate VM(%s): change PM (%s to %s)", vm_name, pm_name_src, pm_name_dst);
   delete old_net_elm;
}

void VMHL13::setBound(double bound){
 p_action->setBound(bound);
}

void VMHL13::setAffinity(Cpu *cpu, unsigned long mask){
 p_action->setAffinity(cpu, mask);
}

/*
 * A surf level object will be useless in the upper layer. Returning the
 * dict_elm of the host.
 **/
surf_resource_t VMHL13::getPm()
{
  return xbt_lib_get_elm_or_null(host_lib, p_subWs->getName());
}

/* Adding a task to a VM updates the VCPU task on its physical machine. */
Action *VMHL13::execute(double size)
{
  double old_cost = p_action->getCost();
  double new_cost = old_cost + size;

  XBT_DEBUG("VM(%s)@PM(%s): update dummy action's cost (%f -> %f)",
      getName(), p_subWs->getName(),
      old_cost, new_cost);

  p_action->setCost(new_cost);

  return p_cpu->execute(size);
}

Action *VMHL13::sleep(double duration) {
  return p_cpu->sleep(duration);
}

/**********
 * Action *
 **********/

//FIME:: handle action cancel

