#pragma once
#include <memory>
#include <vector>
#include <display_interface.h>
#include <input_interface.h>

struct Region;
struct Coord;
struct Environment;
struct TextConfig;
struct TileConfig;
struct FontConfig;

struct GraphicsConfig
{
    std::string name;
    TextConfig* text_cfg;
    FontConfig* font_cfg;
    TileConfig* tile_cfg;
    bool use_unix_gfx;
    bool use_colors;
    bool use_standout;
    bool animate;
};

struct Options
{
    std::string name;
    std::string dll_name;
    Coord screen;
    Coord small_screen;
    bool emulate_ctrl_controls;
    bool is_unix;
    std::vector<GraphicsConfig> gfx_options;
};


extern std::vector<Options> s_options;

struct SdlRogue : public DisplayInterface, public InputInterface
{
    SdlRogue(SDL_Window* window, SDL_Renderer* renderer, std::shared_ptr<Environment> env, int index);
    SdlRogue(SDL_Window* window, SDL_Renderer* renderer, std::shared_ptr<Environment> env, const std::string& filename);
    ~SdlRogue();

    void Run();
    void Quit();

    Environment* GameEnv() const;
    Options options() const;

    //display interface
    virtual void SetDimensions(Coord dimensions) override;
    virtual void UpdateRegion(uint32_t* buf) override;
    virtual void UpdateRegion(uint32_t* info, Region rect) override;
    virtual void MoveCursor(Coord pos) override;
    virtual void SetCursor(bool enable) override;

    //input interface
    virtual char GetChar(bool block, bool for_string, bool *is_replay) override;
    virtual void Flush() override;

    static const char* WindowTitle;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
