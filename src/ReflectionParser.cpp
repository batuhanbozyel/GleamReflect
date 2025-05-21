#include "ReflectionParser.h"
#include "AttributeFactory.h"
#include "Reflection/Database.h"

#include <clang/AST/AST.h>
#include <clang/AST/Attr.h>
#include <clang/Tooling/Tooling.h>

#include <fstream>
#include <cassert>

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

void ReflectionParser::GenerateOutput(const std::string& moduleName,
									  const std::filesystem::path& headerDir, 
									  const std::filesystem::path& binaryDir)
{
	// Generated database
	auto databaseFile = binaryDir / (moduleName + ".Reflection.db");
    {
		std::ofstream file(databaseFile, std::ios::out | std::ios::trunc | std::ios::binary);

		size_t bufferSize = sizeof(Reflection::DatabaseHeader)
			+ mObjectWriter.GetBuffer().size
			+ mContext.mClasses.size() * sizeof(Reflection::ClassDescription)
			+ mContext.mEnums.size() * sizeof(Reflection::EnumDescription)
			+ mStringWriter.GetBuffer().size;

        Reflection::BinaryWriter writer(bufferSize);
		Reflection::DatabaseHeader header = {};
        header.version = GLEAM_REFLECTION_VERSION;
        header.classCount = static_cast<uint32_t>(mContext.mClasses.size());
        header.enumCount = static_cast<uint32_t>(mContext.mEnums.size());
		header.arrayCount = static_cast<uint32_t>(mContext.mArrays.size());
		memcpy(header.magic, "GLEAMREF", sizeof(header.magic));
        writer.Write(header);

        const auto& objectBuffer = mObjectWriter.GetBuffer();
        writer.Write(objectBuffer.data, objectBuffer.size);
        
        auto serializedHeader = static_cast<Reflection::DatabaseHeader*>(writer.GetBuffer().data);
        serializedHeader->classTableOffset = writer.GetCursor();
        writer.Write(mContext.mClasses.data(), mContext.mClasses.size() * sizeof(Reflection::ClassDescription));
        
        serializedHeader->enumTableOffset = writer.GetCursor();
        writer.Write(mContext.mEnums.data(), mContext.mEnums.size() * sizeof(Reflection::EnumDescription));

		serializedHeader->arrayTableOffset = writer.GetCursor();
		writer.Write(mContext.mArrays.data(), mContext.mArrays.size() * sizeof(Reflection::ArrayDescription));

		const auto& stringBuffer = mStringWriter.GetBuffer();
		serializedHeader->stringTableOffset = writer.GetCursor();
		writer.Write(stringBuffer.data, stringBuffer.size);

        const auto& buffer = writer.GetBuffer();
		file.write(reinterpret_cast<const char*>(buffer.data), buffer.size);
    }

	// Initialize database for reflection
	Reflection::Database database;
	bool success = database.Initialize(databaseFile);
	assert(success && "Failed to initialize reflection database");

	std::stringstream generatedCode;
	generatedCode << "#ifndef __GLEAM_REFLECTION__\n";
	generatedCode << "#include <Reflection/Reflection.h>\n";
	mContext.GenerateForwardDecls(generatedCode);

	generatedCode << "namespace Gleam::Reflection {\n\n";
	mContext.GenerateMetaDescs(generatedCode);
	generatedCode << "} // namespace Gleam::Reflection\n";

	generatedCode << "#endif // __GLEAM_REFLECTION__\n";

	// Generated header
	{
		auto filename = headerDir / (moduleName + ".Reflection.generated.h");
		std::ofstream file(filename, std::ios::out | std::ios::trunc);

		auto generatedCodeStr = generatedCode.str();
		file.write(generatedCodeStr.c_str(), generatedCodeStr.length());
	}
	database.Shutdown();
}

