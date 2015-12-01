/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of zig, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#include "parser.hpp"
#include "errmsg.hpp"
#include "semantic_info.hpp"

#include <stdarg.h>
#include <stdio.h>

static const char *bin_op_str(BinOpType bin_op) {
    switch (bin_op) {
        case BinOpTypeInvalid:        return "(invalid)";
        case BinOpTypeBoolOr:         return "||";
        case BinOpTypeBoolAnd:        return "&&";
        case BinOpTypeCmpEq:          return "==";
        case BinOpTypeCmpNotEq:       return "!=";
        case BinOpTypeCmpLessThan:    return "<";
        case BinOpTypeCmpGreaterThan: return ">";
        case BinOpTypeCmpLessOrEq:    return "<=";
        case BinOpTypeCmpGreaterOrEq: return ">=";
        case BinOpTypeBinOr:          return "|";
        case BinOpTypeBinXor:         return "^";
        case BinOpTypeBinAnd:         return "&";
        case BinOpTypeBitShiftLeft:   return "<<";
        case BinOpTypeBitShiftRight:  return ">>";
        case BinOpTypeAdd:            return "+";
        case BinOpTypeSub:            return "-";
        case BinOpTypeMult:           return "*";
        case BinOpTypeDiv:            return "/";
        case BinOpTypeMod:            return "%";
    }
    zig_unreachable();
}

static const char *prefix_op_str(PrefixOp prefix_op) {
    switch (prefix_op) {
        case PrefixOpInvalid: return "(invalid)";
        case PrefixOpNegation: return "-";
        case PrefixOpBoolNot: return "!";
        case PrefixOpBinNot: return "~";
    }
    zig_unreachable();
}

const char *node_type_str(NodeType node_type) {
    switch (node_type) {
        case NodeTypeRoot:
            return "Root";
        case NodeTypeRootExportDecl:
            return "RootExportDecl";
        case NodeTypeFnDef:
            return "FnDef";
        case NodeTypeFnDecl:
            return "FnDecl";
        case NodeTypeFnProto:
            return "FnProto";
        case NodeTypeParamDecl:
            return "ParamDecl";
        case NodeTypeType:
            return "Type";
        case NodeTypeBlock:
            return "Block";
        case NodeTypeBinOpExpr:
            return "BinOpExpr";
        case NodeTypeFnCallExpr:
            return "FnCallExpr";
        case NodeTypeExternBlock:
            return "ExternBlock";
        case NodeTypeDirective:
            return "Directive";
        case NodeTypeReturnExpr:
            return "ReturnExpr";
        case NodeTypeCastExpr:
            return "CastExpr";
        case NodeTypeNumberLiteral:
            return "NumberLiteral";
        case NodeTypeStringLiteral:
            return "StringLiteral";
        case NodeTypeUnreachable:
            return "Unreachable";
        case NodeTypeSymbol:
            return "Symbol";
        case NodeTypePrefixOpExpr:
            return "PrefixOpExpr";
        case NodeTypeUse:
            return "Use";
    }
    zig_unreachable();
}

