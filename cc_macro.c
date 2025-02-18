/* Copyright (C) 2021 Sanne Wouda
 * Copyright (C) 2021 Andrius Štikonas <andrius@stikonas.eu>
 * Copyright (C) 2022 Jan (janneke) Nieuwenhuizen <janneke@gnu.org>
 * This file is part of M2-Planet.
 *
 * M2-Planet is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * M2-Planet is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with M2-Planet.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "cc.h"
#include "gcc_req.h"

void require(int bool, char* error);
int strtoint(char* a);
void line_error_token(struct token_list* list);
struct token_list* eat_token(struct token_list* head);

struct conditional_inclusion
{
	struct conditional_inclusion* prev;
	int include; /* 1 == include, 0 == skip */
	int previous_condition_matched; /* 1 == all subsequent conditions treated as FALSE */
};

struct macro_argument
{
	char* name;
	struct macro_argument* next;
};

struct macro_list
{
	struct macro_list* next;
	char* symbol;
	struct token_list* expansion;
	struct macro_argument* arguments;
};

struct macro_list* macro_env;
struct conditional_inclusion* conditional_inclusion_top;

/* point where we are currently modifying the global_token list */
struct token_list* macro_token;

void init_macro_env(char* sym, char* value, char* source, int num)
{
	struct macro_list* hold = macro_env;
	macro_env = calloc(1, sizeof(struct macro_list));
	macro_env->symbol = sym;
	macro_env->next = hold;
	macro_env->expansion = calloc(1, sizeof(struct token_list));
	macro_env->expansion->s = value;
	macro_env->expansion->filename = source;
	macro_env->expansion->linenumber = num;
	macro_env->arguments = NULL;
}

void eat_current_token(void)
{
	int update_global_token = FALSE;
	if (macro_token == global_token)
		update_global_token = TRUE;

	macro_token = eat_token(macro_token);

	if(update_global_token)
		global_token = macro_token;
}

void eat_newline_tokens(void)
{
	macro_token = global_token;

	while(TRUE)
	{
		if(NULL == macro_token) return;

		if(match("\n", macro_token->s))
		{
			eat_current_token();
		}
		else
		{
			macro_token = macro_token->next;
		}
	}
}

/* returns the first token inserted; inserts *before* point */
struct token_list* insert_tokens(struct token_list* point, struct token_list* token)
{
	struct token_list* copy;
	struct token_list* first = NULL;

	while (NULL != token)
	{
		copy = calloc(1, sizeof(struct token_list));
		copy->s = token->s;
		copy->filename = token->filename;
		copy->linenumber = token->linenumber;

		if(NULL == first)
		{
			first = copy;
		}

		copy->next = point;

		if (NULL != point)
		{
			copy->prev = point->prev;

			if(NULL != point->prev)
			{
				point->prev->next = copy;
			}

			point->prev = copy;
		}

		token = token->next;
	}

	return first;
}

struct macro_list* create_replacement_token(char* symbol, struct token_list* token)
{
	struct macro_list* hold = calloc(1, sizeof(struct macro_list));

	hold->next = NULL;
	hold->symbol = symbol;
	hold->expansion = calloc(1, sizeof(struct token_list));

	hold->expansion->prev = NULL;
	hold->expansion->next = NULL;
	hold->expansion->linenumber = token->linenumber;

	/* Make sure this is cleaned up along with the real ones */
	hold->next = macro_env;
	macro_env = hold;

	return hold;
}

struct macro_list* lookup_macro(struct token_list* token)
{
	if(NULL == token)
	{
		line_error_token(macro_token);
		fputs("null token received in lookup_macro\n", stderr);
		exit(EXIT_FAILURE);
	}

	if(match(token->s, "__LINE__"))
	{
		struct macro_list* hold = create_replacement_token("__LINE__", token);

		hold->expansion->s = int2str(token->linenumber, 10, TRUE);

		return hold;
	}
	else if(match(token->s, "__FILE__"))
	{
		struct macro_list* hold = create_replacement_token("__FILE__", token);

		int string_length = 0;
		/* strlen */
		while(token->filename[string_length] != 0)
		{
			string_length = string_length + 1;
		}

		hold->expansion->s = calloc(string_length + 3, sizeof(char));
		hold->expansion->s[0] = '"';
		hold->expansion->s[string_length] = '"';
		hold->expansion->s[string_length + 1] = 0; /* We don't have '\0' */

		/* memcpy */
		int i;
		for(i = 0; i < string_length; i = i + 1)
		{
			hold->expansion->s[i + 1] = token->filename[i];
		}

		return hold;
	}