EnumHandle ReflectionParser::HandleEnumDecl(ReflectionContext& context, const clang::EnumDecl* enumDecl)
{
    auto enumAnnotateAttr = enumDecl->getAttr<clang::AnnotateAttr>();
    auto enumAnnotation = enumAnnotateAttr->getAnnotation().str();
    
    if (enumAnnotation.find("GENUM") != std::string::npos)
    {
        auto attributes = ParseAttributes(enumAnnotation);
		auto guid = ExtractGuid(attributes);
        assert(guid != Reflection::Attribute::Guid::InvalidGuid() &&  "Enum is missing GUID attribute");
        
        if (const auto enumHandle = context.GetEnumHandle(guid); enumHandle != InvalidMetaIndex)
        {
            return enumHandle; // already processed
        }

		auto nameStr = enumDecl->getName();
		auto name = mStringWriter.Write(nameStr.data(), nameStr.size());

		std::stringstream qualifiedNameSS;
		qualifiedNameSS << context.QualifiedName() << "::" << std::string_view(nameStr);

		auto qualifiedNameStr = qualifiedNameSS.str();
		auto qualifiedName = mStringWriter.Write(qualifiedNameStr.data(), qualifiedNameStr.size());

        auto& astContext = enumDecl->getASTContext();
        uint32_t typeHash = Reflection::Utils::HashString(qualifiedNameStr.c_str());
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
                auto itemAttributes = ParseAttributes(itemAnnotation);
                auto itemGuid = ExtractGuid(itemAttributes);
                assert(itemGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Enum case is missing GUID attribute");

				auto itemNameStr = enumItem->getName();
				auto itemName = mStringWriter.Write(itemNameStr.data(), itemNameStr.size());

				std::stringstream itemQualifiedNameSS;
				itemQualifiedNameSS << context.QualifiedName() << "::" << std::string_view(itemNameStr);

				auto itemQualifiedNameStr = itemQualifiedNameSS.str();
				auto itemQualifiedName = mStringWriter.Write(itemQualifiedNameStr.data(), itemQualifiedNameStr.size());

                Reflection::EnumCaseDescription enumCase({
					itemName,
					itemQualifiedName,
                    itemAttributes,
                    itemGuid,
                    typeHash
                }, enumItem->getInitVal().getExtValue());
                enumCases.emplace_back(enumCase);
            }
        }

        auto cases = mObjectWriter.Write(enumCases.data(), enumCases.size() * sizeof(Reflection::EnumCaseDescription));
        return context.RegisterEnum(Reflection::EnumDescription({ name, qualifiedName, attributes, guid, typeHash }, enumSize, cases));
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
        auto recordAttribs = ParseAttributes(recordAnnotation);
        auto recordGuid = ExtractGuid(recordAttribs);
        assert(recordGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Record is missing GUID attribute");
        
        if (const auto classHandle = context.GetClassHandle(recordGuid); classHandle != InvalidMetaIndex)
        {
            return classHandle; // already processed
        }
        
        auto& astContext = recordDecl->getASTContext();
        size_t classSize = astContext.getTypeSize(astContext.getRecordType(recordDecl)) / 8ul; // Convert bits to bytes

		auto classNameStr = recordDecl->getName();
		auto className = mStringWriter.Write(classNameStr.data(), classNameStr.size());

		std::stringstream qualifiedClassNameSS;
		qualifiedClassNameSS << context.QualifiedName() << "::" << std::string_view(classNameStr);

		auto qualifiedClassNameStr = qualifiedClassNameSS.str();
		auto qualifiedClassName = mStringWriter.Write(qualifiedClassNameStr.data(), qualifiedClassNameStr.size());

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
                auto fieldAttribs = ParseAttributes(annotation);
                auto fieldGuid = ExtractGuid(fieldAttribs);
                assert(fieldGuid != Reflection::Attribute::Guid::InvalidGuid() && "Field is missing GUID attribute");
                
                uint32_t fieldHash = 0;
                Reflection::MetaType fieldMetaType = Reflection::MetaType::Invalid;
				size_t fieldOffset = field->getASTContext().getFieldOffset(field) / 8ul; // Convert bits to bytes
				size_t fieldSize = field->getASTContext().getTypeSize(field->getType()) / 8ul; // Convert bits to bytes
                
                clang::QualType fieldType = field->getType();
                if (fieldType->isArrayType())
                {
                    const clang::ArrayType* arrayType = fieldType->getAsArrayTypeUnsafe();
                    if (arrayType->isConstantArrayType())
                    {
						fieldMetaType = Reflection::MetaType::Array;
						fieldHash = HandleArrayType(context, static_cast<const clang::ConstantArrayType*>(arrayType));
                    }
                }
                else if (fieldType->isRecordType())
                {
                    const clang::RecordType* recordType = fieldType->getAs<clang::RecordType>();
                    
                    std::stringstream qualifiedName;
                    qualifiedName << context.QualifiedName() << "::" << std::string_view(recordType->getDecl()->getName());
                    
					fieldMetaType = Reflection::MetaType::Class;
					fieldHash = Reflection::Utils::HashString(qualifiedName.str().c_str());
                }
                else if (fieldType->isEnumeralType())
                {
                    const clang::EnumType* enumType = fieldType->getAs<clang::EnumType>();
                    
                    std::stringstream qualifiedName;
                    qualifiedName << context.QualifiedName() << "::" << std::string_view(enumType->getDecl()->getName());
                    
					fieldMetaType = Reflection::MetaType::Enum;
					fieldHash = Reflection::Utils::HashString(qualifiedName.str().c_str());
                }
                else if (fieldType->isBuiltinType())
                {
                    const clang::BuiltinType* builtinType = fieldType->getAs<clang::BuiltinType>();
                    
					fieldMetaType = Reflection::MetaType::Primitive;
					fieldHash = BuiltinTypeHash(builtinType);
                }
                else
                {
                    // Unknown type
                    continue;
                }
				auto fieldNameStr = field->getName();
				auto fieldName = mStringWriter.Write(fieldNameStr.data(), fieldNameStr.size());

				std::stringstream fieldQualifiedNameSS;
				fieldQualifiedNameSS << qualifiedClassNameStr << "::" << std::string_view(fieldNameStr);

				auto fieldQualifiedNameStr = fieldQualifiedNameSS.str();
				auto fieldQualifiedName = mStringWriter.Write(fieldQualifiedNameStr.data(), fieldQualifiedNameStr.size());
                fieldDescs.emplace_back(Reflection::FieldDescription({ fieldName, fieldQualifiedName, fieldAttribs, fieldGuid, fieldHash }, fieldOffset, fieldSize, fieldMetaType));
            }
        }
        
        // Process functions
        for (const auto method : recordDecl->methods())
        {
            auto annotateAttr = method->getAttr<clang::AnnotateAttr>();
            auto annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GFUNCTION"))
            {
                auto attribs = ParseAttributes(annotation);
                auto guid = ExtractGuid(attribs);
                assert(guid != Reflection::Attribute::Guid::InvalidGuid() && "Function is missing GUID attribute");

                // TODO: function reflection support
            }
        }
        
        uint32_t typeHash = Reflection::Utils::HashString(qualifiedClassNameStr.c_str());
		auto bases = mObjectWriter.Write(baseClasses.data(), baseClasses.size() * sizeof(ClassHandle));
        auto fields = mObjectWriter.Write(fieldDescs.data(), fieldDescs.size() * sizeof(Reflection::FieldDescription));
        return context.RegisterClass(Reflection::ClassDescription({ className, qualifiedClassName, recordAttribs, recordGuid, typeHash }, classSize, fields, bases));
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
    if (elementType->isConstantArrayType())
    {
        const auto innerArrayType = static_cast<const clang::ConstantArrayType*>(elementType->getAsArrayTypeUnsafe());
        const auto innerArrayHandle = HandleArrayType(context, innerArrayType);
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
	else
	{
		return {};
	}
    return context.RegisterArray(arrayDesc);
}

