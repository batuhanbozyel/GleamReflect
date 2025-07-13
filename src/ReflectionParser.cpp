#include "ReflectionParser.h"
#include "AttributeFactory.h"
#include "Reflection/Database.h"

#include <clang/AST/AST.h>
#include <clang/AST/Attr.h>
#include <clang/Tooling/Tooling.h>

#include <fstream>
#include <cassert>
#include <algorithm>

using namespace Gleam;

ReflectionParser::ReflectionParser()
    : mContext("", "") // global namespace
{
    
}

void ReflectionParser::ParseAST(clang::ASTContext& context)
{
    ParseDecls(context.getTranslationUnitDecl()->decls());
}

void ReflectionParser::ParseDecls(const clang::DeclContext::decl_range& decls)
{
    for (const auto decl : decls)
    {
        if (const auto enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl))
        {
            if (enumDecl->hasAttr<clang::AnnotateAttr>() && enumDecl->isCompleteDefinition())
            {
                HandleEnumDecl(enumDecl);
            }
        }
        else if (const auto recordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
        {
            if (recordDecl->hasAttr<clang::AnnotateAttr>() && recordDecl->isCompleteDefinition())
            {
				HandleRecordDecl(recordDecl);
            }
        }
		else if (const auto templateDecl = llvm::dyn_cast<clang::ClassTemplateDecl>(decl))
		{
			auto recordDecl = templateDecl->getTemplatedDecl();
			if (recordDecl && recordDecl->hasAttr<clang::AnnotateAttr>() && recordDecl->isCompleteDefinition())
			{
				HandleRecordDecl(recordDecl);
			}
		}
		else if (const auto specDecl = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl))
		{
			if (specDecl->hasAttr<clang::AnnotateAttr>() && specDecl->isCompleteDefinition())
			{
				HandleRecordDecl(specDecl);
			}
		}
        else if (const auto namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(decl))
        {
            ParseDecls(namespaceDecl->decls());
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
		file.close();
    }

	// Initialize database for reflection
	Reflection::Database database;
	if (database.Initialize(databaseFile) == false)
	{
		std::cerr << "Failed to initialize reflection database" << std::endl;
		return;
	}

	std::stringstream generatedCode;
	generatedCode << "#pragma once\n";
	generatedCode << "#ifndef __GLEAM_REFLECTION__\n";
	generatedCode << "#include <Reflection/Reflection.h>\n";
	mContext.GenerateForwardDecls(generatedCode);
	for (const auto& header : mHeaders)
	{
		std::filesystem::path headerPath(header);
		std::filesystem::path relativePath = std::filesystem::relative(headerPath, headerDir);
		std::string relativePathStr = relativePath.string();
		std::replace(relativePathStr.begin(), relativePathStr.end(), '\\', '/');
		//generatedCode << "#include \"" << relativePathStr << "\"\n";
	}
	//generatedCode << "\n";

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
		file.close();
	}
	database.Shutdown();
}

