#pragma once

#include <vector>
#include "scoped_page_permissions.h"

namespace iidxtra::util
{
	class code_patch
	{
		public:
			template <std::size_t size>
			code_patch(void* target, std::uint8_t(&&bytes) [size]):
			    _target(target), _size(size)
			{
				// backup the original bytes
				_original.reserve(size);
				_patch.reserve(size);

				std::memcpy(_original.data(), target, size);
				std::memcpy(_patch.data(), bytes, size);
			}

			auto enable() const -> void
			{
				auto guard = scoped_page_permissions { _target, _size, PAGE_EXECUTE_READWRITE };
				std::memcpy(_target, _patch.data(), _size);
			}

			auto disable() const -> void
			{
				auto guard = scoped_page_permissions { _target, _size, PAGE_EXECUTE_READWRITE };
				std::memcpy(_target, _original.data(), _size);
			}
		private:
			void* _target = nullptr;
			std::size_t _size = 0;
			std::vector<std::byte> _original = {};
			std::vector<std::byte> _patch = {};
	};
}