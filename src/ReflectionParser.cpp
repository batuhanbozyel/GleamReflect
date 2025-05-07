#include "ReflectionParser.h"
#include "Attributes.h"
#include "Serialization/BinaryWriter.h"

#include <clang/AST/AST.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/CompilerInstance.h>

#include <fstream>
#include <cassert>
#include <regex>

using namespace Gleam;

ReflectionParser::ReflectionParser()
    : mContext("", "") // global namespace
{
    
}

void ReflectionParser::ParseAST(clang::ASTContext& context)
{
    ParseDecls(mContext, context.getTranslationUnitDecl()->decls());
}

void ReflectionParser::ParseDecls(ReflectionContext& context, const clang::DeclContext::decl_range& decls)
{
    for (const auto decl : decls)
    {
        if (const auto enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl))
        {
            if (enumDecl->hasAttr<clang::AnnotateAttr>())
            {
                HandleEnumDecl(context, enumDecl);
            }
        }
        else if (const auto recordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
        {
            if (recordDecl->hasAttr<clang::AnnotateAttr>() && recordDecl->isCompleteDefinition())
            {
                HandleRecordDecl(context, recordDecl);
            }
        }
        else if (const auto namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(decl))
        {
            ReflectionContext namespaceCtx(namespaceDecl->getName(),
                                           namespaceDecl->getQualifiedNameAsString());
            ParseDecls(namespaceCtx, namespaceDecl->decls());
            context.EmplaceContext(namespaceCtx);
        }
    }
}

void ReflectionParser::GenerateOutput(const std::filesystem::path& outputDir)
{
    std::stringstream generatedCode;
    generatedCode << "#ifndef __GLEAM_REFLECTION__\n";
	generatedCode << "#include <Reflection/Meta.h>\n";
    mContext.GenerateForwardDecls(generatedCode);
    
    generatedCode << "namespace Gleam::Reflection {\n\n";

	generatedCode << "template<typename T>\n";
	generatedCode << "const EnumDescription& GetEnum()\n";
	generatedCode << "{\n";
	generatedCode << "\tstatic_assert(false, \"Enum is not reflected\");\n";
	generatedCode << "}\n\n";

    generatedCode << "template<typename T>\n";
    generatedCode << "const ClassDescription& GetClass()\n";
    generatedCode << "{\n";
    generatedCode << "\tstatic_assert(false, \"Class is not reflected\");\n";
    generatedCode << "}\n\n";

    mContext.GenerateMetaDescs(generatedCode);
    generatedCode << "} // namespace Gleam::Reflection\n";
    
    generatedCode << "#endif // __GLEAM_REFLECTION__\n";
    
    // Generated header
	{
		auto filename = outputDir / "Reflection.generated.h";
		std::ofstream file(filename, std::ios::out | std::ios::trunc);

		auto generatedCodeStr = generatedCode.str();
		file.write(generatedCodeStr.c_str(), generatedCodeStr.length());
	}

	// Generated database
    {
		auto filename = outputDir / "Reflection.db";
		std::ofstream file(filename, std::ios::out | std::ios::trunc | std::ios::binary);
        
        Reflection::BinaryWriter writer;
        
        Reflection::DatabaseHeader header;
        header.version = 0;
        header.classCount = static_cast<uint32_t>(mContext.mClasses.size());
        header.enumCount = static_cast<uint32_t>(mContext.mEnums.size());
		memcpy(header.magic, "GLEAMREF", sizeof(header.magic));
        writer.Write(header);

        const auto& objectBuffer = mObjectWriter.GetBuffer();
        writer.Write(objectBuffer.data, objectBuffer.size);
        
        auto serializedHeader = reinterpret_cast<Reflection::DatabaseHeader*>(writer.GetBuffer().data);
        
        serializedHeader->classTableOffset = writer.GetCursor();
        writer.Write(mContext.mClasses.data(), mContext.mClasses.size() * sizeof(Reflection::ClassDescription));
        
        serializedHeader->enumTableOffset = writer.GetCursor();
        writer.Write(mContext.mEnums.data(), mContext.mEnums.size() * sizeof(Reflection::EnumDescription));
        
        const auto& buffer = writer.GetBuffer();
		file.write(reinterpret_cast<const char*>(buffer.data), writer.GetCursor());
    }
}

