#ifndef LEXER_H_sad
#define LEXER_H_sad

#include <string>
#include <iostream>
#include <stack>

struct Location
{
    std::string source;
    std::string fileName;
    int line;
    int column;
};

class Token
{
public:
    enum Type
    {
        Eof,
        Eol,
        Empty,
        ParseError,
        Comment,

        ShortExpression,

        Literal,
        Operator,

        Expression,
        BraceOpen,
        BraceClose,
        Comma,

        Colon,
        Pipe,

        Space,
        Tab
    };
    Type type;
    std::string a;

    Location location;
    std::string dump() const;
};


struct SourceFile
{
    SourceFile()
        : in(0)
        , line(0)
    {}
    SourceFile(const std::string &fileName, std::istream *in)
        : fileName(fileName)
        , in(in)
        , line(0)
    {}
    std::string fileName;
    std::istream *in;
    int line;
};

class MakefileLexer
{
    std::string line;
    unsigned int vpos;

public:
    std::stack<SourceFile> source;
    MakefileLexer(std::istream *in, const std::string &fileName);
    Token::Type  next(Token &t);
    void insert(std::istream *in, const std::string &fileName);
private:
    std::string getLine(bool &eof);

    bool eol() const { return vpos >= line.size(); }
    int  save() const { return vpos; }
    void restore(int i) { vpos = i; }
    char get() { return line.at(vpos++); }

    bool nextShortExpression(Token &t);
    bool nextShellCommand(Token &t);
    bool nextColon(Token &t);
    bool nextPipe(Token &t);
    bool nextComma(Token &t);
    bool nextBraceOpen(Token &t);
    bool nextBraceClose(Token &t);
    bool nextExpression(Token &t);
    bool nextComment(Token &t);
    bool nextOperator(Token &t);
    bool nextLiteral(Token &t);
    bool nextTab(Token &t);
    bool nextSpace(Token &t);
    bool nextFnDef(Token &t);
};


#endif
