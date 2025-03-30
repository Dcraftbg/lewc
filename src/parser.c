#define FUNC_MAP_DEFINE
#include "parser.h"
#include "darray.h"
#include "statement.h"

void parser_create(Parser* this, Lexer* lexer, Arena* arena, Module* module) {
    memset(this, 0, sizeof(*this));
    this->arena = arena;
    this->lexer = lexer;
    this->module = module;
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
    switch(t.kind) {
    case '[': {
        Type* of = parse_type(parser);
        if(!of) return NULL;
        t = lexer_next(parser->lexer);
        switch(t.kind) {
        case ';': {
            size_t len = 0;
            // TODO: Make this an expression instead of constant integer
            switch((t=lexer_next(parser->lexer)).kind) {
            case TOKEN_INT:
                // TODO: Typecheck this sheizung
                len = t.integer.value;
                break;
            default:
                eprintfln("ERROR %s: Expected integer N for array length (i.e. [<Type>;N], but got %s)", tloc(t), tdisplay(t));
                exit(1);
            }
            if((t=lexer_next(parser->lexer)).kind != ']') {
                eprintfln("ERROR %s Expected `]` after size to end the array (i.e. [<Type>;N] but got %s)", tloc(t), tdisplay(t));
                exit(1);
            }
            return type_new_const_array(parser->arena, of, len);
        } break;
        default:
            eprintfln("ERROR %s: Expected `;` after type to indicate size (i.e. [<Type>;N]) but got %s", tloc(t), tdisplay(t));
            exit(1);
        }
    } break;
    case TOKEN_ATOM: {
        Type** idp = type_table_get(&parser->module->type_table, t.atom->data);
        if(!idp) {
            eprintfln("ERROR %s: Unknown type name: %s", tloc(t), t.atom->data);
            exit(1);
        }
        Type* id = *idp;
        if(ptr_count) return type_ptr(parser->arena, id, ptr_count);
        return id;
    } break;
    case TOKEN_STRUCT: {
        if((t=lexer_next(parser->lexer)).kind != '{') {
            eprintfln("ERROR %s: Unexpected `%s` at the start of structure definition. Expected '{'", tloc(t), tdisplay(t));
            exit(1);
        }
        Struct struc = { 0 };
        while((t=lexer_peak_next(parser->lexer)).kind != '}') {
            if(t.kind != TOKEN_ATOM) {
                eprintfln("ERROR %s Unexpected token in structure definition %s (expected field name)", tloc(t), tdisplay(t));
                exit(1);
            }
            Atom* name = t.atom;
            lexer_eat(parser->lexer, 1);
            if((t=lexer_next(parser->lexer)).kind != ':') {
                eprintfln("ERROR %s Expected : after field name. Found %s", tloc(t), tdisplay(t));
                exit(1);
            }
            Type* type = parse_type(parser);
            if(!type) exit(1);
            struct_add_field(&struc, name, type);
            if((t=lexer_peak_next(parser->lexer)).kind != ',') break;
            lexer_eat(parser->lexer, 1);
        }
        if((t=lexer_next(parser->lexer)).kind != '}') {
            eprintfln("ERROR Expected '}' at the end of a structure definition");
            exit(1);
        }
        return type_new_struct(parser->arena, struc);
    } break;
    default:
        eprintfln("ERROR %s: Expected name of type but got: %s", tloc(t), tdisplay(t));
        exit(1);
    }
}
void parse_func_signature(Parser* parser, FuncSignature* sig) {
    sig->variadic = VARIADIC_NONE;
    Token t = {0};
    if((t=lexer_next(parser->lexer)).kind != '(') {
        eprintfln("ERROR %s: Expected '(' but found %s in function signature", tloc(t), tdisplay(t));
        exit(1);
    }
    for(;;) {
        t = lexer_peak_next(parser->lexer);
        if(t.kind == ')') break;
        Atom* name = NULL;
        if(t.kind == TOKEN_ATOM) {
            name = t.atom;
            lexer_eat(parser->lexer, 1);
        } else if (t.kind == '.' && lexer_peak(parser->lexer, 1).kind == '.' && lexer_peak(parser->lexer, 2).kind == '.') {
            lexer_eat(parser->lexer, 3);
            if ((t = lexer_next(parser->lexer)).kind == '#') {
                if ((t = lexer_next(parser->lexer)).kind != TOKEN_ATOM || strcmp(t.atom->data, "c") != 0) {
                    eprintfln("ERROR %s: Expected c after ... # but found %s", tloc(t), tdisplay(t));
                    exit(1);
                }
                sig->variadic = VARIADIC_C;
            } else {
                eprintfln("ERROR %s: Expected #c after ... but found %s", tloc(t), tdisplay(t));
                exit(1);
            }
            if((t=lexer_peak_next(parser->lexer)).kind != ')') {
                eprintfln("ERROR %s: Expected end of function signature ')' after variadic (...) but got %s", tloc(t), tdisplay(t));
                exit(1);
            }
            break;
        }
        t = lexer_next(parser->lexer);
        if(t.kind != ':') {
            eprintfln("ERROR %s: Expected ':' before argument type but found: %s", tloc(t), tdisplay(t));
            exit(1);
        }
        Type* typeid = parse_type(parser);
        if(!typeid) {
            eprintfln("ERROR %s: Invalid type in signature", tloc(t));
            exit(1);
        }
        t = lexer_peak_next(parser->lexer);
        da_push(&sig->input, ((Arg){ name, typeid }));
        if(t.kind == ')') {
            break;
        } else if (t.kind == ',') {
            lexer_eat(parser->lexer, 1);
        }  else {
            eprintfln("ERROR %s: Expected ')' or ',' but found %s in function signature", tloc(t), tdisplay(t));
            exit(1);
        }
    } 
    if((t=lexer_next(parser->lexer)).kind != ')') {
        eprintfln("ERROR %s: Expected ')' but found %s in function signature",tloc(t),tdisplay(t));
        exit(1);
    }
    sig->output = NULL;
    if(lexer_peak_next(parser->lexer).kind == TOKEN_ARROW) {
        lexer_eat(parser->lexer, 1);
        sig->output = parse_type(parser);
        if(!sig->output) {
            eprintfln("ERROR %s: Invalid return type",tloc(t));
            exit(1);
        }
    }
}

