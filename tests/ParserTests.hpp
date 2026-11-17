// lvml — parser self-tests (no LVGL needed).
#pragma once
#include <string>

class CParserTests
{
public:
    CParserTests() = delete;
    static int Run(); // returns the number of failed checks

private:
    static void Check(bool condition, const std::string& what);
    static void TestMarkup();
    static void TestHtmlMode();
    static void TestStyleSheet();
    static void TestSelectors();
    static void TestValues();

    static int s_failed;
    static int s_passed;
};
