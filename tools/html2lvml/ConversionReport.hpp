// lvml — html2lvml: conversion report.
#pragma once
#include <map>
#include <set>
#include <string>

// Collects what could not be converted, so the report tells the user what to fix by hand.
class CConversionReport
{
public:
    void Drop(const std::string& what, const std::string& reason) { m_dropped[what].insert(reason); }
    void Note(const std::string& text) { m_notes.insert(text); }
    void Asset(const std::string& from, const std::string& to) { m_assets[from] = to; }

    const std::map<std::string, std::set<std::string>>& GetDropped() const { return m_dropped; }
    const std::map<std::string, std::string>&           GetAssets() const { return m_assets; }

    std::string ToString() const;

private:
    std::map<std::string, std::set<std::string>> m_dropped;
    std::set<std::string>                        m_notes;
    std::map<std::string, std::string>           m_assets; // original → path used in the output
};
