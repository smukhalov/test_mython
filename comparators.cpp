#include "comparators.h"
#include "object.h"
#include "object_holder.h"

#include <functional>
#include <optional>
#include <sstream>

using namespace std;

namespace Runtime {

bool Equal(ObjectHolder lhs, ObjectHolder rhs) {
    Object* o1 = lhs.Get();
    Object* o2 = rhs.Get();
    if(o1 == nullptr && o2 == nullptr){
        return true;
    } else if(o1 == nullptr){
        return false;
    } else if(o2 == nullptr){
        return false;
    }

    ValueObject<bool>* b1 = lhs.TryAs<ValueObject<bool>>();
    ValueObject<bool>* b2 = rhs.TryAs<ValueObject<bool>>();
    if(b1 != nullptr && b2 != nullptr){
        return b1->GetValue() == b2->GetValue();
    } else if(b1 != nullptr && b2 == nullptr || b1 == nullptr && b2 != nullptr){
        throw std::invalid_argument("Equal b1 != nullptr && b2 == nullptr || b1 == nullptr && b2 != nullptr");
    }

    Number* number1 = lhs.TryAs<Number>();
    Number* number2 = rhs.TryAs<Number>();
    if(number1 != nullptr && number2 != nullptr){
        return number1->GetValue() == number2->GetValue();
    } else if(number1 != nullptr && number2 == nullptr || number1 == nullptr && number2 != nullptr){
        throw std::invalid_argument("Equal number1 != nullptr && number2 == nullptr || number1 == nullptr && number2 != nullptr");
    }

    String* string1 = lhs.TryAs<String>();
    String* string2 = lhs.TryAs<String>();
    if(string1 != nullptr && string2 != nullptr){
        return string1->GetValue() == string2->GetValue();
    } else if(string1 != nullptr && string2 == nullptr || string1 == nullptr && string2 != nullptr){
        throw std::invalid_argument("Equal string1 != nullptr && string2 == nullptr || string1 == nullptr && string2 != nullptr");
    }

    throw std::invalid_argument("Equal.  Cannot compare ");
}

bool Less(ObjectHolder lhs, ObjectHolder rhs) {
    Object* o1 = lhs.Get();
    Object* o2 = rhs.Get();
    if(o1 == nullptr || o2 == nullptr){
        return false;
    }

    ValueObject<bool>* b1 = lhs.TryAs<ValueObject<bool>>();
    ValueObject<bool>* b2 = rhs.TryAs<ValueObject<bool>>();
    if(b1 != nullptr && b2 != nullptr){
        return b1->GetValue() < b2->GetValue();
    } else if(b1 != nullptr && b2 == nullptr || b1 == nullptr && b2 != nullptr){
        throw std::invalid_argument("Less b1 != nullptr && b2 == nullptr || b1 == nullptr && b2 != nullptr");
    }

    Number* number1 = lhs.TryAs<Number>();
    Number* number2 = rhs.TryAs<Number>();
    if(number1 != nullptr && number2 != nullptr){
        return number1->GetValue() < number2->GetValue();
    } else if(number1 != nullptr && number2 == nullptr || number1 == nullptr && number2 != nullptr){
        throw std::invalid_argument("Less number1 != nullptr && number2 == nullptr || number1 == nullptr && number2 != nullptr");
    }

    String* string1 = lhs.TryAs<String>();
    String* string2 = lhs.TryAs<String>();
    if(string1 != nullptr && string2 != nullptr){
        return string1->GetValue() < string2->GetValue();
    } else if(string1 != nullptr && string2 == nullptr || string1 == nullptr && string2 != nullptr){
        throw std::invalid_argument("Less string1 != nullptr && string2 == nullptr || string1 == nullptr && string2 != nullptr");
    }

    throw std::invalid_argument("Less.  Cannot compare");
}

} /* namespace Runtime */
