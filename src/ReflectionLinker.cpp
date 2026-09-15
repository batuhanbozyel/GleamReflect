#include "ReflectionLinker.h"

#include <fstream>
#include <iostream>
#include <unordered_map>

using namespace Gleam;

bool ReflectionLinker::Read(const std::filesystem::path& path, Module& outModule) const
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (not file.is_open())
	{
		std::cerr << "Failed to open reflection database: " << path << std::endl;
		return false;
	}

	const size_t size = file.tellg();
	if (size < sizeof(Reflection::DatabaseHeader))
	{
		std::cerr << "Reflection database is truncated: " << path << std::endl;
		return false;
	}

	file.seekg(0);
	outModule.data.resize(size);
	file.read(outModule.data.data(), size);

	const auto header = outModule.Header();
	if (memcmp(header->magic, "GLEAMREF", sizeof(header->magic)) != 0)
	{
		std::cerr << "Not a reflection database: " << path << std::endl;
		return false;
	}

	if (header->version != GLEAM_REFLECTION_VERSION)
	{
		std::cerr << "Reflection database " << path << " has version " << header->version
				  << ", expected " << GLEAM_REFLECTION_VERSION << std::endl;
		return false;
	}
	return true;
}

void ReflectionLinker::PatchMeta(Reflection::MetaDescription& meta, char* objects, size_t objectBase, size_t stringBase)
{
	meta.mName.offset += stringBase;
	meta.mQualifiedName.offset += stringBase;
	meta.mAttributes.offset += objectBase;

	const auto attributeCount = meta.mAttributes.size / sizeof(uint32_t);
	auto views = reinterpret_cast<Reflection::BufferView*>(objects + meta.mAttributes.offset + meta.mAttributes.size);
	for (size_t i = 0; i < attributeCount; ++i)
	{
		views[i].offset += objectBase;
	}
}

void ReflectionLinker::PatchClass(Reflection::ClassDescription& classDesc, char* objects, size_t objectBase, size_t stringBase)
{
	PatchMeta(classDesc, objects, objectBase, stringBase);

	classDesc.mFields.offset += objectBase;
	classDesc.mBaseClasses.offset += objectBase;
	classDesc.mTemplateParams.offset += objectBase;

	const auto fieldCount = classDesc.mFields.size / sizeof(Reflection::FieldDescription);
	auto fields = reinterpret_cast<Reflection::FieldDescription*>(objects + classDesc.mFields.offset);
	for (size_t i = 0; i < fieldCount; ++i)
	{
		PatchMeta(fields[i], objects, objectBase, stringBase);
	}
}

void ReflectionLinker::PatchEnum(Reflection::EnumDescription& enumDesc, char* objects, size_t objectBase, size_t stringBase)
{
	PatchMeta(enumDesc, objects, objectBase, stringBase);

	enumDesc.mCases.offset += objectBase;

	const auto caseCount = enumDesc.mCases.size / sizeof(Reflection::EnumCaseDescription);
	auto cases = reinterpret_cast<Reflection::EnumCaseDescription*>(objects + enumDesc.mCases.offset);
	for (size_t i = 0; i < caseCount; ++i)
	{
		PatchMeta(cases[i], objects, objectBase, stringBase);
	}
}

