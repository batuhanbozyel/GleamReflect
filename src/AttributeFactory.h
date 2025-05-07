#pragma once
#include "Reflection/Attribute.h"

#include <functional>
#include <unordered_map>

namespace Gleam::Reflection {

class AttributeFactory
{
public:
    using CreateAttributeFn = std::function<IAttribute*(const std::string& args)>;
    
    static AttributeFactory& Instance()
    {
        static AttributeFactory instance;
        return instance;
    }

    ~AttributeFactory()
    {
        for (auto allocation : mAllocations)
        {
            delete allocation;
        }
    }

    template<AttributeType Attrib>
    void RegisterAttribute()
    {
        uint32_t hash = Attrib::description.hash;
        const char* name = Attrib::description.tag;
        
        auto createFn = [this](const std::string& args) -> IAttribute*
        {
            if constexpr (std::is_constructible_v<Attrib, const std::string&>)
            {
                auto attrib = new Attrib(args);
                mAllocations.push_back(attrib);
                return attrib;
            }
            else if constexpr (std::is_default_constructible_v<Attrib>)
            {
				auto attrib = new Attrib();
				mAllocations.push_back(attrib);
				return attrib;
            }
            else
            {
                return nullptr;
            }
        };
        
        mFactories[hash] = createFn;
        mNameToHash[name] = hash;
    }
    
    IAttribute* CreateAttribute(const std::string& name, const std::string& args) const
    {
        auto nameIt = mNameToHash.find(name);
        if (nameIt == mNameToHash.end())
        {
            return nullptr; // Unknown attribute type
        }
        
        uint32_t hash = nameIt->second;
        auto factoryIt = mFactories.find(hash);
        if (factoryIt == mFactories.end())
        {
            return nullptr; // No factory registered
        }
        
        return factoryIt->second(args);
    }
    
private:
    AttributeFactory() = default;
    
	std::vector<IAttribute*> mAllocations;
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