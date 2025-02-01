#define FUNC_MAP_DEFINE
#include "parser.h"
#include "darray.h"

Statements* funcs_find(FuncMap* funcs, Atom* name) {
    Function* func;
    if((func=func_map_get(funcs, name))) return func->scope;
    return NULL;
}
void funcs_insert(FuncMap* funcs, Atom* name, Type* id, Statements* scope) {
    assert(func_map_insert(funcs, name, (Function){id, scope}));
}
void parser_create(Parser* this, Lexer* lexer, Arena* arena, ProgramState* state) {
    memset(this, 0, sizeof(*this));
    this->arena = arena;
    this->lexer = lexer;
    this->state = state;
    // scope_init(&this->global);
}
Type* parse_type(Parser* parser) {
    // TODO: Pointers
    Token t = {0};
    t = lexer_next(parser->lexer);
    size_t ptr_count = 0;
    while(t.kind == '*') {
        ptr_count++;
        t = lexer_next(parser->lexer);
    }
    if(t.kind != TOKEN_ATOM) {
        eprintfln("ERROR:%s: Expected name of type but got: %s", tloc(t), tdisplay(t));
        exit(1);
    }
    Type** idp = type_table_get(&parser->state->type_table, t.atom->data);
    if(!idp) {
        eprintfln("ERROR:%s: Unknown type name: %s", tloc(t), t.atom->data);
        exit(1);
    }
    Type* id = *idp;
    if(ptr_count) return type_ptr(parser->arena, id, ptr_count);
    return id;
}
void parse_func_signature(Parser* parser, FuncSignature* sig) {
    Token t = {0};
    if((t=lexer_next(parser->lexer)).kind != '(') {
        eprintfln("ERROR:%s: Expected '(' but found %s in function signature", tloc(t), tdisplay(t));
        exit(1);
    }
    for(;;) {
        t = lexer_peak_next(parser->lexer);
        if(t.kind == ')') break;
        Atom* name = NULL;
        if(t.kind == TOKEN_ATOM) {
            name = t.atom;
            lexer_eat(parser->lexer, 1);
        }
        t = lexer_next(parser->lexer);
        if(t.kind != ':') {
            eprintfln("ERROR:%s: Expected ':' before argument type but found: %s", tloc(t), tdisplay(t));
            exit(1);
        }
        Type* typeid = parse_type(parser);
        if(!typeid) {
            eprintfln("ERROR:%s: Invalid type in signature", tloc(t));
            exit(1);
        }
        t = lexer_peak_next(parser->lexer);
        da_push(&sig->input, ((Arg){ name, typeid }));
        if(t.kind == ')') {
            break;
        } else if (t.kind == ',') {
            lexer_eat(parser->lexer, 1);
        } else {
            eprintfln("ERROR: %s: Expected ')' or ',' but found %s in function signature", tloc(t), tdisplay(t));
            exit(1);
        }
    } 
    if((t=lexer_next(parser->lexer)).kind != ')') {
        eprintfln("ERROR:%s: Expected ')' but found %s in function signature",tloc(t),tdisplay(t));
        exit(1);
    }
    sig->output = NULL;
    if(lexer_peak_next(parser->lexer).kind == TOKEN_ARROW) {
        lexer_eat(parser->lexer, 1);
        sig->output = parse_type(parser);
        if(!sig->output) {
            eprintfln("ERROR:%s: Invalid return type",tloc(t));
            exit(1);
        }
    }
}

AST* parse_ast(Parser* parser);
AST* parse_basic(Parser* parser) {
    Token t = lexer_next(parser->lexer);
    switch(t.kind) {
    case '(': {
        AST* v = parse_ast(parser);
        if(!v) return NULL;
        Token t2 = lexer_peak_next(parser->lexer);
        if(t2.kind != ')') {
            eprintfln("ERROR:%s: Expected closing paren but got %s", tloc(t2), tdisplay(t2));
            return NULL;
        }
        lexer_eat(parser->lexer, 1);
        return v;
    } break;
    case TOKEN_ATOM:
        return ast_new_symbol(parser->arena, t.atom);
    case TOKEN_C_STR:
        return ast_new_cstr(parser->arena, t.str, t.str_len); 
    case TOKEN_INT:
        return ast_new_int(parser->arena, t.integer.type, t.integer.value); 
    default:
        eprintfln("ERROR:%s: Unexpected token in expression: %s", tloc(t),tdisplay(t));
        exit(1);
    }
}

#define OPS \
    X('+') \
    X('=') \
    X('&') \
    X(TOKEN_NEQ) \
    X(TOKEN_EQEQ)

// https://en.cppreference.com/w/cpp/language/operator_precedence
int op_prec(int op) {
    switch(op) {
    case '+':
    case '-':
        return 6;
    case TOKEN_NEQ:
    case TOKEN_EQEQ:
        return 10;
    case '&':
        return 11;
    case '=':
        return 16;
    default:
        unreachable("op=%d",op);
        return -1;
    }
}

