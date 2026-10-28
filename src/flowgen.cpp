#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cwchar>

#include <shlwapi.h>

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#pragma comment(lib, "Shlwapi.lib")

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

typedef std::wstring str;
static const size_t npos = str::npos;

static bool isIdStart(wchar_t c) { return c == L'_' || (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c >= 0x80; }
static bool isIdChar(wchar_t c) { return isIdStart(c) || (c >= L'0' && c <= L'9'); }
static bool isSp(wchar_t c) { return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r' || c == L'\f' || c == L'\v'; }

static str trimStr(const str& s) {
    size_t a = 0, b = s.size();
    while (a < b && isSp(s[a])) a++;
    while (b > a && isSp(s[b - 1])) b--;
    return s.substr(a, b - a);
}

static str normStr(const str& s) {
    str o;
    bool sp = false;
    for (size_t i = 0; i < s.size(); i++) {
        if (isSp(s[i])) { sp = true; continue; }
        if (sp && !o.empty()) o += L' ';
        sp = false;
        o += s[i];
    }
    return o;
}
static bool startsWith(const str& s, const wchar_t* p) { return s.compare(0, wcslen(p), p) == 0; }

static size_t findWord(const str& s, const wchar_t* w, size_t from = 0) {
    size_t len = wcslen(w);
    size_t p = from;
    while ((p = s.find(w, p)) != npos) {
        bool l = (p == 0) || !isIdChar(s[p - 1]);
        bool r = (p + len >= s.size()) || !isIdChar(s[p + len]);
        if (l && r) return p;
        p += 1;
    }
    return npos;
}

static size_t skipLit(const str& s, size_t i) {
    wchar_t q = s[i];
    size_t j = i + 1, n = s.size();
    while (j < n && s[j] != q) {
        if (s[j] == L'\\') j++;
        j++;
    }
    return j < n ? j : n - 1;
}

static size_t findMatching(const str& s, size_t i, wchar_t open, wchar_t close) {
    int depth = 0;
    size_t n = s.size();
    for (size_t j = i; j < n; j++) {
        wchar_t c = s[j];
        if (c == L'"' || c == L'\'') { j = skipLit(s, j); continue; }
        if (c == open) depth++;
        else if (c == close) { depth--; if (depth == 0) return j; }
    }
    return npos;
}

static std::vector<str> splitTop(const str& s, wchar_t sep) {
    std::vector<str> parts;
    str cur;
    int depth = 0;
    for (size_t i = 0; i < s.size(); i++) {
        wchar_t c = s[i];
        if (c == L'"' || c == L'\'') {
            size_t j = skipLit(s, i);
            cur += s.substr(i, j - i + 1);
            i = j;
            continue;
        }
        if (c == L'(' || c == L'[' || c == L'{') depth++;
        else if (c == L')' || c == L']' || c == L'}') depth--;
        if (c == sep && depth == 0) { parts.push_back(cur); cur.clear(); }
        else cur += c;
    }
    parts.push_back(cur);
    return parts;
}

static str replaceAll(str s, const str& a, const str& b) {
    if (a.empty()) return s;
    size_t p = 0;
    while ((p = s.find(a, p)) != npos) { s.replace(p, a.size(), b); p += b.size(); }
    return s;
}

static str utf8ToW(const std::string& in) {
    str o;
    size_t i = 0, n = in.size();
    while (i < n) {
        unsigned char c = (unsigned char)in[i];
        uint32_t cp; int extra;
        if (c < 0x80) { cp = c; extra = 0; }
        else if ((c >> 5) == 6) { cp = c & 0x1F; extra = 1; }
        else if ((c >> 4) == 14) { cp = c & 0x0F; extra = 2; }
        else if ((c >> 3) == 30) { cp = c & 0x07; extra = 3; }
        else { cp = c; extra = 0; }
        i++;
        for (int k = 0; k < extra && i < n; k++, i++) cp = (cp << 6) | ((unsigned char)in[i] & 0x3F);
        if (sizeof(wchar_t) == 2 && cp > 0xFFFF) {
            cp -= 0x10000;
            o += (wchar_t)(0xD800 + (cp >> 10));
            o += (wchar_t)(0xDC00 + (cp & 0x3FF));
        } else o += (wchar_t)cp;
    }
    return o;
}
static std::string wToUtf8(const str& in) {
    std::string o;
    for (size_t i = 0; i < in.size(); i++) {
        uint32_t cp = (uint32_t)in[i];
        if (sizeof(wchar_t) == 2 && cp >= 0xD800 && cp < 0xDC00 && i + 1 < in.size()) {
            cp = 0x10000 + ((cp - 0xD800) << 10) + ((uint32_t)in[i + 1] - 0xDC00);
            i++;
        }
        if (cp < 0x80) o += (char)cp;
        else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
        else { o += (char)(0xF0 | (cp >> 18)); o += (char)(0x80 | ((cp >> 12) & 0x3F)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
    }
    return o;
}
static bool validUtf8(const std::string& s) {
    size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char c = (unsigned char)s[i];
        int extra = 0;
        if (c < 0x80) extra = 0;
        else if ((c >> 5) == 6) extra = 1;
        else if ((c >> 4) == 14) extra = 2;
        else if ((c >> 3) == 30) extra = 3;
        else return false;
        for (int k = 1; k <= extra; k++)
            if (i + k >= n || ((unsigned char)s[i + k] >> 6) != 2) return false;
        i += extra + 1;
    }
    return true;
}

static str decodeText(const std::string& b) {
    std::string s = b;
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) s = s.substr(3);
    if (validUtf8(s)) return utf8ToW(s);
    str o;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 0xC0) o += (wchar_t)(0x0410 + (c - 0xC0));
        else if (c == 0xA8) o += (wchar_t)0x0401;
        else if (c == 0xB8) o += (wchar_t)0x0451;
        else o += (wchar_t)c;
    }
    return o;
}

struct Pt {
    double x, y;
    Pt() : x(0), y(0) {}
    Pt(double a, double b) : x(a), y(b) {}
};
enum { K_TERM = 0, K_PROC = 1, K_IO = 2, K_DEC = 3, K_JUNC = 4 };
enum { SD_T = 0, SD_R = 1, SD_B = 2, SD_L = 3 };
static const double LANE_RIGHT = 1e9;

struct Node {
    int id; int kind; str text; double x, y, w, h;
    Node() : id(0), kind(K_PROC), x(0), y(0), w(0), h(0) {}
};
struct Edge {
    int src, dst; str label; int sout, din; bool hasLane; double lane;
    Edge() : src(0), dst(0), sout(SD_B), din(SD_T), hasLane(false), lane(0) {}
};
struct Size { double w, h; };

static double defaultMeasure(const str& s) { return s.size() * 7.4; }
static double (*g_measure)(const str&) = defaultMeasure;
static const double LH = 17;

static std::vector<str> wrapText(const str& text, double maxw) {
    std::vector<str> lines;
    size_t p = 0;
    for (;;) {
        size_t e = text.find(L'\n', p);
        str para = text.substr(p, e == npos ? npos : e - p);
        str cur;
        size_t a = 0;
        while (a <= para.size()) {
            size_t b = para.find(L' ', a);
            str word = para.substr(a, b == npos ? npos : b - a);
            a = (b == npos) ? para.size() + 1 : b + 1;
            if (word.empty()) continue;
            while (g_measure(word) > maxw && word.size() > 1) {
                size_t k = 1;
                while (k < word.size() && g_measure(word.substr(0, k + 1)) <= maxw) k++;
                if (!cur.empty()) { lines.push_back(cur); cur.clear(); }
                lines.push_back(word.substr(0, k));
                word = word.substr(k);
            }
            str cand = cur.empty() ? word : cur + L" " + word;
            if (g_measure(cand) <= maxw) cur = cand;
            else { lines.push_back(cur); cur = word; }
        }
        lines.push_back(cur);
        if (e == npos) break;
        p = e + 1;
    }
    return lines;
}
static double wrapWidth(int kind) {
    switch (kind) {
    case K_DEC: return 150;
    case K_IO: return 230;
    case K_TERM: return 260;
    default: return 240;
    }
}
static std::vector<str> wrapFor(int kind, const str& text) { return wrapText(text, wrapWidth(kind)); }

static Size calcSize(int kind, const str& text) {
    Size z;
    if (kind == K_JUNC) { z.w = 8; z.h = 8; return z; }
    std::vector<str> ln = wrapFor(kind, text);
    double tw = 0;
    for (size_t i = 0; i < ln.size(); i++) tw = std::max(tw, g_measure(ln[i]));
    double th = ln.size() * LH;
    switch (kind) {
    case K_DEC:
        z.w = std::max(120.0, tw * 1.85 + 30);
        z.h = std::max(66.0, th * 1.9 + 24);
        break;
    case K_IO:
        z.w = std::max(110.0, tw + 56);
        z.h = std::max(38.0, th + 20);
        break;
    case K_TERM:
        z.w = std::max(100.0, tw + 40);
        z.h = 40;
        break;
    default:
        z.w = std::max(90.0, tw + 28);
        z.h = std::max(38.0, th + 20);
    }
    return z;
}

struct Diagram {
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    int nextId;
    Diagram() : nextId(1) {}
    Node* find(int id) { for (size_t i = 0; i < nodes.size(); i++) if (nodes[i].id == id) return &nodes[i]; return 0; }
    const Node* find(int id) const { for (size_t i = 0; i < nodes.size(); i++) if (nodes[i].id == id) return &nodes[i]; return 0; }
    void autosize(Node& n) { Size z = calcSize(n.kind, n.text); n.w = z.w; n.h = z.h; }
    int addNode(int kind, const str& text) {
        Node n; n.id = nextId++; n.kind = kind; n.text = text;
        autosize(n);
        nodes.push_back(n);
        return n.id;
    }
    void addEdge(int s, int d, const str& lab, int so, int di, bool hl, double lane) {
        Edge e; e.src = s; e.dst = d; e.label = lab; e.sout = so; e.din = di; e.hasLane = hl; e.lane = lane;
        edges.push_back(e);
    }
    void removeNode(int id) {
        for (size_t i = edges.size(); i-- > 0;)
            if (edges[i].src == id || edges[i].dst == id) edges.erase(edges.begin() + i);
        for (size_t i = 0; i < nodes.size(); i++)
            if (nodes[i].id == id) { nodes.erase(nodes.begin() + i); break; }
    }
};

static int toInt(const str& s) { return (int)wcstol(s.c_str(), 0, 10); }

static str escText(const str& s) {
    str o;
    for (size_t i = 0; i < s.size(); i++) {
        wchar_t c = s[i];
        if (c == L'\\') o += L"\\\\";
        else if (c == L'\n') o += L"\\n";
        else if (c == L'\t') o += L"\\t";
        else if (c == L'\r') {}
        else o += c;
    }
    return o;
}
static str unescText(const str& s) {
    str o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == L'\\' && i + 1 < s.size()) {
            wchar_t n = s[++i];
            o += (n == L'n') ? L'\n' : (n == L't') ? L'\t' : n;
        } else o += s[i];
    }
    return o;
}
static str intStr(int v) {
    if (v == 0) return L"0";
    bool neg = v < 0;
    long long u = neg ? -(long long)v : (long long)v;
    str o;
    while (u > 0) { o.insert(o.begin(), (wchar_t)(L'0' + (int)(u % 10))); u /= 10; }
    return neg ? L"-" + o : o;
}
static str numStr(double v) {
    bool neg = v < 0;
    long long q = (long long)floor(fabs(v) * 1000.0 + 0.5);
    str frac = intStr((int)(q % 1000));
    while (frac.size() < 3) frac.insert(frac.begin(), L'0');
    str o = intStr((int)(q / 1000)) + L"." + frac;
    return (neg && q != 0) ? L"-" + o : o;
}

static str serializeDiagram(const Diagram& d, const str& code, const str& func) {
    str o = L"FLOWGEN\t1\n";
    for (size_t i = 0; i < d.nodes.size(); i++) {
        const Node& n = d.nodes[i];
        o += L"N\t" + intStr(n.id) + L"\t" + intStr(n.kind) + L"\t" + numStr(n.x) + L"\t" + numStr(n.y) + L"\t" + escText(n.text) + L"\n";
    }
    for (size_t i = 0; i < d.edges.size(); i++) {
        const Edge& e = d.edges[i];
        o += L"E\t" + intStr(e.src) + L"\t" + intStr(e.dst) + L"\t" + intStr(e.sout) + L"\t" + intStr(e.din) + L"\t" +
             intStr(e.hasLane ? 1 : 0) + L"\t" + numStr(e.lane) + L"\t" + escText(e.label) + L"\n";
    }
    o += L"F\t" + escText(func) + L"\n";
    o += L"C\t" + escText(code) + L"\n";
    return o;
}
static bool deserializeDiagram(const str& s, Diagram& d, str* code, str* func) {
    Diagram nd;
    size_t p = 0;
    bool head = false;
    while (p < s.size()) {
        size_t e = s.find(L'\n', p);
        str line = s.substr(p, e == npos ? npos : e - p);
        p = (e == npos) ? s.size() : e + 1;
        if (line.empty()) continue;
        std::vector<str> f;
        size_t a = 0;
        for (;;) {
            size_t b = line.find(L'\t', a);
            f.push_back(line.substr(a, b == npos ? npos : b - a));
            if (b == npos) break;
            a = b + 1;
        }
        if (f[0] == L"FLOWGEN") head = true;
        else if (f[0] == L"N" && f.size() >= 6) {
            Node n; n.id = toInt(f[1]); n.kind = toInt(f[2]);
            n.x = wcstod(f[3].c_str(), 0); n.y = wcstod(f[4].c_str(), 0); n.text = unescText(f[5]);
            nd.autosize(n);
            nd.nodes.push_back(n);
            if (n.id >= nd.nextId) nd.nextId = n.id + 1;
        } else if (f[0] == L"E" && f.size() >= 8) {
            Edge ed; ed.src = toInt(f[1]); ed.dst = toInt(f[2]);
            ed.sout = toInt(f[3]); ed.din = toInt(f[4]);
            ed.hasLane = toInt(f[5]) != 0; ed.lane = wcstod(f[6].c_str(), 0); ed.label = unescText(f[7]);
            nd.edges.push_back(ed);
        } else if (f[0] == L"F" && f.size() >= 2 && func) *func = unescText(f[1]);
        else if (f[0] == L"C" && f.size() >= 2 && code) *code = unescText(f[1]);
    }
    if (!head) return false;
    d = nd;
    return true;
}