void ast_print(AstNode *node, int indent) {
    for (int i = 0; i < indent; i += 1) {
        fprintf(stderr, " ");
    }

    switch (node->type) {
        case NodeTypeRoot:
            fprintf(stderr, "%s\n", node_type_str(node->type));
            for (int i = 0; i < node->data.root.top_level_decls.length; i += 1) {
                AstNode *child = node->data.root.top_level_decls.at(i);
                ast_print(child, indent + 2);
            }
            break;
        case NodeTypeRootExportDecl:
            fprintf(stderr, "%s %s '%s'\n", node_type_str(node->type),
                    buf_ptr(&node->data.root_export_decl.type),
                    buf_ptr(&node->data.root_export_decl.name));
            break;
        case NodeTypeFnDef:
            {
                fprintf(stderr, "%s\n", node_type_str(node->type));
                AstNode *child = node->data.fn_def.fn_proto;
                ast_print(child, indent + 2);
                ast_print(node->data.fn_def.body, indent + 2);
                break;
            }
        case NodeTypeFnProto:
            {
                Buf *name_buf = &node->data.fn_proto.name;
                fprintf(stderr, "%s '%s'\n", node_type_str(node->type), buf_ptr(name_buf));

                for (int i = 0; i < node->data.fn_proto.params.length; i += 1) {
                    AstNode *child = node->data.fn_proto.params.at(i);
                    ast_print(child, indent + 2);
                }

                ast_print(node->data.fn_proto.return_type, indent + 2);

                break;
            }
        case NodeTypeBlock:
            {
                fprintf(stderr, "%s\n", node_type_str(node->type));
                for (int i = 0; i < node->data.block.statements.length; i += 1) {
                    AstNode *child = node->data.block.statements.at(i);
                    ast_print(child, indent + 2);
                }
                break;
            }
        case NodeTypeParamDecl:
            {
                Buf *name_buf = &node->data.param_decl.name;
                fprintf(stderr, "%s '%s'\n", node_type_str(node->type), buf_ptr(name_buf));

                ast_print(node->data.param_decl.type, indent + 2);

                break;
            }
        case NodeTypeType:
            switch (node->data.type.type) {
                case AstNodeTypeTypePrimitive:
                    {
                        Buf *name_buf = &node->data.type.primitive_name;
                        fprintf(stderr, "%s '%s'\n", node_type_str(node->type), buf_ptr(name_buf));
                        break;
                    }
                case AstNodeTypeTypePointer:
                    {
                        const char *const_or_mut_str = node->data.type.is_const ? "const" : "mut";
                        fprintf(stderr, "'%s' PointerType\n", const_or_mut_str);

                        ast_print(node->data.type.child_type, indent + 2);
                        break;
                    }
            }
            break;
        case NodeTypeReturnExpr:
            fprintf(stderr, "%s\n", node_type_str(node->type));
            if (node->data.return_expr.expr)
                ast_print(node->data.return_expr.expr, indent + 2);
            break;
        case NodeTypeExternBlock:
            {
                fprintf(stderr, "%s\n", node_type_str(node->type));
                for (int i = 0; i < node->data.extern_block.fn_decls.length; i += 1) {
                    AstNode *child = node->data.extern_block.fn_decls.at(i);
                    ast_print(child, indent + 2);
                }
                break;
            }
        case NodeTypeFnDecl:
            fprintf(stderr, "%s\n", node_type_str(node->type));
            ast_print(node->data.fn_decl.fn_proto, indent + 2);
            break;
        case NodeTypeBinOpExpr:
            fprintf(stderr, "%s %s\n", node_type_str(node->type),
                    bin_op_str(node->data.bin_op_expr.bin_op));
            ast_print(node->data.bin_op_expr.op1, indent + 2);
            ast_print(node->data.bin_op_expr.op2, indent + 2);
            break;
        case NodeTypeFnCallExpr:
            fprintf(stderr, "%s\n", node_type_str(node->type));
            ast_print(node->data.fn_call_expr.fn_ref_expr, indent + 2);
            for (int i = 0; i < node->data.fn_call_expr.params.length; i += 1) {
                AstNode *child = node->data.fn_call_expr.params.at(i);
                ast_print(child, indent + 2);
            }
            break;
        case NodeTypeDirective:
            fprintf(stderr, "%s\n", node_type_str(node->type));
            break;
        case NodeTypeCastExpr:
            fprintf(stderr, "%s\n", node_type_str(node->type));
            ast_print(node->data.cast_expr.prefix_op_expr, indent + 2);
            if (node->data.cast_expr.type)
                ast_print(node->data.cast_expr.type, indent + 2);
            break;
        case NodeTypePrefixOpExpr:
            fprintf(stderr, "%s %s\n", node_type_str(node->type),
                    prefix_op_str(node->data.prefix_op_expr.prefix_op));
            ast_print(node->data.prefix_op_expr.primary_expr, indent + 2);
            break;
        case NodeTypeNumberLiteral:
            fprintf(stderr, "PrimaryExpr Number %s\n",
                    buf_ptr(&node->data.number));
            break;
        case NodeTypeStringLiteral:
            fprintf(stderr, "PrimaryExpr String '%s'\n",
                    buf_ptr(&node->data.string));
            break;
        case NodeTypeUnreachable:
            fprintf(stderr, "PrimaryExpr Unreachable\n");
            break;
        case NodeTypeSymbol:
            fprintf(stderr, "PrimaryExpr Symbol %s\n",
                    buf_ptr(&node->data.symbol));
            break;
        case NodeTypeUse:
            fprintf(stderr, "%s '%s'\n", node_type_str(node->type), buf_ptr(&node->data.use.path));
            break;
    }
}

struct ParseContext {
    Buf *buf;
    AstNode *root;
    ZigList<Token> *tokens;
    ZigList<AstNode *> *directive_list;
    ImportTableEntry *owner;
    ErrColor err_color;
};

__attribute__ ((format (printf, 3, 4)))
__attribute__ ((noreturn))
static void ast_error(ParseContext *pc, Token *token, const char *format, ...) {
    ErrorMsg *err = allocate<ErrorMsg>(1);
    err->line_start = token->start_line;
    err->column_start = token->start_column;
    err->line_end = -1;
    err->column_end = -1;

    va_list ap;
    va_start(ap, format);
    err->msg = buf_vprintf(format, ap);
    va_end(ap);

    err->path = pc->owner->path;
    err->source = pc->owner->source_code;
    err->line_offsets = pc->owner->line_offsets;

    print_err_msg(err, pc->err_color);
    exit(EXIT_FAILURE);
}

static AstNode *ast_create_node_no_line_info(ParseContext *pc, NodeType type) {
    AstNode *node = allocate<AstNode>(1);
    node->type = type;
    node->owner = pc->owner;
    return node;
}

static void ast_update_node_line_info(AstNode *node, Token *first_token) {
    node->line = first_token->start_line;
    node->column = first_token->start_column;
}

static AstNode *ast_create_node(ParseContext *pc, NodeType type, Token *first_token) {
    AstNode *node = ast_create_node_no_line_info(pc, type);
    ast_update_node_line_info(node, first_token);
    return node;
}

static AstNode *ast_create_node_with_node(ParseContext *pc, NodeType type, AstNode *other_node) {
    AstNode *node = ast_create_node_no_line_info(pc, type);
    node->line = other_node->line;
    node->column = other_node->column;
    return node;
}