	struct macro_list* hold = macro_env;

	while (NULL != hold)
	{
		if (match(token->s, hold->symbol))
		{
			/* found! */
			return hold;
		}

		hold = hold->next;
	}

	/* not found! */
	return NULL;
}

void remove_macro(struct token_list* token)
{
	if(NULL == token)
	{
		line_error_token(macro_token);
		fputs("received a null in remove_macro\n", stderr);
		exit(EXIT_FAILURE);
	}

	struct macro_list* hold = macro_env;
	struct macro_list* temp;

	/* Deal with the first element */
	if (match(token->s, hold->symbol)) {
		macro_env = hold->next;
		free(hold);
		return;
	}

	/* Remove element form the middle of linked list */
	while (NULL != hold->next)
	{
		if (match(token->s, hold->next->symbol))
		{
			temp = hold->next;
			hold->next = hold->next->next;
			free(temp);
			return;
		}

		hold = hold->next;
	}

	/* nothing to undefine */
	return;
}

int macro_expression(void);
int macro_variable(void)
{
	int value = 0;
	struct macro_list* hold = lookup_macro(macro_token);
	if (NULL != hold)
	{
		if(NULL == hold->expansion)
		{
			line_error_token(macro_token);
			fputs("hold->expansion is a null\n", stderr);
			exit(EXIT_FAILURE);
		}
		value = strtoint(hold->expansion->s);
	}
	eat_current_token();
	return value;
}

int macro_number(void)
{
	int result = strtoint(macro_token->s);
	eat_current_token();
	return result;
}