AST* parse_ast(Parser* parser);
AST* parse_astcall(Parser* parser, AST* what) {
    Token t;
    if((t=lexer_next(parser->lexer)).kind != '(') {
        eprintfln("ERROR:%s: Expected '(' but found %s in function call", tloc(t), tdisplay(t));
        exit(1);
    }
    CallArgs args = {0};
    for(;;) {
        t = lexer_peak_next(parser->lexer);
        if(t.kind == ')') break;
        AST* value = parse_ast(parser);
        if(!value) {
            call_args_dealloc(&args);
            return NULL;
        }
        da_push(&args, value);
        t = lexer_peak_next(parser->lexer);
        if(t.kind == ')') {
            break;
        } else if (t.kind == ',') {
            lexer_eat(parser->lexer, 1);
        } else {
            eprintfln("ERROR: %s: Expected ')' or ',' but found %s in function call", tloc(t), tdisplay(t));
            exit(1);
        }
    } 
    if((t=lexer_next(parser->lexer)).kind != ')') {
        eprintfln("ERROR:%s: Expected ')' but found %s in function call",tloc(t),tdisplay(t));
        exit(1);
    }
    return ast_new_call(parser->arena, what, args);
}
AST* parse_deref(Parser* parser) {
    assert(lexer_next(parser->lexer).kind == '*');
    AST* rhs = parse_ast(parser);
    if(!rhs) return NULL;
    return ast_new_unary(parser->arena, '*', rhs);
}
AST* parse_ast(Parser* parser) {
    // eprintfln("parse_ast");
    Token t;
    AST* v = NULL;
    t = lexer_peak_next(parser->lexer);
    bool running = true;
    // FIXME: Make this into basic expression as well as function call. Thats kinda important
    switch(t.kind) {
    case '*':
        v = parse_deref(parser);
        break;
    default:
        // eprintfln("(thingy) (parse_basic)");
        v = parse_basic(parser);
        break;
    }
    if(!v) return NULL;
    while(running) {
        t = lexer_peak_next(parser->lexer);
        switch(t.kind) {
        case '(': {
            v = parse_astcall(parser, v);
        } break;
        #define X(op) case op:
        OPS
        #undef X
        {
            int op = t.kind;
            int precedence = op_prec(op);
            lexer_eat(parser->lexer, 1);
            AST* v2 = parse_basic(parser);
            if(!v2) return NULL;
            t = lexer_peak_next(parser->lexer);
            switch(t.kind) {
            case '(':
                v = ast_new_binop(parser->arena, op, v, parse_astcall(parser, v2));
                break;
            #define X(op) case op:
            OPS 
            #undef X
            {
                int newop = t.kind;
                int newprecedence = op_prec(newop);
                if (precedence >= newprecedence) {
                    lexer_eat(parser->lexer, 1);
                    AST* v3 = parse_ast(parser);
                    if(!v3) return NULL;
                    v2 = ast_new_binop(parser->arena, newop, v2, v3);
                }
                v = ast_new_binop(parser->arena, op, v, v2);
            } break;
            default:
                v = ast_new_binop(parser->arena, op, v, v2);
                break;
            }
        } break;
        default:
            running = false;
            break;
        }
    }
    return v;
}

