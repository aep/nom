#include "parser.hpp"
#include "utils.hpp"

#include <fstream>

MakefileParser::MakefileParser()
    : lexer(0)
    , tpos(0)
{

}

Node::Ptr MakefileParser::operator()(MakefileLexer *l)
{
    lexer = l;

    try {
        Node::Ptr node;
        if (globalNode(node)) return node;

        std::cerr << "aborted." << std::endl;
        for (;;) {
            Token t = next();
            std::cerr  << t.dump() << std::endl;
            if (t.type == Token::Eof)
                return Node::Ptr(0);
        }
    } catch (ParserException &e) {
        std::cerr << std::endl;

        if (e.token < 0)
            e.token = save();
        restore(e.token - 1);

        std::cerr << e.text << std::endl;
        std::cerr << next().dump() << std::endl;
        return Node::Ptr(0);
    }
}
bool MakefileParser::shellCommand(ShellCommand::Ptr &cmd)
{
    Token n = next();
    if (n.type != Token::Tab)
        return false;

    //TODO just getting everything until Eol for now
    for (;;) {
        Token bla = next();
        if (bla.type == Token::Eol || bla.type == Token::Eof)
            break;
    }

    ShellCommand *c = new ShellCommand;

    cmd = ShellCommand::Ptr(c);
    return true;
}

bool MakefileParser::generatorNode(Node::Ptr &node)
{
    std::vector<Expression::Ptr> lhs;
    if (!expressionList(lhs))
        return false;


    maybeSpace();

    if (next().type != Token::Colon)
        return false;

    //TODO just getting everything until Eol for now
    for (;;) {
        Token bla = next();
        if (bla.type == Token::Eol || bla.type == Token::Eof)
            break;
    }

    Generator *g  = new Generator;


    ShellCommand::Ptr sh;
    int p = save();
    for (;;) {
        p = save();
        if (shellCommand(sh)) {
            g->commands.push_back(sh);
            continue;
        }
        restore(p);
        if (trash())
            continue;

        restore(p);
        Node::Ptr cond;
        if (ifConditionNode(cond)) {
            std::cerr << "warning: condition inside generator ignored" << std::endl;
            continue;
        }
        restore(p);
        if (elseConditionNode(cond)) {
            std::cerr << "warning: condition inside generator ignored" << std::endl;
            continue;
        }
        restore(p);
        if (endifConditionNode(cond)) {
            std::cerr << "warning: condition inside generator ignored" << std::endl;
            continue;
        }
        restore(p);
        break;
    }

    node = Node::Ptr(g);
    return true;
}

bool MakefileParser::ifConditionNode(Node::Ptr &node)
{
    Token t = next();
    if (t.type != Token::Literal)
        return false;

    if (!startsWith(t.a, "if"))
        return false;

    if (!whiteSpace())
        return false;

    //TODO: just get everything until Eol for now
    for (;;) {
        Token bla = next();
        if (bla.type == Token::Eol || bla.type == Token::Eof)
            break;
    }

    IfCondition *i = new IfCondition;

    node = Node::Ptr(i);
    return true;
}

bool MakefileParser::elseConditionNode(Node::Ptr &node)
{
    Token t = next();
    if (t.type != Token::Literal)
        return false;

    if (t.a != "else")
        return false;

    node = Node::Ptr(new ElseCondition);
    return true;
}

bool MakefileParser::endifConditionNode(Node::Ptr &node)
{
    Token t = next();
    if (t.type != Token::Literal)
        return false;

    if (t.a != "endif")
        return false;

    node = Node::Ptr(new EndifCondition);
    return true;
}

bool MakefileParser::assignmentNode(Node::Ptr &node)
{
    Expression::Ptr lhs;
    if (!expressionNode(lhs))
        return false;

    maybeSpace();
    Token op  = next();
    if (op.type != Token::Operator)
        return false;

    maybeSpace();


    int p = save();
    std::vector<Expression::Ptr> rhs;
    if (!expressionList(rhs, RHSContext)) {
        rhs = std::vector<Expression::Ptr>();
        restore (p);
        Token t = next();
        if (t.type != Token::Eol) {
            throw ParserException("invalid expressionlist");
            return false;
        }
    }

    try {
        std::vector<std::string> elhs = eval(lhs);
        if (rhs.size() == 1 && elhs.size() == 1 && elhs.at(0) == "LOCAL_PATH") {
            std::vector<std::string> erhs = eval(rhs.at(0));
            if (erhs.size() == 1) {
                rhs.clear();
                rhs.push_back(Expression::Ptr(new LiteralExpression(erhs.at(0))));
            }
        }
    } catch (...) {}

    Assignment *i = new Assignment;
    i->lhs = lhs;
    i->rhs = rhs;
    i->op  = op.a;
    node = Node::Ptr(i);
    return true;
}

