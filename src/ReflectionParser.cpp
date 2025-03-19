#include "ReflectionParser.h"
#include "Attributes.h"

#include <clang/AST/AST.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/CompilerInstance.h>

#include <fstream>
#include <cassert>
#include <regex>

using namespace Gleam;

ReflectionParser::ReflectionParser()
    : mContext("", "::") // global namespace
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
    generatedCode << "#ifndef __GLEAM_REFLECTION__\n\n";
    
    // TODO: codegen
    
    generatedCode << "#endif // __GLEAM_REFLECTION__\n";
    
    auto filename = outputDir / "Reflection.gen.hx";
    std::ofstream file(filename, std::ios::out | std::ios::trunc);
    
    auto generatedCodeStr = generatedCode.str();
    file.write(generatedCodeStr.c_str(), generatedCodeStr.length());
}

const Reflection::EnumDescription* ReflectionParser::HandleEnumDecl(ReflectionContext& context, const clang::EnumDecl* enumDecl)
{
    auto enumAnnotateAttr = enumDecl->getAttr<clang::AnnotateAttr>();
    auto enumAnnotation = enumAnnotateAttr->getAnnotation().str();
    
    if (enumAnnotation.find("GENUM") != std::string::npos)
    {
        const auto& attributes = ParseAttributes(enumAnnotation);
        const auto& guid = ExtractGuid(attributes);
        assert(guid != Reflection::Attribute::Guid::InvalidGuid() &&  "Enum is missing GUID attribute");
        
        if (const auto enumDesc = context.GetEnum(guid); enumDesc != nullptr)
        {
            return enumDesc; // already processed
        }
        
        uint32_t typeHash = Reflection::Utils::HashString(enumDecl->getQualifiedNameAsString().c_str());
        Reflection::EnumDescription enumDesc(enumDecl->getName(), attributes, guid, typeHash);
        
        auto& astContext = enumDecl->getASTContext();
        enumDesc.mSize = astContext.getTypeSize(astContext.getEnumType(enumDecl)) / 8ul; // Convert bits to bytes
        
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
                
                Reflection::EnumCaseDescription itemDesc(enumItem->getName(), itemAttributes, itemGuid, typeHash);
                itemDesc.mValue = enumItem->getInitVal().getExtValue();
                enumDesc.mCases.emplace_back(itemDesc);
            }
        }
        return context.RegisterEnum(enumDesc);
    }
    return nullptr;
}