int macro_primary_expr(void)
{
	int defined_has_paren = FALSE;
	int hold;
	require(NULL != macro_token, "got an EOF terminated macro primary expression\n");

	if('-' == macro_token->s[0])
	{
		eat_current_token();
		return -macro_primary_expr();
	}
	else if('!' == macro_token->s[0])
	{
		eat_current_token();
		return !macro_primary_expr();
	}
	else if('(' == macro_token->s[0])
	{
		eat_current_token();
		hold = macro_expression();
		require(')' == macro_token->s[0], "missing ) in macro expression\n");
		eat_current_token();
		return hold;
	}
	else if(match("defined", macro_token->s))
	{
		eat_current_token();

		require(NULL != macro_token, "got an EOF terminated macro defined expression\n");

		if('(' == macro_token->s[0])
		{
			defined_has_paren = TRUE;
			eat_current_token();
		}

		if (NULL != lookup_macro(macro_token))
		{
			hold = TRUE;
		}
		else
		{
			hold = FALSE;
		}
		eat_current_token();

		if(TRUE == defined_has_paren)
		{
			if(NULL == macro_token)
			{
				line_error_token(macro_token);
				fputs("unterminated define ( statement\n", stderr);
				exit(EXIT_FAILURE);
			}
			require(')' == macro_token->s[0], "missing close parenthesis for defined()\n");
			eat_current_token();
		}

		return hold;
	}
	else if(in_set(macro_token->s[0], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"))
	{
		return macro_variable();
	}
	else if(in_set(macro_token->s[0], "0123456789"))
	{
		return macro_number();
	}
	else
	{
		return 0;    /* FIXME: error handling */
	}
}

int macro_additive_expr(void)
{
	int lhs = macro_primary_expr();
	int hold;

	require(NULL != macro_token, "got an EOF terminated macro additive expression\n");
	if(match("+", macro_token->s))
	{
		eat_current_token();
		return lhs + macro_additive_expr();
	}
	else if(match("-", macro_token->s))
	{
		eat_current_token();
		return lhs - macro_additive_expr();
	}
	else if(match("*", macro_token->s))
	{
		eat_current_token();
		return lhs * macro_additive_expr();
	}
	else if(match("/", macro_token->s))
	{
		eat_current_token();
		hold = macro_additive_expr();
		require(0 != hold, "divide by zero not valid even in C macros\n");
		return lhs / hold;
	}
	else if(match("%", macro_token->s))
	{
		eat_current_token();
		hold = macro_additive_expr();
		require(0 != hold, "modulus by zero not valid even in C macros\n");
		return lhs % hold;
	}
	else if(match(">>", macro_token->s))
	{
		eat_current_token();
		return lhs >> macro_additive_expr();
	}
	else if(match("<<", macro_token->s))
	{
		eat_current_token();
		return lhs << macro_additive_expr();
	}
	else
	{
		return lhs;
	}
}

int macro_relational_expr(void)
{
	int lhs = macro_additive_expr();

	if(match("<", macro_token->s))
	{
		eat_current_token();
		return lhs < macro_relational_expr();
	}
	else if(match("<=", macro_token->s))
	{
		eat_current_token();
		return lhs <= macro_relational_expr();
	}
	else if(match(">=", macro_token->s))
	{
		eat_current_token();
		return lhs >= macro_relational_expr();
	}
	else if(match(">", macro_token->s))
	{
		eat_current_token();
		return lhs > macro_relational_expr();
	}
	else if(match("==", macro_token->s))
	{
		eat_current_token();
		return lhs == macro_relational_expr();
	}
	else if(match("!=", macro_token->s))
	{
		eat_current_token();
		return lhs != macro_relational_expr();
	}
	else
	{
		return lhs;
	}
}

int macro_bitwise_expr(void)
{
	int rhs;
	int lhs = macro_relational_expr();

	if(match("&", macro_token->s))
	{
		eat_current_token();
		return lhs & macro_bitwise_expr();
	}
	else if(match("&&", macro_token->s))
	{
		eat_current_token();
		rhs = macro_bitwise_expr();
		return lhs && rhs;
	}
	else if(match("|", macro_token->s))
	{
		eat_current_token();
		rhs = macro_bitwise_expr();
		return lhs | rhs;
	}
	else if(match("||", macro_token->s))
	{
		eat_current_token();
		rhs = macro_bitwise_expr();
		return lhs || rhs;
	}
	else if(match("^", macro_token->s))
	{
		eat_current_token();
		rhs = macro_bitwise_expr();
		return lhs ^ rhs;
	}
	else
	{
		return lhs;
	}
}

int macro_expression(void)
{
	return macro_bitwise_expr();
}

void handle_function_like_macro(struct macro_list* hold)
{

	eat_current_token(); /* Skip '(' */
	struct macro_argument* argument = calloc(1, sizeof(struct macro_argument));
	hold->arguments = argument;

	argument->next = NULL;
	/* A NULL name in the first argument means a function macro without any arguments */
	argument->name = NULL;

	while(macro_token->s[0] != ')')
	{
		/* ... is not tokenized as a single token here */
		if(macro_token->s[0] == '.')
		{
			/* Periods can only be in the macro argument list as a variadic parameter
			 * so if there is a period it's part of a variadic parameter */
			require(macro_token->next != NULL, "EOF in variadic parameter");
			require(macro_token->next->s[0] == '.', "Invalid token '.' in macro parameter list");
			require(macro_token->next->next != NULL, "EOF in second variadic parameter");
			require(macro_token->next->next->s[0] == '.', "Invalid tokens '..' in macro parameter list");

			line_error_token(macro_token);
			fputs("Variadic function-like macros not supported.", stderr);
			exit(EXIT_FAILURE);
		}

		argument->name = macro_token->s;
		eat_current_token(); /* skip past name to comma */

		if(macro_token->s[0] == ',')
		{
			argument->next = calloc(1, sizeof(struct macro_argument));
			argument = argument->next;

			eat_current_token(); /* skip comma */
		}
	}

	eat_current_token(); /* skip past ')' */
}

void handle_define(void)
{
	struct macro_list* hold;
	struct token_list* expansion_end = NULL;

	/* don't use #define statements from non-included blocks */
	int conditional_define = TRUE;
	if(NULL != conditional_inclusion_top)
	{
		if(FALSE == conditional_inclusion_top->include)
		{
			conditional_define = FALSE;
		}
	}

	eat_current_token();

	require(NULL != macro_token, "got an EOF terminated #define\n");
	require('\n' != macro_token->s[0], "unexpected newline after #define\n");

	/* insert new macro */
	hold = calloc(1, sizeof(struct macro_list));
	hold->symbol = macro_token->s;
	hold->next = macro_env;
	hold->arguments = NULL;
	/* provided it isn't in a non-included block */
	if(conditional_define) macro_env = hold;

	/* discard the macro name */
	eat_current_token();

	/* This is the only place in which a token can be whitespace
	 * We need this to distinguish between function-like macros
	 * and normal macros with an opening parens as the first token.
	 * #define FUNCTION_MACRO(x)
	 * #define NOT_FUNCTION_MACRO (x)
	 * */
	if(macro_token->s[0] == ' ')
	{
		eat_current_token();
	}
	else if(macro_token->s[0] == '(')
	{
		handle_function_like_macro(hold);
	}

	while (TRUE)
	{
		require(NULL != macro_token, "got an EOF terminated #define\n");

		if ('\n' == macro_token->s[0])
		{
			if(NULL == expansion_end)
			{
				hold->expansion = NULL;
				expansion_end = macro_token;
				return;
			}
			expansion_end->next = NULL;
			return;
		}

		require(NULL != hold, "#define got something it can't handle\n");

		expansion_end = macro_token;

		/* in the first iteration, we set the first token of the expansion, if
		   it exists */
		if (NULL == hold->expansion)
		{
			hold->expansion = macro_token;
		}

		/* throw away if not used */
		if(!conditional_define && (NULL != hold))
		{
			free(hold);
			hold = NULL;
		}

		eat_current_token();
	}
}

void handle_undef(void)
{
	eat_current_token();
	remove_macro(macro_token);
	eat_current_token();
}

void handle_error(int warning_p)
{
	/* don't use #error statements from non-included blocks */
	int conditional_error = TRUE;
	if(NULL != conditional_inclusion_top)
	{
		if(FALSE == conditional_inclusion_top->include)
		{
			conditional_error = FALSE;
		}
	}
	eat_current_token();
	/* provided it isn't in a non-included block */
	if(conditional_error)
	{
		line_error_token(macro_token);
		if(warning_p) fputs(" warning: #warning ", stderr);
		else fputs(" error: #error ", stderr);
		while (TRUE)
		{
			require(NULL != macro_token, "\nFailed to properly terminate error message with \\n\n");
			if ('\n' == macro_token->s[0]) break;
			fputs(macro_token->s, stderr);
			macro_token = macro_token->next;
			fputs(" ", stderr);
		}
		fputs("\n", stderr);
		if(!warning_p) exit(EXIT_FAILURE);
	}
	while (TRUE)
	{
		require(NULL != macro_token, "\nFailed to properly terminate error message with \\n\n");
		/* discard the error */
		if ('\n' == macro_token->s[0])
		{
			return;
		}
		eat_current_token();
	}
}

void eat_block(void);
void macro_directive(void)
{
	struct conditional_inclusion *t;
	int result;

	/* FIXME: whitespace is allowed between "#"" and "if" */
	if(match("#if", macro_token->s))
	{
		eat_current_token();
		/* evaluate constant integer expression */
		result = macro_expression();
		/* push conditional inclusion */
		t = calloc(1, sizeof(struct conditional_inclusion));
		t->prev = conditional_inclusion_top;
		conditional_inclusion_top = t;
		t->include = TRUE;

		if(FALSE == result)
		{
			t->include = FALSE;
			eat_block();
		}

		t->previous_condition_matched = t->include;
	}
	else if(match("#ifdef", macro_token->s))
	{
		eat_current_token();
		require(NULL != macro_token, "got an EOF terminated macro defined expression\n");
		if (NULL != lookup_macro(macro_token))
		{
			result = TRUE;
			eat_current_token();
		}
		else
		{
			result = FALSE;
			eat_block();
		}

		/* push conditional inclusion */
		t = calloc(1, sizeof(struct conditional_inclusion));
		t->prev = conditional_inclusion_top;
		conditional_inclusion_top = t;
		t->include = TRUE;

		if(FALSE == result)
		{
			t->include = FALSE;
		}

		t->previous_condition_matched = t->include;
	}
	else if(match("#ifndef", macro_token->s))
	{
		eat_current_token();
		require(NULL != macro_token, "got an EOF terminated macro defined expression\n");
		if (NULL != lookup_macro(macro_token))
		{
			result = FALSE;
		}
		else
		{
			result = TRUE;
			eat_current_token();
		}

		/* push conditional inclusion */
		t = calloc(1, sizeof(struct conditional_inclusion));
		t->prev = conditional_inclusion_top;
		conditional_inclusion_top = t;
		t->include = TRUE;

		if(FALSE == result)
		{
			t->include = FALSE;
			eat_block();
		}

		t->previous_condition_matched = t->include;
	}
	else if(match("#elif", macro_token->s))
	{
		require(NULL != macro_token->next, "#elif without leading #if\n");
		eat_current_token();
		result = macro_expression();
		require(NULL != conditional_inclusion_top, "#elif without leading #if\n");
		conditional_inclusion_top->include = result && !conditional_inclusion_top->previous_condition_matched;
		conditional_inclusion_top->previous_condition_matched =
		    conditional_inclusion_top->previous_condition_matched || conditional_inclusion_top->include;
		if(FALSE == result)
		{
			eat_block();
		}
	}
	else if(match("#else", macro_token->s))
	{
		require(NULL != macro_token->next, "#else without leading #if\n");
		eat_current_token();
		require(NULL != conditional_inclusion_top, "#else without leading #if\n");
		conditional_inclusion_top->include = !conditional_inclusion_top->previous_condition_matched;
		if(FALSE == conditional_inclusion_top->include)
		{
			eat_block();
		}
	}
	else if(match("#endif", macro_token->s))
	{
		if(NULL == conditional_inclusion_top)
		{
			line_error_token(macro_token);
			fputs("unexpected #endif\n", stderr);
			exit(EXIT_FAILURE);
		}

		eat_current_token();
		/* pop conditional inclusion */
		t = conditional_inclusion_top;
		conditional_inclusion_top = conditional_inclusion_top->prev;
		free(t);
	}
	else if(match("#define", macro_token->s))
	{
		handle_define();
	}
	else if(match("#undef", macro_token->s))
	{
		handle_undef();
	}
	else if(match("#error", macro_token->s))
	{
		handle_error(FALSE);
	}
	else if(match("#warning", macro_token->s))
	{
		handle_error(TRUE);
	}
	else
	{
		if(!match("#include", macro_token->s))
		{
			/* Put a big fat warning but see if we can just ignore */
			fputs(">>WARNING<<\n>>WARNING<<\n", stderr);
			line_error_token(macro_token);
			fputs("feature: ", stderr);
			fputs(macro_token->s, stderr);
			fputs(" unsupported in M2-Planet\nIgnoring line, may result in bugs\n>>WARNING<<\n>>WARNING<<\n\n", stderr);
		}

		/* unhandled macro directive; let's eat until a newline; om nom nom */
		while(TRUE)
		{
			if(NULL == macro_token)
			{
				return;
			}

			if('\n' == macro_token->s[0])
			{
				return;
			}

			eat_current_token();
		}
	}
}


void eat_until_endif(void)
{
	/* This #if block is nested inside of an #if block that needs to be dropped, lose EVERYTHING */
	do
	{
		require(NULL != macro_token, "Unterminated #if block\n");
		if(match("#if", macro_token->s) || match("#ifdef", macro_token->s) || match("#ifndef", macro_token->s))
		{
			eat_current_token();
			eat_until_endif();
		}

		eat_current_token();
		require(NULL != macro_token, "Unterminated #if block\n");
	} while(!match("#endif", macro_token->s));
}

void eat_block(void)
{
	/* This conditional #if block is wrong, drop everything until the #elif/#else/#endif */
	do
	{
		if(match("#if", macro_token->s) || match("#ifdef", macro_token->s) || match("#ifndef", macro_token->s))
		{
			eat_current_token();
			eat_until_endif();
		}

		eat_current_token();
		require(NULL != macro_token, "Unterminated #if block\n");
		if(match("#elif", macro_token->s)) break;
		if(match("#else", macro_token->s)) break;
		if(match("#endif", macro_token->s)) break;
	} while(TRUE);
	require(NULL != macro_token->prev, "impossible #if block\n");

	/* rewind the newline */
	if(match("\n", macro_token->prev->s)) macro_token = macro_token->prev;
}

struct token_list* deep_copy_token_list(struct token_list* from)
{
	if(from == NULL)
	{
		return NULL;
	}

	struct token_list* to = calloc(1, sizeof(struct token_list));

	to->next = deep_copy_token_list(from->next);
	to->locals = from->locals;
	to->prev = from->prev;
	to->s = from->s;
	to->type = from->type;
	to->filename = from->filename;
	to->arguments = from->arguments;
	to->depth = from->depth;
	to->linenumber = from->linenumber;

	return to;
}

struct token_list* maybe_expand(struct token_list* token)
{
	if(NULL == token)
	{
		line_error_token(macro_token);
		fputs("maybe_expand passed a null token\n", stderr);
		exit(EXIT_FAILURE);
	}

	struct macro_list* hold = lookup_macro(token);
	if(NULL == token->next)
	{
		line_error_token(macro_token);
		fputs("we can't expand a null token: ", stderr);
		fputs(token->s, stderr);
		fputc('\n', stderr);
		exit(EXIT_FAILURE);
	}

	if (NULL == hold)
	{
		return token->next;
	}

	token = eat_token(token);

	struct token_list* expansion = hold->expansion;

	if(hold->arguments != NULL)
	{
		if(token->s[0] != '(')
		{
			line_error_token(macro_token);
			fputs("Function-like-macro called without parameter list\n", stderr);
			exit(EXIT_FAILURE);
		}
		token = eat_token(token); /* skip '(' */

		expansion = deep_copy_token_list(hold->expansion);

		struct token_list* token_before_macro = token->prev;

		struct token_list* start_token = token;
		int parens = 1;
		struct macro_argument* argument = hold->arguments;
		struct token_list* expand_list;
		while (parens != 0)
		{
			if(argument == NULL)
			{
				line_error_token(macro_token);
				fputs("Invalid amount of parameters for function-like macro\n", stderr);
				exit(EXIT_FAILURE);
			}

			if(token->s[0] == '(')
			{
				parens = parens + 1;
			}
			else if (token->s[0] == ')')
			{
				parens = parens - 1;
			}

			if ((token->s[0] == ',' && parens == 1) || (token->s[0] == ')' && parens == 0))
			{
				token->prev->next = NULL;
				if(start_token->next != NULL && argument->name != NULL)
				{
					line_error_token(start_token);
					fputs("Function-like macro with arguments of more than one token are not supported.\n", stderr);
					exit(EXIT_FAILURE);
				}

				expand_list = expansion;

				while(expand_list != NULL)
				{
					if(match(expand_list->s, argument->name))
					{
						expand_list->s = start_token->s;
					}
					expand_list = expand_list->next;
				}

				token->prev->next = token;
				start_token = token->next;
				argument = argument->next;
			}

			require(token->next != NULL, "NULL token found in function-like macro arguments\n");
			token = token->next;
		}

		token_before_macro->next = token;
		token->prev = token_before_macro;
	}
	else
	{
		if(hold->arguments != NULL)
		{
			line_error_token(macro_token);
			fputs("Function-like macro '", stderr);
			fputs(hold->symbol, stderr);
			fputs("' was called like normal macro\n", stderr);
			exit(EXIT_FAILURE);
		}
	}

	if (NULL == hold->expansion)
	{
		return token->next;
	}

	struct token_list* hold2 = insert_tokens(token, expansion);

	return hold2->next;
}

void preprocess(void)
{
	int start_of_line = TRUE;
	macro_token = global_token;

	while(NULL != macro_token)
	{
		if(start_of_line && '#' == macro_token->s[0])
		{
			macro_directive();

			if(macro_token)
			{
				if('\n' != macro_token->s[0])
				{
					line_error_token(macro_token);
					fputs("newline expected at end of macro directive\n", stderr);
					fputs("found: '", stderr);
					fputs(macro_token->s, stderr);
					fputs("'\n", stderr);
					exit(EXIT_FAILURE);
				}
			}
		}
		else if('\n' == macro_token->s[0])
		{
			start_of_line = TRUE;
			macro_token = macro_token->next;
		}
		else
		{
			start_of_line = FALSE;
			if(NULL == conditional_inclusion_top)
			{
				macro_token = maybe_expand(macro_token);
			}
			else if(!conditional_inclusion_top->include)
			{
				/* rewrite the token stream to exclude the current token */
				eat_block();
				start_of_line = TRUE;
			}
			else
			{
				macro_token = maybe_expand(macro_token);
			}
		}
	}
}
