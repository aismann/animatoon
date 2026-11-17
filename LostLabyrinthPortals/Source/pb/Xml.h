// Minimal DOM-style XML reader/writer, enough to replace PureBasic's XML library
// for the game's data files (elements, attributes, text, comments, prolog).
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace pb
{

struct XmlNode
{
    std::string name;
    std::string text;  // concatenated character data
    std::vector<std::pair<std::string, std::string>> attrs;
    std::vector<std::unique_ptr<XmlNode>> children;

    const std::string* attr(const std::string& n) const
    {
        for (auto& a : attrs)
            if (a.first == n)
                return &a.second;
        return nullptr;
    }
    std::string attrOr(const std::string& n, const std::string& def = "") const
    {
        auto p = attr(n);
        return p ? *p : def;
    }
    XmlNode* add(const std::string& childName)
    {
        children.push_back(std::make_unique<XmlNode>());
        children.back()->name = childName;
        return children.back().get();
    }
    XmlNode* add(const std::string& childName, const std::string& childText)
    {
        auto* n = add(childName);
        n->text = childText;
        return n;
    }
    void set(const std::string& n, const std::string& v)
    {
        for (auto& a : attrs)
            if (a.first == n)
            {
                a.second = v;
                return;
            }
        attrs.emplace_back(n, v);
    }
};

struct XmlDoc
{
    std::unique_ptr<XmlNode> root;
    std::string error;

    bool parse(const std::string& src);
    bool load(const std::string& path);
    std::string serialize() const;
    bool save(const std::string& path) const;
};

// PureBasic Val(): leading integer, 0 on garbage; also accepts "$hex"
long long val(const std::string& s);
std::string str(long long v);

}  // namespace pb