bool MakefileParser::literalExpressionNode(Expression::Ptr &node)
{
    Token literal = next();
    if (literal.type != Token::Literal)
        return false;

    LiteralExpression *n =  new LiteralExpression;
    n->literal = literal.a;
    node = Expression::Ptr(n);
    return true;
}

bool MakefileParser::referenceExpressionNode(Expression::Ptr &node)
{
    Token t = next();
    if (t.type != Token::Expression) {
        if (t.type != Token::ShortExpression)
            return false;

        ReferenceExpression *n =  new ReferenceExpression;
        n->reference = t.a;
        node = Expression::Ptr(n);
        return true;
    }
    if (next().type != Token::BraceOpen)
        return false;

    Token literal = next();
    if (literal.type != Token::Literal)
        return false;
    if (next().type != Token::BraceClose)
        return false;


    ReferenceExpression *n =  new ReferenceExpression;
    n->reference = literal.a;
    node = Expression::Ptr(n);
    return true;
}

bool MakefileParser::substitutionReferenceExpressionNode(Expression::Ptr &node)
{
    if (next().type != Token::Expression)
        return false;
    if (next().type != Token::BraceOpen)
        return false;

    Token literal = next();
    if (literal.type != Token::Literal)
        return false;

    if (next().type != Token::Colon)
        return false;

    Expression::Ptr pattern;
    if (!expressionNode(pattern, SubstitutionContext))
        return false;

    if (next().type != Token::Operator)
        return false;

    Expression::Ptr replacement;
    if (!expressionNode(replacement, SubstitutionContext))
        return false;

    if (next().type != Token::BraceClose)
        return false;


    ReferenceExpression *n =  new ReferenceExpression;
    n->reference = literal.a;

    CallExpression *c = new CallExpression;
    c->functionName = Expression::Ptr(new LiteralExpression("patsubst"));
    c->args = {
         std::vector<Expression::Ptr>({ pattern }),
         std::vector<Expression::Ptr>({ replacement }),
         std::vector<Expression::Ptr>({ Expression::Ptr(n) })
    };

    node = Expression::Ptr(c);
    return true;
}

//   Expression = ReferenceExpression | CallExpression | LiteralExpression [ Expression ]

bool MakefileParser::expressionNode(Expression::Ptr &node, ExpressionContext ctx)
{
    std::vector<Expression::Ptr> expr;
    for (;;) {
        Expression::Ptr node2;
        int p = save();
        if (referenceExpressionNode(node2)) {
            expr.push_back(node2);
            continue;
        }
        restore(p);
        if (substitutionReferenceExpressionNode(node2)) {
            expr.push_back(node2);
            continue;
        }
        restore(p);
        if (literalExpressionNode(node2)) {
            expr.push_back(node2);
            continue;
        }
        restore(p);
        if (callExpressionNode(node2)) {
            expr.push_back(node2);
            continue;
        }
        restore(p);
        if (ctx == RHSContext || ctx == CallContext || ctx == SubstitutionContext) {
            // operators within a RHS are not arithmetic, they are literal. ugh.
            Token maybeOp = next();

            if (maybeOp.type == Token::Pipe) {
                expr.push_back(Expression::Ptr(new LiteralExpression("|")));
                continue;
            }
            if (ctx == RHSContext || ctx == CallContext) {
                if (maybeOp.type == Token::Operator) {
                    expr.push_back(Expression::Ptr(new LiteralExpression(maybeOp.a)));
                    continue;
                }
                if (maybeOp.type == Token::Colon) {
                    expr.push_back(Expression::Ptr(new LiteralExpression(":")));
                    continue;
                }
            }
            if (ctx == RHSContext || ctx == SubstitutionContext) {
                if (maybeOp.type == Token::Comma) {
                    expr.push_back(Expression::Ptr(new LiteralExpression(",")));
                    continue;
                }
            }
            restore(p);
        }
        restore(p);
        break;
    }
    if (expr.size()) {
        node = Expression::Ptr(new Expression);
        node->expression = expr;
        return true;
    }
    return false;
}

//   ExpressionList = Expression [ Space ExpressionList ]