enum { S_PROC, S_IO, S_IF, S_WHILE, S_DO, S_FOR, S_SWITCH, S_BREAK, S_CONT, S_RETURN, S_BLOCK };
struct Stmt;
typedef std::shared_ptr<Stmt> SP;
typedef std::vector<SP> List;
struct Case {
    std::vector<str> labels; bool isDefault; List body;
    Case() : isDefault(false) {}
};
struct Stmt {
    int t; str a, b, c; List body, els; std::vector<Case> cases;
    Stmt(int tt) : t(tt) {}
};
static SP newStmt(int t, const str& a = L"") { SP s(new Stmt(t)); s->a = a; return s; }

static str stripComments(const str& s) {
    str o;
    size_t i = 0, n = s.size();
    while (i < n) {
        wchar_t c = s[i];
        if (c == L'"' || c == L'\'') { size_t j = skipLit(s, i); o += s.substr(i, j - i + 1); i = j + 1; }
        else if (c == L'/' && i + 1 < n && s[i + 1] == L'/') { while (i < n && s[i] != L'\n') i++; }
        else if (c == L'/' && i + 1 < n && s[i + 1] == L'*') { size_t j = s.find(L"*/", i + 2); i = (j == npos) ? n : j + 2; o += L' '; }
        else { o += c; i++; }
    }
    return o;
}

static str preprocess(const str& src) {
    str s = stripComments(src), o;
    size_t p = 0;
    for (;;) {
        size_t e = s.find(L'\n', p);
        str line = s.substr(p, e == npos ? npos : e - p);
        str t = trimStr(line);
        bool drop = (!t.empty() && t[0] == L'#') || startsWith(t, L"import ") || startsWith(t, L"package ") || startsWith(t, L"using ");
        if (!drop) o += line;
        o += L'\n';
        if (e == npos) break;
        p = e + 1;
    }
    return o;
}

struct FuncInfo { str name; str body; };
static bool isKeywordName(const str& n) {
    static const wchar_t* kw[] = { L"if", L"for", L"while", L"switch", L"catch", L"return", L"else", L"do", L"sizeof", L"new", L"throw", L"synchronized", 0 };
    for (int i = 0; kw[i]; i++) if (n == kw[i]) return true;
    return false;
}
static void findFunctions(const str& s, std::vector<FuncInfo>& out) {
    size_t n = s.size(), i = 0, last = 0;
    while (i < n) {
        wchar_t c = s[i];
        if (c == L'"' || c == L'\'') { i = skipLit(s, i) + 1; continue; }
        if (c == L'(') {
            size_t close = findMatching(s, i, L'(', L')');
            if (close == npos) break;
            size_t j = close + 1;
            bool ok = false;
            for (;;) {
                while (j < n && isSp(s[j])) j++;
                if (j < n && s[j] == L'{') { ok = true; break; }
                size_t k = j;
                while (k < n && (isIdChar(s[k]) || s[k] == L',' || s[k] == L'.')) k++;
                str w = s.substr(j, k - j);
                if (w == L"const" || w == L"noexcept" || w == L"override" || w == L"final") { j = k; continue; }
                if (w == L"throws") { j = k; while (j < n && s[j] != L'{' && s[j] != L';') j++; continue; }
                break;
            }
            size_t e = i;
            while (e > last && isSp(s[e - 1])) e--;
            size_t b = e;
            while (b > last && (isIdChar(s[b - 1]) || s[b - 1] == L':' || s[b - 1] == L'~')) b--;
            str name = s.substr(b, e - b);
            if (ok && !name.empty() && !isKeywordName(name)) {
                size_t be = findMatching(s, j, L'{', L'}');
                if (be == npos) be = n - 1;
                FuncInfo f; f.name = name; f.body = s.substr(j + 1, be - j - 1);
                out.push_back(f);
                i = be + 1; last = i;
                continue;
            }
            i = close + 1;
            continue;
        }
        if (c == L';') { last = i + 1; i++; continue; }
        if (c == L'{') {
            size_t e = findMatching(s, i, L'{', L'}');
            if (e == npos) e = n - 1;
            str header = s.substr(last, i - last);
            if (findWord(header, L"class") != npos || findWord(header, L"struct") != npos || findWord(header, L"namespace") != npos ||
                findWord(header, L"interface") != npos || findWord(header, L"extern") != npos)
                findFunctions(s.substr(i + 1, e - i - 1), out);
            i = e + 1; last = i;
            continue;
        }
        i++;
    }
}

static bool isPureDecl(const str& t) {
    if (t.find(L'=') != npos || t.find(L'(') != npos || t.find(L"<<") != npos || t.find(L">>") != npos) return false;
    for (size_t i = 0; i < t.size(); i++) {
        wchar_t c = t[i];
        if (!(isIdChar(c) || c == L' ' || c == L',' || c == L'*' || c == L'&' || c == L'[' || c == L']' || c == L'<' || c == L'>' || c == L':' || c == L'.')) return false;
    }
    str u; int depth = 0;
    for (size_t i = 0; i < t.size(); i++) {
        wchar_t c = t[i];
        if (c == L'[' || c == L'<') depth++;
        else if (c == L']' || c == L'>') { if (depth > 0) depth--; }
        else if (depth == 0) u += c;
    }
    std::vector<str> tok; str cur;
    for (size_t i = 0; i < u.size(); i++) {
        wchar_t c = u[i];
        if (isIdChar(c) || c == L':' || c == L'.') cur += c;
        else if (!cur.empty()) { tok.push_back(cur); cur.clear(); }
    }
    if (!cur.empty()) tok.push_back(cur);
    if (tok.size() < 2) return false;
    static const wchar_t* kw[] = { L"return", L"break", L"continue", L"goto", L"throw", L"delete", L"new", L"else", L"case", L"default", L"typedef", 0 };
    for (int k = 0; kw[k]; k++) if (tok[0] == kw[k]) return false;
    return true;
}
static str joinList(const std::vector<str>& v) {
    str o;
    for (size_t i = 0; i < v.size(); i++) { if (i) o += L", "; o += v[i]; }
    return o;
}
static bool hasSystemOut(const str& t) { return t.find(L"System.out") != npos || t.find(L"System.err") != npos; }

static str outText(const str& t) {
    size_t c = findWord(t, L"cout");
    if (c == npos) c = findWord(t, L"cerr");
    if (c != npos && t.find(L"<<", c) != npos) {
        str rest = t.substr(c + 4);
        std::vector<str> parts;
        size_t a = 0;
        for (;;) {
            size_t b = rest.find(L"<<", a);
            str part = trimStr(rest.substr(a, b == npos ? npos : b - a));
            if (!part.empty() && part != L"endl" && part != L"std::endl" && part != L"\"\\n\"" && part != L"'\\n'") parts.push_back(part);
            if (b == npos) break;
            a = b + 2;
        }
        return L"Вывод: " + joinList(parts);
    }
    static const wchar_t* fn[] = { L"printf", L"fprintf", L"puts", L"fputs", L"putchar", L"println", L"print", L"format", 0 };
    size_t best = npos; str bn;
    for (int k = 0; fn[k]; k++) {
        str f = fn[k];
        if ((f == L"println" || f == L"print" || f == L"format") && !hasSystemOut(t)) continue;
        size_t p = findWord(t, fn[k]);
        if (p != npos && (best == npos || p < best)) { best = p; bn = f; }
    }
    if (best != npos) {
        size_t po = t.find(L'(', best);
        if (po != npos) {
            size_t pc = findMatching(t, po, L'(', L')');
            if (pc == npos) pc = t.size();
            str inner = t.substr(po + 1, pc - po - 1);
            std::vector<str> args = splitTop(inner, L','), use;
            for (size_t k = 0; k < args.size(); k++) {
                if (k == 0 && bn == L"fprintf") continue;
                if (k > 0 && bn == L"fputs") break;
                str a = trimStr(replaceAll(args[k], L"\\n", L""));
                if (a.empty() || a == L"\"\"") continue;
                use.push_back(a);
            }
            return L"Вывод: " + joinList(use);
        }
    }
    return L"Вывод: " + t;
}

static str inText(const str& t) {
    size_t g = findWord(t, L"getline");
    if (g != npos) {
        size_t po = t.find(L'(', g);
        if (po != npos) {
            size_t pc = findMatching(t, po, L'(', L')');
            if (pc == npos) pc = t.size();
            std::vector<str> a = splitTop(t.substr(po + 1, pc - po - 1), L',');
            if (a.size() >= 2) return L"Ввод: " + trimStr(a[1]);
        }
    }
    size_t c = findWord(t, L"cin");
    if (c != npos && t.find(L">>", c) != npos) {
        str rest = t.substr(c + 3);
        std::vector<str> parts;
        size_t a = 0;
        for (;;) {
            size_t b = rest.find(L">>", a);
            str part = trimStr(rest.substr(a, b == npos ? npos : b - a));
            if (!part.empty()) parts.push_back(part);
            if (b == npos) break;
            a = b + 2;
        }
        return L"Ввод: " + joinList(parts);
    }
    static const wchar_t* fn[] = { L"scanf", L"scanf_s", L"fscanf", L"fgets", L"gets", L"gets_s", 0 };
    for (int k = 0; fn[k]; k++) {
        size_t p = findWord(t, fn[k]);
        if (p == npos) continue;
        size_t po = t.find(L'(', p);
        if (po == npos) continue;
        size_t pc = findMatching(t, po, L'(', L')');
        if (pc == npos) pc = t.size();
        std::vector<str> args = splitTop(t.substr(po + 1, pc - po - 1), L','), names;
        str f = fn[k];
        size_t skip = (f == L"fscanf") ? 2 : (f == L"scanf" || f == L"scanf_s") ? 1 : 0;
        size_t take = (f == L"fgets" || f == L"gets" || f == L"gets_s") ? 1 : args.size();
        for (size_t q = skip; q < args.size() && q < skip + take; q++) {
            str a = trimStr(args[q]);
            while (!a.empty() && a[0] == L'&') a = trimStr(a.substr(1));
            if (!a.empty()) names.push_back(a);
        }
        return L"Ввод: " + joinList(names);
    }

    size_t eq = npos;
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] == L'=' && (i + 1 >= t.size() || t[i + 1] != L'=') && (i == 0 || (t[i - 1] != L'=' && t[i - 1] != L'!' && t[i - 1] != L'<' && t[i - 1] != L'>'))) { eq = i; break; }
    }
    if (eq != npos) {
        str lhs = trimStr(t.substr(0, eq));
        size_t sp = lhs.find_last_of(L" \t*&");
        str name = (sp == npos) ? lhs : lhs.substr(sp + 1);
        if (!name.empty()) return L"Ввод: " + name;
    }
    return L"Ввод: " + t;
}

static SP makeSimple(const str& raw, bool hide) {
    str t = normStr(raw);
    if (t.empty()) return SP();
    if (hide && isPureDecl(t)) return SP();
    static const wchar_t* ins[] = { L"scanf", L"scanf_s", L"fscanf", L"cin", L"gets", L"gets_s", L"fgets", L"getline", L"getchar", L"readLine",
                                    L"nextInt", L"nextDouble", L"nextLine", L"nextFloat", L"nextLong", L"nextBoolean", L"nextShort", L"nextByte", 0 };
    static const wchar_t* outs[] = { L"printf", L"fprintf", L"puts", L"fputs", L"putchar", L"cout", L"cerr", 0 };
    size_t pin = npos, pout = npos;
    for (int k = 0; ins[k]; k++) { size_t p = findWord(t, ins[k]); if (p != npos && (pin == npos || p < pin)) pin = p; }
    for (int k = 0; outs[k]; k++) { size_t p = findWord(t, outs[k]); if (p != npos && (pout == npos || p < pout)) pout = p; }
    if (hasSystemOut(t)) {
        static const wchar_t* jo[] = { L"println", L"print", L"format", 0 };
        for (int k = 0; jo[k]; k++) { size_t p = findWord(t, jo[k]); if (p != npos && (pout == npos || p < pout)) pout = p; }
    }
    SP s;
    if (pin != npos && (pout == npos || pin < pout)) s = newStmt(S_IO, inText(t));
    else if (pout != npos) s = newStmt(S_IO, outText(t));
    else s = newStmt(S_PROC, t);
    return s;
}

struct Parser {
    str s; size_t i; bool hide;
    Parser(const str& src, bool hd) : s(src), i(0), hide(hd) {}
    bool eof() const { return i >= s.size(); }
    void ws() { while (i < s.size() && isSp(s[i])) i++; }
    str word() {
        size_t k = i;
        if (k < s.size() && isIdStart(s[k])) while (k < s.size() && isIdChar(s[k])) k++;
        return s.substr(i, k - i);
    }
    str paren() {
        ws();
        if (i >= s.size() || s[i] != L'(') return L"";
        size_t j = findMatching(s, i, L'(', L')');
        if (j == npos) { str r = s.substr(i + 1); i = s.size(); return r; }
        str r = s.substr(i + 1, j - i - 1);
        i = j + 1;
        return r;
    }