static AstNode *ast_create_void_type_node(ParseContext *pc, Token *token) {
    AstNode *node = ast_create_node(pc, NodeTypeType, token);
    node->data.type.type = AstNodeTypeTypePrimitive;
    buf_init_from_str(&node->data.type.primitive_name, "void");
    return node;
}

static void ast_buf_from_token(ParseContext *pc, Token *token, Buf *buf) {
    buf_init_from_mem(buf, buf_ptr(pc->buf) + token->start_pos, token->end_pos - token->start_pos);
}

static void parse_string_literal(ParseContext *pc, Token *token, Buf *buf) {
    // skip the double quotes at beginning and end
    // convert escape sequences

    buf_resize(buf, 0);
    bool escape = false;
    for (int i = token->start_pos + 1; i < token->end_pos - 1; i += 1) {
        uint8_t c = *((uint8_t*)buf_ptr(pc->buf) + i);
        if (escape) {
            switch (c) {
                case '\\':
                    buf_append_char(buf, '\\');
                    break;
                case 'r':
                    buf_append_char(buf, '\r');
                    break;
                case 'n':
                    buf_append_char(buf, '\n');
                    break;
                case 't':
                    buf_append_char(buf, '\t');
                    break;
                case '"':
                    buf_append_char(buf, '"');
                    break;
            }
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else {
            buf_append_char(buf, c);
        }
    }
    assert(!escape);
}

__attribute__ ((noreturn))
static void ast_invalid_token_error(ParseContext *pc, Token *token) {
    Buf token_value = BUF_INIT;
    ast_buf_from_token(pc, token, &token_value);
    ast_error(pc, token, "invalid token: '%s'", buf_ptr(&token_value));
}

static AstNode *ast_parse_expression(ParseContext *pc, int *token_index, bool mandatory);
static AstNode *ast_parse_block(ParseContext *pc, int *token_index, bool mandatory);


static void ast_expect_token(ParseContext *pc, Token *token, TokenId token_id) {
    if (token->id != token_id) {
        ast_invalid_token_error(pc, token);
    }
}

static AstNode *ast_parse_directive(ParseContext *pc, int token_index, int *new_token_index) {
    Token *number_sign = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, number_sign, TokenIdNumberSign);

    AstNode *node = ast_create_node(pc, NodeTypeDirective, number_sign);

    Token *name_symbol = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, name_symbol, TokenIdSymbol);

    ast_buf_from_token(pc, name_symbol, &node->data.directive.name);

    Token *l_paren = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, l_paren, TokenIdLParen);

    Token *param_str = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, param_str, TokenIdStringLiteral);

    parse_string_literal(pc, param_str, &node->data.directive.param);

    Token *r_paren = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, r_paren, TokenIdRParen);

    *new_token_index = token_index;
    return node;
}

static void ast_parse_directives(ParseContext *pc, int *token_index,
        ZigList<AstNode *> *directives)
{
    for (;;) {
        Token *token = &pc->tokens->at(*token_index);
        if (token->id == TokenIdNumberSign) {
            AstNode *directive_node = ast_parse_directive(pc, *token_index, token_index);
            directives->append(directive_node);
        } else {
            return;
        }
    }
    zig_unreachable();
}


/*
Type : token(Symbol) | PointerType | token(Unreachable)
PointerType : token(Star) token(Const) Type  | token(Star) token(Mut) Type;
*/
static AstNode *ast_parse_type(ParseContext *pc, int token_index, int *new_token_index) {
    Token *token = &pc->tokens->at(token_index);
    token_index += 1;

    AstNode *node = ast_create_node(pc, NodeTypeType, token);

    if (token->id == TokenIdKeywordUnreachable) {
        node->data.type.type = AstNodeTypeTypePrimitive;
        buf_init_from_str(&node->data.type.primitive_name, "unreachable");
    } else if (token->id == TokenIdSymbol) {
        node->data.type.type = AstNodeTypeTypePrimitive;
        ast_buf_from_token(pc, token, &node->data.type.primitive_name);
    } else if (token->id == TokenIdStar) {
        node->data.type.type = AstNodeTypeTypePointer;
        Token *const_or_mut = &pc->tokens->at(token_index);
        token_index += 1;
        if (const_or_mut->id == TokenIdKeywordMut) {
            node->data.type.is_const = false;
        } else if (const_or_mut->id == TokenIdKeywordConst) {
            node->data.type.is_const = true;
        } else {
            ast_invalid_token_error(pc, const_or_mut);
        }

        node->data.type.child_type = ast_parse_type(pc, token_index, &token_index);
    } else {
        ast_invalid_token_error(pc, token);
    }

    *new_token_index = token_index;
    return node;
}

/*
ParamDecl<node> : token(Symbol) token(Colon) Type {
};
*/
static AstNode *ast_parse_param_decl(ParseContext *pc, int token_index, int *new_token_index) {
    Token *param_name = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, param_name, TokenIdSymbol);

    AstNode *node = ast_create_node(pc, NodeTypeParamDecl, param_name);


    ast_buf_from_token(pc, param_name, &node->data.param_decl.name);

    Token *colon = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, colon, TokenIdColon);

    node->data.param_decl.type = ast_parse_type(pc, token_index, &token_index);

    *new_token_index = token_index;
    return node;
}


