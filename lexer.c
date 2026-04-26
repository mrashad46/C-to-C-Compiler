#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include "string.h"
#include <assert.h>
#include <ctype.h>

#define LEX_GETC_IF(buffer, expr, c)     \
    for (c = peekc(); expr; c = peekc()) \
    {                                    \
        buffer_write(buffer, c);         \
        nextc();                         \
    }

static struct lex_process *lex_process;
static struct token temp_token;
struct token *read_the_next_token();
bool is_a_keyword(const char *str);
bool lex_is_in_expression();

struct token *token_create(struct token *_token)
{
    memcpy(&temp_token, _token, sizeof(struct token));
    temp_token.pos = lex_file_position();
    if(lex_is_in_expression())
    {
        temp_token.between_brakets = buffer_ptr(lex_process->parentheses_buffer);
    }
    return &temp_token;
}

static char peekc()
{
    return lex_process->function->peek_char(lex_process);
}

static char nextc()
{
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.col += 1;
    if(lex_is_in_expression())
    {
        buffer_write(lex_process->parentheses_buffer, c);
    }
    if (c == '\n')
    {
        lex_process->pos.line += 1;
        lex_process->pos.col = 1;
    }
    return c;
}

static void pushc(char c)
{

    lex_process->function->push_char(lex_process, c);
}

char assert_next_char(char expected)
{
    char next_c = nextc();
    assert(next_c == expected);
    return next_c;
}

const char *read_number_str()
{
    const char *number = NULL;
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c >= '0' && c <= '9', c);
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

static struct token *lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

static struct token *handle_whitespace()
{
    struct token *last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }
    nextc();
    return read_the_next_token();
}

unsigned long read_number()
{
    const char *s = read_number_str();
    return atol(s);
}

int lexer_number_type(char c)
{
    int res = NUMBER_TYPE_NORMAL;
    if(c == 'L')
    {
        res = NUMBER_TYPE_LONG;
    }
    else if( c = 'f')
    {
        res = NUMBER_TYPE_FLOAT;
    }
    return res;
}

struct token *token_make_number_with_value(unsigned long number)
{
    int number_type = lexer_number_type(peekc());

    if(number_type != NUMBER_TYPE_NORMAL)
    {
        nextc();
    }
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .lnum = number, .num.type = number_type});
}

struct token *token_make_number()
{
    return token_make_number_with_value(read_number());
}

struct token *token_make_string(char start_delimiter, char end_delimiter)
{
    struct buffer *buff = buffer_create();
    assert(nextc() == start_delimiter);
    char c = nextc();
    for (; c != end_delimiter && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            continue;
        }
        buffer_write(buff, c);
    }
    buffer_write(buff, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_STRING, .sval = buffer_ptr(buff)});
}

static bool op_is_treated_as_one(char op)
{
    return op == '(' ||
           op == '[' ||
           op == '{' ||
           op == ',' ||
           op == '.' ||
           op == '*' ||
           op == '!' ||
           op == '?';
}
static bool is_single_operator(char c)
{
    return c == '+' ||
           c == '-' ||
           c == '/' ||
           c == '*' ||
           c == '=' ||
           c == '>' ||
           c == '<' ||
           c == '|' ||
           c == '&' ||
           c == '^' ||
           c == '%' ||
           c == '!' ||
           c == '(' ||
           c == '[' ||
           c == ',' ||
           c == '.' ||
           c == '.' ||
           c == '~' ||
           c == '?';
}

bool op_valid(const char *op)
{
    return S_EQ(op, "+") ||
           S_EQ(op, "-") ||
           S_EQ(op, "*") ||
           S_EQ(op, "/") ||
           S_EQ(op, "%") ||
           S_EQ(op, "++") ||
           S_EQ(op, "--") ||
           S_EQ(op, "==") ||
           S_EQ(op, "!=") ||
           S_EQ(op, ">=") ||
           S_EQ(op, "<=") ||
           S_EQ(op, "+=") ||
           S_EQ(op, "-=") ||
           S_EQ(op, "*=") ||
           S_EQ(op, "/=") ||
           S_EQ(op, "&&") ||
           S_EQ(op, "||") ||
           S_EQ(op, ">>") ||
           S_EQ(op, "<<") ||
           S_EQ(op, "**") ||
           S_EQ(op, "!") ||
           S_EQ(op, "&") ||
           S_EQ(op, "|") ||
           S_EQ(op, "^") ||
           S_EQ(op, "~") ||
           S_EQ(op, "?") ||
           S_EQ(op, ".") ||
           S_EQ(op, "->") ||
           S_EQ(op, "(") ||
           S_EQ(op, "[") ||
           S_EQ(op, "=") ||
           S_EQ(op, ",");
}