    str stmtText() {
        size_t n = s.size(), st = i;
        int depth = 0;
        while (i < n) {
            wchar_t c = s[i];
            if (c == L'"' || c == L'\'') { i = skipLit(s, i) + 1; continue; }
            if (c == L'(' || c == L'[' || c == L'{') depth++;
            else if (c == L')' || c == L']' || c == L'}') { if (depth == 0) break; depth--; }
            else if (c == L';' && depth == 0) { str r = s.substr(st, i - st); i++; return r; }
            i++;
        }
        if (i > n) i = n;
        return s.substr(st, i - st);
    }
    str readUntilColon() {
        size_t n = s.size(), st = i;
        while (i < n) {
            wchar_t c = s[i];
            if (c == L'"' || c == L'\'') { i = skipLit(s, i) + 1; continue; }
            if (c == L':') { if (i + 1 < n && s[i + 1] == L':') { i += 2; continue; } break; }
            i++;
        }
        str t = normStr(s.substr(st, std::min(i, n) - st));
        if (i < n) i++;
        return t;
    }
    void parseList(List& out) {
        for (;;) {
            ws();
            if (eof() || s[i] == L'}') break;
            size_t b = i;
            parseStmt(out);
            if (i == b) i++;
        }
    }
    void parseAll(List& out) {
        for (;;) { parseList(out); if (eof()) break; i++; }
    }
    void parseStmt(List& out) {
        ws();
        if (eof()) return;
        wchar_t c = s[i];
        if (c == L'{') {
            i++;
            parseList(out);
            ws();
            if (i < s.size() && s[i] == L'}') i++;
            return;
        }
        if (c == L';') { i++; return; }
        str w = word();
        if (w == L"if") {
            i += 2;
            SP st = newStmt(S_IF, normStr(paren()));
            parseStmt(st->body);
            size_t save = i;
            ws();
            if (word() == L"else") { i += 4; parseStmt(st->els); }
            else i = save;
            out.push_back(st);
        } else if (w == L"while") {
            i += 5;
            SP st = newStmt(S_WHILE, normStr(paren()));
            parseStmt(st->body);
            out.push_back(st);
        } else if (w == L"do") {
            i += 2;
            SP st = newStmt(S_DO);
            parseStmt(st->body);
            ws();
            if (word() == L"while") { i += 5; st->a = normStr(paren()); }
            ws();
            if (i < s.size() && s[i] == L';') i++;
            out.push_back(st);
        } else if (w == L"for") {
            i += 3;
            str h = paren();
            SP st = newStmt(S_FOR);
            std::vector<str> parts = splitTop(h, L';');
            if (parts.size() == 3) { st->a = normStr(parts[0]); st->b = normStr(parts[1]); st->c = normStr(parts[2]); }
            else st->b = normStr(h);
            parseStmt(st->body);
            out.push_back(st);
        } else if (w == L"switch") {
            i += 6;
            SP st = newStmt(S_SWITCH, normStr(paren()));
            ws();
            if (i < s.size() && s[i] == L'{') i++;
            Case cur; bool hasLabels = false;
            for (;;) {
                ws();
                if (eof()) break;
                if (s[i] == L'}') { i++; break; }
                str cw = word();
                if (cw == L"case") {
                    i += 4;
                    str l = readUntilColon();
                    if (!cur.body.empty()) { st->cases.push_back(cur); cur = Case(); hasLabels = false; }
                    cur.labels.push_back(l); hasLabels = true;
                } else if (cw == L"default") {
                    i += 7;
                    ws();
                    if (i < s.size() && s[i] == L':') i++;
                    if (!cur.body.empty()) { st->cases.push_back(cur); cur = Case(); hasLabels = false; }
                    cur.isDefault = true; hasLabels = true;
                } else {
                    size_t b = i;
                    parseStmt(cur.body);
                    if (i == b) i++;
                }
            }
            if (hasLabels) st->cases.push_back(cur);
            out.push_back(st);
        } else if (w == L"try") {
            i += 3;
            ws();
            if (i < s.size() && s[i] == L'(') paren();
            parseStmt(out);
            for (;;) {
                size_t save = i;
                ws();
                str k = word();
                if (k == L"catch") { i += 5; paren(); List tmp; parseStmt(tmp); }
                else if (k == L"finally") { i += 7; parseStmt(out); }
                else { i = save; break; }
            }
        } else if (w == L"return") {
            i += 6;
            str e = normStr(stmtText());
            out.push_back(newStmt(S_RETURN, e.empty() ? L"" : L"return " + e));
        } else if (w == L"break") { i += 5; stmtText(); out.push_back(newStmt(S_BREAK)); }
        else if (w == L"continue") { i += 8; stmtText(); out.push_back(newStmt(S_CONT)); }
        else if (w == L"else") { i += 4; }
        else {
            str t = stmtText();
            SP x = makeSimple(t, hide);
            if (x) out.push_back(x);
        }
    }
};

struct Pend { int n; int side; str lab; bool hl; double lane; };
static Pend mkP(int n, int side, const str& lab, bool hl = false, double lane = 0) {
    Pend p; p.n = n; p.side = side; p.lab = lab; p.hl = hl; p.lane = lane; return p;
}
typedef std::vector<Pend> PList;
struct Ext {
    double L, R, H;
    Ext() : L(0), R(0), H(0) {}
    Ext(double l, double r, double h) : L(l), R(r), H(h) {}
};
struct Res {
    int entry; PList exits; double bottom;
    Res() : entry(0), bottom(0) {}
};
struct Ctx {
    bool loop; PList breaks, conts;
    Ctx(bool l) : loop(l) {}
};
static const double GAP_Y = 36, GAP_X = 46, LANEW = 36, LANE_OFF = 8;

static void appendList(PList& a, const PList& b) { a.insert(a.end(), b.begin(), b.end()); }

struct Layouter {
    Diagram d;
    std::vector<Ctx> ctx;
    PList returns;

    int mkNode(int kind, const str& text, double cx, double ytop) {
        int id = d.addNode(kind, text);
        Node* n = d.find(id);
        n->x = cx; n->y = ytop + n->h / 2;
        return id;
    }
    void connect(const PList& pend, int entry) {
        if (!entry) return;
        for (size_t i = 0; i < pend.size(); i++)
            d.addEdge(pend[i].n, entry, pend[i].lab, pend[i].side, SD_T, pend[i].hl, pend[i].lane);
    }
    static str condOf(const Stmt& s) { return s.b.empty() ? L"true" : s.b; }

    Ext mList(const List& l) {
        Ext e; bool first = true;
        for (size_t i = 0; i < l.size(); i++) {
            Ext x = mStmt(*l[i]);
            if (x.H <= 0) continue;
            e.L = std::max(e.L, x.L); e.R = std::max(e.R, x.R);
            e.H += x.H + (first ? 0 : GAP_Y);
            first = false;
        }
        return e;
    }
    bool rewriteIf(const Stmt& s, List& out) {
        if (!s.els.empty() && !s.body.empty() && mList(s.body).H <= 0) {
            SP n = newStmt(S_IF, s.a); n->body = s.body;
            out.push_back(n);
            out.insert(out.end(), s.els.begin(), s.els.end());
            return true;
        }
        return false;
    }
    SP swToIf(const Stmt& s) {
        struct It { str cond; List body; };
        std::vector<It> items;
        List defBody;
        for (size_t i = 0; i < s.cases.size(); i++) {
            const Case& c = s.cases[i];
            if (c.isDefault) { defBody = c.body; continue; }
            str cond;
            for (size_t k = 0; k < c.labels.size(); k++) { if (k) cond += L" или "; cond += s.a + L" == " + c.labels[k]; }
            It it; it.cond = cond; it.body = c.body;
            items.push_back(it);
        }
        if (items.empty()) { SP b = newStmt(S_BLOCK); b->body = defBody; return b; }
        SP node;
        List tail = defBody;
        for (size_t k = items.size(); k-- > 0;) {
            node = newStmt(S_IF, items[k].cond);
            node->body = items[k].body; node->els = tail;
            tail.clear(); tail.push_back(node);
        }
        return node;
    }
    Ext mIf(const Stmt& s) {
        List rw;
        if (rewriteIf(s, rw)) return mList(rw);
        Size D = calcSize(K_DEC, s.a);
        Ext t = mList(s.body);
        bool hasElse = !s.els.empty();
        if (s.body.empty() && !hasElse) return Ext(D.w / 2, D.w / 2, D.h);
        if (hasElse) {
            Ext e = mList(s.els);
            if (t.H <= 0 && e.H <= 0) return Ext(D.w / 2, D.w / 2, D.h);
            double sp = std::max(t.R + e.L + GAP_X, D.w + 30);
            return Ext(std::max(D.w / 2, sp / 2 + t.L), std::max(D.w / 2, sp / 2 + e.R), D.h + GAP_Y + std::max(t.H, e.H));
        }
        return Ext(std::max(D.w / 2, t.L), std::max(D.w / 2, t.R) + LANEW, D.h + (t.H > 0 ? GAP_Y + t.H : 0));
    }
    Ext mStmt(const Stmt& s) {
        switch (s.t) {
        case S_PROC: case S_IO: { Size z = calcSize(s.t == S_PROC ? K_PROC : K_IO, s.a); return Ext(z.w / 2, z.w / 2, z.h); }
        case S_RETURN: { if (s.a.empty()) return Ext(); Size z = calcSize(K_PROC, s.a); return Ext(z.w / 2, z.w / 2, z.h); }
        case S_BREAK: case S_CONT: return Ext();
        case S_BLOCK: return mList(s.body);
        case S_IF: return mIf(s);
        case S_WHILE: {
            Size D = calcSize(K_DEC, s.a.empty() ? L"true" : s.a);
            Ext b = mList(s.body);
            return Ext(std::max(D.w / 2, b.L) + LANEW, std::max(D.w / 2, b.R) + LANEW,
                       D.h + (b.H > 0 ? GAP_Y + b.H + GAP_Y * 0.5 + 4 : 0));
        }
        case S_DO: {
            Size D = calcSize(K_DEC, s.a);
            Ext b = mList(s.body);
            return Ext(std::max(b.L, D.w / 2), std::max(b.R, D.w / 2) + LANEW, (b.H > 0 ? b.H + GAP_Y : 0) + D.h);
        }
        case S_FOR: {
            Size D = calcSize(K_DEC, condOf(s));
            Ext b = mList(s.body);
            double wI = 0, hI = 0, wU = 0, hU = 0;
            if (!s.a.empty()) { Size z = calcSize(K_PROC, s.a); wI = z.w; hI = z.h; }
            if (!s.c.empty()) { Size z = calcSize(K_PROC, s.c); wU = z.w; hU = z.h; }
            double L = std::max(std::max(D.w / 2, b.L), wU / 2) + LANEW;
            double R = std::max(std::max(D.w / 2, b.R), wU / 2) + LANEW;
            double H = (hI > 0 ? hI + GAP_Y : 0) + D.h + (b.H > 0 ? GAP_Y + b.H : 0) + (hU > 0 ? GAP_Y + hU : GAP_Y * 0.5 + 4);
            return Ext(std::max(L, wI / 2), std::max(R, wI / 2), H);
        }
        case S_SWITCH: { SP n = swToIf(s); return mStmt(*n); }
        }
        return Ext();
    }