static void ast_parse_param_decl_list(ParseContext *pc, int token_index, int *new_token_index,
        ZigList<AstNode *> *params)
{
    Token *l_paren = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, l_paren, TokenIdLParen);

    Token *token = &pc->tokens->at(token_index);
    if (token->id == TokenIdRParen) {
        token_index += 1;
        *new_token_index = token_index;
        return;
    }

    for (;;) {
        AstNode *param_decl_node = ast_parse_param_decl(pc, token_index, &token_index);
        params->append(param_decl_node);

        Token *token = &pc->tokens->at(token_index);
        token_index += 1;
        if (token->id == TokenIdRParen) {
            *new_token_index = token_index;
            return;
        } else {
            ast_expect_token(pc, token, TokenIdComma);
        }
    }
    zig_unreachable();
}

static void ast_parse_fn_call_param_list(ParseContext *pc, int token_index, int *new_token_index,
        ZigList<AstNode*> *params)
{
    Token *l_paren = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, l_paren, TokenIdLParen);

    Token *token = &pc->tokens->at(token_index);
    if (token->id == TokenIdRParen) {
        token_index += 1;
        *new_token_index = token_index;
        return;
    }

    for (;;) {
        AstNode *expr = ast_parse_expression(pc, &token_index, true);
        params->append(expr);

        Token *token = &pc->tokens->at(token_index);
        token_index += 1;
        if (token->id == TokenIdRParen) {
            *new_token_index = token_index;
            return;
        } else {
            ast_expect_token(pc, token, TokenIdComma);
        }
    }
    zig_unreachable();
}

/*
GroupedExpression : token(LParen) Expression token(RParen)
*/
static AstNode *ast_parse_grouped_expr(ParseContext *pc, int *token_index, bool mandatory) {
    Token *l_paren = &pc->tokens->at(*token_index);
    if (l_paren->id != TokenIdLParen) {
        if (mandatory) {
            ast_invalid_token_error(pc, l_paren);
        } else {
            return nullptr;
        }
    }

    *token_index += 1;

    AstNode *node = ast_parse_expression(pc, token_index, true);

    Token *r_paren = &pc->tokens->at(*token_index);
    *token_index += 1;
    ast_expect_token(pc, r_paren, TokenIdRParen);

    return node;
}

/*
PrimaryExpression : token(Number) | token(String) | token(Unreachable) | GroupedExpression | Block | token(Symbol)
*/
static AstNode *ast_parse_primary_expr(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);

    if (token->id == TokenIdNumberLiteral) {
        AstNode *node = ast_create_node(pc, NodeTypeNumberLiteral, token);
        ast_buf_from_token(pc, token, &node->data.number);
        *token_index += 1;
        return node;
    } else if (token->id == TokenIdStringLiteral) {
        AstNode *node = ast_create_node(pc, NodeTypeStringLiteral, token);
        parse_string_literal(pc, token, &node->data.string);
        *token_index += 1;
        return node;
    } else if (token->id == TokenIdKeywordUnreachable) {
        AstNode *node = ast_create_node(pc, NodeTypeUnreachable, token);
        *token_index += 1;
        return node;
    } else if (token->id == TokenIdSymbol) {
        AstNode *node = ast_create_node(pc, NodeTypeSymbol, token);
        ast_buf_from_token(pc, token, &node->data.symbol);
        *token_index += 1;
        return node;
    }

    AstNode *block_node = ast_parse_block(pc, token_index, false);
    if (block_node) {
        return block_node;
    }

    AstNode *grouped_expr_node = ast_parse_grouped_expr(pc, token_index, false);
    if (grouped_expr_node) {
        return grouped_expr_node;
    }

    if (!mandatory)
        return nullptr;

    ast_invalid_token_error(pc, token);
}

/*
FnCallExpression : PrimaryExpression token(LParen) list(Expression, token(Comma)) token(RParen) | PrimaryExpression
*/
static AstNode *ast_parse_fn_call_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *primary_expr = ast_parse_primary_expr(pc, token_index, mandatory);
    if (!primary_expr)
        return nullptr;

    Token *l_paren = &pc->tokens->at(*token_index);
    if (l_paren->id != TokenIdLParen)
        return primary_expr;

    AstNode *node = ast_create_node_with_node(pc, NodeTypeFnCallExpr, primary_expr);
    node->data.fn_call_expr.fn_ref_expr = primary_expr;
    ast_parse_fn_call_param_list(pc, *token_index, token_index, &node->data.fn_call_expr.params);

    return node;
}

static PrefixOp tok_to_prefix_op(Token *token) {
    switch (token->id) {
        case TokenIdBang: return PrefixOpBoolNot;
        case TokenIdDash: return PrefixOpNegation;
        case TokenIdTilde: return PrefixOpBinNot;
        default: return PrefixOpInvalid;
    }
}

/*
PrefixOp : token(Not) | token(Dash) | token(Tilde)
*/
static PrefixOp ast_parse_prefix_op(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);
    PrefixOp result = tok_to_prefix_op(token);
    if (result == PrefixOpInvalid) {
        if (mandatory) {
            ast_invalid_token_error(pc, token);
        } else {
            return PrefixOpInvalid;
        }
    }
    *token_index += 1;
    return result;
}

