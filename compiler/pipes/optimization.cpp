#include "compiler/pipes/optimization.h"

VertexPtr OptimizationPass::optimize_set_push_back(VertexAdaptor<op_set> set_op) {
  if (set_op->lhs()->type() != op_index) {
    return set_op;
  }
  VertexAdaptor<op_index> index = set_op->lhs();
  if (index->has_key() && set_op->rl_type != val_none) {
    return set_op;
  }

  VertexPtr a, b, c;
  a = index->array();
  if (index->has_key()) {
    b = index->key();
  }
  c = set_op->rhs();

  VertexPtr result;

  if (!b) {
    // запрещаем '$s[] = ...' для не-массивов; для массивов превращаем в push_back
    PrimitiveType a_ptype = tinf::get_type(a)->get_real_ptype();
    kphp_error (a_ptype == tp_array || a_ptype == tp_var,
                dl_pstr("Can not use [] for %s", type_out(tinf::get_type(a)).c_str()));

    if (set_op->rl_type == val_none) {
      result = VertexAdaptor<op_push_back>::create(a, c);
    } else {
      result = VertexAdaptor<op_push_back_return>::create(a, c);
    }
  } else {
    result = VertexAdaptor<op_set_value>::create(a, b, c);
  }
  result->location = set_op->get_location();
  result->extra_type = op_ex_internal_func;
  result->rl_type = set_op->rl_type;
  return result;
}
void OptimizationPass::collect_concat(VertexPtr root, vector<VertexPtr> *collected) {
  if (root->type() == op_string_build || root->type() == op_concat) {
    for (auto i : *root) {
      collect_concat(i, collected);
    }
  } else {
    collected->push_back(root);
  }
}
VertexPtr OptimizationPass::optimize_string_building(VertexPtr root) {
  vector<VertexPtr> collected;
  collect_concat(root, &collected);
  auto new_root = VertexAdaptor<op_string_build>::create(collected);
  new_root->location = root->get_location();
  new_root->rl_type = root->rl_type;

  return new_root;
}
VertexPtr OptimizationPass::optimize_postfix_inc(VertexPtr root) {
  if (root->rl_type == val_none) {
    auto new_root = VertexAdaptor<op_prefix_inc>::create(root.as<op_postfix_inc>()->expr());
    new_root->rl_type = root->rl_type;
    new_root->location = root->get_location();
    root = new_root;
  }
  return root;
}
VertexPtr OptimizationPass::optimize_postfix_dec(VertexPtr root) {
  if (root->rl_type == val_none) {
    auto new_root = VertexAdaptor<op_prefix_dec>::create(root.as<op_postfix_dec>()->expr());
    new_root->rl_type = root->rl_type;
    new_root->location = root->get_location();
    root = new_root;
  }
  return root;
}
VertexPtr OptimizationPass::optimize_index(VertexAdaptor<op_index> index) {
  bool has_key = index->has_key();
  if (!has_key) {
    if (index->rl_type == val_l) {
      kphp_error (0, "Unsupported []");
    } else {
      kphp_error (0, "Cannot use [] for reading");
    }
    return index;
  }
  if (index->rl_type != val_l) {
    index->extra_type = op_ex_index_rval;
  }
  return index;
}
VertexPtr OptimizationPass::optimize_instance_prop(VertexAdaptor<op_instance_prop> index) {
  if (index->rl_type != val_l) {
    index->extra_type = op_ex_index_rval;
  }
  return index;
}
template<Operation FromOp, Operation ToOp>
VertexPtr OptimizationPass::fix_int_const(VertexPtr from, const string &from_func) {
  VertexPtr *tmp;
  if (from->type() == FromOp) {
    tmp = &from.as<FromOp>()->expr();
  } else if (from->type() == op_func_call &&
             from.as<op_func_call>()->str_val == from_func) {
    tmp = &from.as<op_func_call>()->args()[0];
  } else {
    return from;
  }
  if ((*tmp)->type() == op_minus) {
    tmp = &(*tmp).as<op_minus>()->expr();
  }
  if ((*tmp)->type() != op_int_const) {
    return from;
  }

  auto res = VertexAdaptor<ToOp>::create();
  res->str_val = (*tmp)->get_string();
  //FIXME: it should be a copy
  res->rl_type = from->rl_type;
  *tmp = res;
  return from;
}
VertexPtr OptimizationPass::fix_int_const(VertexPtr root) {
  root = fix_int_const<op_conv_uint, op_uint_const>(root, "uintval");
  root = fix_int_const<op_conv_long, op_long_const>(root, "longval");
  root = fix_int_const<op_conv_ulong, op_ulong_const>(root, "ulongval");
  return root;
}
VertexPtr OptimizationPass::remove_extra_conversions(VertexPtr root) {
  VertexPtr expr = root.as<meta_op_unary_op>()->expr();
  const TypeData *tp = tinf::get_type(expr);
  if (tp->use_or_false() == false) {
    VertexPtr res;
    if ((root->type() == op_conv_int || root->type() == op_conv_int_l) && tp->ptype() == tp_int) {
      res = expr;
    } else if (root->type() == op_conv_bool && tp->ptype() == tp_bool) {
      res = expr;
    } else if (root->type() == op_conv_float && tp->ptype() == tp_float) {
      res = expr;
    } else if (root->type() == op_conv_string && tp->ptype() == tp_string) {
      res = expr;
    } else if ((root->type() == op_conv_array || root->type() == op_conv_array_l) && tp->get_real_ptype() == tp_array) {
      res = expr;
    } else if (root->type() == op_conv_uint && tp->ptype() == tp_UInt) {
      res = expr;
    } else if (root->type() == op_conv_long && tp->ptype() == tp_Long) {
      res = expr;
    } else if (root->type() == op_conv_ulong && tp->ptype() == tp_ULong) {
      res = expr;
      //} else if (root->type() == op_conv_regexp && tp->ptype() != tp_string) {
      //res = expr;
    }
    if (res) {
      res->rl_type = root->rl_type;
      root = res;
    }
  }
  return root;
}
VertexPtr OptimizationPass::on_enter_vertex(VertexPtr root, FunctionPassBase::LocalT *) {
  if (OpInfo::type(root->type()) == conv_op || root->type() == op_conv_array_l || root->type() == op_conv_int_l) {
    root = remove_extra_conversions(root);
  }

  if (root->type() == op_set) {
    root = optimize_set_push_back(root);
  } else if (root->type() == op_string_build || root->type() == op_concat) {
    root = optimize_string_building(root);
  } else if (root->type() == op_postfix_inc) {
    root = optimize_postfix_inc(root);
  } else if (root->type() == op_postfix_dec) {
    root = optimize_postfix_dec(root);
  } else if (root->type() == op_index) {
    root = optimize_index(root);
  } else if (root->type() == op_instance_prop) {
    root = optimize_instance_prop(root);
  }

  root = fix_int_const(root);

  if (root->rl_type != val_none/* && root->rl_type != val_error*/) {
    tinf::get_type(root);
  }
  return root;
}