Statement* parse_statement(Parser* parser, Token t);
Statement* parse_scope(Parser* parser) {
    Token t;
    assert((t=lexer_next(parser->lexer)).kind == '{');
    Statement* scope = statement_scope(parser->arena);
    while((t=lexer_peak_next(parser->lexer)).kind != '}') {
        if(t.kind >= TOKEN_END) {
            if(t.kind >= TOKEN_ERR) {
                eprintfln("ERROR:%s: Lexer error %s", tloc(t), tdisplay(t));
                exit(1);
            } else {
                eprintfln("ERROR:%s: Unexpected token in scope body: %s", tloc(t), tdisplay(t));
                exit(1);
            }
        }
        if(t.kind == ';') {
            lexer_eat(parser->lexer, 1);
            continue;
        }
        da_push(scope->as.scope, parse_statement(parser, t));
    }
    t = lexer_next(parser->lexer);
    if(t.kind != '}') {
        eprintfln("ERROR:%s: Expected '}' at the end of function body, but found: %s", tloc(t), tdisplay(t));
        exit(1);
    }
    return scope;
}
Statement* parse_body(Parser* parser) {
    Token t = lexer_peak_next(parser->lexer);
    switch(t.kind) {
    case '{':
        return parse_scope(parser);
    default:
        eprintfln("ERROR:%s: body must either start with `then` or `{`. Found %s", tloc(t), tdisplay(t));
        exit(1);
    }
    return NULL;
}
Statement* parse_statement(Parser* parser, Token t) {
    switch(t.kind) {
        case TOKEN_RETURN: {
            lexer_eat(parser->lexer, 1);
            AST* ast = parse_ast(parser);
            if(!ast) {
                eprintfln("ERROR:%s: Failed to parse return statement",tloc(t));
                exit(1);
            }
            return statement_return(parser->arena, ast);
        } break;
        case '{':
            return parse_scope(parser);
        case TOKEN_WHILE: {
            lexer_eat(parser->lexer, 1);
            AST* ast = parse_ast(parser);
            if(!ast) {
                eprintfln("ERROR:%s: Failed to parse condition",tloc(t));
                exit(1);
            }
            return statement_while(parser->arena, ast, parse_body(parser));
        }
    }
    AST* ast = parse_ast(parser);
    if(!ast) {
        eprintfln("ERROR:%s: Unknown token in statement: %s", tloc(t), tdisplay(t));
        exit(1);
    }
    return statement_eval(parser->arena, ast);
}
void parse_func_body(Parser* parser, Statements* s) {
    Token t;
    while((t=lexer_peak_next(parser->lexer)).kind != '}') {
        if(t.kind >= TOKEN_END) {
            if(t.kind >= TOKEN_ERR) {
                eprintfln("ERROR:%s: Lexer error %s", tloc(t), tdisplay(t));
                exit(1);
            } else {
                eprintfln("ERROR:%s: Unexpected token in function body: %s", tloc(t), tdisplay(t));
                exit(1);
            }
        }
        if(t.kind == ';') {
            lexer_eat(parser->lexer, 1);
            continue;
        }
        da_push(s, parse_statement(parser, t));
    }
    t = lexer_next(parser->lexer);
    if(t.kind != '}') {
        eprintfln("ERROR:%s: Expected '}' at the end of function body, but found: %s", tloc(t), tdisplay(t));
        exit(1);
    }
}
void parse(Parser* parser, Lexer* lexer, Arena* arena) {
    Token t;
    while((t=lexer_peak_next(parser->lexer)).kind != TOKEN_EOF) {
        if(t.kind >= TOKEN_END) {
            eprintfln("ERROR:%s: Lexer: %s", tloc(t), tdisplay(t));
            exit(1);
        }
        static_assert(TOKEN_COUNT == 269, "Update parser");
        switch(t.kind) {
        case TOKEN_EXTERN: {
            lexer_eat(parser->lexer, 1);
            if((t = lexer_next(parser->lexer)).kind == TOKEN_ATOM && lexer_peak_next(parser->lexer).kind == ':' && lexer_peak(parser->lexer, 1).kind == ':' && lexer_peak(parser->lexer, 2).kind == '(') {
                Atom* name = t.atom;
                lexer_eat(parser->lexer, 2);
                Type* fid = type_new(parser->arena);
                fid->core    = CORE_FUNC;
                fid->attribs = TYPE_ATTRIB_EXTERN;
                parse_func_signature(parser, &fid->signature);
                funcs_insert(&parser->state->funcs, name, fid, NULL);
            } else {
                eprintfln("ERROR:%s: Expected signature of external function to follow the syntax:", tloc(t));
                eprintfln("  extern <func name> :: <(<Arguments>)> (-> <Output Type>)");
                exit(1);
            }
        } break;
        case TOKEN_ATOM: {
            if(lexer_peak(parser->lexer, 1).kind == ':' && lexer_peak(parser->lexer, 2).kind == ':' && lexer_peak(parser->lexer, 3).kind == '(') {
                Atom* name = t.atom;
                lexer_eat(parser->lexer, 3);
                Type* fid = type_new(parser->arena);
                fid->core = CORE_FUNC;

                parse_func_signature(parser, &fid->signature);
                if((t=lexer_next(parser->lexer)).kind != '{') {
                    eprintfln("ERROR:%s: Missing '{' at the start of function. Got: %s", tloc(t), tdisplay(t));
                    exit(1);
                }
                Statements* s = scope_new(parser->arena);
                parse_func_body(parser, s);
                funcs_insert(&parser->state->funcs, name, fid, s);
            } else if (lexer_peak(parser->lexer, 1).kind == ':') {
                Atom* name = t.atom;
                lexer_eat(parser->lexer, 2);
                if(lexer_peak_next(parser->lexer).kind == ':') {
                    eprintfln("ERROR:%s: Type inference in constant defintion isn't allowed yet", tloc(t));
                    exit(1);
                }
                Type* type = parse_type(parser);
                if((t=lexer_next(parser->lexer)).kind != ':') {
                    eprintfln("ERROR:%s: Expected constant to follow the syntax: ", tloc(t));
                    eprintfln("  <const name> : (type) : <expression>");
                    exit(1);
                }
                AST* ast = parse_ast(parser);
                const_tab_insert(&parser->state->consts, name, const_new(arena, ast, type));
            } else {
                eprintfln("ERROR:%s: Unexpected Atom: %s",tloc(t), t.atom->data);
                exit(1);
            }
        } break;
        case ';':
            lexer_eat(parser->lexer, 1);
            break;
        default:
            eprintfln("ERROR:%s:  Unexpected token: %s", tloc(t), tdisplay(t));
            exit(1);
        }
    }
}
