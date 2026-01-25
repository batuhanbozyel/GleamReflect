# GleamReflect
Gleam Reflect is a Clang/LLVM-based reflection system for C++. Designed mainly to provide backward compatible serialization and aid UI development for [Gleam Engine](https://github.com/batuhanbozyel/GleamEngine). It supports C#-like attribute system although currently all attributes are part of reflection tool, not supporting custom attributes.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

## How to Use

### 1. Annotate Your C++ Types

GleamReflect relies on a macro-based annotation system to mark types, fields, and enums for reflection. Each reflected type and fields are associated with a stable GUID and optional attributes (e.g. Serializable).
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