bool ReflectionLinker::Link(const std::vector<std::filesystem::path>& inputs, const std::filesystem::path& output)
{
	if (inputs.empty())
	{
		std::cerr << "No reflection databases to link" << std::endl;
		return false;
	}

	std::vector<Module> modules(inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i)
	{
		if (Read(inputs[i], modules[i]) == false)
		{
			return false;
		}
	}

	size_t objectTotal = 0;
	size_t stringTotal = 0;
	uint32_t classCount = 0;
	uint32_t enumCount = 0;
	uint32_t arrayCount = 0;
	for (auto& module : modules)
	{
		module.objectBase = objectTotal;
		module.stringBase = stringTotal;
		objectTotal += module.ObjectSize();
		stringTotal += module.StringSize();

		classCount += module.Header()->classCount;
		enumCount += module.Header()->enumCount;
		arrayCount += module.Header()->arrayCount;
	}

	Reflection::DatabaseHeader header = {};
	memcpy(header.magic, "GLEAMREF", sizeof(header.magic));
	header.version = GLEAM_REFLECTION_VERSION;
	header.classCount = classCount;
	header.enumCount = enumCount;
	header.arrayCount = arrayCount;
	header.classTableOffset = sizeof(Reflection::DatabaseHeader) + objectTotal;
	header.enumTableOffset = header.classTableOffset + classCount * sizeof(Reflection::ClassDescription);
	header.arrayTableOffset = header.enumTableOffset + enumCount * sizeof(Reflection::EnumDescription);
	header.stringTableOffset = header.arrayTableOffset + arrayCount * sizeof(Reflection::ArrayDescription);

	std::vector<char> buffer(header.stringTableOffset + stringTotal);
	memcpy(buffer.data(), &header, sizeof(Reflection::DatabaseHeader));

	auto objects = buffer.data() + sizeof(Reflection::DatabaseHeader);
	size_t classCursor = 0;
	size_t enumCursor = 0;
	size_t arrayCursor = 0;
	std::unordered_map<uint32_t, std::string> typeHashes;

	for (const auto& module : modules)
	{
		const auto moduleHeader = module.Header();

		memcpy(objects + module.objectBase,
			   module.data.data() + sizeof(Reflection::DatabaseHeader),
			   module.ObjectSize());

		memcpy(buffer.data() + header.stringTableOffset + module.stringBase,
			   module.data.data() + moduleHeader->stringTableOffset,
			   module.StringSize());

		auto classes = reinterpret_cast<Reflection::ClassDescription*>(buffer.data() + header.classTableOffset) + classCursor;
		memcpy(classes,
			   module.data.data() + moduleHeader->classTableOffset,
			   moduleHeader->classCount * sizeof(Reflection::ClassDescription));
		classCursor += moduleHeader->classCount;

		auto enums = reinterpret_cast<Reflection::EnumDescription*>(buffer.data() + header.enumTableOffset) + enumCursor;
		memcpy(enums,
			   module.data.data() + moduleHeader->enumTableOffset,
			   moduleHeader->enumCount * sizeof(Reflection::EnumDescription));
		enumCursor += moduleHeader->enumCount;

		auto arrays = reinterpret_cast<Reflection::ArrayDescription*>(buffer.data() + header.arrayTableOffset) + arrayCursor;
		memcpy(arrays,
			   module.data.data() + moduleHeader->arrayTableOffset,
			   moduleHeader->arrayCount * sizeof(Reflection::ArrayDescription));
		arrayCursor += moduleHeader->arrayCount;

		for (uint32_t i = 0; i < moduleHeader->classCount; ++i)
		{
			PatchClass(classes[i], objects, module.objectBase, module.stringBase);
		}

		for (uint32_t i = 0; i < moduleHeader->enumCount; ++i)
		{
			PatchEnum(enums[i], objects, module.objectBase, module.stringBase);
		}
	}

	auto linkedClasses = reinterpret_cast<const Reflection::ClassDescription*>(buffer.data() + header.classTableOffset);
	auto linkedEnums = reinterpret_cast<const Reflection::EnumDescription*>(buffer.data() + header.enumTableOffset);
	for (uint32_t i = 0; i < classCount; ++i)
	{
		auto [it, inserted] = typeHashes.emplace(linkedClasses[i].TypeHash(), "");
		if (inserted == false)
		{
			std::cerr << "Type is emitted by more than one module: hash " << linkedClasses[i].TypeHash() << std::endl;
		}
	}
	for (uint32_t i = 0; i < enumCount; ++i)
	{
		auto [it, inserted] = typeHashes.emplace(linkedEnums[i].TypeHash(), "");
		if (inserted == false)
		{
			std::cerr << "Type is emitted by more than one module: hash " << linkedEnums[i].TypeHash() << std::endl;
		}
	}

	std::ofstream file(output, std::ios::out | std::ios::trunc | std::ios::binary);
	if (not file.is_open())
	{
		std::cerr << "Failed to write linked reflection database: " << output << std::endl;
		return false;
	}
	file.write(buffer.data(), buffer.size());
	return true;
}