bool MakefileParser::expressionList(std::vector<Expression::Ptr> &expr, ExpressionContext ctx)
{
    for (;;) {
        Expression::Ptr node;
        int p = save();
        if (!expressionNode(node, ctx)) {
            restore(p);
            break;
        }
        expr.push_back(node);
        p = save();
        if (!whiteSpace()) {
            restore(p);
            break;
        }
    }
    return expr.size();
}


// $ ( Expression [ ExpressionList [ , ExpressionList ] ] )
// note that args are two dimensional. first divided by comma then by space

bool MakefileParser::callExpressionNode(Expression::Ptr &node)
{
    if (next().type != Token::Expression)
        return false;
    if (next().type != Token::BraceOpen)
        return false;

    Expression::Ptr arg0;
    if (!expressionNode(arg0))
        return false;

    if (!whiteSpace())
        return false;


    std::vector<std::vector<Expression::Ptr> > args;

    for (;;) {
        std::vector<Expression::Ptr> argl;

        int p = save();
        if (!expressionList(argl, CallContext)) {
            restore(p);

            //is it an empty argument?

            if (next().type == Token::Comma) {
                args.push_back(argl);
                continue;
            } else {
                restore(p);
                break;
            }
        }
        p = save();
        args.push_back(argl);
        if (next().type != Token::Comma) {
            restore(p);
            break;
        }

        maybeSpace();

    }

    if (args.size() < 1) {
        return false;
    }


    if (next().type != Token::BraceClose) {
        throw ParserException("expected ')' ,  ',' or literal");
    }

    CallExpression *c = new CallExpression;
    c->functionName = arg0;
    c->args = args;
    node = Expression::Ptr(c);
    return true;
}

bool MakefileParser::semanticIncludeExpression(Node::Ptr &node)
{
    return false;
    Token t = next();
    if (t.type != Token::Literal)
        return false;

    if (t.a != ("include"))
        return false;

    if (!whiteSpace())
        return false;

    Expression::Ptr r_;
    if (!referenceExpressionNode(r_)) return false;
    ReferenceExpression::Ptr r = std::static_pointer_cast<ReferenceExpression>(r_);

    if (r->reference == "CLEAR_VARS") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_STATIC_LIBRARY") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_SHARED_LIBRARY") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_EXECUTABLE") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_HOST_SHARED_LIBRARY") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_HOST_STATIC_LIBRARY") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_HOST_JAVA_LIBRARY") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_HOST_EXECUTABLE") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_PACKAGE") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_HOST_PREBUILT") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_STATIC_JAVA_LIBRARY") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_JAVA_LIBRARY") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_DROIDDOC") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_RAW_EXECUTABLE") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_MULTI_PREBUILT") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_PREBUILT") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_NATIVE_TEST") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_CTS_EXECUTABLE") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_CTS_PACKAGE") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_CTSCORE_PACKAGE") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_CTS_HOST_JAVA_LIBRARY") {
        node = Node::Ptr(new SemanticIncludeNode(r->reference));
        return true;
    }
    if (r->reference == "BUILD_SYSTEM") {
        int p = save();

        Token l = next();
        if (l.type == Token::Literal && l.a == "/dynamic_binary.mk" ) {
            node = Node::Ptr(new SemanticIncludeNode("dynamic_binary"));
            //TODO
            context.values["linked_module"] = std::vector<Expression::Ptr>({
                    Expression::Ptr(new LiteralExpression("linked_module")) });
            return true;
        } else if (l.type == Token::Literal && l.a == "/binary.mk" ) {
            node = Node::Ptr(new SemanticIncludeNode("binary"));
            return true;
        } else if (l.type == Token::Literal && l.a == "/base_rules.mk" ) {
            node = Node::Ptr(new SemanticIncludeNode("base_rules"));
            return true;
        } else {
            throw ParserException("undefined semantic buildsystem include \"" +  l.a + "\"");
        }
        restore(p);
    }

    return false;
}

bool MakefileParser::include()
{
    Token t = next();
    if (t.type != Token::Literal)
        return false;

    if (t.a != ("include"))
        return false;

    if (!whiteSpace())
        return false;

    int p = save();

    std::vector<Expression::Ptr> rhs;
    if (!expressionList(rhs)) {
        throw ParserException( "include expects argument ");
    }

    if (next().type != Token::Eol) {
        throw ParserException( "expected end of line");
    }

    std::vector<std::string> includes;
    for (auto i = rhs.begin(); i != rhs.end(); i++) {
        amend(includes, eval(*i));
    }


    for (auto i = includes.begin(); i != includes.end(); i++) {
        std::ifstream *in = new std::ifstream(*i);
        lexer->insert(in, *i);
    }
    return true;
}

