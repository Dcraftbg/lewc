#include "parser.h"

Scope* new_scope(Arena* arena, Scope* parent, int kind) {
    Scope* s = arena_alloc(arena, sizeof(*s));
    if(!s) return s;
    memset(s, 0, sizeof(*s));
    s->parent = parent;
    s->kind = kind;
    return s;
}
Scope* funcs_find(Funcs* funcs, const char* name) {
    for(size_t i = 0; i < funcs->len; ++i) {
        if(strcmp(funcs->items[i].name->data, name) == 0) {
            return funcs->items[i].scope;
        }
    }
    return NULL;
}
void funcs_insert(Funcs* funcs, Atom* name, typeid_t id, Scope* scope) {
    da_push(funcs, ((struct FuncPair){name, id, scope}));
}
void parser_create(Parser* this, Lexer* lexer, Arena* arena) {
    static_assert(
        sizeof(Parser) ==
        sizeof(Arena)+
        sizeof(TypeTable)+
        sizeof(Lexer*)+
        sizeof(Scope)+
        sizeof(Scope*)+
        sizeof(Funcs),
        "Update parser_create"
    );
    memset(this, 0, sizeof(*this));
    this->arena = arena;
    type_table_init(&this->type_table);
    this->lexer = lexer;
    this->head = &this->global;
    // scope_init(&this->global);
}
Symbol* symbol_find(Scope* head, Atom* name) {
    Symbol* s;
    while(head) {
        if((s = symtab_symbol_find(&head->symtab, name))) return s;
        head = head->parent;
    }
    return NULL;
}
typeid_t parse_type(Parser* parser) {
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
    typeid_t id = type_table_get_by_name(&parser->type_table, t.atom->data);
    if(id == INVALID_TYPEID) {
        eprintfln("ERROR:%s: Unknown type name: %s", tloc(t), t.atom->data);
        exit(1);
    }
    if(ptr_count) {
        Type t = {
            .core = CORE_PTR,
            .ptr_count = ptr_count,
            .inner_type = id
        };
        return type_table_create(&parser->type_table, t);
    }
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
        typeid_t typeid = parse_type(parser);
        if(typeid == INVALID_TYPEID) {
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
    sig->output = INVALID_TYPEID;
    if(lexer_peak_next(parser->lexer).kind == TOKEN_ARROW) {
        lexer_eat(parser->lexer, 1);
        sig->output = parse_type(parser);
        if(sig->output == INVALID_TYPEID) {
            eprintfln("ERROR:%s: Invalid return type",tloc(t));
            exit(1);
        }
    }
}
ASTValue parse_basic(Parser* parser) {
    Token t = lexer_next(parser->lexer);
    switch(t.kind) {
    case TOKEN_ATOM: {
        Symbol *s = symbol_find(parser->head, t.atom);
        if(!s) {
            eprintfln("ERROR:%s: Unknown variable: %s",tloc(t),t.atom->data);
            exit(1);
        }
        return (ASTValue){.kind=AST_VALUE_SYMBOL, .symbol=t.atom};
    } break;
    case TOKEN_C_STR: {
        return (ASTValue){.kind=AST_VALUE_C_STR, .str=t.str, .str_len=t.str_len};
    } break;
    case TOKEN_INT: {
        return (ASTValue){.kind=AST_VALUE_INT, .integer = { .value = t.integer.value } };
    } break;
    default:
        eprintfln("ERROR:%s: Unexpected token in expression: %s", tloc(t),tdisplay(t));
        exit(1);
    }
}

#define OPS \
    X('+')

int op_prec(int op) {
    switch(op) {
    case '+':
        return 12;
    default:
        eprintfln("UNKNOWN OP: %d",op);
        abort();
        return -1;
    }
}
int parse_astvalue(Parser* parser, ASTValue* result) {
    int e = 0;
    ASTValue v = parse_basic(parser);
    Token t = lexer_peak_next(parser->lexer);
    switch(t.kind) {
    case '(': {
        Token t = {0};
        if((t=lexer_next(parser->lexer)).kind != '(') {
            eprintfln("ERROR:%s: Expected '(' but found %s in function call", tloc(t), tdisplay(t));
            exit(1);
        }
        CallArgs args = {0};
        for(;;) {
            t = lexer_peak_next(parser->lexer);
            if(t.kind == ')') break;
            ASTValue value;
            e = parse_astvalue(parser, &value);
            if(e!=0) {
                call_args_dealloc(&args);
                return e;
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
        result->kind = AST_VALUE_EXPR;
        result->ast = ast_new_call(parser->arena, v, args);
        return 0;
    } break;
    #define X(op) case op:
    OPS
    #undef X
    {
        int op = t.kind;
        int precedence = op_prec(op);
        lexer_eat(parser->lexer, 1);
        ASTValue v2 = parse_basic(parser);
        t = lexer_peak_next(parser->lexer);
        switch(t.kind) {
        #define X(op) case op:
        OPS 
        #undef X
        {
            int newop = t.kind;
            int newprecedence = op_prec(newop);
            if (precedence <= newprecedence) {
                lexer_eat(parser->lexer, 1);
                ASTValue v3;
                if((e = parse_astvalue(parser, &v3)) != 0) {
                    return e;
                }
                v2 = (ASTValue) {
                   AST_VALUE_EXPR,
                   .ast=ast_new(parser->arena, newop, v2, v3)
                };
            }
            AST* ast = ast_new(parser->arena, op, v, v2);
            *result = (ASTValue){AST_VALUE_EXPR, .ast=ast};
            return 0;
        } break;
        default:
            AST* ast = ast_new(parser->arena, op, v, v2);
            *result = (ASTValue){AST_VALUE_EXPR, .ast=ast};
            return 0;
        }
    } break;
    }
    *result = v;
    return 0;
}
Symbol* new_symbol(Arena* arena, Symbol symbol) {
    Symbol* s = (Symbol*)arena_alloc(arena, sizeof(*s));
    if(!s) return NULL;
    *s = symbol;
    return s;
}
Instruction parse_statement(Parser* parser, Token t) {
    switch(t.kind) {
        case TOKEN_RETURN: {
            lexer_eat(parser->lexer, 1);
            ASTValue astvalue;
            int e = parse_astvalue(parser, &astvalue);
            if((e != 0)) {
                eprintfln("ERROR:%s: Failed to parse return statement",tloc(t));
                exit(1);
            }
            return (Instruction) { INST_RETURN, .astvalue=astvalue };
        } break;
    }
    ASTValue astvalue;
    int e = parse_astvalue(parser, &astvalue);
    if(e != 0) {
        eprintfln("ERROR:%s: Unknown token in statement: %s", tloc(t), tdisplay(t));
        exit(1);
    } else {
        return (Instruction) { INST_EVAL, .astvalue=astvalue };
    }
}
void parse_func_body(Parser* parser, Scope* s) {
    Token t;
    assert(s->kind == SCOPE_FUNC);
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
        da_push(&s->insts, parse_statement(parser, t));
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
        static_assert(TOKEN_COUNT == 266, "Update parser");
        switch(t.kind) {
        case '}': {
            lexer_eat(parser->lexer, 1);
            if(parser->head->parent == NULL) {
                eprintfln("ERROR:%s: Too many '}' brackets",tloc(t));
                exit(1);
            }
            Scope* s = parser->head;
            parser->head = parser->head->parent;
            switch(s->kind) {
            case SCOPE_FUNC:
                break;
            }
        } break;
        case TOKEN_EXTERN: {
            lexer_eat(parser->lexer, 1);
            if((t = lexer_next(parser->lexer)).kind == TOKEN_ATOM && lexer_peak_next(parser->lexer).kind == ':' && lexer_peak(parser->lexer, 1).kind == ':' && lexer_peak(parser->lexer, 2).kind == '(') {
                Atom* name = t.atom;
                lexer_eat(parser->lexer, 2);
                if(parser->head->parent != NULL) {
                    eprintfln("ERROR:%s: Nested `extern` definitions are not allowed. Please use `extern` in the global scope", tloc(t));
                    exit(1);
                }

                Type functype={0};
                functype.core    = CORE_FUNC;
                functype.attribs = TYPE_ATTRIB_EXTERN;
                parse_func_signature(parser, &functype.signature);
                if(parser->head->parent != NULL) {
                    eprintfln("ERROR:%s: Nested function definitions are not yet implemented", tloc(t));
                    exit(1);
                }
                typeid_t fid = type_table_create(&parser->type_table, functype);
                Symbol* sym = new_symbol(parser->arena, (Symbol){SYMBOL_FUNC, .typeid=fid});
                symtab_insert(&parser->head->symtab, name, sym);
                funcs_insert(&parser->funcs, name, fid, NULL);
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
                Type functype={0};
                functype.core = CORE_FUNC;

                parse_func_signature(parser, &functype.signature);
                if(parser->head->parent != NULL) {
                    eprintfln("ERROR:%s: Nested function definitions are not yet implemented", tloc(t));
                    exit(1);
                }
                if((t=lexer_next(parser->lexer)).kind != '{') {
                    eprintfln("ERROR:%s: Missing '{' at the start of function. Got: %s", tloc(t), tdisplay(t));
                    exit(1);
                }
                typeid_t fid = type_table_create(&parser->type_table, functype);
                Symbol* sym = new_symbol(parser->arena, (Symbol){SYMBOL_FUNC, .typeid=fid});
                symtab_insert(&parser->head->symtab, name, sym);
                

                Scope* s = new_scope(parser->arena, parser->head, SCOPE_FUNC);
                for(size_t i = 0; i < functype.signature.input.len; ++i) {
                    Arg* arg = &functype.signature.input.items[i];
                    if(arg->name) {
                       symtab_insert(&s->symtab, arg->name, new_symbol(parser->arena, (Symbol){SYMBOL_VARIABLE, .typeid=arg->typeid}));
                    }
                }
                parser->head = s;
                parse_func_body(parser, s);
                parser->head = parser->head->parent;
                funcs_insert(&parser->funcs, name, fid, s);
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
