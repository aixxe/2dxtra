#include <cstdint>
#include <algorithm>
#include <windows.h>
#include "memory.h"

namespace iidxtra::util
{
    auto alloc_near_address(void* target) -> void*
    {
        auto info = SYSTEM_INFO {};
        GetSystemInfo(&info);

        if (info.dwPageSize == 0)
            return nullptr;

        auto const page_size = static_cast<std::uint64_t>(info.dwPageSize);

        auto const start = reinterpret_cast<std::uint64_t>(target) & ~(page_size - 1);
        auto const min = std::min(start - 0x7FFFFF00, reinterpret_cast<std::uint64_t>(info.lpMinimumApplicationAddress));
        auto const max = std::max(start + 0x7FFFFF00, reinterpret_cast<std::uint64_t>(info.lpMaximumApplicationAddress));

        auto offset = std::uint64_t { 1 };
        auto const start_page = start - (start % page_size);
        
        while (true)
        {
            auto const byteOffset = offset * page_size;
            auto const high = start_page + byteOffset;
            auto const low = (start_page > byteOffset) ? start_page - byteOffset : 0;

            if (high < max)
            {
                auto const result = VirtualAlloc(reinterpret_cast<void*>(high),
                    page_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

                if (result)
                    return result;
            }

            if (low > min)
            {
                auto const result = VirtualAlloc(reinterpret_cast<void*>(low),
                    page_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

                if (result != nullptr)
                    return result;
            }

            offset++;

            if (high > max && low < min)
                break;
        }

        return nullptr;
    }
}