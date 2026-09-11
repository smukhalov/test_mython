#include "object.h"
#include "statement.h"

#include <sstream>
#include <string_view>
#include <iostream>

using namespace std;

namespace Runtime {

void ClassInstance::Print(std::ostream& os) {
    const std::string str_name("__str__");
    const Method* m = class_.GetMethod(str_name);
    if(m == nullptr){
        os << (this);
    } else {
        ObjectHolder h = Call(str_name, std::vector<ObjectHolder>());
        Object* o = h.Get(); //
        if(o){
            o->Print(os);
        } else {
            os << "None";
        }
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    const Method* m = class_.GetMethod(method);
    if(m == nullptr) {
        return false;
    }
    if(m->formal_params.size() != argument_count){
        return false;
    }
    return true;
}

const Closure& ClassInstance::Fields() const {
    return closure_;
}

Closure& ClassInstance::Fields() {
    return closure_;
}

ClassInstance::ClassInstance(const Class& cls) : class_(cls) {
}

ObjectHolder ClassInstance::Call(const std::string& method, const std::vector<ObjectHolder>& actual_args) {
    const Method* m = class_.GetMethod(method);
    if(m == nullptr){
        std::stringstream ss;
        ss << "method " << method << " not exists in class " << class_.GetName();
        throw std::invalid_argument(ss.str());
    }

    size_t params_count = actual_args.size();
    if(params_count != m->formal_params.size()){
        throw std::invalid_argument("Wrong parameters count for method " + method);
    }

    for (size_t i = 0; i < params_count; ++i) {
        Fields()[m->formal_params[i]] = actual_args[i];
    }

    std::cout << "ClassInstance::Call Fields().size() = " << Fields().size() << '\n';
    if(Fields().empty()){
        Runtime::ClassInstance object{class_};
        Fields()["self"] = Runtime::ObjectHolder::Share(object);

        //return m->body->Execute(object.Fields());
    }

    return m->body->Execute(Fields());
//    if(auto it = this->Fields().find("self"); it == this->Fields().end()) {
//        ClassInstance ci(class_);
//        for (size_t i = 0; i < params_count; ++i) {
//            ci.Fields()[m->formal_params[i]] = actual_args[i];
//        }
//        this->Fields()["self"] = ObjectHolder::Share(ci);
//    }
//
//    for(const auto& [key, value] : this->Fields()){
//        std::cout << "[" << key << "]" << '\n';
//    }
//    std::cout << "1---------------------\n";
//
//    ObjectHolder aaa =  m->body->Execute(this->Fields());
//    for(const auto& [key, value] : this->Fields()){
//        std::cout << "[" << key << "]" << '\n';
//    }
//    std::cout << "2---------------------\n";
//
//    return aaa; // m->body->Execute(ci.Fields());
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
    : name_(std::move(name)), parent_(parent) {
    size_t count = methods.size();
    methods_.resize(count, nullptr);

    for(size_t i = 0; i < count; ++i){
        Method& m_old = methods[i];
        methods_[i] = new Method(
                std::move(m_old.name),
                std::move(m_old.formal_params),
                std::move(m_old.body));
    }
}

const Method* Class::GetMethod(const std::string& name) const {
    auto it = std::find_if(methods_.begin(), methods_.end(),
                           [&name](Method* m){ return m->name == name; });
    if(it != methods_.end()){
        return *it;
    }
    if(this->parent_ != nullptr){
        const Method* m =this->parent_->GetMethod(name);
        if(m != nullptr){
            return m;
        }
    }
    return nullptr;
}

void Class::Print(ostream& os) {
    os << name_;
}

const std::string& Class::GetName() const {
    return name_;
}

void Bool::Print(std::ostream& os) {
    os << (GetValue() ? "True" : "False");
}

} /* namespace Runtime */
