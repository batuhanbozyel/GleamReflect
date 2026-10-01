#pragma once
#define GLEAM_REFLECTION_VERSION 1

#ifdef __GLEAM_REFLECTION__
#define GLEAM_REFLECTION_EXPAND(...) GLEAM_REFLECTION_EXPAND4(GLEAM_REFLECTION_EXPAND4(GLEAM_REFLECTION_EXPAND4(GLEAM_REFLECTION_EXPAND4(__VA_ARGS__))))
#define GLEAM_REFLECTION_EXPAND4(...) GLEAM_REFLECTION_EXPAND3(GLEAM_REFLECTION_EXPAND3(GLEAM_REFLECTION_EXPAND3(GLEAM_REFLECTION_EXPAND3(__VA_ARGS__))))
#define GLEAM_REFLECTION_EXPAND3(...) GLEAM_REFLECTION_EXPAND2(GLEAM_REFLECTION_EXPAND2(GLEAM_REFLECTION_EXPAND2(GLEAM_REFLECTION_EXPAND2(__VA_ARGS__))))
#define GLEAM_REFLECTION_EXPAND2(...) GLEAM_REFLECTION_EXPAND1(GLEAM_REFLECTION_EXPAND1(GLEAM_REFLECTION_EXPAND1(GLEAM_REFLECTION_EXPAND1(__VA_ARGS__))))
#define GLEAM_REFLECTION_EXPAND1(...) __VA_ARGS__
#define GLEAM_REFLECTION_PARENS ()
#define GLEAM_REFLECTION_ATTRIBUTES(...) __VA_OPT__(GLEAM_REFLECTION_EXPAND(GLEAM_REFLECTION_ATTRIBUTES_HELPER(__VA_ARGS__)))
#define GLEAM_REFLECTION_ATTRIBUTES_HELPER(attribute, ...) , ::Gleam::Reflection::Attribute::attribute __VA_OPT__(GLEAM_REFLECTION_ATTRIBUTES_AGAIN GLEAM_REFLECTION_PARENS (__VA_ARGS__))
#define GLEAM_REFLECTION_ATTRIBUTES_AGAIN() GLEAM_REFLECTION_ATTRIBUTES_HELPER
#define GLEAM_REFLECTION_ANNOTATE(Tag, GuidStr, ...) __attribute__((annotate(Tag, ::Gleam::Reflection::Attribute::Guid(GuidStr) GLEAM_REFLECTION_ATTRIBUTES(__VA_ARGS__))))
#define GLEAM_ATTRIBUTE_ANNOTATION __attribute__((annotate("GATTRIBUTE")))

#define GCLASS(Name, GuidStr, ...) class GLEAM_REFLECTION_ANNOTATE("GCLASS", GuidStr, __VA_ARGS__) Name
#define GSTRUCT(Name, GuidStr, ...) struct GLEAM_REFLECTION_ANNOTATE("GSTRUCT", GuidStr, __VA_ARGS__) Name
#define GENUM(Name, GuidStr, ...) enum class GLEAM_REFLECTION_ANNOTATE("GENUM", GuidStr, __VA_ARGS__) Name
#define GITEM(Name, GuidStr, ...) Name GLEAM_REFLECTION_ANNOTATE("GITEM", GuidStr, __VA_ARGS__)
#define GFUNCTION(GuidStr, ...) GLEAM_REFLECTION_ANNOTATE("GFUNCTION", GuidStr, __VA_ARGS__)
#define GFIELD(GuidStr, ...) GLEAM_REFLECTION_ANNOTATE("GFIELD", GuidStr, __VA_ARGS__)
#else
#define GLEAM_ATTRIBUTE_ANNOTATION
#define GCLASS(Name, GuidStr, ...) class Name
#define GSTRUCT(Name, GuidStr, ...) struct Name
#define GENUM(Name, GuidStr, ...) enum class Name
#define GITEM(Name, GuidStr, ...) Name
#define GFUNCTION(GuidStr, ...)
#define GFIELD(GuidStr, ...)
#endif

#define GLEAM_ATTRIBUTE(tag, ...)                                                                   \
    struct AttributeBase_##tag : Gleam::Reflection::IAttribute {                                    \
        static constexpr auto description = Gleam::Reflection::AttributeDescription(#tag);          \
    }; struct GLEAM_ATTRIBUTE_ANNOTATION tag : AttributeBase_##tag

#define GLEAM_TAG_ATTRIBUTE(tag)                                                                    \
    GLEAM_ATTRIBUTE(tag) {};                                                                        \
    inline constexpr struct tag tag{}

#ifdef __GLEAM_REFLECTION__
#include "Attribute.h"
#endif
