#pragma once
#include "Meta.h"

#include <unordered_map>

namespace Gleam {
class ReflectionParser;
} // namespace Gleam

namespace Gleam::Reflection {

class Database
{
    friend class Gleam::ReflectionParser;
    using EnumMap = std::unordered_map<Attribute::Guid, EnumDescription>;
    using ClassMap = std::unordered_map<Attribute::Guid, ClassDescription>;
public:
    
private:
    EnumMap mGuidToEnum;
    ClassMap mGuidToClass;
    
};

} // namespace Gleam::Reflection
