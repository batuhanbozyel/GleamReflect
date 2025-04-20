#pragma once

#ifdef __GLEAM_REFLECTION__
#define GCLASS(Name, GuidStr, ...) class __attribute__((annotate("GCLASS, Guid(\"" GuidStr "\"), " #__VA_ARGS__ ")"))) Name
#define GSTRUCT(Name, GuidStr, ...) struct __attribute__((annotate("GSTRUCT, Guid(\"" GuidStr "\"), " #__VA_ARGS__ ")"))) Name
#define GENUM(Name, GuidStr, ...) enum class __attribute__((annotate("GENUM, Guid(\"" GuidStr "\"), " #__VA_ARGS__ ")"))) Name
#define GITEM(Name, GuidStr, ...) Name __attribute__((annotate("GITEM, Guid(\"" GuidStr "\"), " #__VA_ARGS__ ")")))
#define GFUNCTION(GuidStr, ...) __attribute__((annotate("GFUNCTION, Guid(\"" GuidStr "\"), " #__VA_ARGS__ ")")))
#define GFIELD(GuidStr, ...) __attribute__((annotate("GFIELD, Guid(\"" GuidStr "\"), " #__VA_ARGS__ ")")))
#else
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
    }; struct tag : AttributeBase_##tag