bool MakefileParser::whiteSpace()
{
    bool had = false;
    for (;;) {
        int p = save();
        switch (next().type) {
            case Token::Space:
            case Token::Tab:
            case Token::Comment:
                had = true;
                continue;
            default:
                restore(p);
                return had;
        }
    }
    return false;
}
void MakefileParser::maybeSpace()
{
    int p = save();
    if (!whiteSpace())
        restore(p);
}

bool MakefileParser::trash()
{
    switch (next().type) {
        case Token::Empty:
        case Token::Comment:
        case Token::Space:
        case Token::Tab:
        case Token::Eol:
            return true;
        default:
            return false;
    };
}

bool MakefileParser::define()
{
    Token t = next();
    if (t.type  != Token::Literal)
        return false;

    if (t.a != "define")
        return false;

    if (!whiteSpace()) {
        throw ParserException("expected whitespace");
        return false;
    }

    t = next();
    if (t.type  != Token::Literal) {
        throw ParserException("expected function name");
        return false;
    }

    std::cerr << "warning: ignoring definition of: \"" << t.a << "\"" << std::endl;

    while (t.type != Token::Eof) {
        t = next();
        if (t.type == Token::Literal)
            if (t.a == "endef")
                return true;
    }
    throw ParserException("premature end of file looking for 'endef'");
    return false;
}

bool MakefileParser::globalNode(Node::Ptr &node)
{
    std::vector<Node::Ptr> items;
    for (;;) {
        Node::Ptr node;
        Expression::Ptr exprn;
        if (eof())
            break;

        int p = save();

        if (trash()) goto success;
        restore(p);

        if (define()) goto success;
        restore(p);

        if (semanticIncludeExpression(node)) goto success;
        restore(p);

        if (include()) goto success;
        restore(p);

        if (assignmentNode(node)) {
            Assignment::Ptr as = std::static_pointer_cast<Assignment>(node);
            std::vector<std::string> lhs = eval(as->lhs);
            if (lhs.size() != 1)
                throw ParserException("internal error: LHS of assignment is not size 1");

            if (as->op == ":=") {
                std::vector<Expression::Ptr> in;
                for (auto i = as->rhs.begin(); i != as->rhs.end(); i++) {
                    auto e = eval(*i);
                    for (auto j = e.begin(); j != e.end(); j++) {
                        in.push_back(Expression::Ptr(new LiteralExpression(*j)));
                    }
                }
                context.values[lhs.at(0)] = in;
            } else if (as->op == "+=") {
                std::vector<Expression::Ptr> in;
                for (auto i = as->rhs.begin(); i != as->rhs.end(); i++) {
                    auto e = eval(*i);
                    for (auto j = e.begin(); j != e.end(); j++) {
                        in.push_back(Expression::Ptr(new LiteralExpression(*j)));
                    }
                }
                amend(context.values[lhs.at(0)], in);
            } else if (as->op == "=") {
                context.values[lhs.at(0)] = as->rhs;
            } else if (as->op == "?=") {
                if (!context.values.count(lhs.at(0))) {
                    std::vector<Expression::Ptr> in;
                    for (auto i = as->rhs.begin(); i != as->rhs.end(); i++) {
                        auto e = eval(*i);
                        for (auto j = e.begin(); j != e.end(); j++) {
                            in.push_back(Expression::Ptr(new LiteralExpression(*j)));
                        }
                    }
                    context.values[lhs.at(0)] = in;
                }
            } else {
                throw ParserException("unknown op " + as->op);
            }

            goto success;
        }
        restore(p);

        if (ifConditionNode(node)) goto success;
        restore(p);

        if (elseConditionNode(node)) goto success;
        restore(p);

        if (endifConditionNode(node)) goto success;
        restore(p);

        if (generatorNode(node)) goto success;
        restore(p);

        // TODO: top level expressions may yield more rules
        if (callExpressionNode(exprn)) {
            // if an assignment with a complex LHS doesn't get parsed correctly,
            // we end up eating the LHS here, which is very confusing during debugging
            // so lets make sure we're followed by EOL
            int pp = save();
            whiteSpace();
            if (next().type != Token::Eol) {
                throw ParserException("internal error: I accidently the whole expression.", pp - 1 );
            }
            restore(pp);
            goto success;
        }
        restore(p);

        // happens when the rest was comments
        if (eof())
            break;
        return false;
success:
        if (node)
            items.push_back(node);
        continue;
    }
    Global *g = new Global;
    g->items = items;
    node = Node::Ptr(g);
    return true;
}


