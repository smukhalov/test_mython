#include "statement.h"
#include "object.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace Ast {

using Runtime::Closure;

void Dump(const std::string& prefix, const Closure& closure, std::ostream& out = std::cout){
    out << prefix << '\n';
    for(const auto& [key, value] : closure){
        out << "[" << key << "]" << '\n';
    }
    out << "-----------------\n";
}

ObjectHolder Assignment::Execute(Closure& closure) {
    Dump("Assignment::Execute before. var_name - " + var_name, closure);
    closure[var_name] = right_value->Execute(closure);
    Dump("Assignment::Execute after", closure);

    return closure[var_name];
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    : var_name(std::move(var)), right_value(std::move(rv)) {
    std::string a = var_name;
}

VariableValue::VariableValue(std::string var_name) {
    dotted_ids_.push_back((std::move(var_name)));
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) : dotted_ids_(std::move(dotted_ids)) {
}

ObjectHolder VariableValue::Execute(Closure& closure) {
    size_t count = dotted_ids_.size();
    for(size_t i = count; i > 0; --i){
        std::stringstream ss;
        bool not_first_element = false;
        for(size_t j = 0; j < i; ++j){
            const std::string& id = dotted_ids_[j];
            if(not_first_element){
                ss << '.';
            } else {
                not_first_element = true;
            }
            ss << id;
        }
        std::string s = ss.str();
        if(auto it = closure.find(s); it != closure.end()){
            ObjectHolder oh = it->second;
            Runtime::ClassInstance* ci = oh.TryAs<Runtime::ClassInstance>();
            if(ci) {
                Dump("ci->Fields()", ci->Fields());

                return ci->Fields()[dotted_ids_[count-1]];
//                std::string qq = dotted_ids_[i-1];
//                std::cout << "qq - " << qq << '\n';
//                return ci->Fields()[dotted_ids_[i-1]];
                //return ci->Fields()["self"];
            }
            return oh;
        }
    }

    std::stringstream ss;
    bool not_first_element = false;
    for(size_t i = 0; i < count; ++i){
        const std::string& id = dotted_ids_[i];
        if(not_first_element){
            ss << '.';
        } else {
            not_first_element = true;
        }
        ss << id;
    }
    std::stringstream ss_closure;
    Dump("VariableValue::Execute", closure, ss_closure);

    throw std::runtime_error("VariableValue::Execute. Variable " + ss.str()
        + " is not found in closure. Closure.size() = " + std::to_string(closure.size())
        + "\n" + ss_closure.str());
}

unique_ptr<Print> Print::Variable(std::string var) {
    return std::make_unique<Print>(std::make_unique<VariableValue>(var));
}

Print::Print(unique_ptr<Statement> argument) {
    args_.push_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args) {
    args_ = std::move(args);
}

ObjectHolder Print::Execute(Closure& closure) {
    size_t count = args_.size();
    for(size_t i = 0; i < count; ++i){
        std::unique_ptr<Statement>& x = args_[i];

        ObjectHolder oh = x->Execute(closure);
        Runtime::Object* o = oh.Get();
        if(o == nullptr){
            *output_ << "None";
        } else {
            o->Print(*output_);
        }

        if(i < count -1){
            *output_ << ' ';
        }
    }
    *output_ << '\n';
    return {};
}

ostream* Print::output_ = &cout;

void Print::SetOutputStream(ostream& output_stream) {
  output_ = &output_stream;
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method, std::vector<std::unique_ptr<Statement>> args)
    : object_(std::move(object)), method_(std::move(method)), args_(std::move(args))
{}

ObjectHolder MethodCall::Execute(Closure& closure) {
    ObjectHolder oh = object_->Execute(closure);
    Runtime::ClassInstance* ci = oh.TryAs<Runtime::ClassInstance>();
    if(ci){
        size_t count = args_.size();
        if(ci->HasMethod(method_, count)){
            std::vector<ObjectHolder> actual_args(count);
            for(size_t i = 0; i < count; ++i){
                actual_args[i] = args_[i]->Execute(closure);
            }
            ci->Call(method_, actual_args);
        }
        return {};
    }
    throw std::runtime_error("MethodCall::Execute oh.TryAs<Runtime::ClassInstance>() == nullptr");
}

ObjectHolder Stringify::Execute(Closure& closure) {
    ObjectHolder  oh = argument_->Execute(closure);
    std::stringstream ss;
    oh->Print(ss);
    return ObjectHolder::Own(Runtime::String(ss.str()));
}

ObjectHolder Add::Execute(Closure& closure) {
    ObjectHolder lh = lhs->Execute(closure);
    ObjectHolder rh = rhs->Execute(closure);

    if(auto lhs = lh.TryAs<Runtime::Number>(), rhs = rh.TryAs<Runtime::Number>();
        lhs && rhs){
        return ObjectHolder::Own(Runtime::Number(lhs->GetValue() + rhs->GetValue()));
    }

    if(auto lhs = lh.TryAs<Runtime::String>(), rhs = rh.TryAs<Runtime::String>();
            lhs && rhs){
        return ObjectHolder::Own(Runtime::String(lhs->GetValue() + rhs->GetValue()));
    }

    if(lh.TryAs<Runtime::ClassInstance>() && (rh.TryAs<Runtime::Number>() || rh.TryAs<Runtime::String>())
        || rh.TryAs<Runtime::ClassInstance>() && (lh.TryAs<Runtime::Number>() || lh.TryAs<Runtime::String>())
    ){
        Runtime::ClassInstance* ci = lh.TryAs<Runtime::ClassInstance>();
        if(!ci){
            ci = rh.TryAs<Runtime::ClassInstance>();
        }

        if(ci->HasMethod("__add__", 1)){
            return ci->Call("__add__", std::vector<ObjectHolder>{
                rh.TryAs<Runtime::Number>() || rh.TryAs<Runtime::String>() ? rh : lh
            });
        }
    }
    throw std::runtime_error("Add::Execute");
}

ObjectHolder Sub::Execute(Closure& closure) {
    ObjectHolder lh = lhs->Execute(closure);
    ObjectHolder rh = rhs->Execute(closure);

    if(auto lhs = lh.TryAs<Runtime::Number>(), rhs = rh.TryAs<Runtime::Number>();
            lhs && rhs){
        return ObjectHolder::Own(Runtime::Number(lhs->GetValue() - rhs->GetValue()));
    }
    throw std::runtime_error("Sub::Execute");
}

ObjectHolder Mult::Execute(Runtime::Closure& closure) {
    ObjectHolder lh = lhs->Execute(closure);
    ObjectHolder rh = rhs->Execute(closure);

    if(auto lhs = lh.TryAs<Runtime::Number>(), rhs = rh.TryAs<Runtime::Number>();
            lhs && rhs){
        return ObjectHolder::Own(Runtime::Number(lhs->GetValue() * rhs->GetValue()));
    }
    throw std::runtime_error("Mult::Execute");
}

ObjectHolder Div::Execute(Runtime::Closure& closure) {
    ObjectHolder lh = lhs->Execute(closure);
    ObjectHolder rh = rhs->Execute(closure);

    if(auto lhs = lh.TryAs<Runtime::Number>(),
            rhs = rh.TryAs<Runtime::Number>();
            lhs && rhs){

        int r = rhs->GetValue();
        if(r == 0){
            throw std::invalid_argument("Div::Execute denominator == 0");
        }
        return ObjectHolder::Own(Runtime::Number(lhs->GetValue() / r));
    }
    throw std::runtime_error("Div::Execute");
}

ObjectHolder Compound::Execute(Closure& closure) {
    std::cout << "Compound::Execute. statements_.count - " << statements_.size() << ", closure.size = " << closure.size() << '\n';
    for(std::unique_ptr<Statement>& x : statements_){
        Dump("Compound::Execute before", closure);
        ObjectHolder  oh = x->Execute(closure);
        Dump("Compound::Execute after", closure);
    }
    return {};
}

ObjectHolder Return::Execute(Closure& closure) {
    return statement_->Execute(closure);
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
    : class_name_(reinterpret_cast<const Runtime::Class&>(*cls).GetName()), cls_(std::move(cls)) {
}

ObjectHolder ClassDefinition::Execute(Runtime::Closure& closure) {
    return cls_;
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv)
  : object_(std::move(object)), field_name_(std::move(field_name)), right_value_(std::move(rv))
{}

ObjectHolder FieldAssignment::Execute(Runtime::Closure& closure) {
    ObjectHolder oh = object_.Execute(closure);
    Runtime::ClassInstance* ci = oh.TryAs<Runtime::ClassInstance>();
    if(!ci){
        throw std::runtime_error("FieldAssignment::Execute. object_ is not Runtime::ClassInstance");
    }
    std::string s = field_name_;
    closure[field_name_] = right_value_->Execute(closure);
    return closure[field_name_];
//    ci->Fields()[field_name_] = right_value_->Execute(closure);
//    return ci->Fields()[field_name_]; // right_value_->Execute(closure);
}

IfElse::IfElse(
  std::unique_ptr<Statement> condition,
  std::unique_ptr<Statement> if_body,
  std::unique_ptr<Statement> else_body
)
{} //TODO!!

ObjectHolder IfElse::Execute(Runtime::Closure& closure) {
    //TODO!!
    throw std::runtime_error("IfElse::Execute");
}

ObjectHolder Or::Execute(Runtime::Closure& closure) {
    //TODO!!
    throw std::runtime_error("Or::Execute");
}

ObjectHolder And::Execute(Runtime::Closure& closure) {
    //TODO!!
    throw std::runtime_error("And::Execute");
}

ObjectHolder Not::Execute(Runtime::Closure& closure) {
    //TODO!!
    throw std::runtime_error("Not::Execute");
}

Comparison::Comparison(
  Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs
) {
} //TODO!!

ObjectHolder Comparison::Execute(Runtime::Closure& closure) {
    //TODO!!
    throw std::runtime_error("Comparison::Execute");
}

NewInstance::NewInstance(
  const Runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args
) : class_(class_), args_(std::move(args))
{}

NewInstance::NewInstance(const Runtime::Class& class_) : NewInstance(class_, {})
{}

ObjectHolder NewInstance::Execute(Runtime::Closure& closure) {
    Runtime::ClassInstance object{class_};
    object.Fields()["self"] = Runtime::ObjectHolder::Share(object);

    const Runtime::Method* m = class_.GetMethod("__init__");
    if(m){
        size_t count = args_.size();
        std::vector<ObjectHolder> actual_args(count);

        for(size_t i = 0; i < count; ++i){
            actual_args[i] = std::move(args_[i]->Execute(closure));
        }

        object.Call("__init__", actual_args);
    }

    return Runtime::ObjectHolder::Own(std::move(object));
}


} /* namespace Ast */