    void doBreak(const PList& pend) { if (!ctx.empty()) appendList(ctx.back().breaks, pend); }
    void doCont(const PList& pend) {
        for (size_t k = ctx.size(); k-- > 0;)
            if (ctx[k].loop) { appendList(ctx[k].conts, pend); return; }
    }
    Res placeList(const List& stmts, double cx, double y, PList pending) {
        Res r; r.bottom = y;
        bool any = false;
        for (size_t i = 0; i < stmts.size(); i++) {
            const Stmt& st = *stmts[i];
            if (st.t == S_BREAK) { doBreak(pending); pending.clear(); continue; }
            if (st.t == S_CONT) { doCont(pending); pending.clear(); continue; }
            if (st.t == S_RETURN && st.a.empty()) {
                for (size_t k = 0; k < pending.size(); k++) { Pend p = pending[k]; p.hl = true; p.lane = LANE_RIGHT; returns.push_back(p); }
                pending.clear();
                continue;
            }
            if (mStmt(st).H <= 0) continue;
            double yt = any ? r.bottom + GAP_Y : y;
            Res x = placeStmt(st, cx, yt);
            if (x.entry == 0) continue;
            connect(pending, x.entry);
            if (r.entry == 0) r.entry = x.entry;
            pending = x.exits;
            r.bottom = x.bottom;
            any = true;
        }
        r.exits = pending;
        return r;
    }
    double backEdge(const PList& back, int Did, double laneL, double bottom) {
        if (back.empty()) return bottom;
        if (back.size() == 1 && back[0].side == SD_B && !back[0].hl && d.find(back[0].n)->kind != K_DEC) {
            d.addEdge(back[0].n, Did, back[0].lab, SD_L, SD_L, true, laneL);
            return bottom;
        }
        int J = mkNode(K_JUNC, L"", d.find(Did)->x, bottom + GAP_Y * 0.4);
        connect(back, J);
        d.addEdge(J, Did, L"", SD_L, SD_L, true, laneL);
        return d.find(J)->y + 4;
    }
    PList breaksWithLane(const PList& b, double lane) {
        PList o;
        for (size_t i = 0; i < b.size(); i++) { Pend p = b[i]; p.hl = true; p.lane = lane; o.push_back(p); }
        return o;
    }
    Res placeIf(const Stmt& s, double cx, double y) {
        List rw;
        if (rewriteIf(s, rw)) return placeList(rw, cx, y, PList());
        Size D = calcSize(K_DEC, s.a);
        int Did = mkNode(K_DEC, s.a, cx, y);
        Res out; out.entry = Did; out.bottom = y + D.h;
        double ytop = y + D.h + GAP_Y;
        Ext t = mList(s.body);
        bool hasElse = !s.els.empty();
        if (s.body.empty() && !hasElse) { out.exits.push_back(mkP(Did, SD_B, L"")); return out; }
        if (hasElse) {
            Ext e = mList(s.els);
            double sp = std::max(t.R + e.L + GAP_X, D.w + 30);
            if (!s.body.empty()) {
                Res r = placeList(s.body, cx - sp / 2, ytop, PList(1, mkP(Did, SD_L, L"да")));
                appendList(out.exits, r.exits);
                if (t.H > 0) out.bottom = std::max(out.bottom, r.bottom);
            } else out.exits.push_back(mkP(Did, SD_L, L"да"));
            Res r2 = placeList(s.els, cx + sp / 2, ytop, PList(1, mkP(Did, SD_R, L"нет")));
            appendList(out.exits, r2.exits);
            if (e.H > 0) out.bottom = std::max(out.bottom, r2.bottom);
            return out;
        }
        if (t.H > 0) {
            Res r = placeList(s.body, cx, ytop, PList(1, mkP(Did, SD_B, L"да")));
            appendList(out.exits, r.exits);
            out.bottom = std::max(out.bottom, r.bottom);
            double lane = cx + std::max(D.w / 2, t.R) + LANEW * 0.5;
            out.exits.push_back(mkP(Did, SD_R, L"нет", true, lane));
            return out;
        }

        placeList(s.body, cx, ytop, PList(1, mkP(Did, SD_R, L"да")));
        out.exits.push_back(mkP(Did, SD_B, L"нет"));
        return out;
    }
    Res placeWhile(const Stmt& s, double cx, double y) {
        str cond = s.a.empty() ? L"true" : s.a;
        Size D = calcSize(K_DEC, cond);
        Ext b = mList(s.body);
        double Lt = std::max(D.w / 2, b.L) + LANEW, Rt = std::max(D.w / 2, b.R) + LANEW;
        double laneL = cx - Lt + LANEW / 2, laneR = cx + Rt - LANEW / 2;
        int Did = mkNode(K_DEC, cond, cx, y);
        Res out; out.entry = Did; out.bottom = y + D.h;
        PList back;
        ctx.push_back(Ctx(true));
        if (!s.body.empty()) {
            Res r = placeList(s.body, cx, y + D.h + GAP_Y, PList(1, mkP(Did, SD_B, L"да")));
            if (r.entry) { out.bottom = std::max(out.bottom, r.bottom); back = r.exits; }
            else back = r.exits;
        }
        Ctx c = ctx.back(); ctx.pop_back();
        appendList(back, breaksWithLane(c.conts, laneR - LANE_OFF));
        out.bottom = std::max(out.bottom, backEdge(back, Did, laneL, out.bottom));
        out.exits.push_back(mkP(Did, SD_R, L"нет", true, laneR + LANE_OFF));
        appendList(out.exits, breaksWithLane(c.breaks, laneR + LANE_OFF));
        return out;
    }
    Res placeFor(const Stmt& s, double cx, double y) {
        str cond = condOf(s);
        Size D = calcSize(K_DEC, cond);
        Ext b = mList(s.body);
        double wU = 0;
        if (!s.c.empty()) wU = calcSize(K_PROC, s.c).w;
        double Lt = std::max(std::max(D.w / 2, b.L), wU / 2) + LANEW;
        double Rt = std::max(std::max(D.w / 2, b.R), wU / 2) + LANEW;
        double laneL = cx - Lt + LANEW / 2, laneR = cx + Rt - LANEW / 2;
        Res out;
        int Iid = 0;
        double yd = y;
        if (!s.a.empty()) {
            Iid = mkNode(K_PROC, s.a, cx, y);
            yd = y + d.find(Iid)->h + GAP_Y;
        }
        int Did = mkNode(K_DEC, cond, cx, yd);
        if (Iid) connect(PList(1, mkP(Iid, SD_B, L"")), Did);
        out.entry = Iid ? Iid : Did;
        out.bottom = yd + D.h;
        PList back;
        ctx.push_back(Ctx(true));
        if (!s.body.empty()) {
            Res r = placeList(s.body, cx, yd + D.h + GAP_Y, PList(1, mkP(Did, SD_B, L"да")));
            if (r.entry) out.bottom = std::max(out.bottom, r.bottom);
            back = r.exits;
        }
        Ctx c = ctx.back(); ctx.pop_back();
        appendList(back, breaksWithLane(c.conts, laneR - LANE_OFF));
        if (!s.c.empty() && !back.empty()) {
            int Uid = mkNode(K_PROC, s.c, cx, out.bottom + GAP_Y);
            connect(back, Uid);
            const Node* U = d.find(Uid);
            out.bottom = U->y + U->h / 2;
            d.addEdge(Uid, Did, L"", SD_L, SD_L, true, laneL);
        } else {
            out.bottom = std::max(out.bottom, backEdge(back, Did, laneL, out.bottom));
        }
        out.exits.push_back(mkP(Did, SD_R, L"нет", true, laneR + LANE_OFF));
        appendList(out.exits, breaksWithLane(c.breaks, laneR + LANE_OFF));
        return out;
    }
    Res placeDo(const Stmt& s, double cx, double y) {
        Size D = calcSize(K_DEC, s.a);
        Ext b = mList(s.body);
        double Rt = std::max(D.w / 2, b.R) + LANEW;
        double laneR = cx + Rt - LANEW / 2;
        ctx.push_back(Ctx(true));
        Res r = placeList(s.body, cx, y, PList());
        Ctx c = ctx.back(); ctx.pop_back();
        double yd = r.entry ? r.bottom + GAP_Y : y;
        int Did = mkNode(K_DEC, s.a, cx, yd);
        PList in = r.exits;
        appendList(in, breaksWithLane(c.conts, laneR - LANE_OFF));
        connect(in, Did);
        if (r.entry) d.addEdge(Did, r.entry, L"да", SD_R, SD_T, true, laneR);
        Res out;
        out.entry = r.entry ? r.entry : Did;
        out.exits.push_back(mkP(Did, SD_B, L"нет"));
        appendList(out.exits, breaksWithLane(c.breaks, laneR + LANE_OFF));
        out.bottom = yd + D.h;
        return out;
    }
    Res placeStmt(const Stmt& s, double cx, double y) {
        Res r;
        switch (s.t) {
        case S_PROC: case S_IO: {
            int n = mkNode(s.t == S_PROC ? K_PROC : K_IO, s.a, cx, y);
            r.entry = n; r.exits.push_back(mkP(n, SD_B, L"")); r.bottom = y + d.find(n)->h;
            return r;
        }
        case S_RETURN: {
            int n = mkNode(K_PROC, s.a, cx, y);
            r.entry = n; r.bottom = y + d.find(n)->h;
            returns.push_back(mkP(n, SD_R, L"", true, LANE_RIGHT));
            return r;
        }
        case S_BLOCK: return placeList(s.body, cx, y, PList());
        case S_IF: return placeIf(s, cx, y);
        case S_WHILE: return placeWhile(s, cx, y);
        case S_FOR: return placeFor(s, cx, y);
        case S_DO: return placeDo(s, cx, y);
        case S_SWITCH: {
            ctx.push_back(Ctx(false));
            SP n = swToIf(s);
            r = placeStmt(*n, cx, y);
            Ctx c = ctx.back(); ctx.pop_back();
            appendList(r.exits, c.breaks);
            return r;
        }
        }
        return r;
    }

    Diagram build(List stmts) {

        if (!stmts.empty() && stmts.back()->t == S_RETURN) {
            SP r = stmts.back();
            if (r->a.empty()) stmts.pop_back();
            else stmts.back() = newStmt(S_PROC, r->a);
        }
        int S = mkNode(K_TERM, L"Начало", 0, 0);
        double sh = d.find(S)->h;
        Res r = placeList(stmts, 0, sh + GAP_Y, PList(1, mkP(S, SD_B, L"")));
        int E = mkNode(K_TERM, L"Конец", 0, r.bottom + GAP_Y);
        PList fin = r.exits;
        appendList(fin, returns);
        connect(fin, E);
        double right = -1e18;
        for (size_t i = 0; i < d.nodes.size(); i++) right = std::max(right, d.nodes[i].x + d.nodes[i].w / 2);
        right += 40;
        for (size_t i = 0; i < d.edges.size(); i++)
            if (d.edges[i].hasLane && d.edges[i].lane >= LANE_RIGHT / 2) d.edges[i].lane = right;
        return d;
    }
};

static const double PI_ = 3.14159265358979323846;

static std::vector<Pt> nodePoly(const Node& n) {
    std::vector<Pt> p;
    double x = n.x, y = n.y, w = n.w, h = n.h;
    switch (n.kind) {
    case K_DEC:
        p.push_back(Pt(x, y - h / 2)); p.push_back(Pt(x + w / 2, y)); p.push_back(Pt(x, y + h / 2)); p.push_back(Pt(x - w / 2, y));
        break;
    case K_IO: {
        double k = 14;
        p.push_back(Pt(x - w / 2 + k, y - h / 2)); p.push_back(Pt(x + w / 2, y - h / 2));
        p.push_back(Pt(x + w / 2 - k, y + h / 2)); p.push_back(Pt(x - w / 2, y + h / 2));
        break;
    }
    case K_TERM: {
        double r = h / 2;
        for (int a = -90; a <= 90; a += 15) { double t = a * PI_ / 180; p.push_back(Pt(x + w / 2 - r + r * cos(t), y + r * sin(t))); }
        for (int a = 90; a <= 270; a += 15) { double t = a * PI_ / 180; p.push_back(Pt(x - w / 2 + r + r * cos(t), y + r * sin(t))); }
        break;
    }
    default:
        p.push_back(Pt(x - w / 2, y - h / 2)); p.push_back(Pt(x + w / 2, y - h / 2));
        p.push_back(Pt(x + w / 2, y + h / 2)); p.push_back(Pt(x - w / 2, y + h / 2));
    }
    return p;
}
static Pt portOf(const Node& n, int side) {
    double k = (n.kind == K_IO) ? 7 : 0;
    switch (side) {
    case SD_T: return Pt(n.x, n.y - n.h / 2);
    case SD_B: return Pt(n.x, n.y + n.h / 2);
    case SD_L: return Pt(n.x - n.w / 2 + k, n.y);
    default: return Pt(n.x + n.w / 2 - k, n.y);
    }
}

static std::vector<Pt> routeEdge(const Diagram& d, const Edge& e) {
    std::vector<Pt> pts;
    const Node* s = d.find(e.src);
    const Node* t = d.find(e.dst);
    if (!s || !t) return pts;
    Pt p0 = portOf(*s, e.sout), p1 = portOf(*t, e.din);
    const double m = 16;
    int so = e.sout, di = e.din;
    bool hl = e.hasLane;
    double lane = e.lane;
    if (di == SD_T) {
        double yt = p1.y - m;
        if (so == SD_B) {
            if (!hl) {
                if (fabs(p0.x - p1.x) < 0.5 && p1.y > p0.y) { pts.push_back(p0); pts.push_back(p1); }
                else if (p1.y > p0.y + 6) {
                    double ym = (p1.y - p0.y > 2 * m) ? yt : (p0.y + p1.y) / 2;
                    pts.push_back(p0); pts.push_back(Pt(p0.x, ym)); pts.push_back(Pt(p1.x, ym)); pts.push_back(p1);
                } else { hl = true; lane = std::max(s->x + s->w / 2, t->x + t->w / 2) + 40; }
            }
            if (hl) {
                pts.clear();
                pts.push_back(p0); pts.push_back(Pt(p0.x, p0.y + m)); pts.push_back(Pt(lane, p0.y + m));
                pts.push_back(Pt(lane, yt)); pts.push_back(Pt(p1.x, yt)); pts.push_back(p1);
            }
        } else if (so == SD_L || so == SD_R) {
            double sg = (so == SD_L) ? -1 : 1;
            if (!hl && (p1.x - p0.x) * sg > 0 && p1.y > p0.y + m) {
                pts.push_back(p0); pts.push_back(Pt(p1.x, p0.y)); pts.push_back(p1);
            } else {
                if (!hl) lane = p0.x + sg * 30;
                pts.push_back(p0); pts.push_back(Pt(lane, p0.y)); pts.push_back(Pt(lane, yt)); pts.push_back(Pt(p1.x, yt)); pts.push_back(p1);
            }
        } else {
            double ym = (p0.y + p1.y) / 2;
            pts.push_back(p0); pts.push_back(Pt(p0.x, ym)); pts.push_back(Pt(p1.x, ym)); pts.push_back(p1);
        }
    } else if (di == SD_L || di == SD_R) {
        double dg = (di == SD_L) ? -1 : 1;
        if (!hl) lane = p1.x + dg * 30;
        double sx = 0, sy = 0;
        switch (so) { case SD_B: sy = m; break; case SD_T: sy = -m; break; case SD_L: sx = -m; break; default: sx = m; }
        Pt o1(p0.x + sx, p0.y + sy);
        pts.push_back(p0); pts.push_back(o1); pts.push_back(Pt(lane, o1.y)); pts.push_back(Pt(lane, p1.y)); pts.push_back(p1);
    } else {
        double ym = (p0.y + p1.y) / 2;
        pts.push_back(p0); pts.push_back(Pt(p0.x, ym)); pts.push_back(Pt(p1.x, ym)); pts.push_back(p1);
    }
    std::vector<Pt> r;
    for (size_t i = 0; i < pts.size(); i++)
        if (r.empty() || fabs(r.back().x - pts[i].x) > 0.01 || fabs(r.back().y - pts[i].y) > 0.01) r.push_back(pts[i]);
    if (r.size() < 2) { r.clear(); r.push_back(p0); r.push_back(Pt(p0.x, p0.y + 1)); }
    return r;
}
static std::vector<Pt> arrowHead(const std::vector<Pt>& pts) {
    std::vector<Pt> a;
    if (pts.size() < 2) return a;
    Pt p = pts[pts.size() - 2], q = pts[pts.size() - 1];
    double dx = q.x - p.x, dy = q.y - p.y, L = sqrt(dx * dx + dy * dy);
    if (L < 1e-9) L = 1;
    double ux = dx / L, uy = dy / L, len = 10, wd = 4.5;
    double bx = q.x - ux * len, by = q.y - uy * len;
    a.push_back(q); a.push_back(Pt(bx - uy * wd, by + ux * wd)); a.push_back(Pt(bx + uy * wd, by - ux * wd));
    return a;
}
static Pt labelPos(const Edge& e, const std::vector<Pt>& pts) {
    Pt p = pts[0];
    switch (e.sout) {
    case SD_B: return Pt(p.x + 15, p.y + 11);
    case SD_T: return Pt(p.x + 15, p.y - 11);
    case SD_L: return Pt(p.x - 15, p.y - 10);
    default: return Pt(p.x + 15, p.y - 10);
    }
}

