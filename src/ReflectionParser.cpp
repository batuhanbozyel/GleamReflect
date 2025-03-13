#include "ReflectionParser.h"
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <cassert>
#include <regex>

namespace Gleam {

ReflectionParser::ReflectionParser()
    : mDatabase()
{
    
}

void ReflectionParser::ParseAST(clang::ASTContext& context)
{
    for (const auto* decl : context.getTranslationUnitDecl()->decls())
    {
        if (const auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl))
        {
            if (enumDecl->hasAttr<clang::AnnotateAttr>())
            {
                HandleEnumDecl(enumDecl);
            }
        }
        else if (const auto* recordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
        {
            if (recordDecl->hasAttr<clang::AnnotateAttr>() && recordDecl->isCompleteDefinition())
            {
                HandleRecordDecl(recordDecl);
            }
        }
    }
}

void ReflectionParser::GenerateOutput(const std::filesystem::path& outputDir)
{
    
}

Reflection::Database::EnumMap::iterator ReflectionParser::HandleEnumDecl(const clang::EnumDecl* enumDecl)
{
    auto enumAnnotateAttr = enumDecl->getAttr<clang::AnnotateAttr>();
    auto enumAnnotation = enumAnnotateAttr->getAnnotation().str();
    
    if (enumAnnotation.find("GENUM"))
    {
        const auto& attributes = ParseAttributes(enumAnnotation);
        const auto& guid = ExtractGuid(attributes);
        assert(guid != Reflection::Attribute::Guid::InvalidGuid() &&  "Enum is missing GUID attribute");
        
        auto it = mDatabase.mGuidToEnum.find(guid);
        if (it != mDatabase.mGuidToEnum.end())
        {
            return it; // already processed
        }
        
        Reflection::EnumDescription enumDesc(enumDecl->getName(), guid);
        for (const auto enumItem : enumDecl->enumerators())
        {
            if (enumItem->hasAttr<clang::AnnotateAttr>() == false)
            {
                continue; // item is not reflected
            }
            
            auto itemAnnotateAttr = enumItem->getAttr<clang::AnnotateAttr>();
            auto itemAnnotation = itemAnnotateAttr->getAnnotation().str();
            
            if (itemAnnotation.find("GITEM"))
            {
                const auto& itemAttributes = ParseAttributes(itemAnnotation);
                const auto& itemGuid = ExtractGuid(itemAttributes);
                assert(itemGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Enum case is missing GUID attribute");
                
                Reflection::EnumCaseDescription itemDesc(enumItem->getName(), itemGuid);
                itemDesc.mValue = enumItem->getInitVal().getExtValue();
                enumDesc.mCases.emplace_back(itemDesc);
            }
        }
        return mDatabase.mGuidToEnum.emplace_hint(mDatabase.mGuidToEnum.end(), guid, enumDesc);
    }
    return mDatabase.mGuidToEnum.end();
}

Reflection::Database::ClassMap::iterator ReflectionParser::HandleRecordDecl(const clang::CXXRecordDecl* recordDecl)
{
    auto recordAnnotateAttr = recordDecl->getAttr<clang::AnnotateAttr>();
    auto recordAnnotation = recordAnnotateAttr->getAnnotation().str();
    
    if (recordAnnotation.find("GCLASS") || recordAnnotation.find("GSTRUCT"))
    {
        const auto& recordAttribs = ParseAttributes(recordAnnotation);
        const auto& recordGuid = ExtractGuid(recordAttribs);
        assert(recordGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Record is missing GUID attribute");
        
        auto it = mDatabase.mGuidToClass.find(recordGuid);
        if (it != mDatabase.mGuidToClass.end())
        {
            return it; // already processed;
        }
        
        Reflection::ClassDescription classDesc(recordDecl->getName(), recordGuid);
        
        // Process bases
        for (const auto base : recordDecl->bases())
        {
            const auto baseRecord = base.getType()->getAsCXXRecordDecl();
            if (baseRecord && baseRecord->hasAttr<clang::AnnotateAttr>() && baseRecord->isCompleteDefinition())
            {
                auto baseIt = HandleRecordDecl(baseRecord);
                if (baseIt != mDatabase.mGuidToClass.end())
                {
                    classDesc.mBaseClasses.emplace_back(baseIt->second);
                }
            }
        }
        
        // Process fields
        for (const auto field : recordDecl->fields())
        {
            auto annotateAttr = field->getAttr<clang::AnnotateAttr>();
            auto annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GFIELD"))
            {
                const auto& attribs = ParseAttributes(annotation);
                const auto& guid = ExtractGuid(attribs);
                assert(guid != Reflection::Attribute::Guid::InvalidGuid() && "Field is missing GUID attribute");
                
                Reflection::FieldDescription fieldDesc(field->getName(), guid);
                fieldDesc.mOffset = field->getASTContext().getFieldOffset(field) / 8; // Convert bits to bytes
                fieldDesc.mSize = field->getASTContext().getTypeSize(field->getType()) / 8; // Convert bits to bytes

                clang::QualType fieldType = field->getType();
                if (fieldType->isArrayType())
                {
                    const clang::ArrayType* arrayType = fieldType->getAsArrayTypeUnsafe();
                    const clang::QualType elementType = arrayType->getElementType();
                    
                    fieldDesc.mType = Reflection::MetaType::Array;
                    fieldDesc.mTypeHash = elementType.getTypePtr()->getTypeClass(); // TODO: use a proper hashing function
                }
                else if (fieldType->isRecordType())
                {
                    const clang::RecordType* recordType = fieldType->getAs<clang::RecordType>();
                    
                    fieldDesc.mType = Reflection::MetaType::Class;
                    fieldDesc.mTypeHash = recordType->getDecl()->getTypeForDecl()->getTypeClass(); // TODO: use a proper hashing function
                }
                else if (fieldType->isEnumeralType())
                {
                    const clang::EnumType* enumType = fieldType->getAs<clang::EnumType>();
                    
                    fieldDesc.mType = Reflection::MetaType::Enum;
                    fieldDesc.mTypeHash = enumType->getDecl()->getTypeForDecl()->getTypeClass(); // TODO: use a proper hashing function
                }
                else if (fieldType->isBuiltinType())
                {
                    const clang::BuiltinType* builtinType = fieldType->getAs<clang::BuiltinType>();
                    
                    fieldDesc.mType = Reflection::MetaType::Primitive;
                    switch (builtinType->getKind())
                    {
                        case clang::BuiltinType::Bool:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Bool);
                            break;
                        case clang::BuiltinType::WChar_U:
                        case clang::BuiltinType::WChar_S:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::WChar);
                            break;
                        case clang::BuiltinType::Char_U:
                        case clang::BuiltinType::Char_S:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Char);
                            break;
                        case clang::BuiltinType::SChar:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Int8);
                            break;
                        case clang::BuiltinType::Short:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Int16);
                            break;
                        case clang::BuiltinType::Int:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Int32);
                            break;
                        case clang::BuiltinType::Long:
                        case clang::BuiltinType::LongLong:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Int64);
                            break;
                        case clang::BuiltinType::UChar:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::UInt8);
                            break;
                        case clang::BuiltinType::UShort:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::UInt16);
                            break;
                        case clang::BuiltinType::UInt:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::UInt32);
                            break;
                        case clang::BuiltinType::ULong:
                        case clang::BuiltinType::ULongLong:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::UInt64);
                            break;
                        case clang::BuiltinType::Float:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Float);
                            break;
                        case clang::BuiltinType::Double:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Double);
                            break;
                        case clang::BuiltinType::Void:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Void);
                            break;
                        default:
                            fieldDesc.mTypeHash = static_cast<uint32_t>(Reflection::PrimitiveType::Invalid);
                            break;
                    }
                }
                else
                {
                    // Unknown type
                    continue;
                }
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
        return mDatabase.mGuidToClass.emplace_hint(mDatabase.mGuidToClass.end(), recordGuid, classDesc);
    }
    return mDatabase.mGuidToClass.end();
}

std::vector<AttributePair> ReflectionParser::ParseAttributes(const std::string& annotation)
{
    std::regex attrRegex(R"(\b([A-Za-z0-9_]+)(?:\(([^)]*)\))?)");
    std::sregex_iterator it(annotation.begin(), annotation.end(), attrRegex);
    std::sregex_iterator end;
 
    std::vector<AttributePair> attributes;
    for (; it != end; ++it)
    {
        std::string attrName = (*it)[1].str();
        std::string argsStr = (*it)[2].str();
        attributes.emplace_back(AttributePair{
            .description = Reflection::AttributeDescription(attrName.c_str()),
            .arguments = argsStr });
    }
    return attributes;
}

Reflection::Attribute::Guid ReflectionParser::ExtractGuid(const std::vector<AttributePair>& attributes)
{
    for (const auto& attr : attributes)
    {
        if (attr.description.hash == Reflection::Utils::HashString("Guid"))
        {
            return Reflection::Attribute::Guid(attr.arguments);
        }
    }
    return Reflection::Attribute::Guid::InvalidGuid();
}

} // namespace Gleam