#define INIT_PRECEDENCE (100)
#define BINOPS \
    X('.') \
    X('+') \
    X('-') \
    X('/') \
    X('%') \
    X('*') \
    X('&') \
    X('^') \
    X('|') \
    X('=') \
    X('<') \
    X('>') \
    X(TOKEN_LTEQ) \
    X(TOKEN_GTEQ) \
    X(TOKEN_SHL) \
    X(TOKEN_SHR) \
    X(TOKEN_NEQ) \
    X(TOKEN_EQEQ) \
    X(TOKEN_BOOL_OR) \
    X(TOKEN_BOOL_AND)

// https://en.cppreference.com/w/cpp/language/operator_precedence
int binop_prec(int op) {
    switch(op) {
    case '.':
        return 2;
    case '*':
    case '/':
    case '%':
        return 5;
    case '+':
    case '-':
        return 6;
    case TOKEN_SHL:
    case TOKEN_SHR:
        return 7;
    case '<':
    case TOKEN_LTEQ:
    case '>':
    case TOKEN_GTEQ:
        return 9;
    case TOKEN_NEQ:
    case TOKEN_EQEQ:
        return 10;
    case '&':
        return 11;
    case '^':
        return 12;
    case '|':
        return 13;
    case TOKEN_BOOL_AND:
        return 14;
    case TOKEN_BOOL_OR:
        return 15;
    case '=':
        return 16;
    default:
        unreachable("op=%d",op);
        return -1;
    }
}

#define UNARYOPS \
    X('*') \
    X('&')

// https://en.cppreference.com/w/cpp/language/operator_precedence
int unaryop_prec(int op) {
    switch(op) {
    case '*':
    case '&':
        return 3;
    default:
        unreachable("op=%d",op);
        return -1;
    }
}