enum { P_LINE, P_POLY, P_TEXT, P_DOT };
struct Prim {
    int type; std::vector<Pt> pts; unsigned fill, line; double w; str text; Pt at;
    Prim() : type(P_LINE), fill(0xFFFFFF), line(0), w(1.4) {}
};
static const unsigned COL_BLACK = 0x000000, COL_WHITE = 0xFFFFFF, COL_TERM = 0xE6E6E6, COL_SEL = 0x1A73E8, COL_CONN = 0xE67E00;

static std::vector<Prim> buildScene(const Diagram& d, int selType, int selId, int connSrc) {
    std::vector<Prim> out;
    for (size_t i = 0; i < d.edges.size(); i++) {
        const Edge& e = d.edges[i];
        std::vector<Pt> pts = routeEdge(d, e);
        if (pts.size() < 2) continue;
        bool sel = (selType == 2 && selId == (int)i);
        unsigned col = sel ? COL_SEL : COL_BLACK;
        Prim l; l.type = P_LINE; l.pts = pts; l.line = col; l.w = sel ? 2.4 : 1.4;
        out.push_back(l);
        const Node* dn = d.find(e.dst);
        if (dn && dn->kind != K_JUNC) {
            Prim a; a.type = P_POLY; a.pts = arrowHead(pts); a.fill = col; a.line = col; a.w = 1;
            out.push_back(a);
        }
        if (!e.label.empty()) {
            Prim t; t.type = P_TEXT; t.text = e.label; t.at = labelPos(e, pts); t.line = sel ? COL_SEL : 0x202020;
            out.push_back(t);
        }
    }
    for (size_t i = 0; i < d.nodes.size(); i++) {
        const Node& n = d.nodes[i];
        bool sel = (selType == 1 && selId == n.id);
        if (n.kind == K_JUNC) {
            Prim p; p.type = P_DOT; p.at = Pt(n.x, n.y); p.w = 4; p.line = sel ? COL_SEL : COL_BLACK;
            out.push_back(p);
            continue;
        }
        Prim p; p.type = P_POLY; p.pts = nodePoly(n);
        p.fill = (n.kind == K_TERM) ? COL_TERM : COL_WHITE;
        p.line = (n.id == connSrc) ? COL_CONN : (sel ? COL_SEL : COL_BLACK);
        p.w = (sel || n.id == connSrc) ? 2.4 : 1.4;
        out.push_back(p);
        std::vector<str> ln = wrapFor(n.kind, n.text);
        double y0 = n.y - (ln.size() - 1) * LH / 2;
        for (size_t k = 0; k < ln.size(); k++) {
            Prim t; t.type = P_TEXT; t.text = ln[k]; t.at = Pt(n.x, y0 + k * LH); t.line = COL_BLACK;
            out.push_back(t);
        }
    }
    return out;
}

struct Bounds { double x0, y0, x1, y1; };
static Bounds sceneBounds(const std::vector<Prim>& pr) {
    Bounds b; b.x0 = b.y0 = 1e18; b.x1 = b.y1 = -1e18;
    for (size_t i = 0; i < pr.size(); i++) {
        const Prim& p = pr[i];
        if (p.type == P_TEXT || p.type == P_DOT) {
            double r = (p.type == P_DOT) ? p.w : 4;
            b.x0 = std::min(b.x0, p.at.x - r); b.x1 = std::max(b.x1, p.at.x + r);
            b.y0 = std::min(b.y0, p.at.y - r); b.y1 = std::max(b.y1, p.at.y + r);
        } else for (size_t k = 0; k < p.pts.size(); k++) {
            b.x0 = std::min(b.x0, p.pts[k].x); b.x1 = std::max(b.x1, p.pts[k].x);
            b.y0 = std::min(b.y0, p.pts[k].y); b.y1 = std::max(b.y1, p.pts[k].y);
        }
    }
    if (b.x0 > b.x1) { b.x0 = 0; b.y0 = 0; b.x1 = 400; b.y1 = 300; }
    return b;
}

static int hitNode(const Diagram& d, double x, double y) {
    for (size_t i = d.nodes.size(); i-- > 0;) {
        const Node& n = d.nodes[i];
        double dx = fabs(x - n.x), dy = fabs(y - n.y);
        if (n.kind == K_JUNC) { if (dx < 8 && dy < 8) return n.id; continue; }
        if (n.kind == K_DEC) { if (dx / (n.w / 2) + dy / (n.h / 2) <= 1.0) return n.id; }
        else if (dx <= n.w / 2 && dy <= n.h / 2) return n.id;
    }
    return 0;
}
static double distSeg(Pt p, Pt a, Pt b) {
    double dx = b.x - a.x, dy = b.y - a.y, L2 = dx * dx + dy * dy;
    double t = (L2 < 1e-9) ? 0 : ((p.x - a.x) * dx + (p.y - a.y) * dy) / L2;
    t = std::max(0.0, std::min(1.0, t));
    double qx = a.x + t * dx, qy = a.y + t * dy;
    return sqrt((p.x - qx) * (p.x - qx) + (p.y - qy) * (p.y - qy));
}
static int hitEdge(const Diagram& d, double x, double y, double tol) {
    for (size_t i = d.edges.size(); i-- > 0;) {
        std::vector<Pt> pts = routeEdge(d, d.edges[i]);
        for (size_t k = 0; k + 1 < pts.size(); k++)
            if (distSeg(Pt(x, y), pts[k], pts[k + 1]) <= tol) return (int)i;
    }
    return -1;
}

static uint32_t crcTable[256];
static void initCrc() {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crcTable[n] = c;
    }
}
static uint32_t crc32Of(const uint8_t* b, size_t n, uint32_t crc = 0xFFFFFFFFu) {
    for (size_t i = 0; i < n; i++) crc = crcTable[(crc ^ b[i]) & 255] ^ (crc >> 8);
    return crc;
}
static void put32(std::vector<uint8_t>& o, uint32_t v) {
    o.push_back((uint8_t)(v >> 24)); o.push_back((uint8_t)(v >> 16)); o.push_back((uint8_t)(v >> 8)); o.push_back((uint8_t)v);
}
static void pngChunk(std::vector<uint8_t>& o, const char* type, const std::vector<uint8_t>& data) {
    put32(o, (uint32_t)data.size());
    std::vector<uint8_t> td(type, type + 4);
    td.insert(td.end(), data.begin(), data.end());
    o.insert(o.end(), td.begin(), td.end());
    put32(o, crc32Of(td.data(), td.size()) ^ 0xFFFFFFFFu);
}
struct BitWriter {
    std::vector<uint8_t>& o; uint32_t acc; int n;
    BitWriter(std::vector<uint8_t>& out) : o(out), acc(0), n(0) {}
    void put(uint32_t v, int bits) {
        acc |= v << n; n += bits;
        while (n >= 8) { o.push_back((uint8_t)(acc & 255)); acc >>= 8; n -= 8; }
    }
    void huff(uint32_t code, int len) {
        uint32_t r = 0;
        for (int i = 0; i < len; i++) r = (r << 1) | ((code >> i) & 1);
        put(r, len);
    }
    void flush() { if (n > 0) { o.push_back((uint8_t)(acc & 255)); acc = 0; n = 0; } }
};
static void putLit(BitWriter& bw, int sym) {
    if (sym < 144) bw.huff(0x30 + sym, 8);
    else if (sym < 256) bw.huff(0x190 + (sym - 144), 9);
    else if (sym < 280) bw.huff(sym - 256, 7);
    else bw.huff(0xC0 + (sym - 280), 8);
}
static void putMatchDist1(BitWriter& bw, int len) {
    static const int base[] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
    static const int extra[] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
    int idx = 28;
    while (idx > 0 && base[idx] > len) idx--;
    putLit(bw, 257 + idx);
    if (extra[idx]) bw.put((uint32_t)(len - base[idx]), extra[idx]);
    bw.huff(0, 5);
}
static std::vector<uint8_t> encodePngGray(const uint8_t* pix, int w, int h) {
    initCrc();

    std::vector<uint8_t> raw;
    raw.reserve((size_t)(w + 1) * h);
    for (int y = 0; y < h; y++) {
        const uint8_t* row = pix + (size_t)y * w;
        raw.push_back(1);
        for (int x = 0; x < w; x++) raw.push_back((uint8_t)(row[x] - (x ? row[x - 1] : 0)));
    }
    std::vector<uint8_t> z;
    z.push_back(0x78); z.push_back(0x01);
    BitWriter bw(z);
    bw.put(1, 1); bw.put(1, 2);
    size_t n = raw.size(), i = 0;
    while (i < n) {
        int run = 0;
        if (i > 0) while (i + run < n && run < 258 && raw[i + run] == raw[i - 1]) run++;
        if (run >= 3) { putMatchDist1(bw, run); i += run; }
        else { putLit(bw, raw[i]); i++; }
    }
    putLit(bw, 256);
    bw.flush();
    uint32_t a = 1, b = 0;
    for (size_t k = 0; k < n; k++) { a = (a + raw[k]) % 65521; b = (b + a) % 65521; }
    put32(z, (b << 16) | a);
    std::vector<uint8_t> png;
    static const uint8_t sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    png.insert(png.end(), sig, sig + 8);
    std::vector<uint8_t> ihdr;
    put32(ihdr, (uint32_t)w); put32(ihdr, (uint32_t)h);
    ihdr.push_back(8); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
    pngChunk(png, "IHDR", ihdr);
    pngChunk(png, "IDAT", z);
    pngChunk(png, "IEND", std::vector<uint8_t>());
    return png;
}

#ifdef _WIN32

enum {
    IDC_EDIT = 101, IDC_COMBO = 102, IDC_BUILD = 103, IDC_STATUS = 104, IDC_LABEL = 105, IDC_IN_EDIT = 110
};
enum {
    IDM_OPEN_CODE = 201, IDM_SAVE_PROJ, IDM_OPEN_PROJ, IDM_EXPORT, IDM_EXIT,
    IDM_UNDO = 211, IDM_REDO, IDM_DELETE,
    IDM_ADD_PROC = 221, IDM_ADD_DEC, IDM_ADD_IO, IDM_ADD_TERM, IDM_EDIT_TEXT,
    IDM_BUILD = 231, IDM_HIDE_DECL,
    IDM_ZOOM_IN = 241, IDM_ZOOM_OUT, IDM_ZOOM_100,
    IDM_T_PROC = 301, IDM_T_DEC, IDM_T_IO, IDM_T_TERM, IDM_CONNECT = 310,
    IDM_E_LABEL = 321, IDM_E_OUT_B, IDM_E_OUT_R, IDM_E_OUT_L, IDM_E_IN_T, IDM_E_IN_L, IDM_E_IN_R, IDM_E_DEL
};

static HINSTANCE g_hInst;
static HDC g_measDC;

struct DragState {
    bool on, moved; int id; double lx, ly; str snap;
    DragState() : on(false), moved(false), id(0), lx(0), ly(0) {}
};
struct App {
    HWND hwnd, hEdit, hCombo, hBuild, hLabel, hCanvas, hStatus;
    HMENU hMenu;
    HFONT fontUI, fontCode;
    int splitX, dpi;
    bool dragSplit, hideDecl, ctxValid;
    Diagram d;
    double zoom;
    int selType, selId, connSrc;
    Pt org, ctxPos;
    int scrollX, scrollY;
    std::vector<str> undo, redo;
    std::vector<FuncInfo> funcs;
    str curFunc;
    DragState drag;
    App() : hwnd(0), hEdit(0), hCombo(0), hBuild(0), hLabel(0), hCanvas(0), hStatus(0), hMenu(0), fontUI(0), fontCode(0),
            splitX(400), dpi(96), dragSplit(false), hideDecl(true), ctxValid(false), zoom(1.0),
            selType(0), selId(0), connSrc(0), scrollX(0), scrollY(0) {}
};
static App g;

static str getText(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::vector<wchar_t> b((size_t)n + 2);
    GetWindowTextW(h, &b[0], n + 1);
    return str(&b[0]);
}
static str toCrLf(const str& s) {
    str o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == L'\n') o += L"\r\n";
        else if (s[i] != L'\r') o += s[i];
    }
    return o;
}
static str fromCrLf(const str& s) { return replaceAll(s, L"\r\n", L"\n"); }
static void setStatus(const str& s) { SendMessageW(g.hStatus, SB_SETTEXTW, 0, (LPARAM)s.c_str()); }
static int LX(LPARAM lp) { return (int)(short)LOWORD(lp); }
static int LY(LPARAM lp) { return (int)(short)HIWORD(lp); }
static COLORREF toRef(unsigned c) { return RGB((c >> 16) & 255, (c >> 8) & 255, c & 255); }

