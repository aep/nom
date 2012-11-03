#include "lexer.hpp"
#include "utils.hpp"

#include <sstream>

MakefileLexer::MakefileLexer(std::istream *in, const std::string &fileName)
    : vpos(0)
{
    source.push(SourceFile(fileName, in));
}

std::string MakefileLexer::getLine(bool &eof)
{
    while (source.size() && source.top().in->eof()) {
        delete source.top().in;
        source.pop();
    }

    if (source.empty()) {
        eof = true;
        return std::string();
    }

    std::string l;
    std::getline (*source.top().in, l);
    if (!*source.top().in) {
        delete source.top().in;
        source.pop();
        eof = true;
        return l;
    }

    source.top().line++;
    return l;
}

void MakefileLexer::insert(std::istream *in, const std::string &fileName)
{
    source.push(SourceFile(fileName, in));
}

bool MakefileLexer::nextOperator(Token &t)
{
    std::string capture;
    while (!eol()) {
        char c = get();
        switch (c) {
            case '?':
            case '+':
            case ':':
                 capture.push_back(c);
                 break;
            case '=':
                 capture.push_back(c);
                 t.a = capture;
                 return true;
            default:
                 return false;
        }
    }
    if (capture.size() && endsWith(capture, "=")) {
        t.a = capture;
        return true;
    }
    return false;

}

bool MakefileLexer::nextLiteral(Token &t)
{
    bool forcecapture = false;
    std::string capture;
    while (!eol()) {
        int p = save();
        char c = get();
        switch (c) {
            case '\\':
                if (!eol())
                    capture.push_back(get());
                continue;
            case '#':
                return false;
            case '"':
            case '\'':
                forcecapture = true;
                for (;;) {
                    if (eol())
                        return false;
                    char d = get();
                    if (d == '\\')
                        capture.push_back(get());
                    else if (d == c)
                        break;
                    else
                        capture.push_back(d);
                }
                break;
            case '?':
            case ':':
            case '=':
            case ',':
            case '$':
            case '}':
            case '{':
            case ')':
            case '(':
            case '\t':
            case ' ':
                restore(p);
                if (capture.size()) {
                    t.a = capture;
                    return true;
                } else {
                    return false;
                }
                break;
            default:
                capture.push_back(c);
                break;
        }
    }
    if (forcecapture || capture.size()) {
        t.a = capture;
        return true;
    }
    return false;
}

bool MakefileLexer::nextShellCommand(Token &t)
{
    if (eol())
        return false;
    if (get() != '\t')
        return false;
    while (!eol()){
        t.a += get();
    }
    return true;
}

bool MakefileLexer::nextPipe(Token &)
{
    if (eol())
        return false;
    return (get() == '|');
}

bool MakefileLexer::nextColon(Token &)
{
    if (eol())
        return false;
    return (get() == ':');
}

bool MakefileLexer::nextComma(Token &)
{
    if (eol())
        return false;
    return (get() == ',');
}

bool MakefileLexer::nextBraceOpen(Token &)
{
    if (eol())
        return false;
    char c = get();
    return (c == '(' || c == '{');
}

bool MakefileLexer::nextBraceClose(Token &)
{
    if (eol())
        return false;
    char c = get();
    return (c == ')' || c == '}');
}

bool MakefileLexer::nextExpression(Token &)
{
    if (eol())
        return false;
    return (get() == '$');
}

bool MakefileLexer::nextShortExpression(Token &t)
{
    if (!nextExpression(t))
        return false;

    std::string capture;
    while (!eol()) {
        int p = save();
        char c = get();

        if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            capture.push_back(c);
        else {
            restore(p);
            break;
        }
    }
    if (capture.size()) {
        t.a = capture;
        return true;
    }
    return false;

}

bool MakefileLexer::nextComment(Token &t)
{
    if (eol())
        return false;
    if (get() != '#')
        return false;
    while (!eol()){
        t.a += get();
    }
    return true;
}

