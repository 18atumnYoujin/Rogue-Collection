#pragma once
#include <memory>
#include "RogueCore\display_interface.h"
#include "RogueCore\coord.h"

struct _CHAR_INFO;
struct _SMALL_RECT;

struct SdlWindow : public DisplayInterface
{
    SdlWindow();
    ~SdlWindow();

    virtual void Draw(_CHAR_INFO* info, Coord dimensions);
    virtual void Draw(_CHAR_INFO* info, Coord dimensions, _SMALL_RECT rect);
    virtual void MoveCursor(Coord pos);
    virtual void SetCursor(bool enable);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