static HFONT makeFont(int px, bool aa, const wchar_t* face) {
    return CreateFontW(-px, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       aa ? ANTIALIASED_QUALITY : CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
}
static double winMeasure(const str& s) {
    if (s.empty()) return 0;
    SIZE sz;
    GetTextExtentPoint32W(g_measDC, s.c_str(), (int)s.size(), &sz);
    return (double)sz.cx;
}
static void initMeasure() {
    g_measDC = CreateCompatibleDC(NULL);
    SelectObject(g_measDC, makeFont(13, false, L"Arial"));
    g_measure = winMeasure;
}

static bool readFileBytes(const str& path, std::string& out) {
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return false;
    char buf[8192]; size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
    fclose(f);
    return true;
}
static bool writeFileBytes(const str& path, const void* data, size_t n) {
    FILE* f = _wfopen(path.c_str(), L"wb");
    if (!f) return false;
    size_t w = fwrite(data, 1, n, f);
    fclose(f);
    return w == n;
}

static void drawScene(HDC dc, const std::vector<Prim>& pr, double sc, double ox, double oy, bool aa) {
    HFONT font = makeFont((int)(13 * sc + 0.5), aa, L"Arial");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, TA_CENTER | TA_TOP);
    TEXTMETRICW tm;
    GetTextMetricsW(dc, &tm);
    for (size_t i = 0; i < pr.size(); i++) {
        const Prim& p = pr[i];
        if (p.type == P_LINE || p.type == P_POLY) {
            std::vector<POINT> v;
            for (size_t k = 0; k < p.pts.size(); k++) {
                POINT t;
                t.x = (LONG)floor(p.pts[k].x * sc + ox + 0.5);
                t.y = (LONG)floor(p.pts[k].y * sc + oy + 0.5);
                v.push_back(t);
            }
            if (v.size() < 2) continue;
            int pw = std::max(1, (int)(p.w * sc + 0.5));
            HPEN pen = CreatePen(PS_SOLID, pw, toRef(p.line));
            HGDIOBJ op = SelectObject(dc, pen);
            if (p.type == P_POLY) {
                HBRUSH br = CreateSolidBrush(toRef(p.fill));
                HGDIOBJ ob = SelectObject(dc, br);
                Polygon(dc, &v[0], (int)v.size());
                SelectObject(dc, ob);
                DeleteObject(br);
            } else Polyline(dc, &v[0], (int)v.size());
            SelectObject(dc, op);
            DeleteObject(pen);
        } else if (p.type == P_DOT) {
            int cx = (int)floor(p.at.x * sc + ox + 0.5), cy = (int)floor(p.at.y * sc + oy + 0.5);
            int r = std::max(2, (int)(p.w * sc + 0.5));
            HPEN pen = CreatePen(PS_SOLID, 1, toRef(p.line));
            HBRUSH br = CreateSolidBrush(toRef(p.line));
            HGDIOBJ op = SelectObject(dc, pen), ob = SelectObject(dc, br);
            Ellipse(dc, cx - r, cy - r, cx + r + 1, cy + r + 1);
            SelectObject(dc, op); SelectObject(dc, ob);
            DeleteObject(pen); DeleteObject(br);
        } else if (p.type == P_TEXT) {
            SetTextColor(dc, toRef(p.line));
            int x = (int)floor(p.at.x * sc + ox + 0.5);
            int y = (int)floor(p.at.y * sc + oy - tm.tmHeight / 2.0 + 0.5);
            TextOutW(dc, x, y, p.text.c_str(), (int)p.text.size());
        }
    }
    SelectObject(dc, oldFont);
    DeleteObject(font);
}

static bool exportPng(const Diagram& d, const str& path) {
    std::vector<Prim> pr = buildScene(d, 0, 0, 0);
    Bounds b = sceneBounds(pr);
    const double margin = 30;
    double scale = 2.0;
    int W = (int)((b.x1 - b.x0 + 2 * margin) * scale), H = (int)((b.y1 - b.y0 + 2 * margin) * scale);
    while ((double)W * H > 90e6 && scale > 0.5) {
        scale -= 0.25;
        W = (int)((b.x1 - b.x0 + 2 * margin) * scale); H = (int)((b.y1 - b.y0 + 2 * margin) * scale);
    }
    if (W < 1 || H < 1) return false;
    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = W;
    bi.bmiHeader.biHeight = -H;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = 0;
    HDC dc = CreateCompatibleDC(NULL);
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!bmp || !bits) { DeleteDC(dc); return false; }
    HGDIOBJ old = SelectObject(dc, bmp);
    PatBlt(dc, 0, 0, W, H, WHITENESS);
    drawScene(dc, pr, scale, (-b.x0 + margin) * scale, (-b.y0 + margin) * scale, true);
    GdiFlush();
    std::vector<uint8_t> gray((size_t)W * H);
    const uint32_t* px = (const uint32_t*)bits;
    for (size_t i = 0; i < gray.size(); i++) {
        uint32_t p = px[i];
        uint32_t r = (p >> 16) & 255, gg = (p >> 8) & 255, bb = p & 255;
        gray[i] = (uint8_t)((r * 30 + gg * 59 + bb * 11) / 100);
    }
    SelectObject(dc, old);
    DeleteObject(bmp);
    DeleteDC(dc);
    std::vector<uint8_t> png = encodePngGray(&gray[0], W, H);
    return writeFileBytes(path, &png[0], png.size());
}