/*
PrefixOpExpression : PrefixOp FnCallExpression | FnCallExpression
*/
static AstNode *ast_parse_prefix_op_expr(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);
    PrefixOp prefix_op = ast_parse_prefix_op(pc, token_index, false);
    if (prefix_op == PrefixOpInvalid)
        return ast_parse_fn_call_expr(pc, token_index, mandatory);

    AstNode *primary_expr = ast_parse_fn_call_expr(pc, token_index, true);
    AstNode *node = ast_create_node(pc, NodeTypePrefixOpExpr, token);
    node->data.prefix_op_expr.primary_expr = primary_expr;
    node->data.prefix_op_expr.prefix_op = prefix_op;

    return node;
}


/*
CastExpression : PrefixOpExpression token(as) Type | PrefixOpExpression
*/
static AstNode *ast_parse_cast_expression(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *prefix_op_expr = ast_parse_prefix_op_expr(pc, token_index, mandatory);
    if (!prefix_op_expr)
        return nullptr;

    Token *as_kw = &pc->tokens->at(*token_index);
    if (as_kw->id != TokenIdKeywordAs)
        return prefix_op_expr;
    *token_index += 1;

    AstNode *node = ast_create_node(pc, NodeTypeCastExpr, as_kw);
    node->data.cast_expr.prefix_op_expr = prefix_op_expr;

    node->data.cast_expr.type = ast_parse_type(pc, *token_index, token_index);

    return node;
}

static BinOpType tok_to_mult_op(Token *token) {
    switch (token->id) {
        case TokenIdStar: return BinOpTypeMult;
        case TokenIdSlash: return BinOpTypeDiv;
        case TokenIdPercent: return BinOpTypeMod;
        default: return BinOpTypeInvalid;
    }
}

/*
MultiplyOperator : token(Star) | token(Slash) | token(Percent)
*/
static BinOpType ast_parse_mult_op(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);
    BinOpType result = tok_to_mult_op(token);
    if (result == BinOpTypeInvalid) {
        if (mandatory) {
            ast_invalid_token_error(pc, token);
        } else {
            return BinOpTypeInvalid;
        }
    }
    *token_index += 1;
    return result;
}

/*
MultiplyExpression : CastExpression MultiplyOperator CastExpression | CastExpression
*/
static AstNode *ast_parse_mult_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_cast_expression(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    BinOpType mult_op = ast_parse_mult_op(pc, token_index, false);
    if (mult_op == BinOpTypeInvalid)
        return operand_1;

    AstNode *operand_2 = ast_parse_cast_expression(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = mult_op;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}

static BinOpType tok_to_add_op(Token *token) {
    switch (token->id) {
        case TokenIdPlus: return BinOpTypeAdd;
        case TokenIdDash: return BinOpTypeSub;
        default: return BinOpTypeInvalid;
    }
}

/*
AdditionOperator : token(Plus) | token(Minus)
*/
static BinOpType ast_parse_add_op(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);
    BinOpType result = tok_to_add_op(token);
    if (result == BinOpTypeInvalid) {
        if (mandatory) {
            ast_invalid_token_error(pc, token);
        } else {
            return BinOpTypeInvalid;
        }
    }
    *token_index += 1;
    return result;
}

/*
AdditionExpression : MultiplyExpression AdditionOperator MultiplyExpression | MultiplyExpression
*/
static AstNode *ast_parse_add_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_mult_expr(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    BinOpType add_op = ast_parse_add_op(pc, token_index, false);
    if (add_op == BinOpTypeInvalid)
        return operand_1;

    AstNode *operand_2 = ast_parse_mult_expr(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = add_op;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}

static BinOpType tok_to_bit_shift_op(Token *token) {
    switch (token->id) {
        case TokenIdBitShiftLeft: return BinOpTypeBitShiftLeft;
        case TokenIdBitShiftRight: return BinOpTypeBitShiftRight;
        default: return BinOpTypeInvalid;
    }
}

/*
BitShiftOperator : token(BitShiftLeft | token(BitShiftRight)
*/
static BinOpType ast_parse_bit_shift_op(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);
    BinOpType result = tok_to_bit_shift_op(token);
    if (result == BinOpTypeInvalid) {
        if (mandatory) {
            ast_invalid_token_error(pc, token);
        } else {
            return BinOpTypeInvalid;
        }
    }
    *token_index += 1;
    return result;
}

