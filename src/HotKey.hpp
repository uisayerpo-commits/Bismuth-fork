#pragma once

#include "common.hpp"

#define DECLARE_HOTKEY(K, F) static HotKey GEODE_CONCAT(__HOTKEY_, __LINE__)(K, []() F);

// Same as DECLARE_HOTKEY, but the key is only used (and only consumed,
// so the game still sees it) when COND is true when it is pressed.
#define DECLARE_HOTKEY_IF(K, COND, F) \
    static HotKey GEODE_CONCAT(__HOTKEY_, __LINE__)(K, []() F, []() -> bool { return (COND); });

struct HotKey {
    cocos2d::enumKeyCodes key;
    std::function<void()> pressed;
    std::function<bool()> condition;
    HotKey* next = nullptr;

    static HotKey* hotKeysHead;
    static HotKey* hotKeysTail;

    inline HotKey(
        cocos2d::enumKeyCodes key,
        std::function<void()> pressed,
        std::function<bool()> condition = nullptr
    )
        : key(key), pressed(pressed), condition(condition)
    {
        if (hotKeysTail == nullptr)
            hotKeysHead = this;
        else
            hotKeysTail->next = this;
        hotKeysTail = this;
    }
};