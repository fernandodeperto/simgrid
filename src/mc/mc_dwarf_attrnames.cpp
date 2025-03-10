/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning: autogenerated, do not edit! */

#include <xbt/base.h>
#include <dwarf.h>

#include "mc_object_info.h"

/** \brief Get the name of an attribute (DW_AT_*) from its code
 *
 *  \param attr attribute code (see the DWARF specification)
 *  \return name of the attribute
 */
const char *MC_dwarf_attrname(int attr)
{
  switch (attr) {
  case 0x01: return "DW_AT_sibling";
  case 0x02: return "DW_AT_location";
  case 0x03: return "DW_AT_name";
  case 0x09: return "DW_AT_ordering";
  case 0x0a: return "DW_AT_subscr_data";
  case 0x0b: return "DW_AT_byte_size";
  case 0x0c: return "DW_AT_bit_offset";
  case 0x0d: return "DW_AT_bit_size";
  case 0x0f: return "DW_AT_element_list";
  case 0x10: return "DW_AT_stmt_list";
  case 0x11: return "DW_AT_low_pc";
  case 0x12: return "DW_AT_high_pc";
  case 0x13: return "DW_AT_language";
  case 0x14: return "DW_AT_member";
  case 0x15: return "DW_AT_discr";
  case 0x16: return "DW_AT_discr_value";
  case 0x17: return "DW_AT_visibility";
  case 0x18: return "DW_AT_import";
  case 0x19: return "DW_AT_string_length";
  case 0x1a: return "DW_AT_common_reference";
  case 0x1b: return "DW_AT_comp_dir";
  case 0x1c: return "DW_AT_const_value";
  case 0x1d: return "DW_AT_containing_type";
  case 0x1e: return "DW_AT_default_value";
  case 0x20: return "DW_AT_inline";
  case 0x21: return "DW_AT_is_optional";
  case 0x22: return "DW_AT_lower_bound";
  case 0x25: return "DW_AT_producer";
  case 0x27: return "DW_AT_prototyped";
  case 0x2a: return "DW_AT_return_addr";
  case 0x2c: return "DW_AT_start_scope";
  case 0x2e: return "DW_AT_bit_stride";
  case 0x2f: return "DW_AT_upper_bound";
  case 0x31: return "DW_AT_abstract_origin";
  case 0x32: return "DW_AT_accessibility";
  case 0x33: return "DW_AT_address_class";
  case 0x34: return "DW_AT_artificial";
  case 0x35: return "DW_AT_base_types";
  case 0x36: return "DW_AT_calling_convention";
  case 0x37: return "DW_AT_count";
  case 0x38: return "DW_AT_data_member_location";
  case 0x39: return "DW_AT_decl_column";
  case 0x3a: return "DW_AT_decl_file";
  case 0x3b: return "DW_AT_decl_line";
  case 0x3c: return "DW_AT_declaration";
  case 0x3d: return "DW_AT_discr_list";
  case 0x3e: return "DW_AT_encoding";
  case 0x3f: return "DW_AT_external";
  case 0x40: return "DW_AT_frame_base";
  case 0x41: return "DW_AT_friend";
  case 0x42: return "DW_AT_identifier_case";
  case 0x43: return "DW_AT_macro_info";
  case 0x44: return "DW_AT_namelist_item";
  case 0x45: return "DW_AT_priority";
  case 0x46: return "DW_AT_segment";
  case 0x47: return "DW_AT_specification";
  case 0x48: return "DW_AT_static_link";
  case 0x49: return "DW_AT_type";
  case 0x4a: return "DW_AT_use_location";
  case 0x4b: return "DW_AT_variable_parameter";
  case 0x4c: return "DW_AT_virtuality";
  case 0x4d: return "DW_AT_vtable_elem_location";
  case 0x4e: return "DW_AT_allocated";
  case 0x4f: return "DW_AT_associated";
  case 0x50: return "DW_AT_data_location";
  case 0x51: return "DW_AT_byte_stride";
  case 0x52: return "DW_AT_entry_pc";
  case 0x53: return "DW_AT_use_UTF8";
  case 0x54: return "DW_AT_extension";
  case 0x55: return "DW_AT_ranges";
  case 0x56: return "DW_AT_trampoline";
  case 0x57: return "DW_AT_call_column";
  case 0x58: return "DW_AT_call_file";
  case 0x59: return "DW_AT_call_line";
  case 0x5a: return "DW_AT_description";
  case 0x5b: return "DW_AT_binary_scale";
  case 0x5c: return "DW_AT_decimal_scale";
  case 0x5d: return "DW_AT_small";
  case 0x5e: return "DW_AT_decimal_sign";
  case 0x5f: return "DW_AT_digit_count";
  case 0x60: return "DW_AT_picture_string";
  case 0x61: return "DW_AT_mutable";
  case 0x62: return "DW_AT_threads_scaled";
  case 0x63: return "DW_AT_explicit";
  case 0x64: return "DW_AT_object_pointer";
  case 0x65: return "DW_AT_endianity";
  case 0x66: return "DW_AT_elemental";
  case 0x67: return "DW_AT_pure";
  case 0x68: return "DW_AT_recursive";
  case 0x69: return "DW_AT_signature";
  case 0x6a: return "DW_AT_main_subprogram";
  case 0x6b: return "DW_AT_data_bit_offset";
  case 0x6c: return "DW_AT_const_expr";
  case 0x6d: return "DW_AT_enum_class";
  case 0x6e: return "DW_AT_linkage_name";
  case 0x2000: return "DW_AT_lo_user";
  case 0x2001: return "DW_AT_MIPS_fde";
  case 0x2002: return "DW_AT_MIPS_loop_begin";
  case 0x2003: return "DW_AT_MIPS_tail_loop_begin";
  case 0x2004: return "DW_AT_MIPS_epilog_begin";
  case 0x2005: return "DW_AT_MIPS_loop_unroll_factor";
  case 0x2006: return "DW_AT_MIPS_software_pipeline_depth";
  case 0x2007: return "DW_AT_MIPS_linkage_name";
  case 0x2008: return "DW_AT_MIPS_stride";
  case 0x2009: return "DW_AT_MIPS_abstract_name";
  case 0x200a: return "DW_AT_MIPS_clone_origin";
  case 0x200b: return "DW_AT_MIPS_has_inlines";
  case 0x200c: return "DW_AT_MIPS_stride_byte";
  case 0x200d: return "DW_AT_MIPS_stride_elem";
  case 0x200e: return "DW_AT_MIPS_ptr_dopetype";
  case 0x200f: return "DW_AT_MIPS_allocatable_dopetype";
  case 0x2010: return "DW_AT_MIPS_assumed_shape_dopetype";
  case 0x2011: return "DW_AT_MIPS_assumed_size";
  case 0x2101: return "DW_AT_sf_names";
  case 0x2102: return "DW_AT_src_info";
  case 0x2103: return "DW_AT_mac_info";
  case 0x2104: return "DW_AT_src_coords";
  case 0x2105: return "DW_AT_body_begin";
  case 0x2106: return "DW_AT_body_end";
  case 0x2107: return "DW_AT_GNU_vector";
  case 0x2108: return "DW_AT_GNU_guarded_by";
  case 0x2109: return "DW_AT_GNU_pt_guarded_by";
  case 0x210a: return "DW_AT_GNU_guarded";
  case 0x210b: return "DW_AT_GNU_pt_guarded";
  case 0x210c: return "DW_AT_GNU_locks_excluded";
  case 0x210d: return "DW_AT_GNU_exclusive_locks_required";
  case 0x210e: return "DW_AT_GNU_shared_locks_required";
  case 0x210f: return "DW_AT_GNU_odr_signature";
  case 0x2110: return "DW_AT_GNU_template_name";
  case 0x2111: return "DW_AT_GNU_call_site_value";
  case 0x2112: return "DW_AT_GNU_call_site_data_value";
  case 0x2113: return "DW_AT_GNU_call_site_target";
  case 0x2114: return "DW_AT_GNU_call_site_target_clobbered";
  case 0x2115: return "DW_AT_GNU_tail_call";
  case 0x2116: return "DW_AT_GNU_all_tail_call_sites";
  case 0x2117: return "DW_AT_GNU_all_call_sites";
  case 0x2118: return "DW_AT_GNU_all_source_call_sites";
  case 0x2119: return "DW_AT_GNU_macros";
  case 0x3fff: return "DW_AT_hi_user";
  default:
    return "DW_AT_unknown";
  }
}