EnumHandle ReflectionParser::HandleEnumDecl(const clang::EnumDecl* enumDecl)
{
	if (enumDecl == nullptr)
	{
		return {};
	}

	if (enumDecl->isInvalidDecl())
	{
		return {}; // Invalid declaration, skip processing
	}

    auto enumAnnotateAttr = enumDecl->getAttr<clang::AnnotateAttr>();
	if (enumAnnotateAttr == nullptr)
	{
		std::cerr << enumDecl->getName().str() << " is not reflected" << std::endl;
		return {};
	}

    auto enumAnnotation = enumAnnotateAttr->getAnnotation().str();
    if (enumAnnotation.find("GENUM") != std::string::npos)
    {
        auto attributes = ParseAttributes(enumAnnotation);
		auto guid = ExtractGuid(attributes);
		if (guid == Reflection::Attribute::Guid::InvalidGuid())
		{
			std::cerr << enumDecl->getName().str() << " is missing GUID attribute" << std::endl;
			return {};
		}

		auto nameStr = enumDecl->getName();
		auto name = mStringWriter.Write(nameStr.data(), nameStr.size());
		auto& context = GetDeclReflectionContext(enumDecl->getParent());

		std::stringstream qualifiedNameSS;
		qualifiedNameSS << context.QualifiedName() << "::" << std::string_view(nameStr);

		auto qualifiedNameStr = qualifiedNameSS.str();
		auto qualifiedName = mStringWriter.Write(qualifiedNameStr.data(), qualifiedNameStr.size());

		auto& astContext = enumDecl->getASTContext();
		uint32_t typeHash = Reflection::Utils::HashString(qualifiedNameStr.c_str());
		size_t enumSize = astContext.getTypeSize(astContext.getEnumType(enumDecl)) / 8ul; // Convert bits to bytes

		if (const auto enumHandle = context.GetEnumHandle(guid); enumHandle != InvalidMetaIndex)
		{
			if (typeHash == mContext.mEnums[enumHandle].TypeHash())
			{
				return enumHandle; // already processed
			}
			std::cerr << enumDecl->getName().str() << " GUID already exists" << std::endl;
			return {};
		}

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
				if (itemGuid == Reflection::Attribute::Guid::InvalidGuid())
				{
					std::cerr << enumDecl->getName().str() << "::" << enumItem->getName().str() << " is missing GUID attribute" << std::endl;
					continue;
				}

				if (mContext.Contains(itemGuid))
				{
					std::cerr << enumDecl->getName().str() << "::" << enumItem->getName().str()<< " GUID already exists" << std::endl;
					return {};
				}

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
        auto handle = context.RegisterEnum(Reflection::EnumDescription({ name, qualifiedName, attributes, guid, typeHash }, enumSize, cases));
		if (handle == InvalidMetaIndex)
		{
			std::cerr << enumDecl->getName().str() << " GUID already exists" << std::endl;
			return {};
		}

		auto headerPath = ExtractHeaderPath(enumDecl->getLocation(), enumDecl->getASTContext());
		if (headerPath.empty() == false)
		{
			mHeaders.insert(headerPath);
		}
		else
		{
			std::cerr << "Failed to extract header path for " << enumDecl->getName().str() << std::endl;
		}
		return handle;
    }
    return {};
}