const Reflection::ClassDescription* ReflectionParser::HandleRecordDecl(ReflectionContext& context, const clang::CXXRecordDecl* recordDecl)
{
    if (recordDecl == nullptr)
    {
        return nullptr;
    }
    
    auto recordAnnotateAttr = recordDecl->getAttr<clang::AnnotateAttr>();
    auto recordAnnotation = recordAnnotateAttr->getAnnotation().str();
    
    if (recordAnnotation.find("GCLASS") != std::string::npos || recordAnnotation.find("GSTRUCT") != std::string::npos)
    {
        const auto& recordAttribs = ParseAttributes(recordAnnotation);
        const auto& recordGuid = ExtractGuid(recordAttribs);
        assert(recordGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Record is missing GUID attribute");
        
        if (const auto classDesc = context.GetClass(recordGuid); classDesc != nullptr)
        {
            return classDesc; // already processed
        }
        
        uint32_t typeHash = Reflection::Utils::HashString(recordDecl->getQualifiedNameAsString().c_str());
        Reflection::ClassDescription classDesc(recordDecl->getName(), recordAttribs, recordGuid, typeHash);
        
        auto& astContext = recordDecl->getASTContext();
        classDesc.mSize = astContext.getTypeSize(astContext.getRecordType(recordDecl)) / 8ul; // Convert bits to bytes
        
        // Process bases
        for (const auto base : recordDecl->bases())
        {
            const auto baseRecord = base.getType()->getAsCXXRecordDecl();
            if (baseRecord && baseRecord->hasAttr<clang::AnnotateAttr>() && baseRecord->isCompleteDefinition())
            {
                auto baseDesc = HandleRecordDecl(context, baseRecord);
                if (baseDesc != nullptr)
                {
                    classDesc.mBaseClasses.emplace_back(*baseDesc);
                }
            }
        }
        
        // Process fields
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
                Reflection::MetaType type = Reflection::MetaType::Invalid;
                
                clang::QualType fieldType = field->getType();
                if (fieldType->isArrayType())
                {
                    const clang::ArrayType* arrayType = fieldType->getAsArrayTypeUnsafe();
                    if (arrayType->isConstantArrayType())
                    {
                        type = Reflection::MetaType::Array;
                        typeHash = HandleArrayType(context, static_cast<const clang::ConstantArrayType*>(arrayType))->TypeHash();
                    }
                }
                else if (fieldType->isRecordType())
                {
                    const clang::RecordType* recordType = fieldType->getAs<clang::RecordType>();
                    
                    type = Reflection::MetaType::Class;
                    typeHash = Reflection::Utils::HashString(recordType->getDecl()->getQualifiedNameAsString().c_str());
                }
                else if (fieldType->isEnumeralType())
                {
                    const clang::EnumType* enumType = fieldType->getAs<clang::EnumType>();
                    
                    type = Reflection::MetaType::Enum;
                    typeHash = Reflection::Utils::HashString(enumType->getDecl()->getQualifiedNameAsString().c_str());
                }
                else if (fieldType->isBuiltinType())
                {
                    const clang::BuiltinType* builtinType = fieldType->getAs<clang::BuiltinType>();
                    
                    type = Reflection::MetaType::Primitive;
                    typeHash = BuiltinTypeHash(builtinType);
                }
                else
                {
                    // Unknown type
                    continue;
                }
                
                Reflection::FieldDescription fieldDesc(field->getName(), attribs, guid, typeHash);
                fieldDesc.mOffset = field->getASTContext().getFieldOffset(field) / 8ul; // Convert bits to bytes
                fieldDesc.mSize = field->getASTContext().getTypeSize(field->getType()) / 8ul; // Convert bits to bytes
                fieldDesc.mType = type;
                classDesc.mFields.emplace_back(fieldDesc);
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
            }
        }
        return context.RegisterClass(classDesc);
    }
    return nullptr;
}

const Reflection::ArrayDescription* ReflectionParser::HandleArrayType(ReflectionContext& context, const clang::ConstantArrayType* arrayType)
{
    if (arrayType == nullptr)
    {
        return nullptr;
    }
    
    Reflection::ArrayDescription arrayDesc;
    arrayDesc.mSize = arrayType->getSizeBitWidth() / 8ul; // Convert to bytes
    
    const clang::QualType elementType = arrayType->getElementType();
    if (elementType->isArrayType())
    {
        const auto innerArrayType = elementType->getAsArrayTypeUnsafe();
        if (innerArrayType == nullptr || innerArrayType->isConstantArrayType() == false)
        {
            return nullptr;
        }
        
        const auto innerArrayDesc = HandleArrayType(context, static_cast<const clang::ConstantArrayType*>(innerArrayType));
        if (innerArrayDesc == nullptr)
        {
            return nullptr;
        }
        arrayDesc.mStride = innerArrayDesc->GetSize();
        arrayDesc.mElementHash = innerArrayDesc->TypeHash();
        arrayDesc.mElementType = Reflection::MetaType::Array;
    }
    else if (elementType->isRecordType())
    {
        const auto recordType = elementType->getAs<clang::RecordType>();
        const auto classDesc = HandleRecordDecl(context, recordType->getAsCXXRecordDecl());
        if (classDesc == nullptr)
        {
            return nullptr;
        }
        arrayDesc.mStride = classDesc->GetSize();
        arrayDesc.mElementHash = classDesc->TypeHash();
        arrayDesc.mElementType = Reflection::MetaType::Class;
    }
    else if (elementType->isEnumeralType())
    {
        const clang::EnumType* enumType = elementType->getAs<clang::EnumType>();
        const auto enumDesc = HandleEnumDecl(context, enumType->getDecl());
        if (enumDesc == nullptr)
        {
            return nullptr;
        }
        arrayDesc.mStride = enumDesc->GetSize();
        arrayDesc.mElementHash = enumDesc->TypeHash();
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
