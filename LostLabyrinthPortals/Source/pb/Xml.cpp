#include "pb/Xml.h"

#include "pb/Platform.h"

#include <cctype>
#include <cstdlib>

namespace pb
{

namespace
{
struct Parser
{
    const std::string& s;
    size_t p = 0;
    std::string err;

    explicit Parser(const std::string& src) : s(src) {}

    bool eof() const { return p >= s.size(); }
    void ws()
    {
        while (!eof() && std::isspace((unsigned char)s[p]))
            ++p;
    }
    bool startsWith(const char* t) const { return s.compare(p, std::char_traits<char>::length(t), t) == 0; }

    static std::string unescape(const std::string& in)
    {
        std::string out;
        out.reserve(in.size());
        for (size_t i = 0; i < in.size(); ++i)
        {
            if (in[i] != '&')
            {
                out += in[i];
                continue;
            }
            size_t e = in.find(';', i);
            if (e == std::string::npos)
            {
                out += in[i];
                continue;
            }
            std::string ent = in.substr(i + 1, e - i - 1);
            if (ent == "amp")
                out += '&';
            else if (ent == "lt")
                out += '<';
            else if (ent == "gt")
                out += '>';
            else if (ent == "quot")
                out += '"';
            else if (ent == "apos")
                out += '\'';
            else if (!ent.empty() && ent[0] == '#')
            {
                long cp = ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X') ? std::strtol(ent.c_str() + 2, nullptr, 16)
                                                                              : std::strtol(ent.c_str() + 1, nullptr, 10);
                // UTF-8 encode
                if (cp < 0x80)
                    out += (char)cp;
                else if (cp < 0x800)
                {
                    out += (char)(0xC0 | (cp >> 6));
                    out += (char)(0x80 | (cp & 0x3F));
                }
                else
                {
                    out += (char)(0xE0 | (cp >> 12));
                    out += (char)(0x80 | ((cp >> 6) & 0x3F));
                    out += (char)(0x80 | (cp & 0x3F));
                }
            }
            else
            {
                out += in.substr(i, e - i + 1);
            }
            i = e;
        }
        return out;
    }

    void skipMisc()
    {
        for (;;)
        {
            ws();
            if (startsWith("<?"))
            {
                size_t e = s.find("?>", p);
                p = e == std::string::npos ? s.size() : e + 2;
            }
            else if (startsWith("<!--"))
            {
                size_t e = s.find("-->", p);
                p = e == std::string::npos ? s.size() : e + 3;
            }
            else if (startsWith("<!"))
            {
                size_t e = s.find('>', p);
                p = e == std::string::npos ? s.size() : e + 1;
            }
            else
                break;
        }
    }

    std::string readName()
    {
        size_t b = p;
        while (!eof() && !std::isspace((unsigned char)s[p]) && s[p] != '>' && s[p] != '/' && s[p] != '=')
            ++p;
        return s.substr(b, p - b);
    }

