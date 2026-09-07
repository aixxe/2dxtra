#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
#include "scoped_page_permissions.h"

namespace iidxtra::util
{
	class code_patch
	{
		public:
			template <std::size_t size>
			code_patch(void* target, std::uint8_t(&&bytes) [size]):
			    _target(target), _original(size), _patch(bytes, bytes + size)
			{
				std::memcpy(_original.data(), target, size);
			}

			auto enable() const -> void
				{ write(_patch); }

			auto disable() const -> void
				{ write(_original); }
		private:
			auto write(const std::vector<std::uint8_t>& bytes) const -> void
			{
				auto guard = scoped_page_permissions { _target, bytes.size(), PAGE_EXECUTE_READWRITE };
				std::memcpy(_target, bytes.data(), bytes.size());
				FlushInstructionCache(GetCurrentProcess(), _target, bytes.size());
			}

			void* _target = nullptr;
			std::vector<std::uint8_t> _original = {};
			std::vector<std::uint8_t> _patch = {};
	};

	class branch_patch
	{
		public:
			explicit branch_patch(std::uint8_t* target): _target(target)
			{
				auto const op = target[0];

				if (op >= 0x70 && op <= 0x7F)
					_size = 2, _patch = { 0xEB, target[1] };
				else if (op == 0x0F && (target[1] & 0xF0) == 0x80)
				{
					auto rel = std::int32_t {};
					std::memcpy(&rel, target + 2, sizeof(rel));
					rel += 1;

					_size = 6;
					_patch = { 0xE9, 0, 0, 0, 0, 0x90 };
					std::memcpy(_patch.data() + 1, &rel, sizeof(rel));
				}

				if (_size != 0)
					_original.assign(target, target + _size);
			}

			auto valid() const -> bool
				{ return _size != 0; }

			auto enable() const -> void
				{ write(_patch); }

			auto disable() const -> void
				{ write(_original); }
		private:
			auto write(const std::vector<std::uint8_t>& bytes) const -> void
			{
				if (_size == 0)
					return;

				auto guard = scoped_page_permissions { _target, _size, PAGE_EXECUTE_READWRITE };
				std::memcpy(_target, bytes.data(), _size);
				FlushInstructionCache(GetCurrentProcess(), _target, _size);
			}

			std::uint8_t* _target = nullptr;
			std::size_t _size = 0;
			std::vector<std::uint8_t> _original = {};
			std::vector<std::uint8_t> _patch = {};
	};
}
