#ifndef MAKEFILEPARSER_H_WUAST
#define MAKEFILEPARSER_H_WUAST

#include "lexer.hpp"

#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <map>


class ParserException : public std::exception
{
public:
    virtual ~ParserException() throw() {}
    ParserException(const std::string &t, int l = -2)
        : token(l)
        , text(t)
    {
    }
    int token;
    std::string text;
    virtual const char* what() const throw() {
        return text.data();
    }
};

struct Node
{
    typedef std::shared_ptr<Node> Ptr;

    enum Type {
        Invalid,
        Global,
        Expression,
        Assignment,
        ReferenceExpression,
        LiteralExpression,
        CallExpression,
        IfCondition,
        ElseCondition,
        EndifCondition,
        ShellCommand,
        Generator,
        SemanticInclude
    };
    Type type;
    Node(Type t)
        : type (t)
    {}

    std::string dump() const;
};

struct Global: Node
{
    typedef std::shared_ptr<Global> Ptr;

    Global()
        : Node(Node::Global)
    {}
    std::vector<Node::Ptr> items;
};

struct Expression : Node
{
    typedef std::shared_ptr<Expression> Ptr;

    Expression(Node::Type t = Node::Expression)
        : Node(t)
    {}

    std::vector<Expression::Ptr> expression;
};

struct Assignment : Node
{
    typedef std::shared_ptr<Assignment> Ptr;

    Assignment()
        : Node(Node::Assignment)
    {}

    std::string op;
    Expression::Ptr lhs;
    std::vector<Expression::Ptr> rhs;
};

struct ReferenceExpression : Expression
{
    typedef std::shared_ptr<ReferenceExpression> Ptr;

    ReferenceExpression ()
        : Expression(Node::ReferenceExpression)
    {}

    std::string reference;
};

struct LiteralExpression : Expression
{
    typedef std::shared_ptr<LiteralExpression> Ptr;

    LiteralExpression (const std::string &literal = std::string())
        : Expression(Node::LiteralExpression)
        , literal(literal)
    {}

    std::string literal;
};

struct CallExpression : Expression
{
    typedef std::shared_ptr<CallExpression> Ptr;

    CallExpression ()
        : Expression(Node::CallExpression)
    {}

    Expression::Ptr functionName;
    std::vector<std::vector<Expression::Ptr> > args;
};

struct Condition : Node
{
    Condition(Node::Type t)
        : Node(t)
    {}
};

struct IfCondition : Condition
{
    typedef std::shared_ptr<Condition> Ptr;

    IfCondition()
        : Condition(Node::IfCondition)
    {}
};

struct ElseCondition : Condition
{
    ElseCondition()
        : Condition(Node::ElseCondition)
    {}
};

struct EndifCondition : Condition
{
    EndifCondition()
        : Condition(Node::EndifCondition)
    {}
};

struct ShellCommand : Node
{
    typedef std::shared_ptr<ShellCommand> Ptr;

    ShellCommand()
        : Node(Node::ShellCommand)
    {}
    std::string command;
};

struct Generator : Node
{
    typedef std::shared_ptr<Generator> Ptr;

    Generator()
        : Node(Node::Generator)
    {}

    std::vector<ShellCommand::Ptr> commands;
};

struct SemanticIncludeNode : Node
{
    SemanticIncludeNode(std::string expr)
        : Node(Node::SemanticInclude)
        , expr(expr)
    {}
    std::string expr;
};

class MakefileParser
{
public:
    struct Context
    {
        typedef std::vector<std::string> (*Function)(MakefileParser *, std::vector<std::vector<std::string> >);
        std::map<std::string, Function> functions;
        std::map<std::string, std::vector<Expression::Ptr> > values;
    };
    Context context;

    MakefileParser();

    Node::Ptr operator()(MakefileLexer *lexer);
    MakefileLexer *lexer;

    std::vector<std::string> eval(Expression::Ptr);
private:
    bool shellCommand(ShellCommand::Ptr &);
    bool generatorNode(Node::Ptr &);

    bool ifConditionNode(Node::Ptr &);
    bool elseConditionNode(Node::Ptr &);
    bool endifConditionNode(Node::Ptr &);

    bool literalExpressionNode(Expression::Ptr &);
    bool referenceExpressionNode(Expression::Ptr &);
    bool substitutionReferenceExpressionNode(Expression::Ptr &);
    bool callExpressionNode(Expression::Ptr &);

    enum ExpressionContext
    {
        LHSContext,
        RHSContext,
        CallContext,
        SubstitutionContext

    };

    bool expressionNode(Expression::Ptr &, ExpressionContext ctx = LHSContext);
    bool expressionList(std::vector<Expression::Ptr> &, ExpressionContext ctx = LHSContext);

    bool semanticIncludeExpression(Node::Ptr &);

    bool assignmentNode(Node::Ptr &);
    bool globalNode(Node::Ptr &);
    bool include();

    bool whiteSpace();
    void maybeSpace();
    bool trash();
    bool define();




    std::vector<Token> tokens;
    int tpos;

    bool  eof();
    Token next();
    int   save();
    void  restore(int);

};


#endif