Reflection::BufferView ReflectionParser::ParseAttributes(const std::string& annotation)
{
    std::regex attrRegex(R"(\b([A-Za-z0-9_]+)(?:\(([^)]*)\))?)");
    std::sregex_iterator it(annotation.begin(), annotation.end(), attrRegex);
    std::sregex_iterator end;
 
    std::vector<uint32_t> attributeHashes;
	std::vector<Reflection::BufferView> attributeViews;
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
        
        if (auto attr = Reflection::AttributeFactory::Instance().CreateAttribute(attrName, argsStr); attr.ptr != nullptr)
		{
			auto attrView = mObjectWriter.Write(attr.ptr, attr.size);
			attributeHashes.emplace_back(attr.hash);
			attributeViews.emplace_back(attrView);
			delete attr.ptr;
        }
    }
	auto attributes = mObjectWriter.Write(attributeHashes.data(), attributeHashes.size() * sizeof(uint32_t));
	mObjectWriter.Write(attributeViews.data(), attributeViews.size() * sizeof(Reflection::BufferView));
    return attributes;
}

Reflection::Attribute::Guid ReflectionParser::ExtractGuid(const Reflection::BufferView& attributes) const
{
	auto attribs = Reflection::Utils::OffsetPointer<uint32_t>(mObjectWriter.GetBuffer().data, attributes.offset);
	auto numAttribs = attributes.size / sizeof(uint32_t);

	for (uint32_t i = 0; i < numAttribs; ++i)
	{
		if (attribs[i] == Reflection::Utils::HashString("Guid"))
		{
			auto view = Reflection::Utils::OffsetPointer<Reflection::BufferView>(mObjectWriter.GetBuffer().data, attributes.offset + attributes.size + i * sizeof(Reflection::BufferView));
			auto guid = Reflection::Utils::OffsetPointer<Reflection::Attribute::Guid>(mObjectWriter.GetBuffer().data, view->offset);
			assert(view->size == sizeof(Reflection::Attribute::Guid) && "Attribute view does not match with GUID");
			return *guid;
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

REGISTER_ATTRIBUTE(Gleam::Reflection::Attribute, Guid);
REGISTER_ATTRIBUTE(Gleam::Reflection::Attribute, Version);
REGISTER_ATTRIBUTE(Gleam::Reflection::Attribute, EntityComponent);
REGISTER_ATTRIBUTE(Gleam::Reflection::Attribute, Serializable);
REGISTER_ATTRIBUTE(Gleam::Reflection::Attribute, PrettyName);
