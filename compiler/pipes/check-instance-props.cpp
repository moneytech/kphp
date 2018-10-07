#include "compiler/pipes/check-instance-props.h"

#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"

VertexPtr CheckInstanceProps::on_enter_vertex(VertexPtr v, LocalT *) {

  if (v->type() == op_instance_prop) {
    ClassPtr klass = resolve_class_of_arrow_access(current_function, v);
    if (klass) {   // если null, то ошибка доступа к непонятному свойству уже кинулась в resolve_class_of_arrow_access()
      VarPtr var = klass->find_var(v->get_string());

      if (!kphp_error(var,
                      dl_pstr("Invalid property access ...->%s: does not exist in class %s", v->get_string().c_str(), klass->name.c_str()))) {
        v->set_var_id(var);
        init_class_instance_var(v, var, klass);
      }
    }
  }

  return v;
}
void CheckInstanceProps::init_class_instance_var(VertexPtr v, VarPtr var, ClassPtr klass) {
  kphp_assert(var->class_id == klass);

  // сейчас phpdoc у class var'а парсится каждый раз при ->обращении;
  // это уйдёт, когда вместо phpdoc_token будем хранить что-то более умное (что парсится 1 раз и хранит type_rule)
  if (var->phpdoc_token) {
    std::string var_name, type_str;
    if (PhpDocTypeRuleParser::find_tag_in_phpdoc(var->phpdoc_token->str_val, php_doc_tag::var, var_name, type_str)) {
      VertexPtr doc_type = phpdoc_parse_type(type_str, klass->init_function);
      if (!kphp_error(doc_type,
                      dl_pstr("Failed to parse phpdoc of %s::$%s", klass->name.c_str(), var->name.c_str()))) {
        doc_type->location = klass->root->location;
        auto doc_type_check = VertexAdaptor<op_lt_type_rule>::create(doc_type);
        v->type_rule = doc_type_check;
      }
    }
  }
}