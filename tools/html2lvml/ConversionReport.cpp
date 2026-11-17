#include "ConversionReport.hpp"

std::string CConversionReport::ToString() const
{
    std::string out;
    if (!m_assets.empty()) {
        out += "Assets to provide in a format LVGL decodes (PNG):\n";
        for (const auto& a : m_assets) out += "  " + a.first + "  ->  " + a.second + "\n";
    }
    if (!m_dropped.empty()) {
        out += "Not converted:\n";
        for (const auto& d : m_dropped) {
            out += "  " + d.first + ":";
            size_t n = 0;
            for (const std::string& r : d.second) {
                if (n++ == 4) { out += " …"; break; }
                out += (n == 1 ? " " : "; ") + r;
            }
            out += "\n";
        }
    }
    if (!m_notes.empty()) {
        out += "Notes:\n";
        for (const std::string& n : m_notes) out += "  " + n + "\n";
    }
    return out;
}