void read_op_flush_back_keep_first(struct buffer *buffer)
{
    char *data = buffer_ptr(buffer);
    char len = buffer->len;
    for (int i = len - 1; i >= 1; i--)
    {
        if (data[i] == 0x00)
            continue;
        pushc(data[i]);
    }
}
const char *read_op()
{
    bool single_operator = true;
    char op = nextc();
    struct buffer *buff = buffer_create();
    buffer_write(buff, op);
    if (!op_is_treated_as_one(op))
    {
        op = peekc();
        if (is_single_operator(op))
        {
            buffer_write(buff, op);
            nextc();
            single_operator = false;
        }
    }
    buffer_write(buff, 0x00);
    char *op_str = buffer_ptr(buff);
    if (!single_operator)
    {
        // we have to validate it
        if (!op_valid(op_str))
        {
            read_op_flush_back_keep_first(buff);
            op_str[1] = 0x00; // Truncate to the first character
        }
    }
    else if (!op_valid(op_str))
    {
        compiler_error(lex_process->compiler, "Invalid operator: %s\n", op_str);
    }

    return op_str;
}

static void lex_new_expression()
{
    lex_process->current_expression_count++;
    if (lex_process->current_expression_count == 1)
    {
        lex_process->parentheses_buffer = buffer_create();
    }
}

static void lex_end_expression()
{
    lex_process->current_expression_count--;
    if (lex_process->current_expression_count < 0)
    {
        compiler_error(lex_process->compiler, "missing closing parenthesis\n");
    }
}

bool lex_is_in_expression()
{
    return lex_process->current_expression_count > 0;
}

struct token *token_make_operator_or_string()
{
    char operator = peekc();
    if (operator == '<')
    {
        struct token *last_token = lexer_last_token();
        if (token_is_keyword(last_token, "include"))
        {
            return token_make_string('<', '>');
        }
    }
    struct token *token = token_create(&(struct token){.type = TOKEN_TYPE_OPERATOR, .sval = read_op()});

    if (operator == '(')
    {
        lex_new_expression();
    }

    return token;
}
struct token *token_make_symbol()
{
    char c = nextc();
    if (c == ')')
    {
        lex_end_expression();
    }
    return token_create(&(struct token){.type = TOKEN_TYPE_SYMBOL, .cval = c});
}

struct token *token_make_identifier_or_keyword()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_', c);
    buffer_write(buffer, 0x00);

    if(is_a_keyword(buffer_ptr(buffer)))
    {
        return token_create(&(struct token){.type = TOKEN_TYPE_KEYWORD, .sval = buffer_ptr(buffer)});
    }
    return token_create(&(struct token){.type = TOKEN_TYPE_IDENTIFIER, .sval = buffer_ptr(buffer)});
}

struct token *token_make_special_identifier()
{
    char c = peekc();
    if (isalpha(c) || c == '_')
    {
        return token_make_identifier_or_keyword();
    }
    return NULL;
}

bool is_a_keyword(const char *str)
{
    return S_EQ(str, "if") ||
           S_EQ(str, "else") ||
           S_EQ(str, "while") ||
           S_EQ(str, "for") ||
           S_EQ(str, "return") ||
           S_EQ(str, "break") ||
           S_EQ(str, "continue") ||
           S_EQ(str, "int") ||
           S_EQ(str, "char") ||
           S_EQ(str, "void") ||
           S_EQ(str, "long") ||
           S_EQ(str, "short") ||
           S_EQ(str, "unsigned") ||
           S_EQ(str, "signed") ||
           S_EQ(str, "float") ||
           S_EQ(str, "double") ||
           S_EQ(str, "struct") ||
           S_EQ(str, "union") ||
           S_EQ(str, "enum") ||
           S_EQ(str, "typedef") ||
           S_EQ(str, "const") ||
           S_EQ(str, "volatile") ||
           S_EQ(str, "static") ||
           S_EQ(str, "extern") ||
           S_EQ(str, "register")||
           S_EQ(str, "goto") ||
           S_EQ(str, "sizeof") ||
           S_EQ(str, "switch") ||
           S_EQ(str, "case") ||
           S_EQ(str, "default") ||
           S_EQ(str, "do") ||
           S_EQ(str, "inline") ||
           S_EQ(str, "restrict") ;
}

struct token *token_make_newline()
{
    nextc();
    return token_create(&(struct token){.type = TOKEN_TYPE_NEWLINE});
}