struct InputCtx { str text; bool ok, multi, done; HWND hEdit; };
static LRESULT CALLBACK InputProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    InputCtx* c = (InputCtx*)GetWindowLongPtrW(h, GWLP_USERDATA);
    switch (m) {
    case WM_CREATE: {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lp;
        c = (InputCtx*)cs->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)c);
        DWORD es = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL;
        if (c->multi) es |= ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL; else es |= ES_AUTOHSCROLL;
        str init = toCrLf(c->text);
        c->hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", init.c_str(), es, 10, 10, 380, c->multi ? 110 : 24, h, (HMENU)(INT_PTR)IDC_IN_EDIT, g_hInst, NULL);
        int by = c->multi ? 134 : 44;
        HWND ok = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 226, by, 80, 26, h, (HMENU)(INT_PTR)IDOK, g_hInst, NULL);
        HWND ca = CreateWindowExW(0, L"BUTTON", L"Отмена", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 310, by, 80, 26, h, (HMENU)(INT_PTR)IDCANCEL, g_hInst, NULL);
        HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessageW(c->hEdit, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageW(ok, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageW(ca, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageW(c->hEdit, EM_SETSEL, 0, -1);
        SetFocus(c->hEdit);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            if (c) { c->text = fromCrLf(getText(c->hEdit)); c->ok = true; }
            DestroyWindow(h);
            return 0;
        }
        if (LOWORD(wp) == IDCANCEL) { DestroyWindow(h); return 0; }
        break;
    case WM_SETFOCUS: if (c && c->hEdit) SetFocus(c->hEdit); return 0;
    case WM_CLOSE: DestroyWindow(h); return 0;
    case WM_DESTROY: if (c) c->done = true; return 0;
    }
    return DefWindowProcW(h, m, wp, lp);
}
static bool inputBox(const wchar_t* title, str& text, bool multi) {
    static bool reg = false;
    if (!reg) {
        WNDCLASSEXW wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = InputProc;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"FlowGenInput";
        RegisterClassExW(&wc);
        reg = true;
    }
    InputCtx c;
    c.text = text; c.ok = false; c.multi = multi; c.done = false; c.hEdit = NULL;
    RECT rc; rc.left = 0; rc.top = 0; rc.right = 400; rc.bottom = multi ? 176 : 84;
    DWORD st = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    AdjustWindowRectEx(&rc, st, FALSE, WS_EX_DLGMODALFRAME);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    RECT pr;
    GetWindowRect(g.hwnd, &pr);
    int x = pr.left + ((pr.right - pr.left) - w) / 2, y = pr.top + ((pr.bottom - pr.top) - h) / 2;
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"FlowGenInput", title, st, x, y, w, h, g.hwnd, NULL, g_hInst, &c);
    if (!dlg) return false;
    EnableWindow(g.hwnd, FALSE);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    MSG msg;
    while (!c.done && GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000)) {
            SendMessageW(dlg, WM_COMMAND, IDOK, 0);
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000) && msg.hwnd == c.hEdit) {
            SendMessageW(c.hEdit, EM_SETSEL, 0, -1);
            continue;
        }
        if (!IsDialogMessageW(dlg, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    if (IsWindow(dlg)) DestroyWindow(dlg);
    EnableWindow(g.hwnd, TRUE);
    SetForegroundWindow(g.hwnd);
    if (c.ok) { text = c.text; return true; }
    return false;
}

static bool pickFile(bool save, const wchar_t* filter, const wchar_t* defExt, str& path) {
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    wchar_t buf[MAX_PATH * 2];
    buf[0] = 0;
    if (save && !path.empty()) wcsncpy(buf, path.c_str(), MAX_PATH * 2 - 1);
    buf[MAX_PATH * 2 - 1] = 0;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH * 2;
    ofn.lpstrDefExt = defExt;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (!ok) return false;
    path = buf;
    return true;
}

static Pt toWorld(int mx, int my) { return Pt((mx + g.scrollX) / g.zoom + g.org.x, (my + g.scrollY) / g.zoom + g.org.y); }
static Pt viewCenter() {
    RECT rc;
    GetClientRect(g.hCanvas, &rc);
    return toWorld(rc.right / 2, rc.bottom / 2);
}
static Bounds diagBounds() { return sceneBounds(buildScene(g.d, 0, 0, 0)); }
static void updateScroll() {
    if (!g.hCanvas) return;
    RECT rc;
    rc.left = rc.top = rc.right = rc.bottom = 0;
    GetClientRect(g.hCanvas, &rc);
    Bounds b = diagBounds();
    g.org = Pt(b.x0 - 60, b.y0 - 60);
    int cw = (int)((b.x1 - b.x0 + 120) * g.zoom), ch = (int)((b.y1 - b.y0 + 120) * g.zoom);
    SCROLLINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0; si.nMax = std::max(0, cw - 1); si.nPage = (UINT)std::max(0, (int)rc.right); si.nPos = g.scrollX;
    SetScrollInfo(g.hCanvas, SB_HORZ, &si, TRUE);
    si.nMax = std::max(0, ch - 1); si.nPage = (UINT)std::max(0, (int)rc.bottom); si.nPos = g.scrollY;
    SetScrollInfo(g.hCanvas, SB_VERT, &si, TRUE);
    si.fMask = SIF_POS;
    GetScrollInfo(g.hCanvas, SB_HORZ, &si); g.scrollX = si.nPos;
    GetScrollInfo(g.hCanvas, SB_VERT, &si); g.scrollY = si.nPos;
    InvalidateRect(g.hCanvas, NULL, FALSE);
}
static void refresh() { updateScroll(); }
static void setZoom(double z, int mx, int my) {
    z = std::max(0.25, std::min(4.0, z));
    Pt w = toWorld(mx, my);
    g.zoom = z;
    Bounds b = diagBounds();
    g.org = Pt(b.x0 - 60, b.y0 - 60);
    g.scrollX = (int)((w.x - g.org.x) * z - mx);
    g.scrollY = (int)((w.y - g.org.y) * z - my);
    updateScroll();
}
static void onScroll(int bar, WPARAM wp) {
    SCROLLINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(g.hCanvas, bar, &si);
    int pos = si.nPos;
    switch (LOWORD(wp)) {
    case SB_LINEUP: pos -= 30; break;
    case SB_LINEDOWN: pos += 30; break;
    case SB_PAGEUP: pos -= (int)si.nPage; break;
    case SB_PAGEDOWN: pos += (int)si.nPage; break;
    case SB_THUMBTRACK: case SB_THUMBPOSITION: pos = si.nTrackPos; break;
    }
    si.fMask = SIF_POS;
    si.nPos = pos;
    SetScrollInfo(g.hCanvas, bar, &si, TRUE);
    GetScrollInfo(g.hCanvas, bar, &si);
    if (bar == SB_HORZ) g.scrollX = si.nPos; else g.scrollY = si.nPos;
    InvalidateRect(g.hCanvas, NULL, FALSE);
}

static void pushUndo() {
    g.undo.push_back(serializeDiagram(g.d, L"", L""));
    if (g.undo.size() > 100) g.undo.erase(g.undo.begin());
    g.redo.clear();
}
static void doUndo() {
    if (g.undo.empty()) return;
    g.redo.push_back(serializeDiagram(g.d, L"", L""));
    Diagram nd;
    if (deserializeDiagram(g.undo.back(), nd, 0, 0)) g.d = nd;
    g.undo.pop_back();
    g.selType = 0; g.connSrc = 0;
    refresh();
}
static void doRedo() {
    if (g.redo.empty()) return;
    g.undo.push_back(serializeDiagram(g.d, L"", L""));
    Diagram nd;
    if (deserializeDiagram(g.redo.back(), nd, 0, 0)) g.d = nd;
    g.redo.pop_back();
    g.selType = 0; g.connSrc = 0;
    refresh();
}

static void fillFuncs(const str& want) {
    str code = fromCrLf(getText(g.hEdit));
    g.funcs.clear();
    findFunctions(preprocess(code), g.funcs);
    SendMessageW(g.hCombo, CB_RESETCONTENT, 0, 0);
    int sel = 0;
    for (size_t i = 0; i < g.funcs.size(); i++) {
        SendMessageW(g.hCombo, CB_ADDSTRING, 0, (LPARAM)g.funcs[i].name.c_str());
        if (g.funcs[i].name == want) sel = (int)i;
    }
    if (!g.funcs.empty()) SendMessageW(g.hCombo, CB_SETCURSEL, (WPARAM)sel, 0);
}
static void buildCurrent() {
    int i = (int)SendMessageW(g.hCombo, CB_GETCURSEL, 0, 0);
    if (i < 0 || i >= (int)g.funcs.size()) return;
    Parser p(g.funcs[(size_t)i].body, g.hideDecl);
    List l;
    p.parseAll(l);
    Layouter lay;
    Diagram nd = lay.build(l);
    pushUndo();
    g.d = nd;
    g.selType = 0; g.connSrc = 0;
    g.curFunc = g.funcs[(size_t)i].name;
    g.scrollX = 0; g.scrollY = 0;
    refresh();
    setStatus(L"Функция " + g.curFunc + L": блоков " + intStr((int)g.d.nodes.size()) +
              L".  Мышь: перенос блока; двойной щелчок - правка текста; правая кнопка - меню.");
}
static void doBuild() {
    str want = g.curFunc.empty() ? str(L"main") : g.curFunc;
    int cur = (int)SendMessageW(g.hCombo, CB_GETCURSEL, 0, 0);
    if (cur >= 0) {
        int len = (int)SendMessageW(g.hCombo, CB_GETLBTEXTLEN, (WPARAM)cur, 0);
        if (len > 0 && len < 4096) {
            std::vector<wchar_t> nm((size_t)len + 2);
            if (SendMessageW(g.hCombo, CB_GETLBTEXT, (WPARAM)cur, (LPARAM)&nm[0]) != CB_ERR) want = str(&nm[0]);
        }
    }
    fillFuncs(want);
    if (g.funcs.empty()) {
        MessageBoxW(g.hwnd, L"В тексте не найдено ни одной функции.\nВставьте код программы и повторите.", L"FlowGen", MB_OK | MB_ICONINFORMATION);
        return;
    }
    buildCurrent();
}

static void editNode(int id) {
    Node* n = g.d.find(id);
    if (!n) return;
    str t = n->text;
    if (!inputBox(L"Текст блока", t, true)) return;
    pushUndo();
    n = g.d.find(id);
    n->text = t;
    g.d.autosize(*n);
    refresh();
}
static void editEdgeLabel(int idx) {
    if (idx < 0 || idx >= (int)g.d.edges.size()) return;
    str t = g.d.edges[(size_t)idx].label;
    if (!inputBox(L"Подпись стрелки", t, false)) return;
    pushUndo();
    g.d.edges[(size_t)idx].label = t;
    refresh();
}
static void addNodeCmd(int kind) {
    Pt p = g.ctxValid ? g.ctxPos : viewCenter();
    g.ctxValid = false;
    pushUndo();
    str def = (kind == K_DEC) ? L"условие?" : (kind == K_TERM) ? L"Начало" : (kind == K_IO) ? L"Ввод/вывод" : L"действие";
    int id = g.d.addNode(kind, def);
    Node* n = g.d.find(id);
    n->x = p.x; n->y = p.y;
    g.selType = 1; g.selId = id;
    refresh();
    editNode(id);
}
static void deleteSelection() {
    if (g.selType == 1) {
        pushUndo();
        g.d.removeNode(g.selId);
    } else if (g.selType == 2 && g.selId >= 0 && g.selId < (int)g.d.edges.size()) {
        pushUndo();
        g.d.edges.erase(g.d.edges.begin() + g.selId);
    } else return;
    g.selType = 0;
    refresh();
}
static void setNodeKind(int kind) {
    if (g.selType != 1) return;
    Node* n = g.d.find(g.selId);
    if (!n || n->kind == kind) return;
    pushUndo();
    n = g.d.find(g.selId);
    n->kind = kind;
    g.d.autosize(*n);
    refresh();
}
static void setEdgeSide(bool out, int side) {
    if (g.selType != 2 || g.selId < 0 || g.selId >= (int)g.d.edges.size()) return;
    pushUndo();
    Edge& e = g.d.edges[(size_t)g.selId];
    if (out) e.sout = side; else e.din = side;
    e.hasLane = false;
    refresh();
}
static void startConnect(int id) {
    g.connSrc = id;
    setStatus(L"Укажите блок, в который должна вести стрелка (Esc - отмена).");
    InvalidateRect(g.hCanvas, NULL, FALSE);
}
static void finishConnect(int dst) {
    int src = g.connSrc;
    g.connSrc = 0;
    const Node* s = g.d.find(src);
    if (!s || dst == 0 || dst == src) {
        setStatus(L"Соединение отменено.");
        InvalidateRect(g.hCanvas, NULL, FALSE);
        return;
    }
    int so = SD_B;
    str lab;
    if (s->kind == K_DEC) {
        bool used[4] = { false, false, false, false };
        for (size_t i = 0; i < g.d.edges.size(); i++) if (g.d.edges[i].src == src) used[g.d.edges[i].sout] = true;
        so = !used[SD_B] ? SD_B : (!used[SD_R] ? SD_R : SD_L);
        lab = (so == SD_B) ? L"да" : L"нет";
        if (!inputBox(L"Подпись стрелки", lab, false)) { InvalidateRect(g.hCanvas, NULL, FALSE); return; }
    }
    pushUndo();
    g.d.addEdge(src, dst, lab, so, SD_T, false, 0);
    g.selType = 2; g.selId = (int)g.d.edges.size() - 1;
    refresh();
    setStatus(L"Стрелка добавлена. Стороны подключения можно изменить правой кнопкой мыши по стрелке.");
}

static void openCode() {
    str path;
    if (!pickFile(false, L"Исходный код (*.c;*.cpp;*.h;*.hpp;*.java;*.txt)\0*.c;*.cpp;*.cc;*.h;*.hpp;*.java;*.txt\0Все файлы\0*.*\0", L"", path)) return;
    std::string bytes;
    if (!readFileBytes(path, bytes)) { MessageBoxW(g.hwnd, L"Не удалось прочитать файл.", L"FlowGen", MB_OK | MB_ICONERROR); return; }
    SetWindowTextW(g.hEdit, toCrLf(decodeText(bytes)).c_str());
    g.curFunc = L"main";
    doBuild();
}
static void saveProject() {
    str path;
    if (!pickFile(true, L"Проект FlowGen (*.flow)\0*.flow\0", L"flow", path)) return;
    str s = serializeDiagram(g.d, fromCrLf(getText(g.hEdit)), g.curFunc);
    std::string u = wToUtf8(s);
    if (!writeFileBytes(path, u.data(), u.size())) MessageBoxW(g.hwnd, L"Не удалось сохранить файл.", L"FlowGen", MB_OK | MB_ICONERROR);
    else setStatus(L"Проект сохранён: " + path);
}
static void openProject() {
    str path;
    if (!pickFile(false, L"Проект FlowGen (*.flow)\0*.flow\0Все файлы\0*.*\0", L"flow", path)) return;
    std::string bytes;
    if (!readFileBytes(path, bytes)) { MessageBoxW(g.hwnd, L"Не удалось прочитать файл.", L"FlowGen", MB_OK | MB_ICONERROR); return; }
    Diagram nd; str code, func;
    if (!deserializeDiagram(decodeText(bytes), nd, &code, &func)) { MessageBoxW(g.hwnd, L"Файл не является проектом FlowGen.", L"FlowGen", MB_OK | MB_ICONERROR); return; }
    pushUndo();
    SetWindowTextW(g.hEdit, toCrLf(code).c_str());
    g.curFunc = func;
    fillFuncs(func);
    g.d = nd;
    g.selType = 0; g.connSrc = 0; g.scrollX = 0; g.scrollY = 0;
    refresh();
    setStatus(L"Проект открыт: " + path);
}
static void exportPngCmd() {
    if (g.d.nodes.empty()) { MessageBoxW(g.hwnd, L"Схема пуста.", L"FlowGen", MB_OK | MB_ICONINFORMATION); return; }
    str path = g.curFunc.empty() ? str(L"blok-shema.png") : g.curFunc + L".png";
    if (!pickFile(true, L"Изображение PNG (*.png)\0*.png\0", L"png", path)) return;
    if (exportPng(g.d, path)) setStatus(L"Экспортировано: " + path);
    else MessageBoxW(g.hwnd, L"Не удалось сохранить PNG.", L"FlowGen", MB_OK | MB_ICONERROR);
}
static void onCommand(int id) {
    switch (id) {
    case IDM_OPEN_CODE: openCode(); break;
    case IDM_SAVE_PROJ: saveProject(); break;
    case IDM_OPEN_PROJ: openProject(); break;
    case IDM_EXPORT: exportPngCmd(); break;
    case IDM_EXIT: DestroyWindow(g.hwnd); break;
    case IDM_UNDO: doUndo(); break;
    case IDM_REDO: doRedo(); break;
    case IDM_DELETE: deleteSelection(); break;
    case IDM_ADD_PROC: addNodeCmd(K_PROC); break;
    case IDM_ADD_DEC: addNodeCmd(K_DEC); break;
    case IDM_ADD_IO: addNodeCmd(K_IO); break;
    case IDM_ADD_TERM: addNodeCmd(K_TERM); break;
    case IDM_EDIT_TEXT:
        if (g.selType == 1) editNode(g.selId); else if (g.selType == 2) editEdgeLabel(g.selId);
        break;
    case IDM_BUILD: doBuild(); break;
    case IDM_HIDE_DECL:
        g.hideDecl = !g.hideDecl;
        CheckMenuItem(g.hMenu, IDM_HIDE_DECL, MF_BYCOMMAND | (g.hideDecl ? MF_CHECKED : MF_UNCHECKED));
        break;
    case IDM_ZOOM_IN: { RECT rc; GetClientRect(g.hCanvas, &rc); setZoom(g.zoom * 1.25, rc.right / 2, rc.bottom / 2); break; }
    case IDM_ZOOM_OUT: { RECT rc; GetClientRect(g.hCanvas, &rc); setZoom(g.zoom / 1.25, rc.right / 2, rc.bottom / 2); break; }
    case IDM_ZOOM_100: { RECT rc; GetClientRect(g.hCanvas, &rc); setZoom(g.dpi / 96.0, rc.right / 2, rc.bottom / 2); break; }
    case IDM_T_PROC: setNodeKind(K_PROC); break;
    case IDM_T_DEC: setNodeKind(K_DEC); break;
    case IDM_T_IO: setNodeKind(K_IO); break;
    case IDM_T_TERM: setNodeKind(K_TERM); break;
    case IDM_CONNECT: if (g.selType == 1) startConnect(g.selId); break;
    case IDM_E_LABEL: if (g.selType == 2) editEdgeLabel(g.selId); break;
    case IDM_E_OUT_B: setEdgeSide(true, SD_B); break;
    case IDM_E_OUT_R: setEdgeSide(true, SD_R); break;
    case IDM_E_OUT_L: setEdgeSide(true, SD_L); break;
    case IDM_E_IN_T: setEdgeSide(false, SD_T); break;
    case IDM_E_IN_L: setEdgeSide(false, SD_L); break;
    case IDM_E_IN_R: setEdgeSide(false, SD_R); break;
    case IDM_E_DEL: deleteSelection(); break;
    }
}

static void showContext(HWND h, int mx, int my) {
    Pt w = toWorld(mx, my);
    int nid = hitNode(g.d, w.x, w.y);
    int eid = nid ? -1 : hitEdge(g.d, w.x, w.y, 6.0 / g.zoom);
    HMENU m = CreatePopupMenu();
    if (nid) {
        g.selType = 1; g.selId = nid;
        HMENU t = CreatePopupMenu();
        AppendMenuW(t, MF_STRING, IDM_T_PROC, L"Процесс");
        AppendMenuW(t, MF_STRING, IDM_T_DEC, L"Условие");
        AppendMenuW(t, MF_STRING, IDM_T_IO, L"Ввод / вывод");
        AppendMenuW(t, MF_STRING, IDM_T_TERM, L"Начало / конец");
        AppendMenuW(m, MF_STRING, IDM_EDIT_TEXT, L"Изменить текст...");
        AppendMenuW(m, MF_POPUP, (UINT_PTR)t, L"Тип блока");
        AppendMenuW(m, MF_STRING, IDM_CONNECT, L"Провести стрелку к...");
        AppendMenuW(m, MF_SEPARATOR, 0, NULL);
        AppendMenuW(m, MF_STRING, IDM_DELETE, L"Удалить блок");
    } else if (eid >= 0) {
        g.selType = 2; g.selId = eid;
        HMENU o = CreatePopupMenu(), i = CreatePopupMenu();
        AppendMenuW(o, MF_STRING, IDM_E_OUT_B, L"Снизу");
        AppendMenuW(o, MF_STRING, IDM_E_OUT_R, L"Справа");
        AppendMenuW(o, MF_STRING, IDM_E_OUT_L, L"Слева");
        AppendMenuW(i, MF_STRING, IDM_E_IN_T, L"Сверху");
        AppendMenuW(i, MF_STRING, IDM_E_IN_L, L"Слева");
        AppendMenuW(i, MF_STRING, IDM_E_IN_R, L"Справа");
        AppendMenuW(m, MF_STRING, IDM_E_LABEL, L"Подпись...");
        AppendMenuW(m, MF_POPUP, (UINT_PTR)o, L"Выход из блока");
        AppendMenuW(m, MF_POPUP, (UINT_PTR)i, L"Вход в блок");
        AppendMenuW(m, MF_SEPARATOR, 0, NULL);
        AppendMenuW(m, MF_STRING, IDM_E_DEL, L"Удалить стрелку");
    } else {
        g.selType = 0;
        g.ctxPos = w; g.ctxValid = true;
        AppendMenuW(m, MF_STRING, IDM_ADD_PROC, L"Добавить блок: процесс");
        AppendMenuW(m, MF_STRING, IDM_ADD_DEC, L"Добавить блок: условие");
        AppendMenuW(m, MF_STRING, IDM_ADD_IO, L"Добавить блок: ввод / вывод");
        AppendMenuW(m, MF_STRING, IDM_ADD_TERM, L"Добавить блок: начало / конец");
    }
    InvalidateRect(h, NULL, FALSE);
    POINT pt; pt.x = mx; pt.y = my;
    ClientToScreen(h, &pt);
    int cmd = (int)TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, h, NULL);
    DestroyMenu(m);
    if (cmd) onCommand(cmd);
    g.ctxValid = false;
}