ClassHandle ReflectionParser::HandleRecordDecl(const clang::CXXRecordDecl* recordDecl)
{
    if (recordDecl == nullptr)
    {
        return {};
    }

	if (recordDecl->isInvalidDecl())
	{
		return {}; // Invalid declaration, skip processing
	}
    
    auto recordAnnotateAttr = recordDecl->getAttr<clang::AnnotateAttr>();
	if (recordAnnotateAttr == nullptr)
	{
		std::cerr << recordDecl->getName().str() << " is not reflected" << std::endl;
		return {}; // No annotation attribute, skip processing
	}

    auto recordAnnotation = recordAnnotateAttr->getAnnotation().str();
    if (recordAnnotation.find("GCLASS") != std::string::npos || recordAnnotation.find("GSTRUCT") != std::string::npos)
    {
        auto recordAttribs = ParseAttributes(recordAnnotation);
        auto recordGuid = ExtractGuid(recordAttribs);
		if (recordGuid == Reflection::Attribute::Guid::InvalidGuid())
		{
			std::cerr << recordDecl->getName().str() << " is missing GUID attribute" << std::endl;
			return {};
		}
		auto& astContext = recordDecl->getASTContext();
		auto recordType = astContext.getRecordType(recordDecl);
		size_t classSize = 0;

		auto classNameStr = recordDecl->getName();
		auto className = mStringWriter.Write(classNameStr.data(), classNameStr.size());
		auto& context = GetDeclReflectionContext(recordDecl->getParent());

		std::stringstream qualifiedClassNameSS;
		qualifiedClassNameSS << context.QualifiedName() << "::" << std::string_view(classNameStr);

		auto qualifiedClassNameStr = qualifiedClassNameSS.str();
		auto qualifiedClassName = mStringWriter.Write(qualifiedClassNameStr.data(), qualifiedClassNameStr.size());
		uint32_t typeHash = Reflection::Utils::HashString(qualifiedClassNameStr.c_str());
        
        if (const auto classHandle = context.GetClassHandle(recordGuid); classHandle != InvalidMetaIndex)
        {
			if (typeHash == mContext.mClasses[classHandle].TypeHash())
			{
				return classHandle; // already processed
			}
			std::cerr << recordDecl->getName().str() << " GUID already exists" << std::endl;
			return {};
        }

		// Process template parameters
		std::string templateDeclStr;
		std::vector<Reflection::TemplateParameterDescription> templateParamDescs;
		if (const auto templateDecl = recordDecl->getDescribedClassTemplate(); templateDecl != nullptr)
		{
			return {}; // Template declarations are not supported yet
		}
		else if (const auto specDecl = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(recordDecl); specDecl != nullptr)
		{
			if (specDecl->getTemplateSpecializationKind() == clang::TSK_ExplicitInstantiationDefinition
				|| specDecl->getTemplateSpecializationKind() == clang::TSK_ImplicitInstantiation
				|| specDecl->getTemplateSpecializationKind() == clang::TSK_ExplicitSpecialization)
			{
				const auto& args = specDecl->getTemplateInstantiationArgs();
				for (const auto& arg : args.asArray())
				{
					auto argType = arg.getAsType();
					if (argType->isBuiltinType())
					{
						const auto argBuiltinType = argType->getAs<clang::BuiltinType>();
						templateParamDescs.emplace_back(Reflection::MetaType::Primitive, BuiltinTypeHash(argBuiltinType));
					}
					else if (argType->isRecordType())
					{
						const auto argRecordType = argType->getAs<clang::RecordType>();
						const auto argRecordDecl = argRecordType->getAsCXXRecordDecl();

						auto argClassHandle = HandleRecordDecl(argRecordDecl);
						if (argClassHandle != InvalidMetaIndex)
						{
							templateParamDescs.emplace_back(Reflection::MetaType::Class, argClassHandle);
						}
					}
					else if (argType->isEnumeralType())
					{
						const auto argEnumType = argType->getAs<clang::EnumType>();
						const auto argEnumDecl = argEnumType->getDecl();

						auto argEnumHandle = HandleEnumDecl(argEnumDecl);
						if (argEnumHandle != InvalidMetaIndex)
						{
							templateParamDescs.emplace_back(Reflection::MetaType::Enum, argEnumHandle);
						}
					}
				}
				templateDeclStr = ExtractTemplateDeclaration(specDecl);
			}
			else
			{
				return {}; // Template partial specialization reflection is not supported yet
			}
		}
		classSize = astContext.getTypeSize(recordType) / 8ul; // Convert bits to bytes

        // Process bases
		std::vector<ClassHandle> baseClasses;
        for (const auto& base : recordDecl->bases())
        {
            const auto baseRecord = base.getType()->getAsCXXRecordDecl();
            if (baseRecord && baseRecord->hasAttr<clang::AnnotateAttr>() && baseRecord->isCompleteDefinition())
            {
                auto baseHandle = HandleRecordDecl(baseRecord);
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
			if (field->hasAttr<clang::AnnotateAttr>() == false)
			{
				continue; // item is not reflected
			}

            auto annotateAttr = field->getAttr<clang::AnnotateAttr>();
            auto annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GFIELD") != std::string::npos)
            {
                auto fieldAttribs = ParseAttributes(annotation);
                auto fieldGuid = ExtractGuid(fieldAttribs);
				if (fieldGuid == Reflection::Attribute::Guid::InvalidGuid())
				{
					std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " is missing GUID attribute" << std::endl;
					continue;
				}

				if (mContext.Contains(fieldGuid))
				{
					std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " GUID already exists" << std::endl;
					continue;
				}

				std::vector<Reflection::TemplateParameterDescription> fieldTemplateParamDescs;
                Reflection::MetaType fieldMetaType = Reflection::MetaType::Invalid;
                uint32_t fieldHash = 0;
				size_t fieldOffset = 0;
				size_t fieldSize = 0;
                
                clang::QualType fieldType = field->getType();
                if (fieldType->isArrayType())
                {
                    const clang::ArrayType* arrayType = fieldType->getAsArrayTypeUnsafe();
                    if (arrayType->isConstantArrayType())
                    {
						fieldMetaType = Reflection::MetaType::Array;
						fieldHash = HandleArrayType(static_cast<const clang::ConstantArrayType*>(arrayType));
						if (fieldHash == 0)
						{
							std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " is not reflected" << std::endl;
							continue;
						}

						fieldOffset = field->getASTContext().getFieldOffset(field) / 8ul; // Convert bits to bytes
						fieldSize = field->getASTContext().getTypeSize(fieldType) / 8ul; // Convert bits to bytes
                    }
					else
					{
						std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " is not reflected" << std::endl;
						continue;
					}
                }
                else if (fieldType->isRecordType())
                {
                    const clang::RecordType* recordType = fieldType->getAs<clang::RecordType>();
					const clang::CXXRecordDecl* fieldDecl = recordType->getAsCXXRecordDecl();

					if (fieldDecl->hasAttr<clang::AnnotateAttr>() == false)
					{
						std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " is not reflected" << std::endl;
						continue;
					}
                    
					auto fieldHandle = HandleRecordDecl(fieldDecl);
					if (fieldHandle == InvalidMetaIndex)
					{
						std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " is not reflected" << std::endl;
						continue;
					}

					fieldMetaType = Reflection::MetaType::Class;
					fieldHash = mContext.mClasses[fieldHandle].TypeHash();

					if (const auto specDecl = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(fieldDecl))
					{
						if (specDecl->getTemplateSpecializationKind() == clang::TSK_ExplicitInstantiationDefinition || specDecl->getTemplateSpecializationKind() == clang::TSK_ImplicitInstantiation)
						{
							fieldOffset = field->getASTContext().getFieldOffset(field) / 8ul; // Convert bits to bytes
							fieldSize = field->getASTContext().getTypeSize(fieldType) / 8ul; // Convert bits to bytes

							const auto& args = specDecl->getTemplateInstantiationArgs();
							for (const auto& arg : args.asArray())
							{
								auto argType = arg.getAsType();
								if (argType->isBuiltinType())
								{
									const auto argBuiltinType = argType->getAs<clang::BuiltinType>();
									fieldTemplateParamDescs.emplace_back(Reflection::MetaType::Primitive, BuiltinTypeHash(argBuiltinType));
								}
								else if (argType->isRecordType())
								{
									const auto argRecordType = argType->getAs<clang::RecordType>();
									const auto argRecordDecl = argRecordType->getAsCXXRecordDecl();

									auto argClassHandle = HandleRecordDecl(argRecordDecl);
									if (argClassHandle != InvalidMetaIndex)
									{
										fieldTemplateParamDescs.emplace_back(Reflection::MetaType::Class, argClassHandle);
									}
								}
								else if (argType->isEnumeralType())
								{
									const auto argEnumType = argType->getAs<clang::EnumType>();
									const auto argEnumDecl = argEnumType->getDecl();

									auto argEnumHandle = HandleEnumDecl(argEnumDecl);
									if (argEnumHandle != InvalidMetaIndex)
									{
										fieldTemplateParamDescs.emplace_back(Reflection::MetaType::Enum, argEnumHandle);
									}
								}
							}
						}
						else
						{
							std::cerr << "Template specialization kind reflection is not supported yet for field " << recordDecl->getName().str() << "::" << field->getName().str() << std::endl;
							continue;
						}
					}
					else
					{
						fieldOffset = field->getASTContext().getFieldOffset(field) / 8ul; // Convert bits to bytes
						fieldSize = field->getASTContext().getTypeSize(fieldType) / 8ul; // Convert bits to bytes
					}
                }
                else if (fieldType->isEnumeralType())
                {
                    const clang::EnumType* enumType = fieldType->getAs<clang::EnumType>();
					const clang::EnumDecl* enumDecl = enumType->getDecl();

					if (enumDecl->hasAttr<clang::AnnotateAttr>() == false)
					{
						std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " is not reflected" << std::endl;
						continue;
					}

					auto fieldHandle = HandleEnumDecl(enumDecl);
					if (fieldHandle == InvalidMetaIndex)
					{
						std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " is not reflected" << std::endl;
						continue;
					}

                    std::stringstream qualifiedName;
                    qualifiedName << context.QualifiedName() << "::" << std::string_view(enumDecl->getName());
                    
					fieldMetaType = Reflection::MetaType::Enum;
					fieldHash = mContext.mEnums[fieldHandle].TypeHash();
					fieldOffset = field->getASTContext().getFieldOffset(field) / 8ul; // Convert bits to bytes
					fieldSize = field->getASTContext().getTypeSize(fieldType) / 8ul; // Convert bits to bytes
                }
                else if (fieldType->isBuiltinType())
                {
                    const clang::BuiltinType* builtinType = fieldType->getAs<clang::BuiltinType>();
                    
					fieldMetaType = Reflection::MetaType::Primitive;
					fieldHash = BuiltinTypeHash(builtinType);
					fieldOffset = field->getASTContext().getFieldOffset(field) / 8ul; // Convert bits to bytes
					fieldSize = field->getASTContext().getTypeSize(fieldType) / 8ul; // Convert bits to bytes
                }
                else
                {
                    // Unknown type
					std::cerr << recordDecl->getName().str() << "::" << field->getName().str() << " is not reflected" << std::endl;
                    continue;
                }
				auto fieldNameStr = field->getName();
				auto fieldName = mStringWriter.Write(fieldNameStr.data(), fieldNameStr.size());

				std::stringstream fieldQualifiedNameSS;
				fieldQualifiedNameSS << qualifiedClassNameStr << "::" << std::string_view(fieldNameStr);

				auto fieldQualifiedNameStr = fieldQualifiedNameSS.str();
				auto fieldQualifiedName = mStringWriter.Write(fieldQualifiedNameStr.data(), fieldQualifiedNameStr.size());
				auto fieldTemplateParams = mObjectWriter.Write(fieldTemplateParamDescs.data(), fieldTemplateParamDescs.size());
                fieldDescs.emplace_back(Reflection::FieldDescription({ fieldName, fieldQualifiedName, fieldAttribs, fieldGuid, fieldHash }, fieldTemplateParams, fieldOffset, fieldSize, fieldMetaType));
            }
        }
        
        // Process functions
        for (const auto method : recordDecl->methods())
        {
			if (method->hasAttr<clang::AnnotateAttr>() == false)
			{
				continue; // item is not reflected
			}

            auto annotateAttr = method->getAttr<clang::AnnotateAttr>();
            auto annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GFUNCTION"))
            {
                auto attribs = ParseAttributes(annotation);
                auto guid = ExtractGuid(attribs);
				if (guid == Reflection::Attribute::Guid::InvalidGuid())
				{
					std::cerr << recordDecl->getName().str() << "::" << method->getName().str() << " is missing GUID attribute" << std::endl;
					continue;
				}

				if (mContext.Contains(guid))
				{
					std::cerr << recordDecl->getName().str() << "::" << method->getName().str() << " GUID already exists" << std::endl;
					continue;
				}

                // TODO: function reflection support
            }
        }
        
		auto bases = mObjectWriter.Write(baseClasses.data(), baseClasses.size() * sizeof(ClassHandle));
        auto fields = mObjectWriter.Write(fieldDescs.data(), fieldDescs.size() * sizeof(Reflection::FieldDescription));
		auto templateParams = mObjectWriter.Write(templateParamDescs.data(), templateParamDescs.size() * sizeof(Reflection::TemplateParameterDescription));
		auto handle = context.RegisterClass(Reflection::ClassDescription({ className, qualifiedClassName, recordAttribs, recordGuid, typeHash }, classSize, fields, bases, templateParams), templateDeclStr);
		if (handle == InvalidMetaIndex)
		{
			std::cerr << recordDecl->getName().str() << " GUID already exists" << std::endl;
			return {};
		}

		auto headerPath = ExtractHeaderPath(recordDecl->getLocation(), recordDecl->getASTContext());
		if (headerPath.empty() == false)
		{
			mHeaders.insert(headerPath);
		}
		else
		{
			std::cerr << "Failed to extract header path for " << recordDecl->getName().str() << std::endl;
		}
		return handle;
    }
    return {};
}

ArrayHandle ReflectionParser::HandleArrayType(const clang::ConstantArrayType* arrayType)
{
    if (arrayType == nullptr)
    {
        return {};
    }
    
    size_t arraySize = arrayType->getSizeBitWidth() / 8ul; // Convert to bytes
    const clang::QualType elementType = arrayType->getElementType();
    
    if (elementType->isConstantArrayType())
    {
        const auto innerArrayType = static_cast<const clang::ConstantArrayType*>(elementType->getAsArrayTypeUnsafe());
        const auto innerArrayHandle = HandleArrayType(innerArrayType);
        if (innerArrayHandle == InvalidMetaIndex)
        {
            return {};
        }

        const auto& innerArrayDesc = mContext.mArrays[innerArrayHandle];
        Reflection::ArrayDescription arrayDesc(Reflection::MetaType::Array, innerArrayHandle, arraySize);
    	return mContext.RegisterArray(arrayDesc);
    }
    else if (elementType->isRecordType())
    {
        const auto recordType = elementType->getAs<clang::RecordType>();
        const auto classHandle = HandleRecordDecl(recordType->getAsCXXRecordDecl());
        if (classHandle == InvalidMetaIndex)
        {
            return {};
        }

        const auto& classDesc = mContext.mClasses[classHandle];
        Reflection::ArrayDescription arrayDesc(Reflection::MetaType::Class, classDesc.TypeHash(), arraySize);
    	return mContext.RegisterArray(arrayDesc);
    }
    else if (elementType->isEnumeralType())
    {
        const clang::EnumType* enumType = elementType->getAs<clang::EnumType>();
        const auto enumHandle = HandleEnumDecl(enumType->getDecl());
        if (enumHandle == InvalidMetaIndex)
        {
            return {};
        }

        const auto& enumDesc = mContext.mEnums[enumHandle];
		Reflection::ArrayDescription arrayDesc(Reflection::MetaType::Enum, enumDesc.TypeHash(), arraySize);
    	return mContext.RegisterArray(arrayDesc);
    }
    else if (elementType->isBuiltinType())
    {
        const auto builtinType = elementType->getAs<clang::BuiltinType>();
		Reflection::ArrayDescription arrayDesc(Reflection::MetaType::Primitive, BuiltinTypeHash(builtinType), arraySize);
    	return mContext.RegisterArray(arrayDesc);
	}
	return {};
}

ReflectionContext& ReflectionParser::GetDeclReflectionContext(const clang::DeclContext* declContext)
{
	if (declContext == nullptr)
	{
		return mContext;
	}

	while (declContext)
	{
		if (const auto namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(declContext))
		{
			std::string namespaceName = namespaceDecl->getNameAsString();
			std::string namespaceQualifiedName = namespaceDecl->getQualifiedNameAsString();

			size_t idx = namespaceQualifiedName.find("Gleam::Reflection::External::");
			if (idx != std::string::npos)
			{
				namespaceQualifiedName = namespaceQualifiedName.substr(idx + std::strlen("Gleam::Reflection::External::"));
			}
			return mContext.EmplaceContext(namespaceName, namespaceQualifiedName);
		}
		declContext = declContext->getParent();
	}
	return mContext;
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

			if (view->size != sizeof(Reflection::Attribute::Guid))
			{
				std::cerr << "Attribute view does not match with GUID" << std::endl;
				return Reflection::Attribute::Guid::InvalidGuid();
			}
			return *guid;
		}
	}
    return Reflection::Attribute::Guid::InvalidGuid();
}

