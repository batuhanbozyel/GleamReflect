#pragma once
#include "Reflection/Meta.h"
#include "Reflection/IDatabase.h"

#include <filesystem>
#include <vector>
#include <map>
#include <string_view>

namespace Gleam {

class ReflectionLinker
{
public:

	bool Link(const std::vector<std::filesystem::path>& inputs, const std::filesystem::path& output, const std::filesystem::path& natvis);

private:

	using AttributeTypeMap = std::map<uint32_t, std::pair<Reflection::AttributeTypeDescription, std::string_view>>;

	struct Module
	{
		std::vector<char> data;
		size_t objectBase = 0;
		size_t stringBase = 0;

		const Reflection::DatabaseHeader* Header() const
		{
			return reinterpret_cast<const Reflection::DatabaseHeader*>(data.data());
		}

		size_t ObjectSize() const
		{
			return Header()->classTableOffset - sizeof(Reflection::DatabaseHeader);
		}

		size_t StringSize() const
		{
			return data.size() - Header()->stringTableOffset;
		}
	};

	bool Read(const std::filesystem::path& path, Module& outModule) const;

	static void PatchMeta(Reflection::MetaDescription& meta, char* objects, size_t objectBase, size_t stringBase);

	static void PatchClass(Reflection::ClassDescription& classDesc, char* objects, size_t objectBase, size_t stringBase);

	static void PatchEnum(Reflection::EnumDescription& enumDesc, char* objects, size_t objectBase, size_t stringBase);

	static bool WriteNatvis(const std::filesystem::path& path, const AttributeTypeMap& attributeTypes);
};

} // namespace Gleam