EnumHandle ReflectionParser::HandleEnumDecl(ReflectionContext& context, const clang::EnumDecl* enumDecl)
{
    auto enumAnnotateAttr = enumDecl->getAttr<clang::AnnotateAttr>();
    auto enumAnnotation = enumAnnotateAttr->getAnnotation().str();
    
    if (enumAnnotation.find("GENUM") != std::string::npos)
    {
        const auto& attributes = ParseAttributes(enumAnnotation);
        const auto& guid = ExtractGuid(attributes);
        assert(guid != Reflection::Attribute::Guid::InvalidGuid() &&  "Enum is missing GUID attribute");
        
        if (const auto enumHandle = context.GetEnumHandle(guid); enumHandle != InvalidMetaIndex)
        {
            return enumHandle; // already processed
        }
        
        std::stringstream qualifiedName;
        qualifiedName << context.QualifiedName() << "::" << std::string_view(enumDecl->getName());
        
        auto& astContext = enumDecl->getASTContext();
        uint32_t typeHash = Reflection::Utils::HashString(qualifiedName.str().c_str());
        size_t enumSize = astContext.getTypeSize(astContext.getEnumType(enumDecl)) / 8ul; // Convert bits to bytes

		std::vector<Reflection::EnumCaseDescription> enumCases;
        for (const auto enumItem : enumDecl->enumerators())
        {
            if (enumItem->hasAttr<clang::AnnotateAttr>() == false)
            {
                continue; // item is not reflected
            }
            
            auto itemAnnotateAttr = enumItem->getAttr<clang::AnnotateAttr>();
            auto itemAnnotation = itemAnnotateAttr->getAnnotation().str();
            
            if (itemAnnotation.find("GITEM") != std::string::npos)
            {
                const auto& itemAttributes = ParseAttributes(itemAnnotation);
                const auto& itemGuid = ExtractGuid(itemAttributes);
                assert(itemGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Enum case is missing GUID attribute");
                
                Reflection::EnumCaseDescription enumCase({
                    enumItem->getName(),
                    context.Name(),
                    context.QualifiedName(),
                    itemAttributes,
                    itemGuid,
                    typeHash
                }, enumItem->getInitVal().getExtValue());
                enumCases.emplace_back(enumCase);
            }
        }
        Reflection::BufferView cases = mObjectWriter.Write(enumCases.data(), enumCases.size() * sizeof(Reflection::EnumCaseDescription));
        return context.RegisterEnum(Reflection::EnumDescription({ enumDecl->getName(), context.Name(), context.QualifiedName(), attributes, guid, typeHash }, enumSize, cases));
    }
    return {};
}

ClassHandle ReflectionParser::HandleRecordDecl(ReflectionContext& context, const clang::CXXRecordDecl* recordDecl)
{
    if (recordDecl == nullptr)
    {
        return {};
    }
    
    auto recordAnnotateAttr = recordDecl->getAttr<clang::AnnotateAttr>();
    auto recordAnnotation = recordAnnotateAttr->getAnnotation().str();
    
    if (recordAnnotation.find("GCLASS") != std::string::npos || recordAnnotation.find("GSTRUCT") != std::string::npos)
    {
        const auto& recordAttribs = ParseAttributes(recordAnnotation);
        const auto& recordGuid = ExtractGuid(recordAttribs);
        assert(recordGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Record is missing GUID attribute");
        
        if (const auto classHandle = context.GetClassHandle(recordGuid); classHandle != InvalidMetaIndex)
        {
            return classHandle; // already processed
        }
        
        auto& astContext = recordDecl->getASTContext();
        size_t classSize = astContext.getTypeSize(astContext.getRecordType(recordDecl)) / 8ul; // Convert bits to bytes
        
        // Process bases
		std::vector<ClassHandle> baseClasses;
        for (const auto& base : recordDecl->bases())
        {
            const auto baseRecord = base.getType()->getAsCXXRecordDecl();
            if (baseRecord && baseRecord->hasAttr<clang::AnnotateAttr>() && baseRecord->isCompleteDefinition())
            {
                auto baseHandle = HandleRecordDecl(context, baseRecord);
                if (baseHandle != InvalidMetaIndex)
                {
                    baseClasses.emplace_back(baseHandle);
                }
            }
        }
        
        // Process fields
        std::vector<Reflection::FieldDescription> fieldDescs;
        for (const auto field : recordDecl->fields())
        {
            auto annotateAttr = field->getAttr<clang::AnnotateAttr>();
            auto annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GFIELD") != std::string::npos)
            {
                const auto& attribs = ParseAttributes(annotation);
                const auto& guid = ExtractGuid(attribs);
                assert(guid != Reflection::Attribute::Guid::InvalidGuid() && "Field is missing GUID attribute");
                
                uint32_t typeHash = 0;
                Reflection::MetaType metaType = Reflection::MetaType::Invalid;
				size_t fieldOffset = field->getASTContext().getFieldOffset(field) / 8ul; // Convert bits to bytes
				size_t fieldSize = field->getASTContext().getTypeSize(field->getType()) / 8ul; // Convert bits to bytes
                
                clang::QualType fieldType = field->getType();
                if (fieldType->isArrayType())
                {
                    const clang::ArrayType* arrayType = fieldType->getAsArrayTypeUnsafe();
                    if (arrayType->isConstantArrayType())
                    {
                        metaType = Reflection::MetaType::Array;
                        typeHash = HandleArrayType(context, static_cast<const clang::ConstantArrayType*>(arrayType));
                    }
                }
                else if (fieldType->isRecordType())
                {
                    const clang::RecordType* recordType = fieldType->getAs<clang::RecordType>();
                    
                    std::stringstream qualifiedName;
                    qualifiedName << context.QualifiedName() << "::" << std::string_view(recordType->getDecl()->getName());
                    
                    metaType = Reflection::MetaType::Class;
                    typeHash = Reflection::Utils::HashString(qualifiedName.str().c_str());
                }
                else if (fieldType->isEnumeralType())
                {
                    const clang::EnumType* enumType = fieldType->getAs<clang::EnumType>();
                    
                    std::stringstream qualifiedName;
                    qualifiedName << context.QualifiedName() << "::" << std::string_view(enumType->getDecl()->getName());
                    
                    metaType = Reflection::MetaType::Enum;
                    typeHash = Reflection::Utils::HashString(qualifiedName.str().c_str());
                }
                else if (fieldType->isBuiltinType())
                {
                    const clang::BuiltinType* builtinType = fieldType->getAs<clang::BuiltinType>();
                    
                    metaType = Reflection::MetaType::Primitive;
                    typeHash = BuiltinTypeHash(builtinType);
                }
                else
                {
                    // Unknown type
                    continue;
                }
                fieldDescs.emplace_back(Reflection::FieldDescription({ field->getName(), context.Name(), context.QualifiedName(), attribs, guid, typeHash }, fieldOffset, fieldSize, metaType));
            }
        }
        
        // Process functions
        for (const auto method : recordDecl->methods())
        {
            auto annotateAttr = method->getAttr<clang::AnnotateAttr>();
            auto annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GFUNCTION"))
            {
                const auto& attribs = ParseAttributes(annotation);
                const auto& guid = ExtractGuid(attribs);
                assert(guid != Reflection::Attribute::Guid::InvalidGuid() && "Function is missing GUID attribute");

                // TODO: function reflection support
            }
        }
        
        std::stringstream qualifiedName;
        qualifiedName << context.QualifiedName() << "::" << std::string_view(recordDecl->getName());
        
        uint32_t typeHash = Reflection::Utils::HashString(qualifiedName.str().c_str());
		Reflection::BufferView bases = mObjectWriter.Write(baseClasses.data(), baseClasses.size() * sizeof(ClassHandle));
        Reflection::BufferView fields = mObjectWriter.Write(fieldDescs.data(), fieldDescs.size() * sizeof(Reflection::FieldDescription));
        return context.RegisterClass(Reflection::ClassDescription({ recordDecl->getName(), context.Name(), context.QualifiedName(), recordAttribs, recordGuid, typeHash }, classSize, fields, bases));
    }
    return {};
}

ArrayHandle ReflectionParser::HandleArrayType(ReflectionContext& context, const clang::ConstantArrayType* arrayType)
{
    if (arrayType == nullptr)
    {
        return {};
    }
    
    Reflection::ArrayDescription arrayDesc;
    arrayDesc.mSize = arrayType->getSizeBitWidth() / 8ul; // Convert to bytes
    
    const clang::QualType elementType = arrayType->getElementType();
    if (elementType->isArrayType())
    {
        const auto innerArrayType = elementType->getAsArrayTypeUnsafe();
        if (innerArrayType == nullptr || innerArrayType->isConstantArrayType() == false)
        {
            return {};
        }
        
        const auto innerArrayHandle = HandleArrayType(context, static_cast<const clang::ConstantArrayType*>(innerArrayType));
        if (innerArrayHandle == InvalidMetaIndex)
        {
            return {};
        }
        const auto& innerArrayDesc = context.mArrays[innerArrayHandle];
        arrayDesc.mStride = innerArrayDesc.GetSize();
        arrayDesc.mElementHash = innerArrayHandle;
        arrayDesc.mElementType = Reflection::MetaType::Array;
    }
    else if (elementType->isRecordType())
    {
        const auto recordType = elementType->getAs<clang::RecordType>();
        const auto classHandle = HandleRecordDecl(context, recordType->getAsCXXRecordDecl());
        if (classHandle == InvalidMetaIndex)
        {
            return {};
        }
        const auto& classDesc = context.mClasses[classHandle];
        arrayDesc.mStride = classDesc.GetSize();
        arrayDesc.mElementHash = classDesc.TypeHash();
        arrayDesc.mElementType = Reflection::MetaType::Class;
    }
    else if (elementType->isEnumeralType())
    {
        const clang::EnumType* enumType = elementType->getAs<clang::EnumType>();
        const auto enumHandle = HandleEnumDecl(context, enumType->getDecl());
        if (enumHandle == InvalidMetaIndex)
        {
            return {};
        }
        const auto& enumDesc = context.mEnums[enumHandle];
        arrayDesc.mStride = enumDesc.GetSize();
        arrayDesc.mElementHash = enumDesc.TypeHash();
        arrayDesc.mElementType = Reflection::MetaType::Enum;
    }
    else if (elementType->isBuiltinType())
    {
        const auto builtinType = elementType->getAs<clang::BuiltinType>();
        arrayDesc.mStride = BuiltinTypeSize(builtinType);
        arrayDesc.mElementHash = BuiltinTypeHash(builtinType);
        arrayDesc.mElementType = Reflection::MetaType::Primitive;
    }
    return context.RegisterArray(arrayDesc);
}

std::vector<Reflection::IAttribute*> ReflectionParser::ParseAttributes(const std::string& annotation) const
{
    std::regex attrRegex(R"(\b([A-Za-z0-9_]+)(?:\(([^)]*)\))?)");
    std::sregex_iterator it(annotation.begin(), annotation.end(), attrRegex);
    std::sregex_iterator end;
 
    std::vector<Reflection::IAttribute*> attributes;
    for (; it != end; ++it)
    {
        std::string attrName = (*it)[1].str();
        std::string argsStr = (*it)[2].str();

        if (attrName == "GCLASS" || 
            attrName == "GSTRUCT" || 
            attrName == "GENUM" || 
            attrName == "GITEM" || 
            attrName == "GFUNCTION" || 
            attrName == "GFIELD")
        {
			continue; // skip macro attributes
        }
        
        if (auto attr = Reflection::AttributeFactory::Instance().CreateAttribute(attrName, argsStr); attr != nullptr)
        {
			attributes.emplace_back(attr);
        }
    }
    return attributes;
}

Reflection::Attribute::Guid ReflectionParser::ExtractGuid(const std::vector<Reflection::IAttribute*>& attributes) const
{
    for (const auto attr : attributes)
    {
        if (attr->GetDescription().hash == Reflection::Utils::HashString("Guid"))
        {
            return *static_cast<const Reflection::Attribute::Guid*>(attr);
        }
    }
    return Reflection::Attribute::Guid::InvalidGuid();
}

size_t ReflectionParser::BuiltinTypeSize(const clang::BuiltinType* type) const
{
    switch (type->getKind())
    {
        case clang::BuiltinType::Bool:
            return sizeof(bool);
        case clang::BuiltinType::WChar_U:
        case clang::BuiltinType::WChar_S:
            return sizeof(wchar_t);
        case clang::BuiltinType::Char_U:
        case clang::BuiltinType::Char_S:
            return sizeof(char);
        case clang::BuiltinType::SChar:
            return sizeof(int8_t);
        case clang::BuiltinType::Short:
            return sizeof(int16_t);
        case clang::BuiltinType::Int:
            return sizeof(int);
        case clang::BuiltinType::Long:
        case clang::BuiltinType::LongLong:
            return sizeof(int64_t);
        case clang::BuiltinType::UChar:
            return sizeof(uint8_t);
        case clang::BuiltinType::UShort:
            return sizeof(uint16_t);
        case clang::BuiltinType::UInt:
            return sizeof(uint32_t);
        case clang::BuiltinType::ULong:
        case clang::BuiltinType::ULongLong:
            return sizeof(uint64_t);
        case clang::BuiltinType::Float:
            return sizeof(float);
        case clang::BuiltinType::Double:
            return sizeof(double);
        case clang::BuiltinType::Void:
        default:
            return 0;
    }
}

uint32_t ReflectionParser::BuiltinTypeHash(const clang::BuiltinType* type) const
{
    switch (type->getKind())
    {
        case clang::BuiltinType::Bool:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Bool);
        case clang::BuiltinType::WChar_U:
        case clang::BuiltinType::WChar_S:
            return static_cast<uint32_t>(Reflection::PrimitiveType::WChar);
        case clang::BuiltinType::Char_U:
        case clang::BuiltinType::Char_S:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Char);
        case clang::BuiltinType::SChar:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Int8);
        case clang::BuiltinType::Short:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Int16);
        case clang::BuiltinType::Int:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Int32);
        case clang::BuiltinType::Long:
        case clang::BuiltinType::LongLong:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Int64);
        case clang::BuiltinType::UChar:
            return static_cast<uint32_t>(Reflection::PrimitiveType::UInt8);
        case clang::BuiltinType::UShort:
            return static_cast<uint32_t>(Reflection::PrimitiveType::UInt16);
        case clang::BuiltinType::UInt:
            return static_cast<uint32_t>(Reflection::PrimitiveType::UInt32);
        case clang::BuiltinType::ULong:
        case clang::BuiltinType::ULongLong:
            return static_cast<uint32_t>(Reflection::PrimitiveType::UInt64);
        case clang::BuiltinType::Float:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Float);
        case clang::BuiltinType::Double:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Double);
        case clang::BuiltinType::Void:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Void);
        default:
            return static_cast<uint32_t>(Reflection::PrimitiveType::Invalid);
    }
}