    std::unique_ptr<XmlNode> element()
    {
        if (eof() || s[p] != '<')
        {
            err = "expected '<'";
            return nullptr;
        }
        ++p;
        auto n = std::make_unique<XmlNode>();
        n->name = readName();
        for (;;)
        {
            ws();
            if (eof())
            {
                err = "unexpected end inside tag <" + n->name + ">";
                return nullptr;
            }
            if (s[p] == '/')
            {
                ++p;
                if (!eof() && s[p] == '>')
                    ++p;
                return n;
            }
            if (s[p] == '>')
            {
                ++p;
                break;
            }
            std::string an = readName();
            if (an.empty())
            {
                err = "bad attribute in <" + n->name + ">";
                return nullptr;
            }
            ws();
            std::string av;
            if (!eof() && s[p] == '=')
            {
                ++p;
                ws();
                if (!eof() && (s[p] == '"' || s[p] == '\''))
                {
                    char q = s[p++];
                    size_t e = s.find(q, p);
                    if (e == std::string::npos)
                    {
                        err = "unterminated attribute value";
                        return nullptr;
                    }
                    av = unescape(s.substr(p, e - p));
                    p = e + 1;
                }
            }
            n->attrs.emplace_back(an, av);
        }
        // content
        for (;;)
        {
            if (eof())
            {
                err = "missing closing tag for <" + n->name + ">";
                return nullptr;
            }
            if (startsWith("</"))
            {
                size_t e = s.find('>', p);
                p = e == std::string::npos ? s.size() : e + 1;
                return n;
            }
            if (startsWith("<!--"))
            {
                size_t e = s.find("-->", p);
                p = e == std::string::npos ? s.size() : e + 3;
                continue;
            }
            if (startsWith("<![CDATA["))
            {
                size_t e = s.find("]]>", p);
                if (e == std::string::npos)
                    e = s.size();
                n->text += s.substr(p + 9, e - p - 9);
                p = e + 3;
                continue;
            }
            if (s[p] == '<')
            {
                auto c = element();
                if (!c)
                    return nullptr;
                n->children.push_back(std::move(c));
                continue;
            }
            size_t e = s.find('<', p);
            if (e == std::string::npos)
                e = s.size();
            n->text += unescape(s.substr(p, e - p));
            p = e;
        }
    }
};

std::string escape(const std::string& in, bool attr)
{
    std::string out;
    for (char c : in)
    {
        switch (c)
        {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"':
            if (attr)
                out += "&quot;";
            else
                out += c;
            break;
        default: out += c;
        }
    }
    return out;
}

void writeNode(std::string& out, const XmlNode& n, int depth)
{
    std::string ind((size_t)depth * 2, ' ');
    out += ind + "<" + n.name;
    for (auto& a : n.attrs)
        out += " " + a.first + "=\"" + escape(a.second, true) + "\"";
    bool hasText = !trim(n.text).empty();
    if (n.children.empty() && !hasText)
    {
        out += "/>\n";
        return;
    }
    out += ">";
    if (hasText)
        out += escape(trim(n.text), false);
    if (!n.children.empty())
    {
        out += "\n";
        for (auto& c : n.children)
            writeNode(out, *c, depth + 1);
        out += ind;
    }
    out += "</" + n.name + ">\n";
}
}  // namespace

bool XmlDoc::parse(const std::string& source)
{
    // the original data files are ISO-8859-1; transcode to UTF-8 so Axmol can render them
    std::string converted;
    const std::string* srcp = &source;
    {
        std::string head = lcase(source.substr(0, 200));
        if (head.find("iso-8859-1") != std::string::npos || head.find("windows-1252") != std::string::npos)
        {
            converted.reserve(source.size() + source.size() / 16);
            for (unsigned char c : source)
            {
                if (c < 0x80)
                    converted += (char)c;
                else
                {
                    converted += (char)(0xC0 | (c >> 6));
                    converted += (char)(0x80 | (c & 0x3F));
                }
            }
            srcp = &converted;
        }
    }
    const std::string& src = *srcp;
    Parser ps(src);
    // skip UTF-8 BOM
    if (src.size() >= 3 && (unsigned char)src[0] == 0xEF && (unsigned char)src[1] == 0xBB && (unsigned char)src[2] == 0xBF)
        ps.p = 3;
    ps.skipMisc();
    root = ps.element();
    if (!root)
        error = ps.err.empty() ? "no root element" : ps.err;
    return root != nullptr;
}

bool XmlDoc::load(const std::string& path)
{
    std::string data;
    if (!readFile(path, data))
    {
        error = "cannot open " + path;
        return false;
    }
    return parse(data);
}

std::string XmlDoc::serialize() const
{
    std::string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    if (root)
        writeNode(out, *root, 0);
    return out;
}

bool XmlDoc::save(const std::string& path) const
{
    return writeFile(path, serialize());
}

long long val(const std::string& s)
{
    size_t i = 0;
    while (i < s.size() && std::isspace((unsigned char)s[i]))
        ++i;
    if (i < s.size() && s[i] == '$')
        return std::strtoll(s.c_str() + i + 1, nullptr, 16);
    return std::strtoll(s.c_str() + i, nullptr, 10);
}

std::string str(long long v)
{
    return std::to_string(v);
}

}  // namespace pb
