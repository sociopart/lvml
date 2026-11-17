#include "LvmlCursor.hpp"
#include "TextUtils.hpp"

std::unordered_map<const lv_obj_t*, CLvmlCursor::ECursor>& CLvmlCursor::Map()
{
    static std::unordered_map<const lv_obj_t*, ECursor> map;
    return map;
}

bool CLvmlCursor::Parse(std::string_view name, ECursor& out)
{
    struct SName { const char* name; ECursor cursor; };
    static const SName kNames[] = {
        { "default", ECursor::Default },       { "auto", ECursor::Default },      { "pointer", ECursor::Pointer },
        { "hand", ECursor::Pointer },          { "grab", ECursor::Pointer },      { "grabbing", ECursor::Pointer },
        { "text", ECursor::Text },             { "move", ECursor::Move },         { "all-scroll", ECursor::Move },
        { "not-allowed", ECursor::NotAllowed }, { "no-drop", ECursor::NotAllowed }, { "wait", ECursor::Wait },
        { "progress", ECursor::Progress },     { "crosshair", ECursor::Crosshair }, { "help", ECursor::Help },
    };
    const std::string n = CTextUtils::ToLower(CTextUtils::Trim(name));
    for (const SName& k : kNames)
        if (n == k.name) { out = k.cursor; return true; }
    return false;
}

void CLvmlCursor::Set(lv_obj_t* obj, ECursor cursor)
{
    Map()[obj] = cursor;
}

void CLvmlCursor::Clear(lv_obj_t* obj)
{
    Map().erase(obj);
}

CLvmlCursor::ECursor CLvmlCursor::Resolve(lv_obj_t* obj)
{
    const auto& map = Map();
    for (; obj; obj = lv_obj_get_parent(obj)) {
        const auto it = map.find(obj);
        if (it != map.end()) return it->second;
    }
    return ECursor::Default;
}
