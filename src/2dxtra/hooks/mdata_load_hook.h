#pragma once

namespace iidxtra::mdata_load_hook
{
	auto gather_custom_sets(const std::filesystem::path& path) -> void;

	auto install_hook() -> void;
}