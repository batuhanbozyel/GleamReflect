# GleamReflect
Gleam Reflect is a Clang/LLVM-based reflection system for C++. Designed mainly to provide backward compatible serialization and aid UI development for [Gleam Engine](https://github.com/batuhanbozyel/GleamEngine). It supports a C#-like attribute system with user-defined attributes.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

## How to Use

### 1. Annotate Your C++ Types

GleamReflect relies on a macro-based annotation system to mark types, fields, and enums for reflection. Each reflected type and fields are associated with a stable GUID and optional attributes. `Guid` is the only built-in attribute; everything else (e.g. `Serializable` below) is user-defined, see [Custom Attributes](#custom-attributes).
Include the reflection macros header in files you want to reflect:
```cpp
#include <Reflection/Macro.h>

GSTRUCT(Size, "67C7965D-59AD-4506-9DE9-C1902B126DDA", Serializable)
{
    GFIELD("9D34DD6A-2C5D-47A6-8B1C-652C1847B590", Serializable)
    float width = 0.0f;

    GFIELD("C655B9C2-CE7C-4827-8221-0D2EB0584418", Serializable)
    float height = 0.0f;
};

GENUM(WindowFlag, "193C9225-5267-46DD-BAED-9CBE464BB5CE", Serializable)
{
    GITEM(BorderlessFullscreen, "4C1556D5-3768-4253-B326-4914439C5392") = 0,
    GITEM(ExclusiveFullscreen, "1F337950-09AE-4B7C-AACC-9BED99630418") = 1,
    GITEM(MaximizedWindow, "3BE7B62B-CCC5-4377-87EE-765073A2E2B6") = 2,
    GITEM(CustomWindow, "AF98F11E-512E-4ECA-8BEE-FA152AF2B210") = 3
};
```

#### Custom Attributes

Attributes are plain C++ types declared inside `namespace Gleam::Reflection::Attribute`. The reflection tool evaluates them as constant expressions with Clang and stores their bytes in the database, so no registration is needed.

```cpp
#include <Reflection/Attribute.h>

namespace Gleam::Reflection::Attribute {

// Flag attribute, used without parentheses: GFIELD("...", Serializable)
GLEAM_TAG_ATTRIBUTE(Serializable);

// Data attribute, used as a constructor call: GFIELD("...", Range(0.0f, 1.0f))
GLEAM_ATTRIBUTE(Range)
{
    float min;
    float max;

    constexpr Range(float min, float max)
        : min(min), max(max)
    {
    }
};

} // namespace Gleam::Reflection::Attribute
```

Requirements:
* Attributes must be constructible in a constant expression (`constexpr` constructors, every member initialized).
* Members may be arithmetic types, enums, fixed-size arrays and nested structs/unions. Pointers, references and bit-fields are rejected.
* Annotation arguments are split by the preprocessor, so use `Range(0.0f, 1.0f)` instead of `Range{0.0f, 1.0f}`.

Query attributes at runtime with `HasAttribute<Attribute::Range>()` / `GetAttribute<Attribute::Range>()`.

The tool parses every header as its own translation unit, so each annotated header must include the definitions of the attributes it uses (e.g. through a header that defines them and includes `<Reflection/Attribute.h>`).

### 2. Run the Reflection Tool

`GleamReflect` is executed as a standalone tool that parses your C++ headers and source files using Clang/LLVM. It generates both a binary reflection database and a C++ header for runtime access.

The tool expects the following arguments:

- `--module`  
  Module name. This determines the output file names:
  - `ModuleName.Reflection.generated.h`
  - `ModuleName.Reflection.db`

- `--header-dir`  
  Output directory for the generated header file.

- `--binary-dir`  
  Output directory for the generated reflection database.

- Header and source files to be parsed by the tool.

- Any additional compiler flags required to compile your code  
  (include paths, defines, C++ standard, etc.).

### 3. Generated Outputs

The reflection tool produces two artifacts:

* **Binary Reflection Database (.Reflection.db)**
Stores all reflection metadata in a compact binary format suitable for fast runtime access.

* **Generated Header File (.Reflection.generated.h)**
Provides strongly-typed runtime helper functions for querying reflection data from the database.

#### Linking Modules

Databases of multiple modules can be merged into a single database loaded by the application:

```
CppReflection --link --output=<linked.db> [--natvis=<file.natvis>] <module.db>...
```

When `--natvis` is given, the linker also generates a natvis file that visualizes reflection descriptions together with their typed attribute values. Add it to the executable target alongside `GleamReflect.natvis`, which covers the remaining reflection types.

### 4. Initialize Reflection at Runtime

At application startup, initialize the reflection database by loading the generated binary file:

```cpp
Gleam::Reflection::Database reflection;
reflection.Initialize("Path.To.ModuleName.Reflection.db");
```

### 5. Accessing Reflection Metadata at Runtime
Runtime access to reflection metadata is provided through the companion header:
```cpp
#include "Reflection/Reflection.h"

// Templated accessor
const auto& classDesc = Gleam::Reflection::GetClass<Gleam::EngineConfig>();

// or fetch via qualified typename/typehash
// type hash is generated from qualified name
const ClassDescription* byHash = Gleam::Reflection::GetClass(typeHash);
const ClassDescription* byName = Gleam::Reflection::GetClass("Gleam::EngineConfig");

// for primitives, type hash is identical to PrimitiveType enum value
auto floatDesc = Gleam::Reflection::GetPrimitive<float>();
auto intType   = Gleam::Reflection::GetPrimitiveType(hash);
```