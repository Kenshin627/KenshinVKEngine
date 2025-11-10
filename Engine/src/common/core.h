#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "logger.h"

#define KS_CORE_ASSERT(x, ...) { if(!(x)) { KS_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define KS_CORE_ASSERT_NO_BREAK(x, ...) { if(!(x)) { KS_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); } }

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            KS_CORE_ERROR("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)          