AST* parse_ast(Parser* parser, int expr_precedence);
bool peak_is_func(Lexer* lexer) {
#define defer_return(x) do { result=(x); goto defer; } while(0)
    bool result = true;
    Snapshot snap = lexer_snap_take(lexer);
    if(lexer_next(lexer).kind != '(') defer_return(false);
    if(lexer_peak_next(lexer).kind == TOKEN_ATOM) {
        lexer_eat(lexer, 1);
        if(lexer_next(lexer).kind == ':') defer_return(true);
    }
    if(lexer_next(lexer).kind != ')') defer_return(false);
    if(lexer_peak_next(lexer).kind == TOKEN_ARROW) {
        lexer_eat(lexer, 1);
        defer_return(true);
    }
    if(lexer_next(lexer).kind != '{') defer_return(false);
defer:
    lexer_snap_restore(lexer, snap);
    return result;
}
Statement* parse_statement(Parser* parser, Token t);
void parse_func_body(Parser* parser, Statements* s) {
    Token t;
    while((t=lexer_peak_next(parser->lexer)).kind != '}') {
        if(t.kind >= TOKEN_END) {
            if(t.kind >= TOKEN_ERR) {
                eprintfln("ERROR %s: Lexer error %s", tloc(t), tdisplay(t));
                exit(1);
            } else {
                eprintfln("ERROR %s: Unexpected token in function body: %s", tloc(t), tdisplay(t));
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
        eprintfln("ERROR %s: Expected '}' at the end of function body, but found: %s", tloc(t), tdisplay(t));
        exit(1);
    }
}
AST* parse_basic(Parser* parser) {
    Token t;
    if(peak_is_func(parser->lexer)) {
        Type* fid = type_new(parser->arena);
        fid->core    = CORE_FUNC;
        parse_func_signature(parser, &fid->signature);
        if((t=lexer_next(parser->lexer)).kind != '{') {
            eprintfln("ERROR %s: Missing '{' at the start of function. Got: %s", tloc(t), tdisplay(t));
            exit(1);
        }
        Statements* s = scope_new(parser->arena);
        parse_func_body(parser, s);
        // TODO: This step is kinda unnecessary now
        Function* f = func_new(parser->arena, fid, s);
        return ast_new_func(parser->arena, f);
    }
    t = lexer_next(parser->lexer);
    switch(t.kind) {
    #define X(c) case c:
    UNARYOPS
    #undef X 
    {
        AST* rhs = parse_ast(parser, unaryop_prec(t.kind));
        if(!rhs) return NULL;
        return ast_new_unary(parser->arena, t.kind, rhs);
    } break;
    case '(': {
        AST* v = parse_ast(parser, INIT_PRECEDENCE);
        if(!v) return NULL;
        Token t2 = lexer_peak_next(parser->lexer);
        if(t2.kind != ')') {
            eprintfln("ERROR %s: Expected closing paren but got %s", tloc(t2), tdisplay(t2));
            return NULL;
        }
        lexer_eat(parser->lexer, 1);
        return v;
    } break;
    case TOKEN_CAST:
        if((t=lexer_next(parser->lexer)).kind != '(') {
            eprintfln("ERROR %s: Expected '(' after cast keyword but got %s", tloc(t), tdisplay(t));
            return NULL;
        }
        AST* what = parse_ast(parser, INIT_PRECEDENCE);
        if(!what) return NULL;
        if((t=lexer_next(parser->lexer)).kind != ',') {
            eprintfln("ERROR %s: Expected ',' after cast but got %s", tloc(t), tdisplay(t));
            return NULL;
        }
        Type* into = parse_type(parser);
        if((t=lexer_next(parser->lexer)).kind != ')') {
            eprintfln("ERROR %s: Expected ')' at the end of cast but got %s", tloc(t), tdisplay(t));
            return NULL;
        }
        return ast_new_cast(parser->arena, what, into);
    case TOKEN_NULL:
        return ast_new_null(parser->arena);
    case TOKEN_ATOM:
        return ast_new_symbol(parser->arena, t.atom);
    case TOKEN_C_STR:
        return ast_new_cstr(parser->arena, t.str, t.str_len); 
    case TOKEN_INT:
        return ast_new_int(parser->arena, t.integer.type, t.integer.value); 
    default:
        eprintfln("ERROR %s: Unexpected token in expression: %s", tloc(t),tdisplay(t));
        exit(1);
    }
}

AST* parse_astcall(Parser* parser, AST* what) {
    Token t;
    if((t=lexer_next(parser->lexer)).kind != '(') {
        eprintfln("ERROR %s: Expected '(' but found %s in function call", tloc(t), tdisplay(t));
        exit(1);
    }
    CallArgs args = {0};
    for(;;) {
        t = lexer_peak_next(parser->lexer);
        if(t.kind == ')') break;
        AST* value = parse_ast(parser, INIT_PRECEDENCE);
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
            eprintfln("ERROR %s: Expected ')' or ',' but found %s in function call", tloc(t), tdisplay(t));
            exit(1);
        }
    } 
    if((t=lexer_next(parser->lexer)).kind != ')') {
        eprintfln("ERROR %s: Expected ')' but found %s in function call",tloc(t),tdisplay(t));
        exit(1);
    }
    return ast_new_call(parser->arena, what, args);
}
AST* parse_subscript(Parser* parser, AST* what) {
    Token t;
    if((t=lexer_next(parser->lexer)).kind != '[') {
        eprintfln("ERROR %s: Subsript must start with '['. Got %s", tloc(t), tdisplay(t));
        return NULL;
    }
    AST* v = parse_ast(parser, INIT_PRECEDENCE);
    if(!v) return NULL;
    t = lexer_peak_next(parser->lexer);
    if(t.kind != ']') {
        eprintfln("ERROR %s: Expected closing ']' paren but got %s", tloc(t), tdisplay(t));
        return NULL;
    }
    lexer_eat(parser->lexer, 1);
    return ast_new_subscript(parser->arena, what, v);
}
AST* parse_ast(Parser* parser, int expr_precedence) {
    Token t;
    AST* v = NULL;
    t = lexer_peak_next(parser->lexer);
    // FIXME: Make this into basic expression as well as function call. Thats kinda important
    v = parse_basic(parser);
    if(!v) return NULL;
    while(true) {
        // TODO: Return NULL on failure to parse something
        t = lexer_peak_next(parser->lexer);
        switch(t.kind) {
        case '(': {
            if (2 > expr_precedence) return v;
            v = parse_astcall(parser, v);
        } break;
        case '[': {
            if (2 > expr_precedence) return v;
            v = parse_subscript(parser, v);
        } break;
        #define X(op) case op:
        BINOPS
        #undef X
        {
            int binop = t.kind;
            int bin_precedence = binop_prec(binop);
            if (bin_precedence > expr_precedence) return v;
            lexer_eat(parser->lexer, 1);
            Snapshot snap = lexer_snap_take(parser->lexer);
            AST* v2 = parse_basic(parser);
            if(!v2) return NULL;
            t = lexer_peak_next(parser->lexer);
            int next_prec = -1;
            int next_op = 0;
            switch(t.kind) {
            #define X(op) case op:
            BINOPS
            #undef X
                next_prec = binop_prec(next_op = t.kind);
                break;
            case '[':
                next_prec = 2;
                break;
            case '(':
                next_prec = 2;
                break;
            }
            if (bin_precedence > next_prec) {
                lexer_snap_restore(parser->lexer, snap);
                v2 = parse_ast(parser, bin_precedence);
            }
            v = ast_new_binop(parser->arena, binop, v, v2);
        } break;
        default:
            return v;
        }
    }
    return v;
}

Statement* parse_scope(Parser* parser) {
    Token t;
    assert((t=lexer_next(parser->lexer)).kind == '{');
    Statement* scope = statement_scope(parser->arena);
    while((t=lexer_peak_next(parser->lexer)).kind != '}') {
        if(t.kind >= TOKEN_END) {
            if(t.kind >= TOKEN_ERR) {
                eprintfln("ERROR %s: Lexer error %s", tloc(t), tdisplay(t));
                exit(1);
            } else {
                eprintfln("ERROR %s: Unexpected token in scope body: %s", tloc(t), tdisplay(t));
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
        eprintfln("ERROR %s: Expected '}' at the end of function body, but found: %s", tloc(t), tdisplay(t));
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
        eprintfln("ERROR %s: body must either start with `then` or `{`. Found %s", tloc(t), tdisplay(t));
        exit(1);
    }
    return NULL;
}
// TODO: Maybe this should be errorable?
Statement* parse_statement(Parser* parser, Token t) {
    switch(t.kind) {
        case TOKEN_DEFER: 
            lexer_eat(parser->lexer, 1);
            return statement_defer(parser->arena, parse_statement(parser, lexer_peak_next(parser->lexer)));
        case TOKEN_RETURN: {
            lexer_eat(parser->lexer, 1);
            if(lexer_peak_next(parser->lexer).kind == ';') {
                lexer_eat(parser->lexer, 1);
                return statement_return(parser->arena, NULL);
            }
            AST* ast = parse_ast(parser, INIT_PRECEDENCE);
            if(!ast) {
                eprintfln("ERROR %s: Failed to parse return statement",tloc(t));
                exit(1);
            }
            return statement_return(parser->arena, ast);
        } break;
        case TOKEN_ATOM: {
            Atom* name = t.atom;
            if(lexer_peak(parser->lexer, 1).kind == ':') {
                lexer_eat(parser->lexer, 2);
                Type* type = NULL;
                if(lexer_peak_next(parser->lexer).kind != '=') {
                    type = parse_type(parser);
                    if(!type) exit(1);
                }
                AST* init = NULL;
                if(lexer_peak_next(parser->lexer).kind == '=') {
                    lexer_eat(parser->lexer, 1);
                    // TODO: Maybe the precedence isn't correct. Just make sure it is
                    if(!(init = parse_ast(parser, INIT_PRECEDENCE))) 
                        exit(1);
                }
                return statement_local_def(parser->arena, name, symbol_new_var(parser->arena, type, init));
            }
        } break;
        case '{':
            return parse_scope(parser);
        case TOKEN_LOOP:
            lexer_eat(parser->lexer, 1);
            return statement_loop(parser->arena, parse_body(parser));
        case TOKEN_IF: {
            lexer_eat(parser->lexer, 1);
            AST* ast = parse_ast(parser, INIT_PRECEDENCE);
            if(!ast) {
                eprintfln("ERROR %s: Failed to parse condition", tloc(t));
                exit(1);
            }
            Statement* body = parse_body(parser);
            Statement* elze = NULL;
            if(lexer_peak_next(parser->lexer).kind == TOKEN_ELSE) {
                lexer_eat(parser->lexer, 1);
                elze = parse_statement(parser, lexer_peak_next(parser->lexer));
            } 
            return statement_if(parser->arena, ast, body, elze);
        }
        case TOKEN_WHILE: {
            lexer_eat(parser->lexer, 1);
            AST* ast = parse_ast(parser, INIT_PRECEDENCE);
            if(!ast) {
                eprintfln("ERROR %s: Failed to parse condition",tloc(t));
                exit(1);
            }
            return statement_while(parser->arena, ast, parse_body(parser));
        }
    }
    AST* ast = parse_ast(parser, INIT_PRECEDENCE);
    if(!ast) {
        eprintfln("ERROR %s: Unknown token in statement: %s", tloc(t), tdisplay(t));
        exit(1);
    }
    return statement_eval(parser->arena, ast);
}
void parse(Parser* parser, Arena* arena) {
    Token t;
    while((t=lexer_peak_next(parser->lexer)).kind != TOKEN_EOF) {
        if(t.kind >= TOKEN_END) {
            eprintfln("ERROR %s: Lexer: %s", tloc(t), tdisplay(t));
            exit(1);
        }
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
                // TODO: This step is kinda unnecessary now
                Function* f = func_new(parser->arena, fid, NULL);
                Symbol* sym = symbol_new_constant(parser->arena, fid, ast_new_func(parser->arena, f));
                sym_tab_insert(&parser->module->symtab_root.symtab, name, sym);
                da_push(&parser->module->symbols, ((ModuleSymbol){sym, name}));
            } else {
                eprintfln("ERROR %s: Expected signature of external function to follow the syntax:", tloc(t));
                eprintfln("  extern <func name> :: <(<Arguments>)> (-> <Output Type>)");
                exit(1);
            }
        } break;
        case TOKEN_ATOM: {
            if (lexer_peak(parser->lexer, 1).kind == ':' && lexer_peak(parser->lexer, 2).kind == ':' && lexer_peak(parser->lexer, 3).kind == TOKEN_TYPEDEF) {
                Atom* name = t.atom;
                lexer_eat(parser->lexer, 4);
                Type* base_type = parse_type(parser);
                if(!base_type) exit(1);
                Type* type = type_new(parser->arena);
                memcpy(type, base_type, sizeof(*type));
                type->name = name->data;
                // TODO: Redefining a type is kinda bad
                // Definitely need to check the type.
                // TODO: maybe collect the symbols first like with the syntactical analysis instead of 
                // Directly inserting and checking from a table in the future
                type_table_insert(&parser->module->type_table, name->data, type);
            } else if (lexer_peak(parser->lexer, 1).kind == ':') {
                Atom* name = t.atom;
                lexer_eat(parser->lexer, 2);
                Type* type = NULL;
                if(lexer_peak_next(parser->lexer).kind != ':') {
                    type = parse_type(parser);
                    if(!type) exit(1);
                }
                if((t=lexer_next(parser->lexer)).kind != ':') {
                    eprintfln("ERROR %s: Expected constant to follow the syntax: ", tloc(t));
                    eprintfln("  <const name> : (type) : <expression>");
                    exit(1);
                }
                AST* ast = parse_ast(parser, INIT_PRECEDENCE);
                Symbol* sym = symbol_new_constant(arena, type, ast);
                sym_tab_insert(&parser->module->symtab_root.symtab, name, sym);
                da_push(&parser->module->symbols, ((ModuleSymbol){sym, name}));
            } else {
                eprintfln("ERROR %s: Unexpected Atom: %s",tloc(t), t.atom->data);
                exit(1);
            }
        } break;
        case ';':
            lexer_eat(parser->lexer, 1);
            break;
        default:
            eprintfln("ERROR %s:  Unexpected token: %s", tloc(t), tdisplay(t));
            exit(1);
        }
    }
}
