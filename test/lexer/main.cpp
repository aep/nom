#include "../../make/lexer.hpp"

#include <iostream>
#include <fstream>

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "usage: blurp Android.mk" << std::endl;
        return 3;
    }

    std::ifstream *in = new std::ifstream(argv[1]);
    MakefileLexer  lex(in, argv[1]);

    std::ostream &o = std::cout;

    for (;;) {
        Token t;
        Token::Type tt  = lex.next(t);
        t.type = tt;

        if (tt == Token::Eof)
            break;

        o << t.dump() << std::endl;
    }



    return 0;
}
