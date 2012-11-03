#include <iostream>
#include <fstream>

#include "../../make/lexer.hpp"
#include "../../make/parser.hpp"

#include <stdexcept>

class SemanticException : public std::exception
{
public:
    virtual ~SemanticException() throw() {}
    SemanticException(std::string t, Node::Ptr node)
        : node(node)
        , text(t)
    {}
    Node::Ptr node;
    std::string text;
    virtual const char* what() const throw() {
        return text.c_str();
    }
};

std::vector<std::string> dummy(MakefileParser *that, std::vector<std::vector<std::string> > args)
{
    return std::vector<std::string>();
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "usage: blurp Android.mk" << std::endl;
        return 3;
    }

    std::ifstream *in = new std::ifstream(argv[1]);
    MakefileLexer  lexer(in, argv[1]);

    MakefileParser parser;
    parser.context.functions["patsubst"] = &dummy;

    Node::Ptr root = parser(&lexer);

    if (!root)
        return 1;

    if (root->type != Node::Global)
        throw new SemanticException("expected global", root);


    Global::Ptr global = std::static_pointer_cast<Global>(root);


    std::ostream &qo = std::cout;

    for (auto i = global->items.begin(); i != global->items.end(); i++) {
        Node::Ptr node = *i;
        qo << node->dump() << std::endl;
        if (node->type == Node::Assignment) {
            Assignment::Ptr a = std::static_pointer_cast<Assignment>(node);
            for (auto i = a->lhs->expression.begin(); i != a->lhs->expression.end(); i++) {
                qo << " " << (*i)->dump() << " ";
            }
            qo << std::endl;
            qo << " " << a->op << std::endl;
            for (auto i = a->rhs.begin(); i != a->rhs.end(); i++) {
                qo << " " << (*i)->dump() << " ";
            }
            qo << std::endl;
        } else if (node->type == Node::Generator) {
            Generator::Ptr a = std::static_pointer_cast<Generator>(node);
            for (auto i = a->commands.begin(); i != a->commands.end(); i++) {
                qo << " " << (*i)->dump() << std::endl;
            }
        } else if (node->type == Node::CallExpression) {
            CallExpression::Ptr a = std::static_pointer_cast<CallExpression>(node);
            qo << a->functionName->dump();
            for (auto i = a->args.begin(); i != a->args.end(); i++) {
                for (auto j = i->begin(); j != i->end(); j++) {
                    qo << " " << (*j)->dump();
                }
                qo << " , ";
            }
            qo << std::endl;
        }
    }


    return 0;
}