bool MakefileLexer::nextSpace(Token &)
{
    bool had = false;
    while (!eol()) {
        int p = save();
        char c = get();
        if (c != ' ' && c != '\t') {
            restore(p);
            return had;
        }
        had = true;
    }
    return had;
}

bool MakefileLexer::nextTab(Token &)
{
    bool had = false;
    while (!eol()) {
        int p = save();
        char c = get();
        if (c != '\t' ) {
            restore(p);
            return had;
        }
        had = true;
    }
    return had;
}

Token::Type MakefileLexer::next(Token &t)
{

    t.a = std::string();
    t.location.column   = save();
    if (source.empty()) {
        std::cerr << "warning: somehow this file ends prematurely" << std::endl;
        t.location.line     = 0;
        t.location.fileName = "?";
    } else {
        t.location.line     = source.top().line;
        t.location.fileName = source.top().fileName;
    }

    //TODO: wasting lots of memory here
    if (t.location.source.empty())
        t.location.source   = line;

    if (line.empty()) {
        vpos = 0;
        bool atEnd = false;
        line = getLine(atEnd);
        if (atEnd) {
            t.location.column = 0;
            return Token::Eof;
        }
        if (line.empty()) {
            t.location.column = 0;
            return Token::Empty;
        }
        while (endsWith(line, "\\")) {
            line.resize(line.size() - 1);
            atEnd = false;
            line += getLine(atEnd);
            if (atEnd)
                break;
        }
        if (line.at(0) == '#') {
            t.a = line;
            line.clear();
            return Token::Comment;
        }
    }
    if (eol()) {
        line.clear();
        return Token::Eol;
    }


    //TODO: wasting lots of memory here
    if (t.location.source.empty())
        t.location.source   = line;

    int p = save();
    t.location.column = p;

    if (nextTab(t)) return Token::Tab;
    restore(p);

    if (nextSpace(t)) return Token::Space;
    restore(p);

    if (nextComment(t)) return Token::Comment;
    restore(p);

    if (nextComma(t)) return Token::Comma;
    restore(p);

    if (nextPipe(t))  return Token::Pipe;
    restore(p);

    if (nextShortExpression(t)) return Token::ShortExpression;
    restore(p);

    if (nextExpression(t)) return Token::Expression;
    restore(p);

    if (nextBraceOpen(t)) return Token::BraceOpen;
    restore(p);

    if (nextBraceClose(t)) return Token::BraceClose;
    restore(p);

    if (nextOperator(t)) return Token::Operator;
    restore(p);

    if (nextLiteral(t)) return Token::Literal;
    restore(p);

    if (nextColon(t)) return Token::Colon;
    restore(p);

    line.clear();
    return Token::ParseError;
}

std::string Token::dump() const
{
    std::string r;
    switch (type) {
        case Token::Eof:
            r = "Eof "; break;
        case Token::Eol:
            r = "Eol "; break;
        case Token::Empty:
            r = "Empty "; break;
        case Token::ParseError:
            r = "ParseError "; break;
        case Token::Comment:
            r = "Comment "; break;
        case Token::Literal:
            r = "Literal "; break;
        case Token::Operator:
            r = "Operator "; break;
        case Token::Expression:
            r = "Expression ";  break;
        case Token::Tab:
            r = "Tab";  break;
        case Token::Pipe:
            r = "Pipe ";  break;
        case Token::Colon:
            r = "Colon ";  break;
        case Token::Comma:
            r = "Comma ";  break;
        case Token::BraceOpen:
            r = "BraceOpen ";  break;
        case Token::BraceClose:
            r = "BraceClose ";  break;
        case Token::Space:
            r = "Space ";  break;
        case Token::ShortExpression:
            r = "ShortExpression";  break;
    }

    std::stringstream oss;
    oss << location.fileName << " :"
        << location.line     << ":"
        << location.column   << ":"
        << r << std::endl
        << location.source << std::endl
        << std::string(location.column, ' ') << "^";

    return oss.str();

}
