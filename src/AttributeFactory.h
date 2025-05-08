#pragma once
#include "Reflection/Attribute.h"

#include <functional>
#include <unordered_map>

namespace Gleam::Reflection {

struct AttributeHandle
{
	IAttribute* ptr = nullptr;
	size_t size = 0;
	uint32_t hash = 0;
};

class AttributeFactory
{
public:
    using CreateAttributeFn = std::function<AttributeHandle(const std::string& args)>;
    
    static AttributeFactory& Instance()
    {
        static AttributeFactory instance;
        return instance;
    }

    template<AttributeType Attrib>
    void RegisterAttribute()
    {
        mNameToHash[Attrib::description.tag] = Attrib::description.hash;
        mFactories[Attrib::description.hash] = [this](const std::string& args) -> AttributeHandle
        {
            if constexpr (std::is_constructible_v<Attrib, const std::string&>)
            {
				return AttributeHandle{ new Attrib(args), sizeof(Attrib), Attrib::description.hash };
            }
            else if constexpr (std::is_default_constructible_v<Attrib>)
            {
				return AttributeHandle{ new Attrib(), sizeof(Attrib), Attrib::description.hash };
            }
            else
            {
				return {};
            }
        };
    }
    
	AttributeHandle CreateAttribute(const std::string& name, const std::string& args) const
    {
        auto nameIt = mNameToHash.find(name);
        if (nameIt == mNameToHash.end())
        {
			return {}; // Unknown attribute type
        }
        
        uint32_t hash = nameIt->second;
        auto factoryIt = mFactories.find(hash);
        if (factoryIt == mFactories.end())
        {
			return {}; // No factory registered
        }
        
        return factoryIt->second(args);
    }
    
private:
    AttributeFactory() = default;
    
    std::unordered_map<std::string, uint32_t> mNameToHash;
    std::unordered_map<uint32_t, CreateAttributeFn> mFactories;
};

template<AttributeType Attrib>
struct AttributeRegistrar
{
    AttributeRegistrar()
    {
        AttributeFactory::Instance().RegisterAttribute<Attrib>();
    }
};

#define REGISTER_ATTRIBUTE(Namespace, Type) \
    namespace Namespace { \
		static inline Gleam::Reflection::AttributeRegistrar<Type> g##Type##Registrar; \
	}

} // namespace Gleam::Reflection