/*
BitShiftExpression : AdditionExpression BitShiftOperator AdditionExpression | AdditionExpression
*/
static AstNode *ast_parse_bit_shift_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_add_expr(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    BinOpType bit_shift_op = ast_parse_bit_shift_op(pc, token_index, false);
    if (bit_shift_op == BinOpTypeInvalid)
        return operand_1;

    AstNode *operand_2 = ast_parse_add_expr(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = bit_shift_op;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}


/*
BinaryAndExpression : BitShiftExpression token(BinAnd) BitShiftExpression | BitShiftExpression
*/
static AstNode *ast_parse_bin_and_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_bit_shift_expr(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    if (token->id != TokenIdBinAnd)
        return operand_1;
    *token_index += 1;

    AstNode *operand_2 = ast_parse_bit_shift_expr(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = BinOpTypeBinAnd;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}

/*
BinaryXorExpression : BinaryAndExpression token(BinXor) BinaryAndExpression | BinaryAndExpression
*/
static AstNode *ast_parse_bin_xor_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_bin_and_expr(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    if (token->id != TokenIdBinXor)
        return operand_1;
    *token_index += 1;

    AstNode *operand_2 = ast_parse_bin_and_expr(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = BinOpTypeBinXor;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}

/*
BinaryOrExpression : BinaryXorExpression token(BinOr) BinaryXorExpression | BinaryXorExpression
*/
static AstNode *ast_parse_bin_or_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_bin_xor_expr(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    if (token->id != TokenIdBinOr)
        return operand_1;
    *token_index += 1;

    AstNode *operand_2 = ast_parse_bin_xor_expr(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = BinOpTypeBinOr;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}

static BinOpType tok_to_cmp_op(Token *token) {
    switch (token->id) {
        case TokenIdCmpEq: return BinOpTypeCmpEq;
        case TokenIdCmpNotEq: return BinOpTypeCmpNotEq;
        case TokenIdCmpLessThan: return BinOpTypeCmpLessThan;
        case TokenIdCmpGreaterThan: return BinOpTypeCmpGreaterThan;
        case TokenIdCmpLessOrEq: return BinOpTypeCmpLessOrEq;
        case TokenIdCmpGreaterOrEq: return BinOpTypeCmpGreaterOrEq;
        default: return BinOpTypeInvalid;
    }
}

static BinOpType ast_parse_comparison_operator(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);
    BinOpType result = tok_to_cmp_op(token);
    if (result == BinOpTypeInvalid) {
        if (mandatory) {
            ast_invalid_token_error(pc, token);
        } else {
            return BinOpTypeInvalid;
        }
    }
    *token_index += 1;
    return result;
}

/*
ComparisonExpression : BinaryOrExpression ComparisonOperator BinaryOrExpression | BinaryOrExpression
*/
static AstNode *ast_parse_comparison_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_bin_or_expr(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    BinOpType cmp_op = ast_parse_comparison_operator(pc, token_index, false);
    if (cmp_op == BinOpTypeInvalid)
        return operand_1;

    AstNode *operand_2 = ast_parse_bin_or_expr(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = cmp_op;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}

/*
BoolAndExpression : ComparisonExpression token(BoolAnd) ComparisonExpression | ComparisonExpression
 */
static AstNode *ast_parse_bool_and_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_comparison_expr(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    if (token->id != TokenIdBoolAnd)
        return operand_1;
    *token_index += 1;

    AstNode *operand_2 = ast_parse_comparison_expr(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = BinOpTypeBoolAnd;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}

/*
ReturnExpression : token(Return) option(Expression)
*/
static AstNode *ast_parse_return_expr(ParseContext *pc, int *token_index, bool mandatory) {
    Token *return_tok = &pc->tokens->at(*token_index);
    if (return_tok->id == TokenIdKeywordReturn) {
        *token_index += 1;
        AstNode *node = ast_create_node(pc, NodeTypeReturnExpr, return_tok);
        node->data.return_expr.expr = ast_parse_expression(pc, token_index, false);
        return node;
    } else if (mandatory) {
        ast_invalid_token_error(pc, return_tok);
    } else {
        return nullptr;
    }
}

/*
BoolOrExpression : BoolAndExpression token(BoolOr) BoolAndExpression | BoolAndExpression
*/
static AstNode *ast_parse_bool_or_expr(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *operand_1 = ast_parse_bool_and_expr(pc, token_index, mandatory);
    if (!operand_1)
        return nullptr;

    Token *token = &pc->tokens->at(*token_index);
    if (token->id != TokenIdBoolOr)
        return operand_1;
    *token_index += 1;

    AstNode *operand_2 = ast_parse_bool_and_expr(pc, token_index, true);

    AstNode *node = ast_create_node(pc, NodeTypeBinOpExpr, token);
    node->data.bin_op_expr.op1 = operand_1;
    node->data.bin_op_expr.bin_op = BinOpTypeBoolOr;
    node->data.bin_op_expr.op2 = operand_2;

    return node;
}

/*
Expression : BoolOrExpression | ReturnExpression
*/
static AstNode *ast_parse_expression(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);

    AstNode *return_expr = ast_parse_return_expr(pc, token_index, false);
    if (return_expr)
        return return_expr;

    AstNode *bool_or_expr = ast_parse_bool_or_expr(pc, token_index, false);
    if (bool_or_expr)
        return bool_or_expr;

    if (!mandatory)
        return nullptr;

    ast_invalid_token_error(pc, token);
}

/*
ExpressionStatement : Expression token(Semicolon)
*/
static AstNode *ast_parse_expression_statement(ParseContext *pc, int *token_index) {
    AstNode *expr_node = ast_parse_expression(pc, token_index, true);

    Token *semicolon = &pc->tokens->at(*token_index);
    *token_index += 1;
    ast_expect_token(pc, semicolon, TokenIdSemicolon);

    return expr_node;
}

/*
Statement : ExpressionStatement
*/
static AstNode *ast_parse_statement(ParseContext *pc, int *token_index) {
    return ast_parse_expression_statement(pc, token_index);
}

/*
Block : token(LBrace) many(Statement) token(RBrace);
*/
static AstNode *ast_parse_block(ParseContext *pc, int *token_index, bool mandatory) {
    Token *l_brace = &pc->tokens->at(*token_index);

    if (l_brace->id != TokenIdLBrace) {
        if (mandatory) {
            ast_invalid_token_error(pc, l_brace);
        } else {
            return nullptr;
        }
    }
    *token_index += 1;

    AstNode *node = ast_create_node(pc, NodeTypeBlock, l_brace);

    for (;;) {
        Token *token = &pc->tokens->at(*token_index);
        if (token->id == TokenIdRBrace) {
            *token_index += 1;
            return node;
        } else {
            AstNode *statement_node = ast_parse_statement(pc, token_index);
            node->data.block.statements.append(statement_node);
        }
    }
    zig_unreachable();
}

/*
FnProto : many(Directive) option(FnVisibleMod) token(Fn) token(Symbol) ParamDeclList option(token(Arrow) Type)
*/
static AstNode *ast_parse_fn_proto(ParseContext *pc, int *token_index, bool mandatory) {
    Token *token = &pc->tokens->at(*token_index);

    FnProtoVisibMod visib_mod;

    if (token->id == TokenIdKeywordPub) {
        visib_mod = FnProtoVisibModPub;
        *token_index += 1;

        Token *fn_token = &pc->tokens->at(*token_index);
        *token_index += 1;
        ast_expect_token(pc, fn_token, TokenIdKeywordFn);
    } else if (token->id == TokenIdKeywordExport) {
        visib_mod = FnProtoVisibModExport;
        *token_index += 1;

        Token *fn_token = &pc->tokens->at(*token_index);
        *token_index += 1;
        ast_expect_token(pc, fn_token, TokenIdKeywordFn);
    } else if (token->id == TokenIdKeywordFn) {
        visib_mod = FnProtoVisibModPrivate;
        *token_index += 1;
    } else if (mandatory) {
        ast_invalid_token_error(pc, token);
    } else {
        return nullptr;
    }

    AstNode *node = ast_create_node(pc, NodeTypeFnProto, token);
    node->data.fn_proto.visib_mod = visib_mod;
    node->data.fn_proto.directives = pc->directive_list;
    pc->directive_list = nullptr;


    Token *fn_name = &pc->tokens->at(*token_index);
    *token_index += 1;
    ast_expect_token(pc, fn_name, TokenIdSymbol);

    ast_buf_from_token(pc, fn_name, &node->data.fn_proto.name);


    ast_parse_param_decl_list(pc, *token_index, token_index, &node->data.fn_proto.params);

    Token *arrow = &pc->tokens->at(*token_index);
    if (arrow->id == TokenIdArrow) {
        *token_index += 1;
        node->data.fn_proto.return_type = ast_parse_type(pc, *token_index, token_index);
    } else {
        node->data.fn_proto.return_type = ast_create_void_type_node(pc, arrow);
    }

    return node;
}

/*
FnDef : FnProto Block
*/
static AstNode *ast_parse_fn_def(ParseContext *pc, int *token_index, bool mandatory) {
    AstNode *fn_proto = ast_parse_fn_proto(pc, token_index, mandatory);
    if (!fn_proto)
        return nullptr;
    AstNode *node = ast_create_node_with_node(pc, NodeTypeFnDef, fn_proto);

    node->data.fn_def.fn_proto = fn_proto;
    node->data.fn_def.body = ast_parse_block(pc, token_index, true);

    return node;
}

/*
FnDecl : FnProto token(Semicolon)
*/
static AstNode *ast_parse_fn_decl(ParseContext *pc, int token_index, int *new_token_index) {
    AstNode *fn_proto = ast_parse_fn_proto(pc, &token_index, true);
    AstNode *node = ast_create_node_with_node(pc, NodeTypeFnDecl, fn_proto);

    node->data.fn_decl.fn_proto = fn_proto;

    Token *semicolon = &pc->tokens->at(token_index);
    token_index += 1;
    ast_expect_token(pc, semicolon, TokenIdSemicolon);

    *new_token_index = token_index;
    return node;
}

/*
Directive : token(NumberSign) token(Symbol) token(LParen) token(String) token(RParen)
*/
/*
ExternBlock : many(Directive) token(Extern) token(LBrace) many(FnProtoDecl) token(RBrace)
*/
static AstNode *ast_parse_extern_block(ParseContext *pc, int *token_index, bool mandatory) {
    Token *extern_kw = &pc->tokens->at(*token_index);
    if (extern_kw->id != TokenIdKeywordExtern) {
        if (mandatory)
            ast_invalid_token_error(pc, extern_kw);
        else
            return nullptr;
    }
    *token_index += 1;

    AstNode *node = ast_create_node(pc, NodeTypeExternBlock, extern_kw);

    node->data.extern_block.directives = pc->directive_list;
    pc->directive_list = nullptr;

    Token *l_brace = &pc->tokens->at(*token_index);
    *token_index += 1;
    ast_expect_token(pc, l_brace, TokenIdLBrace);

    for (;;) {
        Token *directive_token = &pc->tokens->at(*token_index);
        assert(!pc->directive_list);
        pc->directive_list = allocate<ZigList<AstNode*>>(1);
        ast_parse_directives(pc, token_index, pc->directive_list);

        Token *token = &pc->tokens->at(*token_index);
        if (token->id == TokenIdRBrace) {
            if (pc->directive_list->length > 0) {
                ast_error(pc, directive_token, "invalid directive");
            }
            pc->directive_list = nullptr;

            *token_index += 1;
            return node;
        } else {
            AstNode *child = ast_parse_fn_decl(pc, *token_index, token_index);
            node->data.extern_block.fn_decls.append(child);
        }
    }


    zig_unreachable();
}

/*
RootExportDecl : many(Directive) token(Export) token(Symbol) token(String) token(Semicolon)
*/
static AstNode *ast_parse_root_export_decl(ParseContext *pc, int *token_index, bool mandatory) {
    assert(mandatory == false);

    Token *export_kw = &pc->tokens->at(*token_index);
    if (export_kw->id != TokenIdKeywordExport)
        return nullptr;

    Token *export_type = &pc->tokens->at(*token_index + 1);
    if (export_type->id != TokenIdSymbol)
        return nullptr;

    *token_index += 2;

    AstNode *node = ast_create_node(pc, NodeTypeRootExportDecl, export_kw);
    node->data.root_export_decl.directives = pc->directive_list;
    pc->directive_list = nullptr;

    ast_buf_from_token(pc, export_type, &node->data.root_export_decl.type);

    Token *export_name = &pc->tokens->at(*token_index);
    *token_index += 1;
    ast_expect_token(pc, export_name, TokenIdStringLiteral);

    parse_string_literal(pc, export_name, &node->data.root_export_decl.name);

    Token *semicolon = &pc->tokens->at(*token_index);
    *token_index += 1;
    ast_expect_token(pc, semicolon, TokenIdSemicolon);

    return node;
}

/*
Use : many(Directive) token(Use) token(String) token(Semicolon)
*/
static AstNode *ast_parse_use(ParseContext *pc, int *token_index, bool mandatory) {
    assert(mandatory == false);

    Token *use_kw = &pc->tokens->at(*token_index);
    if (use_kw->id != TokenIdKeywordUse)
        return nullptr;
    *token_index += 1;

    Token *use_name = &pc->tokens->at(*token_index);
    *token_index += 1;
    ast_expect_token(pc, use_name, TokenIdStringLiteral);

    Token *semicolon = &pc->tokens->at(*token_index);
    *token_index += 1;
    ast_expect_token(pc, semicolon, TokenIdSemicolon);

    AstNode *node = ast_create_node(pc, NodeTypeUse, use_kw);

    parse_string_literal(pc, use_name, &node->data.use.path);

    node->data.use.directives = pc->directive_list;
    pc->directive_list = nullptr;

    return node;
}

/*
TopLevelDecl : FnDef | ExternBlock | RootExportDecl | Use
*/
static void ast_parse_top_level_decls(ParseContext *pc, int *token_index, ZigList<AstNode *> *top_level_decls) {
    for (;;) {
        Token *directive_token = &pc->tokens->at(*token_index);
        assert(!pc->directive_list);
        pc->directive_list = allocate<ZigList<AstNode*>>(1);
        ast_parse_directives(pc, token_index, pc->directive_list);

        AstNode *root_export_decl_node = ast_parse_root_export_decl(pc, token_index, false);
        if (root_export_decl_node) {
            top_level_decls->append(root_export_decl_node);
            continue;
        }

        AstNode *fn_def_node = ast_parse_fn_def(pc, token_index, false);
        if (fn_def_node) {
            top_level_decls->append(fn_def_node);
            continue;
        }

        AstNode *extern_node = ast_parse_extern_block(pc, token_index, false);
        if (extern_node) {
            top_level_decls->append(extern_node);
            continue;
        }

        AstNode *use_node = ast_parse_use(pc, token_index, false);
        if (use_node) {
            top_level_decls->append(use_node);
            continue;
        }

        if (pc->directive_list->length > 0) {
            ast_error(pc, directive_token, "invalid directive");
        }
        pc->directive_list = nullptr;

        return;
    }
    zig_unreachable();
}

/*
Root : many(TopLevelDecl) token(EOF)
 */
static AstNode *ast_parse_root(ParseContext *pc, int *token_index) {
    AstNode *node = ast_create_node(pc, NodeTypeRoot, &pc->tokens->at(*token_index));

    ast_parse_top_level_decls(pc, token_index, &node->data.root.top_level_decls);

    if (*token_index != pc->tokens->length - 1) {
        ast_invalid_token_error(pc, &pc->tokens->at(*token_index));
    }

    return node;
}

AstNode *ast_parse(Buf *buf, ZigList<Token> *tokens, ImportTableEntry *owner, ErrColor err_color) {
    ParseContext pc = {0};
    pc.err_color = err_color;
    pc.owner = owner;
    pc.buf = buf;
    pc.tokens = tokens;
    int token_index = 0;
    pc.root = ast_parse_root(&pc, &token_index);
    return pc.root;
}