// token buffering

bool MakefileParser::eof()
{
    while (tpos + 1 > tokens.size()) {
        Token t;
        t.type = lexer->next(t);
        tokens.push_back(t);
    }
    return (tokens.back().type == Token::Eof);
}

Token MakefileParser::next()
{
    while (tpos + 1 > tokens.size()) {
        Token t;
        t.type = lexer->next(t);
        tokens.push_back(t);
    }

    if (tokens.at(tpos).type == Token::Comment) {
        ++tpos;
        return next();
    }

    return tokens.at(tpos++);
}

int MakefileParser::save()
{
    return tpos;
}

void MakefileParser::restore(int i)
{
    tpos = i;
}

std::vector<std::string> MakefileParser::eval(Expression::Ptr expr)
{
    if (expr->type == Node::LiteralExpression) {
        LiteralExpression::Ptr lit = std::static_pointer_cast<LiteralExpression>(expr);
        return  std::vector<std::string>({lit->literal});
    } else if (expr->type == Node::ReferenceExpression) {
        ReferenceExpression::Ptr r = std::static_pointer_cast<ReferenceExpression>(expr);

        if (!context.values.count(r->reference)) {
            //weird gnumake behaviour: user defined functions may be called without "call"
            //is this the reason android only builds with gnumake 3.81 ?

            if (context.functions.count(r->reference)) {
                Context::Function f = context.functions[r->reference];
                try {
                    return f(this, std::vector<std::vector<std::string> >());
                } catch (...) {}
            }

            throw ParserException("undefined reference \"" + r->reference + "\"");
        }

        std::vector<std::string> ret;
        for (auto i = context.values[r->reference].begin(); i != context.values[r->reference].end(); i++) {
            amend(ret,eval(*i));
        }
        return ret;

    } else if (expr->type == Node::CallExpression) {
        CallExpression::Ptr r = std::static_pointer_cast<CallExpression>(expr);

        std::vector<std::vector<std::string> > sargs;
        for (auto i = r->args.begin(); i != r->args.end(); i++) {
            std::vector<std::string> s;
            for (auto j = i->begin(); j != i->end(); j++) {
                amend(s, eval(*j));
            }
            sargs.push_back(s);
        }

        std::string functionName = eval(r->functionName).at(0);


        if (functionName == "call") {
            if (sargs.size () < 1)
                throw ParserException("expecting functionname argument to call");

            if (sargs.at(0).size() != 1)
                throw ParserException("first argument to 'call' may not contain spaces");

            functionName = sargs.front().front();
            sargs.erase(sargs.begin());
        }

        if (context.functions.count(functionName)) {
            Context::Function f = context.functions[functionName];
            return f(this, sargs);
        }

        throw ParserException("undefined function \"" + functionName + "\"");

    } else if (expr->type == Node::Expression) {
        if (expr->expression.size() == 1) {
            return eval(expr->expression.at(0));
        } else {
            std::string ret;
            for (auto i = expr->expression.begin(); i != expr->expression.end(); i++) {
                std::vector<std::string> lr = eval(*i);
                if (lr.size() > 1) {
                    throw ParserException("internal error: nested eval returned multiple values."
                            "In makefile terms this means the expressions result contains spaces. can't handle this.");
                }
                if (lr.size() > 0)
                    ret += lr.front();
            }
            return std::vector<std::string>({ ret });
        }
    }

    throw ParserException("internal error: undefined type of expression");
}



std::string Node::dump() const
{
    std::string m;
    switch (type)
    {
        case Global:
            m = "Global";
            break;
        case Expression:
            m = "Expression";
            break;
        case ReferenceExpression:
            m = "ReferenceExpression";
            break;
        case LiteralExpression:
            m = "\"" + ((::LiteralExpression*)(this))->literal + "\"" ;
            break;
        case CallExpression:
            m = "CallExpression";
            break;
        case IfCondition:
            m = "IfCondition";
            break;
        case ElseCondition:
            m = "ElseCondition";
            break;
        case EndifCondition:
            m = "EndifCondition";
            break;
        case ShellCommand:
            m = "ShellCommand";
            break;
        case Generator:
            m = "Generator";
            break;
        case Assignment:
            m = "Assignment";
            break;
        case SemanticInclude:
            m = "SemanticInclude (" + ((SemanticIncludeNode*)(this))->expr + ")";
            break;
        case Invalid:
        default:
            m = "Invalid";
            break;
    };
    return m;
}



