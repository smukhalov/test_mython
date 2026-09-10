#include "object_holder.h"
#include "object.h"

namespace Runtime {

ObjectHolder ObjectHolder::Share(Object& object) {
  return ObjectHolder(std::shared_ptr<Object>(&object, [](auto*) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
  return ObjectHolder();
}

Object& ObjectHolder::operator *() {
  return *Get();
}

const Object& ObjectHolder::operator *() const {
  return *Get();
}

Object* ObjectHolder::operator ->() {
  return Get();
}

const Object* ObjectHolder::operator ->() const {
  return Get();
}

Object* ObjectHolder::Get() {
  return data_.get();
}

const Object* ObjectHolder::Get() const {
  return data_.get();
}

ObjectHolder::operator bool() const {
  return Get();
}

bool IsTrue(ObjectHolder object) {
    Object* o = object.Get();
    if(o == nullptr){
        return false;
    }

    ValueObject<bool>* b = object.TryAs<ValueObject<bool>>();
    if(b != nullptr){
        return b->GetValue();
    }

    Number* number = object.TryAs<Number>();
    if(number != nullptr){
        return number->GetValue() != 0;
    }

    String* s = object.TryAs<String>();
    if(s != nullptr){
        return s->GetValue() != "";
    }

    return true;
}

}