std::string ReflectionParser::ExtractHeaderPath(const clang::SourceLocation& loc, clang::ASTContext& context) const
{
	if (loc.isInvalid())
		return "";

	auto& sourceManager = context.getSourceManager();
	auto presumedLoc = sourceManager.getPresumedLoc(loc);

	if (presumedLoc.isInvalid())
		return "";

	std::string filename = presumedLoc.getFilename();
	return filename;
}

std::string ReflectionParser::ExtractTemplateDeclaration(const clang::ClassTemplateSpecializationDecl* specDecl) const
{
	std::string decl;

	const auto specializedTemplateDecl = specDecl->getSpecializedTemplate();
	const auto templateParams = specializedTemplateDecl->getTemplateParameters();

	clang::PrintingPolicy policy = specializedTemplateDecl->getASTContext().getLangOpts();
	policy.SuppressDefaultTemplateArgs = true;

	llvm::raw_string_ostream templateDeclOS(decl);
	templateDeclOS << "template<";

	for (unsigned i = 0; i < templateParams->size(); ++i)
	{
		if (i > 0) templateDeclOS << ", ";

		auto param = templateParams->getParam(i);
		templateDeclOS << ExtractTemplateParameter(param, policy);
	}
	templateDeclOS << ">";

	return decl;
}

std::string ReflectionParser::ExtractTemplateParameter(const clang::NamedDecl* param, const clang::PrintingPolicy& policy) const
{
	std::string name;
	llvm::raw_string_ostream nameOS(name);

	if (auto typeParam = llvm::dyn_cast<clang::TemplateTypeParmDecl>(param))
	{
		if (typeParam->wasDeclaredWithTypename())
			nameOS << "typename ";
		else
			nameOS << "class ";
		nameOS << typeParam->getName();
	}
	else if (auto nonTypeParam = llvm::dyn_cast<clang::NonTypeTemplateParmDecl>(param))
	{
		nonTypeParam->getType().print(nameOS, policy);
		nameOS << " " << nonTypeParam->getName();
	}
	else if (auto templateTemplateParam = llvm::dyn_cast<clang::TemplateTemplateParmDecl>(param))
	{
		nameOS << "template<";
		auto innerParams = templateTemplateParam->getTemplateParameters();
		for (unsigned j = 0; j < innerParams->size(); ++j)
		{
			if (j > 0) nameOS << ", ";

			auto innerParam = innerParams->getParam(j);
			ExtractTemplateParameter(innerParam, policy);
		}
		nameOS << "> class " << templateTemplateParam->getName();
	}
	return name;
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