static LRESULT CALLBACK CanvasProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_ERASEBKGND: return 1;
    case WM_SIZE: if (g.hCanvas == h) updateScroll(); return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc;
        GetClientRect(h, &rc);
        int w = rc.right, hh = rc.bottom;
        if (w > 0 && hh > 0) {
            HDC mem = CreateCompatibleDC(dc);
            HBITMAP bmp = CreateCompatibleBitmap(dc, w, hh);
            HGDIOBJ old = SelectObject(mem, bmp);
            PatBlt(mem, 0, 0, w, hh, WHITENESS);
            std::vector<Prim> pr = buildScene(g.d, g.selType, g.selId, g.connSrc);
            drawScene(mem, pr, g.zoom, -g.org.x * g.zoom - g.scrollX, -g.org.y * g.zoom - g.scrollY, false);
            BitBlt(dc, 0, 0, w, hh, mem, 0, 0, SRCCOPY);
            SelectObject(mem, old);
            DeleteObject(bmp);
            DeleteDC(mem);
        }
        EndPaint(h, &ps);
        return 0;
    }
    case WM_HSCROLL: onScroll(SB_HORZ, wp); return 0;
    case WM_VSCROLL: onScroll(SB_VERT, wp); return 0;
    case WM_MOUSEWHEEL: {
        int delta = (int)(short)HIWORD(wp);
        if (LOWORD(wp) & MK_CONTROL) {
            POINT pt; pt.x = LX(lp); pt.y = LY(lp);
            ScreenToClient(h, &pt);
            setZoom(g.zoom * (delta > 0 ? 1.15 : 1.0 / 1.15), pt.x, pt.y);
        } else {
            onScroll((LOWORD(wp) & MK_SHIFT) ? SB_HORZ : SB_VERT, delta > 0 ? SB_LINEUP : SB_LINEDOWN);
            onScroll((LOWORD(wp) & MK_SHIFT) ? SB_HORZ : SB_VERT, delta > 0 ? SB_LINEUP : SB_LINEDOWN);
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        SetFocus(h);
        int mx = LX(lp), my = LY(lp);
        Pt w = toWorld(mx, my);
        int nid = hitNode(g.d, w.x, w.y);
        if (g.connSrc) { finishConnect(nid); return 0; }
        if (nid) {
            g.selType = 1; g.selId = nid;
            g.drag.on = true; g.drag.moved = false; g.drag.id = nid;
            g.drag.lx = w.x; g.drag.ly = w.y;
            g.drag.snap = serializeDiagram(g.d, L"", L"");
            SetCapture(h);
        } else {
            int e = hitEdge(g.d, w.x, w.y, 6.0 / g.zoom);
            if (e >= 0) { g.selType = 2; g.selId = e; } else g.selType = 0;
        }
        InvalidateRect(h, NULL, FALSE);
        return 0;
    }
    case WM_MOUSEMOVE:
        if (g.drag.on) {
            Pt w = toWorld(LX(lp), LY(lp));
            Node* n = g.d.find(g.drag.id);
            if (n) {
                if (!g.drag.moved) {
                    for (size_t i = 0; i < g.d.edges.size(); i++)
                        if (g.d.edges[i].src == g.drag.id || g.d.edges[i].dst == g.drag.id) g.d.edges[i].hasLane = false;
                }
                n->x += w.x - g.drag.lx; n->y += w.y - g.drag.ly;
                g.drag.lx = w.x; g.drag.ly = w.y;
                g.drag.moved = true;
                InvalidateRect(h, NULL, FALSE);
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (g.drag.on) {
            g.drag.on = false;
            ReleaseCapture();
            if (g.drag.moved) {
                g.undo.push_back(g.drag.snap);
                if (g.undo.size() > 100) g.undo.erase(g.undo.begin());
                g.redo.clear();
                updateScroll();
            }
        }
        return 0;
    case WM_LBUTTONDBLCLK: {
        Pt w = toWorld(LX(lp), LY(lp));
        int nid = hitNode(g.d, w.x, w.y);
        if (nid) { g.selType = 1; g.selId = nid; editNode(nid); }
        else {
            int e = hitEdge(g.d, w.x, w.y, 6.0 / g.zoom);
            if (e >= 0) { g.selType = 2; g.selId = e; editEdgeLabel(e); }
        }
        return 0;
    }
    case WM_RBUTTONUP:
        if (g.connSrc) { g.connSrc = 0; setStatus(L"Соединение отменено."); InvalidateRect(h, NULL, FALSE); return 0; }
        showContext(h, LX(lp), LY(lp));
        return 0;
    case WM_KEYDOWN: {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (wp == VK_DELETE) deleteSelection();
        else if (wp == VK_ESCAPE) { g.connSrc = 0; g.selType = 0; InvalidateRect(h, NULL, FALSE); }
        else if (ctrl && wp == 'Z') doUndo();
        else if (ctrl && wp == 'Y') doRedo();
        return 0;
    }
    }
    return DefWindowProcW(h, m, wp, lp);
}

static void layoutMain() {
    RECT rc;
    GetClientRect(g.hwnd, &rc);
    SendMessageW(g.hStatus, WM_SIZE, 0, 0);
    RECT sr;
    GetWindowRect(g.hStatus, &sr);
    int statusH = sr.bottom - sr.top;
    int W = rc.right, H = rc.bottom - statusH;
    int maxSplit = std::max(200, W - 300);
    g.splitX = std::max(200, std::min(g.splitX, maxSplit));
    const int pad = 6, rowH = 24;
    int leftW = g.splitX - pad;
    MoveWindow(g.hLabel, pad, pad, leftW, 18, TRUE);
    int rowY = H - pad - rowH;
    int btnW = 150;
    MoveWindow(g.hEdit, pad, pad + 22, leftW, rowY - pad - (pad + 22), TRUE);
    MoveWindow(g.hCombo, pad, rowY, leftW - btnW - pad, 240, TRUE);
    MoveWindow(g.hBuild, pad + leftW - btnW, rowY - 1, btnW, rowH + 2, TRUE);
    MoveWindow(g.hCanvas, g.splitX + 6, pad, W - g.splitX - 6 - pad, H - 2 * pad, TRUE);
}

static LRESULT CALLBACK MainProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_CREATE: {
        g.hwnd = h;
        HDC sdc = GetDC(NULL);
        g.dpi = GetDeviceCaps(sdc, LOGPIXELSX);
        ReleaseDC(NULL, sdc);
        if (g.dpi < 96) g.dpi = 96;
        g.zoom = g.dpi / 96.0;
        g.fontUI = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        g.fontCode = CreateFontW(-(14 * g.dpi / 96), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
        g.hLabel = CreateWindowExW(0, L"STATIC", L"Код программы:", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, h, (HMENU)(INT_PTR)IDC_LABEL, g_hInst, NULL);
        g.hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
                                  ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN, 0, 0, 10, 10, h, (HMENU)(INT_PTR)IDC_EDIT, g_hInst, NULL);
        g.hCombo = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                                   0, 0, 10, 10, h, (HMENU)(INT_PTR)IDC_COMBO, g_hInst, NULL);
        g.hBuild = CreateWindowExW(0, L"BUTTON", L"Построить схему (F5)", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   0, 0, 10, 10, h, (HMENU)(INT_PTR)IDC_BUILD, g_hInst, NULL);
        g.hCanvas = CreateWindowExW(WS_EX_CLIENTEDGE, L"FlowGenCanvas", L"", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP,
                                    0, 0, 10, 10, h, NULL, g_hInst, NULL);
        g.hStatus = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, h, (HMENU)(INT_PTR)IDC_STATUS, g_hInst, NULL);
        SendMessageW(g.hLabel, WM_SETFONT, (WPARAM)g.fontUI, TRUE);
        SendMessageW(g.hCombo, WM_SETFONT, (WPARAM)g.fontUI, TRUE);
        SendMessageW(g.hBuild, WM_SETFONT, (WPARAM)g.fontUI, TRUE);
        SendMessageW(g.hStatus, WM_SETFONT, (WPARAM)g.fontUI, TRUE);
        SendMessageW(g.hEdit, WM_SETFONT, (WPARAM)g.fontCode, TRUE);
        SendMessageW(g.hEdit, EM_SETHILITE, 0x7FFFFFFF, 0);
        setStatus(L"Вставьте код и нажмите \"Построить схему\" (F5), либо выберите Файл - Образец.");
        return 0;
    }
    case WM_SIZE: layoutMain(); return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mi = (MINMAXINFO*)lp;
        mi->ptMinTrackSize.x = 700; mi->ptMinTrackSize.y = 400;
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id == IDC_BUILD && code == BN_CLICKED) { doBuild(); return 0; }
        if (id == IDC_COMBO && code == CBN_SELCHANGE) { doBuild(); return 0; }
        if (id >= 200) { onCommand(id); return 0; }
        break;
    }
    case WM_SETCURSOR: {
        if ((HWND)wp == h && LOWORD(lp) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(h, &pt);
            if (pt.x >= g.splitX && pt.x < g.splitX + 6) { SetCursor(LoadCursor(NULL, IDC_SIZEWE)); return TRUE; }
        }
        break;
    }
    case WM_LBUTTONDOWN:
        if (LX(lp) >= g.splitX && LX(lp) < g.splitX + 6) { g.dragSplit = true; SetCapture(h); }
        return 0;
    case WM_MOUSEMOVE:
        if (g.dragSplit) { g.splitX = LX(lp) - 3; layoutMain(); }
        return 0;
    case WM_LBUTTONUP:
        if (g.dragSplit) { g.dragSplit = false; ReleaseCapture(); }
        return 0;
    case WM_SETFOCUS: SetFocus(g.hEdit); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, wp, lp);
}

static HMENU buildMenu() {
    HMENU mb = CreateMenu();
    HMENU f = CreatePopupMenu(), e = CreatePopupMenu(), v = CreatePopupMenu(), s = CreatePopupMenu(), hp = CreatePopupMenu();
    AppendMenuW(f, MF_STRING, IDM_OPEN_CODE, L"Открыть исходный код...\tCtrl+O");
    AppendMenuW(f, MF_STRING, IDM_OPEN_PROJ, L"Открыть проект...");
    AppendMenuW(f, MF_STRING, IDM_SAVE_PROJ, L"Сохранить проект...\tCtrl+S");
    AppendMenuW(f, MF_SEPARATOR, 0, NULL);
    AppendMenuW(f, MF_STRING, IDM_EXPORT, L"Экспорт в PNG...\tCtrl+E");
    AppendMenuW(f, MF_SEPARATOR, 0, NULL);
    AppendMenuW(f, MF_STRING, IDM_EXIT, L"Выход");
    AppendMenuW(e, MF_STRING, IDM_UNDO, L"Отменить\tCtrl+Z");
    AppendMenuW(e, MF_STRING, IDM_REDO, L"Повторить\tCtrl+Y");
    AppendMenuW(e, MF_SEPARATOR, 0, NULL);
    AppendMenuW(e, MF_STRING, IDM_EDIT_TEXT, L"Изменить текст...");
    AppendMenuW(e, MF_STRING, IDM_DELETE, L"Удалить\tDel");
    AppendMenuW(e, MF_SEPARATOR, 0, NULL);
    AppendMenuW(e, MF_STRING, IDM_ADD_PROC, L"Добавить: процесс");
    AppendMenuW(e, MF_STRING, IDM_ADD_DEC, L"Добавить: условие");
    AppendMenuW(e, MF_STRING, IDM_ADD_IO, L"Добавить: ввод / вывод");
    AppendMenuW(e, MF_STRING, IDM_ADD_TERM, L"Добавить: начало / конец");
    AppendMenuW(v, MF_STRING, IDM_BUILD, L"Построить схему\tF5");
    AppendMenuW(v, MF_STRING | MF_CHECKED, IDM_HIDE_DECL, L"Скрывать объявления без значения");
    AppendMenuW(v, MF_SEPARATOR, 0, NULL);
    AppendMenuW(v, MF_STRING, IDM_ZOOM_IN, L"Увеличить");
    AppendMenuW(v, MF_STRING, IDM_ZOOM_OUT, L"Уменьшить");
    AppendMenuW(v, MF_STRING, IDM_ZOOM_100, L"Масштаб 100%");
    AppendMenuW(mb, MF_POPUP, (UINT_PTR)f, L"Файл");
    AppendMenuW(mb, MF_POPUP, (UINT_PTR)e, L"Правка");
    AppendMenuW(mb, MF_POPUP, (UINT_PTR)v, L"Вид");
    return mb;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    g_hInst = hInst;
    typedef BOOL (WINAPI *PFN_DPI)(void);
    HMODULE u32 = GetModuleHandleW(L"user32.dll");
    if (u32) {
        PFN_DPI f = (PFN_DPI)(void*)GetProcAddress(u32, "SetProcessDPIAware");
        if (f) f();
    }
    INITCOMMONCONTROLSEX ic;
    ic.dwSize = sizeof(ic);
    ic.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&ic);
    initMeasure();

	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	PathRemoveFileSpecA(exePath);

	char iconPath[MAX_PATH];
	PathCombineA(iconPath, exePath, "src\\icon.ico");

	HICON hIcon = (HICON)LoadImageA(
		NULL,
		iconPath,
		IMAGE_ICON,
		0, 0,
		LR_LOADFROMFILE | LR_DEFAULTSIZE
	);

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon   = hIcon;
	wc.hIconSm = hIcon;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"FlowGenMain";
    RegisterClassExW(&wc);

    WNDCLASSEXW cc;
    ZeroMemory(&cc, sizeof(cc));
    cc.cbSize = sizeof(cc);
    cc.style = CS_DBLCLKS;
    cc.lpfnWndProc = CanvasProc;
    cc.hInstance = hInst;
    cc.hCursor = LoadCursor(NULL, IDC_ARROW);
    cc.hbrBackground = NULL;
    cc.lpszClassName = L"FlowGenCanvas";
    RegisterClassExW(&cc);

    g.hMenu = buildMenu();
    HWND hwnd = CreateWindowExW(0, L"FlowGenMain", L"FlowGen", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800, NULL, g.hMenu, hInst, NULL);
    if (!hwnd) return 1;
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);
    layoutMain();

    ACCEL acc[4];
    acc[0].fVirt = FVIRTKEY | FCONTROL; acc[0].key = 'O'; acc[0].cmd = IDM_OPEN_CODE;
    acc[1].fVirt = FVIRTKEY | FCONTROL; acc[1].key = 'S'; acc[1].cmd = IDM_SAVE_PROJ;
    acc[2].fVirt = FVIRTKEY | FCONTROL; acc[2].key = 'E'; acc[2].cmd = IDM_EXPORT;
    acc[3].fVirt = FVIRTKEY;            acc[3].key = VK_F5; acc[3].cmd = IDM_BUILD;
    HACCEL hAcc = CreateAcceleratorTableW(acc, 4);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            wchar_t cls[16];
            GetClassNameW(msg.hwnd, cls, 16);
            if (wcscmp(cls, L"Edit") == 0) { SendMessageW(msg.hwnd, EM_SETSEL, 0, -1); continue; }
        }
        if (TranslateAcceleratorW(hwnd, hAcc, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
#endif