struct token *token_make_oneline_comment(){
    struct buffer* buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c != '\n' && c != EOF,c);
    buffer_write(buffer, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *token_make_multiline_comment(){
    struct buffer* buffer = buffer_create();
    char c = 0;

    while(1)
    {
        LEX_GETC_IF(buffer, c != EOF && c!= '*' ,c);
        if(c == EOF)
        {
            compiler_error(lex_process->compiler, "comment was not closed\n");
        }
        else if(c == '*')
        {
            c = nextc();
            if(peekc() == '/')
            {
                nextc();
                break;
            }
        }

    }
    buffer_write(buffer, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *handle_comment()
{
    char c = peekc();
    if(c == '/')
    {
        nextc();
        if(peekc() == '/')
        {
            return token_make_oneline_comment();
        }
        else if(peekc() == '*')
        {
            return token_make_multiline_comment();
        }else
        {
            pushc('/');
            return token_make_operator_or_string();
        }
    }
        return NULL;
}

char lex_getc_escaped_char(char c)
{
    char co = 0;
    switch (c)
    {
        case 'n':
            co = '\n';
            break;
        case 't':
            co = '\t';
            break;
        case 'r':   
            co = '\r';
            break;
        case '\\':          
            co = '\\';
            break;
        case '\'':
            co = '\'';
            break;
    }

    return co;
}

void lexer_pop_token()
{
    vector_pop(lex_process->token_vec);

}

bool is_hex_char(char c)
{
 c = tolower(c);
 return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

bool is_binary_char(char c)
{

 return (c == '0' || c <= '1');
}

const char *read_hex_number_str()
{
    struct buffer* buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, is_hex_char(c), c );

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

struct token *token_make_special_number_hexadecimal()
{
    nextc();

    unsigned long number = 0;
    const char *number_str = read_hex_number_str();
    number = strtol(number_str, 0, 16);
    return token_make_number_with_value(number);
}

void lexer_validate_binary_string(const char *str)
{
    int len = strlen(str);
    for(int i = 0; i < len; i++)
    {
         if(str[i] != '0' && str[i] != '1')
         {
             compiler_error(lex_process->compiler, " the binary number is not valid \n");
         }
    }
}

struct token *token_make_special_number_binary()
{
    nextc();

    unsigned long number = 0;
    const char *number_str = read_number_str();
    lexer_validate_binary_string(number_str);
    number = strtol(number_str, 0, 2);
    return token_make_number_with_value(number);
}

struct token *token_make_special_number()
{
    struct token *token = NULL;
    //get the last token 
    struct token *last_token = lexer_last_token();
    if(!last_token || (!(last_token->type == TOKEN_TYPE_NUMBER && last_token->lnum == 0)))
    {
        return token_make_identifier_or_keyword();
    }
    lexer_pop_token();

    char c = peekc();
    if(c == 'x')
    {
        token = token_make_special_number_hexadecimal();
    }
    else if(c == 'b')
    {
        token = token_make_special_number_binary();
    }
    return token;

}



struct token *token_make_qoutes()
{
    assert_next_char('\'');
    char c = nextc();
    if(c == '\\')
    {
        c = nextc();
        c = lex_getc_escaped_char(c);

    }

    if(nextc() != '\'')
    {
        compiler_error(lex_process->compiler, "you opened a qoute ' but did not close it with a ' character '\n");

    }

    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .cval = c});
}

struct token *read_the_next_token()
{
    struct token *token = NULL;
    char c = peekc();
    token = handle_comment();
    if(token)
    {
        return token;
    }
    switch (c)
    {
    NUMERIC_CASE:
        token = token_make_number();
        break;
    case 'x':
    case 'b':
        token = token_make_special_number();
        break;
 
    SYMBOL_CASE:
        token = token_make_symbol();
        break;

    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        break;

    case '"':
        token = token_make_string('"', '"');
        break;
    
    case '\'':
        token = token_make_qoutes();
        break;

    case '\n':
        token = token_make_newline();
        break;

    case ' ':
    case '\t':
        token = handle_whitespace();
        break;
    case EOF:

     break; 
    default:
         token = token_make_special_identifier();
         if (!token)
         {
            compiler_error(lex_process->compiler, "Unexpected token \n");
         }
    }
    return token;
}

int lex(struct lex_process *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token *token = read_the_next_token();
    while (token)
    {
        vector_push(process->token_vec, token);
        token = read_the_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK; // Success
}

struct pos lex_file_position()
{
    return lex_process->pos;
}