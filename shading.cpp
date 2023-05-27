#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <omp.h>
#include <mutex>
#include <queue>
#include "shading.hpp"

#define ERR(x) fprintf(stderr, x);

namespace ShadingLang
{

	std::string Tokenizer::GetStringUntil(char C)
	{
		std::string Out;
		while (_At < _Code.size())
		{
			if (_Code[_At] == C) break;
			Out.push_back(_Code[_At++]);
		}
		return Out;
	}
	std::string Tokenizer::GetStringUntilEither(char C0, char C1)
	{
		std::string Out;
		while (_At < _Code.size())
		{
			if (_Code[_At] == C0 || _Code[_At] == C1) break;
			Out.push_back(_Code[_At++]);
		}
		return Out;
	}
	size_t Tokenizer::TellNext(char C)
	{
		for (int I = _At; I < _Code.size(); I++)
		{
			if (_Code[I] == C) return I;
		}
		return 0xFFFFFFFF;
	}
	size_t Tokenizer::TellNextArgEnd()
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if ((_Code[I] == ',' || _Code[I] == ')') && Counter == 0) return I;
			if (_Code[I] == '(') Counter++;
			if (_Code[I] == ')') Counter--;
		}
		return 0xFFFFFFFF;
	}
	size_t Tokenizer::TellNextInitEnd()
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if ((_Code[I] == ',' || _Code[I] == '}') && Counter == 0) return I;
			if (_Code[I] == '{') Counter++;
			if (_Code[I] == '}') Counter--;
		}
		return 0xFFFFFFFF;
	}

	size_t Tokenizer::TellNextRParamEnd()
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if (_Code[I] == ')' && Counter == 0) return I;
			if (_Code[I] == '(') Counter++;
			if (_Code[I] == ')') Counter--;
		}
		return 0xFFFFFFFF;
	}
	size_t Tokenizer::TellScopeEnd()
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if (_Code[I] == '}' && Counter == 0) return I;
			if (_Code[I] == '{') Counter++;
			if (_Code[I] == '}') Counter--;
		}
		return 0xFFFFFFFF;
	}
	size_t Tokenizer::TellNextSemi()
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if (_Code[I] == ';' && Counter == 0) return I;
			if (_Code[I] == '{') Counter++;
			if (_Code[I] == '}') Counter--;
		}
		return 0xFFFFFFFF;
	}
	size_t Tokenizer::TellNextArgEndCustom(char Delim)
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if ((_Code[I] == Delim || _Code[I] == ')') && Counter == 0) return I;
			if (_Code[I] == '(') Counter++;
			if (_Code[I] == ')') Counter--;
		}
		return 0xFFFFFFFF;
	}
	bool Tokenizer::IsOpChar(char C)
	{
		return C == '+' || C == '-' || C == '*' || C == '/' || C == '=' || C == '<' || C == '>' || C == '.';
	}
	bool Tokenizer::IsOpCharNum(char C)
	{
		return C == '+' || C == '-' || C == '*' || C == '/' || C == '=' || C == '<' || C == '>';
	}
	size_t Tokenizer::TellExprPartEnd()
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if ((IsOpChar(_Code[I]) || _Code[I] == ']' || _Code[I] == ')' || _Code[I] == ',' || _Code[I] == '}' || _Code[I] == ';') && Counter == 0) return I;
			if (_Code[I] == '[') Counter++;
			if (_Code[I] == ']') Counter--;
			if (_Code[I] == '(') Counter++;
			if (_Code[I] == ')') Counter--;
		}
		return 0xFFFFFFFF;
	}
	size_t Tokenizer::TellExprPartEndNum()
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if ((IsOpCharNum(_Code[I]) || _Code[I] == ']' || _Code[I] == ')' || _Code[I] == ',' || _Code[I] == '}' || _Code[I] == ';') && Counter == 0 && _Code[I - 1] != 'f') return I;
			if (_Code[I] == '[') Counter++;
			if (_Code[I] == ']') Counter--;
			if (_Code[I] == '(') Counter++;
			if (_Code[I] == ')') Counter--;
		}
		return 0xFFFFFFFF;
	}
	size_t Tokenizer::TellNextEnd(char C)
	{
		int Counter = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if (_Code[I] == C && Counter == 0) return I;
			if (_Code[I] == '[') Counter++;
			if (_Code[I] == ']') Counter--;
			if (_Code[I] == '{') Counter++;
			if (_Code[I] == '}') Counter--;
			if (_Code[I] == '(') Counter++;
			if (_Code[I] == ')') Counter--;
		}
		return 0xFFFFFFFF;
	}
	std::string Tokenizer::GetStringExprPartEnd()
	{
		size_t End = TellExprPartEnd();

		if (End == 0xFFFFFFFF)
		{
			ERR("Can't find end of subexpr!\n");
			return "";
		}

		std::string Str;
		while (_At < End)
		{
			Str.push_back(_Code[_At++]);
		}
		return Str;
	}
	VarType Tokenizer::Str2Type(std::string X)
	{
		if (X == "vec2") return TYPE_VEC2;
		else if (X == "vec3") return TYPE_VEC3;
		else if (X == "vec4") return TYPE_VEC4;
		else if (X == "float") return TYPE_FLOAT;
		else if (X == "int") return TYPE_INT;
		else if (X == "tex2d") return TYPE_TEXTURE;
		return TYPE_UNKNOWN;
	}

	bool Tokenizer::TokenizeTempAssertName(std::string Name)
	{
		int J = 0;
		for (int I = _At; I < _Code.size(); I++)
		{
			if (J == Name.size())
			{
				_At = I;
				return true;
			}
			if (_Code[I] != Name[J]) return false;
			J++;
		}
		return false;
	}

	void Tokenizer::TokenizeTempNameType(std::string& Name, VarType& Type, char Delim)
	{
		std::string TypeName = GetStringUntil(':');
		Type = Str2Type(TypeName);
		_At++;

		if (Type == TYPE_UNKNOWN)
		{
			ERR("Unknown type before name!\n");
			return;
		}

		while (_Code[_At] != Delim)
		{
			Name.push_back(_Code[_At++]);
		}
	}
	Variable* Tokenizer::TokenizeTempParam()
	{
		Variable* Var = new Variable();

		if (_Code[_At] == '&')
		{
			Var->Ref = true;
			_At++;
		}

		std::string TypeName = GetStringUntil(':');
		_At++;
		std::string ParamName = GetStringUntilEither(',', ')');

		Var->Type = Str2Type(TypeName);
		Var->Name = ParamName;
		return Var;
	}
	void Tokenizer::TokenizeTempParams(Scope* OutScope)
	{
		if (_Code[_At] != '(')
		{
			ERR("Internal error! Params couldn't find (\n");
			return;
		}
		while (_Code[_At] != ')')
		{
			_At++;
			OutScope->Variables.push_back(TokenizeTempParam());
		}
		_At++;
	}
	Token* Tokenizer::TokenizeTempInit(Scope* OutScope)
	{
		if (_Code[_At] != '{')
		{
			ERR("Internal error! Initializer list couldn't find {\n");
			return 0;
		}
		_At++;
		if (_Code[_At] == '}')
		{
			ERR("Empty initializer lists are illegal\n");
			return 0;
		}
		Token* Tok = new Token();
		Tok->Type = TOK_INITIALIZER;
		size_t End = TellNextInitEnd();
		while (_Code[_At - 1] != '}')
		{
			End = TellNextInitEnd();
			Tok->InitArgs.push_back(TokenizeExpr(OutScope, TellNextInitEnd()));
		}
		_At = End + 1;
		return Tok;
	}
	bool Tokenizer::IsDigit(char C)
	{
		return C >= '0' && C <= '9';
	}
	Variable* Tokenizer::FindVariableIn(Scope* S, std::string Name)
	{
		for (Variable* Var : S->Variables)
		{
			if (Var->Name == Name)
			{
				return Var;
			}
		}
		if (S->Parent) return FindVariableIn(S->Parent, Name);
		return 0;
	}
	Token* Tokenizer::TokenizeIf(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_IF;
		Tok->NextScope = new Scope();
		Tok->NextScope->Parent = CurScope;
		Tok->First = TokenizeExpr(Tok->NextScope, TellNextEnd(')'));
		TokenizeTempCode(Tok->NextLines, Tok->NextScope);
		return Tok;
	}
	Token* Tokenizer::TokenizeWhile(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_WHILE;
		Tok->NextScope = new Scope();
		Tok->NextScope->Parent = CurScope;
		Tok->First = TokenizeExpr(Tok->NextScope, TellNextEnd(')'));
		TokenizeTempCode(Tok->NextLines, Tok->NextScope);
		return Tok;
	}
	Token* Tokenizer::TokenizeSample(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_SAMPLE;
		Tok->First = TokenizeExpr(CurScope, TellNextEnd(','));
		Tok->Second = TokenizeExpr(CurScope, TellNextEnd(')'));
		return Tok;
	}
	Token* Tokenizer::TokenizeDot(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_DOT;
		Tok->First = TokenizeExpr(CurScope, TellNextEnd(','));
		Tok->Second = TokenizeExpr(CurScope, TellNextEnd(')'));
		return Tok;
	}
	Token* Tokenizer::TokenizeCross(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_CROSS;
		Tok->First = TokenizeExpr(CurScope, TellNextEnd(','));
		Tok->Second = TokenizeExpr(CurScope, TellNextEnd(')'));
		return Tok;
	}
	Token* Tokenizer::TokenizeLength(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_LENGTH;
		Tok->First = TokenizeExpr(CurScope, TellNextEnd(')'));
		return Tok;
	}
	Token* Tokenizer::TokenizeAbs(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_ABS;
		Tok->First = TokenizeExpr(CurScope, TellNextEnd(')'));
		return Tok;
	}
	Token* Tokenizer::TokenizeSin(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_SIN;
		Tok->First = TokenizeExpr(CurScope, TellNextEnd(')'));
		return Tok;
	}
	Token* Tokenizer::TokenizeCos(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_COS;
		Tok->First = TokenizeExpr(CurScope, TellNextEnd(')'));
		return Tok;
	}
	Token* Tokenizer::TokenizeClamp01(Scope* CurScope)
	{
		Token* Tok = new Token();
		Tok->Type = TOK_CLAMP01;
		Tok->First = TokenizeExpr(CurScope, TellNextEnd(')'));
		return Tok;
	}
	Token* Tokenizer::TokenizeExprPart(Scope* CurScope)
	{
		// Scan for built-in functions
		size_t OldAt = _At;
		std::string Builtin = GetStringUntil('(');
		_At++;
		if (Builtin == "if")
		{
			
			return TokenizeIf(CurScope);
		}
		else if (Builtin == "while")
		{
			return TokenizeWhile(CurScope);
		}
		else if (Builtin == "sample")
		{
			return TokenizeSample(CurScope);
		}
		else if (Builtin == "dot")
		{
			return TokenizeDot(CurScope);
		}
		else if (Builtin == "cross")
		{
			return TokenizeCross(CurScope);
		}
		else if (Builtin == "length")
		{
			return TokenizeLength(CurScope);
		}
		else if (Builtin == "abs")
		{
			return TokenizeAbs(CurScope);
		}
		else if (Builtin == "sin")
		{
			return TokenizeSin(CurScope);
		}
		else if (Builtin == "cos")
		{
			return TokenizeCos(CurScope);
		}
		else if (Builtin == "clamp01")
		{
			return TokenizeClamp01(CurScope);
		}
		_At = OldAt;

		if (_Code[_At] == '{')
		{
			return TokenizeTempInit(CurScope);
		}
		if (_Code[_At] == '(')
		{
			_At++;
			size_t End = TellNextRParamEnd();
			Token* Tok = TokenizeExpr(CurScope, End);
			return Tok;
		}
		Token* Tok = new Token();
		size_t End = TellExprPartEnd();
		if (IsDigit(_Code[_At]))
		{
			
			End = TellExprPartEndNum();
			if (_Code[_At + 1] == 'f')
			{
				_At += 2;
				std::string fStr;
				while (_At < End)
				{
					if (!IsDigit(_Code[_At]) && _Code[_At] != '.' && _Code[_At] != '-')
					{
						ERR("Detected non-digit in number\n");
						return 0;
					}
					fStr.push_back(_Code[_At++]);
				}
				Tok->Type = TOK_CONST;
				Tok->Const.Type = TYPE_FLOAT;
				Tok->Const.X = std::atof(fStr.c_str());
			}
			else
			{
				std::vector<int> Digits;
				while (_At < End)
				{
					if (!IsDigit(_Code[_At]))
					{
						ERR("Detected non-digit in number\n");
						return 0;
					}
					Digits.insert(Digits.begin(), _Code[_At++] - '0');
				}
				int Val = 0;
				int Multiply = 1;
				for (int Digit : Digits)
				{
					Val += Digit * Multiply;
					Multiply *= 10;
				}
				Tok->Type = TOK_CONST;
				Tok->Const.Type = TYPE_INT;
				Tok->Const.I = Val;
			}
		}
		else
		{
			Tok->Type = TOK_VAR;

			std::string Name;
			while (_At < End && _Code[_At] != '.' && _Code[_At] != ':')
			{
				Name.push_back(_Code[_At++]);
			}
			if (_Code[_At] == ':')
			{
				VarType Type = Str2Type(Name);
				if (Type == TYPE_UNKNOWN)
				{
					ERR("Variable decl has unknown type\n");
					return NULL;
				}
				_At++;
				Name = GetStringExprPartEnd();
				Variable* NewVar = new Variable();
				NewVar->Name = Name;
				NewVar->Type = Type;
				CurScope->Variables.push_back(NewVar);
				Tok->Var = NewVar;
			}
			else
			{
				Tok->Var = FindVariableIn(CurScope, Name);
				if (Tok->Var == 0)
				{
					fprintf(stderr, "Variable '%s' not found\n", Name.c_str());
					return 0;
				}
				
			}
		}
		
		return Tok;
	}
	TokenType Tokenizer::TokenizeOperator()
	{
		std::string S;
		while (IsOpChar(_Code[_At]))
		{
			S.push_back(_Code[_At++]);
		}
		if (S == "+") return TOK_ADD;
		else if (S == "-") return TOK_SUB;
		else if (S == "*") return TOK_MUL;
		else if (S == "/") return TOK_DIV;
		else if (S == ".") return TOK_SWIZZLE;
		else if (S == "=") return TOK_ASSIGN;
		else if (S == "<") return TOK_CMPLT;
		else if (S == ">") return TOK_CMPGT;
		else if (S == "==") return TOK_CMPEQ;
		return TOK_ADD;
	}
	Token* Tokenizer::TokenizeExpr(Scope* CurScope, size_t End)
	{
		if (_At == End)
		{
			ERR("Empty expressions are illegal\n");
			return 0;
		}

		Token* First = TokenizeExprPart(CurScope);

		while (_At < End)
		{
			TokenType Type = TokenizeOperator();
			Token* SurroundTok = new Token();
			if (Type == TOK_SWIZZLE)
			{
				SurroundTok->Type = Type;
				SurroundTok->First = First;
				size_t NewEnd = TellExprPartEnd();
				while (_At < NewEnd)
				{
					SurroundTok->Swizzle.push_back(_Code[_At++]);
				}
				First = SurroundTok;
				continue;
			}
			if (Type == TOK_ASSIGN)
			{
				SurroundTok->First = First;
				SurroundTok->Second = TokenizeExpr(CurScope, End);
				SurroundTok->Type = Type;
				return SurroundTok;
			}
			SurroundTok->First = First;
			SurroundTok->Second = TokenizeExprPart(CurScope);
			SurroundTok->Type = Type;
			First = SurroundTok;
		}
		_At = End + 1;
		return First;
	}

	void Tokenizer::TokenizeTempCode(std::vector<Token*>& OutLines, Scope* OutScope)
	{
		if (_Code[_At] != '{')
		{
			ERR("Internal error! Code couldn't find {\n");
			return;
		}
		_At++;
		size_t ScopeEnd = TellScopeEnd();
		while (_At < ScopeEnd)
		{
			size_t NextSemi = TellNextSemi();
			OutLines.push_back(TokenizeExpr(OutScope, NextSemi));
			_At = NextSemi + 1;
		}
		_At++;
	}

	Token* Tokenizer::TokenizeTempArg(Scope* CurScope)
	{
		size_t NextArgEnd = TellNextArgEnd();
		if (NextArgEnd == 0xFFFFFFFF)
		{
			ERR("Argument is expected to have , or ) after it\n");
			return 0;
		}
		return TokenizeExpr(CurScope, NextArgEnd);
	}

	Token* Tokenizer::TokenizeTempArgDelim(Scope* CurScope, char Delim)
	{
		size_t NextArgEnd = TellNextArgEndCustom(Delim);
		if (NextArgEnd == 0xFFFFFFFF)
		{
			ERR("Argument is expected to have , or ) after it\n");
			return 0;
		}
		return TokenizeExpr(CurScope, NextArgEnd);
	}

	Function* Tokenizer::TokenizeFunction()
	{
		Function* Func = new Function();
		Func->BaseScope = new Scope();
		TokenizeTempNameType(Func->Name, Func->ReturnType, '(');
		TokenizeTempParams(Func->BaseScope);
		Func->ArgCount = Func->BaseScope->Variables.size();
		TokenizeTempCode(Func->Lines, Func->BaseScope);
		return Func;
	}
	std::vector<Function*> Tokenizer::Tokenize(std::string Code)
	{
		_Functions.erase(_Functions.begin(), _Functions.end());
		_At = 0;
		_Code = Code;
		_Code.erase(std::remove(_Code.begin(), _Code.end(), ' '));
		_Code.erase(std::remove(_Code.begin(), _Code.end(), '\n'));
		_Code.erase(std::remove(_Code.begin(), _Code.end(), '\t'));
		_Code.erase(_Code.begin() + _Code.find('$'), _Code.end());
		while (_At < _Code.size())
		{
			_Functions.push_back(TokenizeFunction());
		}
		return _Functions;
	}

	Variable ExecuteToken(Token* Tok)
	{
		if (Tok->Type == TOK_VAR)
		{
			return *Tok->Var;
		}
		else if (Tok->Type == TOK_SWIZZLE)
		{
			if (Tok->Swizzle.size() > 4 || Tok->Swizzle.size() == 0)
			{
				ERR("Invalid swizzle\n");
				return {};
			}
			Variable First = ExecuteToken(Tok->First);
			Variable Out;
			int Counter = 0;
			for (char C : Tok->Swizzle)
			{
				float Val;
				if (C == 'x')
				{
					Val = First.X;
				}
				else if (C == 'y')
				{
					Val = First.Y;
				}
				else if (C == 'z')
				{
					Val = First.Z;
				}
				else if (C == 'w')
				{
					Val = First.W;
				}
				else if (C == 'i')
				{
					Val = First.I;
				}
				if (Counter == 0)
				{
					Out.X = Val;
				}
				else if (Counter == 1)
				{
					Out.Y = Val;
				}
				else if (Counter == 2)
				{
					Out.Z = Val;
				}
				else if (Counter == 3)
				{
					Out.W = Val;
				}
				Counter++;
			}
			if (Counter == 1)
			{
				Out.Type = TYPE_FLOAT;
			}
			else if (Counter == 2)
			{
				Out.Type = TYPE_VEC2;
			}
			else if (Counter == 3)
			{
				Out.Type = TYPE_VEC3;
			}
			else if (Counter == 4)
			{
				Out.Type = TYPE_VEC4;
			}
			return Out;
		}
		else if (Tok->Type == TOK_CONST)
		{
			return Tok->Const;
		}
		else if (Tok->Type == TOK_IF)
		{
			Variable Conditional = ExecuteToken(Tok->First);
			if (Conditional.Conditional)
			{
				for (Token* Line : Tok->NextLines)
				{
					ExecuteToken(Line);
				}
			}
			return {};
		}
		else if (Tok->Type == TOK_WHILE)
		{
			while (true)
			{
				Variable Conditional = ExecuteToken(Tok->First);
				if (!Conditional.Conditional) break;
				for (Token* Line : Tok->NextLines)
				{
					ExecuteToken(Line);
				}
			}
			return {};
		}
		else if (Tok->Type == TOK_SAMPLE)
		{
			Variable Texture = ExecuteToken(Tok->First);
			if (Texture.Type != TYPE_TEXTURE)
			{
				ERR("Attempted to sample non-texture\n");
				return {};
			}
			Variable Coord = ExecuteToken(Tok->Second);
			if (Coord.Type != TYPE_VEC2)
			{
				ERR("Attempted to sample texture with non-vec2:\n");
				return {};
			}

			int CoordX = fmod(Coord.X, 1.0f) * (Texture.Tex.ResX - 1);
			int CoordY = fmod(Coord.Y, 1.0f) * (Texture.Tex.ResY - 1);

			Variable Out;
			Out.Type = TYPE_VEC3;
			Out.X = (*Texture.Tex.Data)[(CoordX + CoordY * Texture.Tex.ResX) * 3 + 0];
			Out.Y = (*Texture.Tex.Data)[(CoordX + CoordY * Texture.Tex.ResX) * 3 + 1];
			Out.Z = (*Texture.Tex.Data)[(CoordX + CoordY * Texture.Tex.ResX) * 3 + 2];

			return Out;
		}
		else if (Tok->Type == TOK_ASSIGN)
		{
			if (Tok->First->Type != TOK_VAR)
			{
				ERR("Expected first operand of '=' to be an lvalue\n");
				return {};
			}
			std::string OldName = Tok->First->Var->Name;
			Variable Second = ExecuteToken(Tok->Second);
			if (Tok->First->Var->Type != Second.Type)
			{
				ERR("Operands of '=' don't match\n");
				return {};
			}
			*Tok->First->Var = Second;
			Tok->First->Var->Name = OldName;
			return *Tok->First->Var;
		}
		else if (Tok->Type == TOK_CMPEQ)
		{
			Variable First = ExecuteToken(Tok->First);
			Variable Second = ExecuteToken(Tok->Second);
			if (First.Type != Second.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}
			if (First.Type != TYPE_INT && First.Type != TYPE_FLOAT)
			{
				ERR("Can only use `int` or `float` for comparisons\n");
				return {};
			}
			Variable Out;
			Out.Conditional = (First.Type == TYPE_FLOAT ? First.X : First.I) == (Second.Type == TYPE_FLOAT ? Second.X : Second.I);
			return Out;
		}
		else if (Tok->Type == TOK_CMPLT)
		{
			Variable First = ExecuteToken(Tok->First);
			Variable Second = ExecuteToken(Tok->Second);
			if (First.Type != Second.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}
			if (First.Type != TYPE_INT && First.Type != TYPE_FLOAT)
			{
				ERR("Can only use `int` or `float` for comparisons\n");
				return {};
			}
			Variable Out;
			Out.Conditional = (First.Type == TYPE_FLOAT ? First.X : First.I) < (Second.Type == TYPE_FLOAT ? Second.X : Second.I);
			return Out;
		}
		else if (Tok->Type == TOK_CMPGT)
		{
			Variable First = ExecuteToken(Tok->First);
			Variable Second = ExecuteToken(Tok->Second);
			if (First.Type != Second.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}
			if (First.Type != TYPE_INT && First.Type != TYPE_FLOAT)
			{
				ERR("Can only use `int` or `float` for comparisons\n");
				return {};
			}
			Variable Out;
			Out.Conditional = (First.Type == TYPE_FLOAT ? First.X : First.I) > (Second.Type == TYPE_FLOAT ? Second.X : Second.I);
			return Out;
		}
		else if (Tok->Type == TOK_ADD)
		{
			Variable First = ExecuteToken(Tok->First);
			Variable Second = ExecuteToken(Tok->Second);
			if (First.Type != Second.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}
			if (First.Type == TYPE_VEC2)
			{
				First.X += Second.X;
				First.Y += Second.Y;
			}
			else if (First.Type == TYPE_VEC3)
			{
				First.X += Second.X;
				First.Y += Second.Y;
				First.Z += Second.Z;
			}
			else if (First.Type == TYPE_VEC4)
			{
				First.X += Second.X;
				First.Y += Second.Y;
				First.Z += Second.Z;
				First.W += Second.W;
			}
			else if (First.Type == TYPE_FLOAT)
			{
				First.X += Second.X;
			}
			else if (First.Type == TYPE_INT)
			{
				First.I += Second.I;
			}
			return First;
		}
		else if (Tok->Type == TOK_SUB)
		{
			Variable First = ExecuteToken(Tok->First);
			Variable Second = ExecuteToken(Tok->Second);
			if (First.Type != Second.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}
			if (First.Type == TYPE_VEC2)
			{
				First.X -= Second.X;
				First.Y -= Second.Y;
			}
			else if (First.Type == TYPE_VEC3)
			{
				First.X -= Second.X;
				First.Y -= Second.Y;
				First.Z -= Second.Z;
			}
			else if (First.Type == TYPE_VEC4)
			{
				First.X -= Second.X;
				First.Y -= Second.Y;
				First.Z -= Second.Z;
				First.W -= Second.W;
			}
			else if (First.Type == TYPE_FLOAT)
			{
				First.X -= Second.X;
			}
			else if (First.Type == TYPE_INT)
			{
				First.I -= Second.I;
			}
			return First;
		}
		else if (Tok->Type == TOK_MUL)
		{
			Variable First = ExecuteToken(Tok->First);
			Variable Second = ExecuteToken(Tok->Second);
			if (First.Type != Second.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}
			if (First.Type == TYPE_VEC2)
			{
				First.X *= Second.X;
				First.Y *= Second.Y;
			}
			else if (First.Type == TYPE_VEC3)
			{
				First.X *= Second.X;
				First.Y *= Second.Y;
				First.Z *= Second.Z;
			}
			else if (First.Type == TYPE_VEC4)
			{
				First.X *= Second.X;
				First.Y *= Second.Y;
				First.Z *= Second.Z;
				First.W *= Second.W;
			}
			else if (First.Type == TYPE_FLOAT)
			{
				First.X *= Second.X;
			}
			else if (First.Type == TYPE_INT)
			{
				First.I *= Second.I;
			}
			return First;
		}
		else if (Tok->Type == TOK_DIV)
		{
			Variable First = ExecuteToken(Tok->First);
			Variable Second = ExecuteToken(Tok->Second);
			if (First.Type != Second.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}
			if (First.Type == TYPE_VEC2)
			{
				First.X /= Second.X;
				First.Y /= Second.Y;
			}
			else if (First.Type == TYPE_VEC3)
			{
				First.X /= Second.X;
				First.Y /= Second.Y;
				First.Z /= Second.Z;
			}
			else if (First.Type == TYPE_VEC4)
			{
				First.X /= Second.X;
				First.Y /= Second.Y;
				First.Z /= Second.Z;
				First.W /= Second.W;
			}
			else if (First.Type == TYPE_FLOAT)
			{
				First.X /= Second.X;
			}
			else if (First.Type == TYPE_INT)
			{
				First.I /= Second.I;
			}
			return First;
		}
		else if (Tok->Type == TOK_INITIALIZER)
		{
			if (Tok->InitArgs.size() > 4)
			{
				ERR("Too many values in initializer list!\n");
			}
			if (Tok->InitArgs.size() == 1)
			{
				Variable V0 = ExecuteToken(Tok->InitArgs[0]);
				return V0;
			}
			else if (Tok->InitArgs.size() == 2)
			{
				Variable V0 = ExecuteToken(Tok->InitArgs[0]);
				Variable V1 = ExecuteToken(Tok->InitArgs[1]);

				Variable Out;
				Out.Type = TYPE_VEC2;
				Out.X = V0.Type == TYPE_FLOAT ? V0.X : V0.I;
				Out.Y = V1.Type == TYPE_FLOAT ? V1.X : V1.I;
				return Out;
			}
			else if (Tok->InitArgs.size() == 3)
			{
				Variable V0 = ExecuteToken(Tok->InitArgs[0]);
				Variable V1 = ExecuteToken(Tok->InitArgs[1]);
				Variable V2 = ExecuteToken(Tok->InitArgs[2]);

				Variable Out;
				Out.Type = TYPE_VEC3;
				Out.X = V0.Type == TYPE_FLOAT ? V0.X : V0.I;
				Out.Y = V1.Type == TYPE_FLOAT ? V1.X : V1.I;
				Out.Z = V2.Type == TYPE_FLOAT ? V2.X : V2.I;
				return Out;
			}
			else if (Tok->InitArgs.size() == 4)
			{
				Variable V0 = ExecuteToken(Tok->InitArgs[0]);
				Variable V1 = ExecuteToken(Tok->InitArgs[1]);
				Variable V2 = ExecuteToken(Tok->InitArgs[2]);
				Variable V3 = ExecuteToken(Tok->InitArgs[3]);

				Variable Out;
				Out.Type = TYPE_VEC4;
				Out.X = V0.Type == TYPE_FLOAT ? V0.X : V0.I;
				Out.Y = V1.Type == TYPE_FLOAT ? V1.X : V1.I;
				Out.Z = V2.Type == TYPE_FLOAT ? V2.X : V2.I;
				Out.W = V3.Type == TYPE_FLOAT ? V3.X : V3.I;
				return Out;
			}
			ERR("Initializer list failed\n");
			return {};
		}
	}
	void ExecuteCode(std::vector<Function*> Compiled, std::string Entrypoint)
	{
		for (Function* Func : Compiled)
		{
			if (Func->Name == Entrypoint)
			{
				for (Token* Line : Func->Lines)
				{
					ExecuteToken(Line);
				}
			}
		}
	}
	void Manager::Use(std::string Filename)
	{
		std::ifstream file(Filename);
		std::stringstream s;
		s << file.rdbuf();

		Tokenizer tokenizer;
		_Instance = tokenizer.Tokenize(s.str());
		Executable Exec;
		Exec.Compile(_Instance);
		for (int I = 0; I < omp_get_num_procs(); I++)
		{
			Executable* NewExec = new Executable();
			NewExec->_Code = Exec._Code;
			NewExec->_Vars = Exec._Vars;
			NewExec->_Consts = Exec._Consts;
			NewExec->_OutVars = Exec._OutVars;
			_Execs.push_back(NewExec);
		}
	}
	void ExecuteThread(Executable* WorkOn, std::mutex* CountMux, std::atomic<int>* Count, int ResX, int ResY, uint32_t* OutBuff, float* DepthBuff)
	{
		Variable* FragCoord = WorkOn->GetVariable("FragCoord");
		Variable* OutColor = WorkOn->GetVariable("OutColor");
		size_t CacheIndex = 0;
		for (Variable* _Var : WorkOn->_Vars)
		{
			if (_Var == OutColor)
			{
				break;
			}
			CacheIndex++;
		}

		while (*Count < ResX * ResY)
		{
			CountMux->lock();
			(*Count)++;
			int CurX = ((*Count) % (ResX * ResY)) % ResX;
			int CurY = ((*Count) % (ResX * ResY)) / ResX;
			CountMux->unlock();
			FragCoord->X = CurX;
			FragCoord->Y = CurY;
			for (int I = 0; I < WorkOn->_Vars.size();I++)
			{ 
				WorkOn->_OutVars[I] = *WorkOn->_Vars[I];
			}
			if (DepthBuff[CurX + CurY * ResX] == 0.0f)
			{
				OutBuff[CurX + CurY * ResX] = 0xFF000000;
				continue;
			}
			
			WorkOn->_Mux.lock();
			
			WorkOn->Execute();
			uint32_t R = fmax(WorkOn->_OutVars[CacheIndex].X, 0) * 255;
			uint32_t G = fmax(WorkOn->_OutVars[CacheIndex].Y, 0) * 255;
			uint32_t B = fmax(WorkOn->_OutVars[CacheIndex].Z, 0) * 255;

			R = fmin(R, 255);
			G = fmin(G, 255);
			B = fmin(B, 255);

			OutBuff[CurX + CurY * ResX] = 0xFF000000 | (R << 16) | (G << 8) | B;
			WorkOn->_Mux.unlock();
		}
	}
	void Manager::Execute(int ResX, int ResY, uint32_t* OutBuff, float* DepthBuff)
	{
		const size_t NumExecs = _Execs.size();
		const size_t NumProcs = omp_get_num_procs();
		_Count = 0;
		for (int I = 0; I < NumProcs - 1; I++)
		{
			std::thread* CurThread = new std::thread(ExecuteThread, _Execs[I], &_CountMutex, &_Count, ResX, ResY, OutBuff, DepthBuff);
			_Threads.push_back(CurThread);
		}
	}
	Variable* Manager::GetVar(std::string FuncName, std::string VarName)
	{
		return _Execs[0]->GetVariable(VarName);
	}
	int Manager::GetCount()
	{
		return _Count;
	}
	void Manager::Wait(int ResX, int ResY)
	{
		while (_Count < ResX * ResY);
	}
	void Executable::Execute()
	{
		_Ip = 0;
		while (_Ip < _Code.size())
		{
			uint8_t First = _Code[_Ip++];

			if (First < TOK_FIRSTPOINT_)
			{
				if (First == TOK_ASSIGN_TO_REG)
				{
					// ASSIGN TO REG
					uint8_t OP0 = _Code[_Ip++];
					uint16_t OP1 = _Code[_Ip++];
					OP1 |= _Code[_Ip++] << 8;
					_Regs[OP0] = _OutVars[OP1];
				}
				else if (First == TOK_ASSIGN_TO_REG_CONST)
				{
					// ASSIGN TO REG (CONST)
					uint8_t OP0 = _Code[_Ip++];
					uint16_t OP1 = _Code[_Ip++];
					OP1 |= _Code[_Ip++] << 8;
					_Regs[OP0] = _Consts[OP1];
				}

				else if (First == TOK_ADD)
				{
					// ADD
					uint8_t OP0 = _Code[_Ip++];
					uint8_t OP1 = _Code[_Ip++];
					_Regs[OP0].X += _Regs[OP1].X;
					_Regs[OP0].Y += _Regs[OP1].Y;
					_Regs[OP0].Z += _Regs[OP1].Z;
					_Regs[OP0].W += _Regs[OP1].W;
					_Regs[OP0].I += _Regs[OP1].I;
				}
				else if (First == TOK_SUB)
				{
					// SUB
					uint8_t OP0 = _Code[_Ip++];
					uint8_t OP1 = _Code[_Ip++];
					_Regs[OP0].X -= _Regs[OP1].X;
					_Regs[OP0].Y -= _Regs[OP1].Y;
					_Regs[OP0].Z -= _Regs[OP1].Z;
					_Regs[OP0].W -= _Regs[OP1].W;
					_Regs[OP0].I -= _Regs[OP1].I;
				}
				else if (First == TOK_MUL)
				{
					// MUL
					uint8_t OP0 = _Code[_Ip++];
					uint8_t OP1 = _Code[_Ip++];
					_Regs[OP0].X *= _Regs[OP1].X;
					_Regs[OP0].Y *= _Regs[OP1].Y;
					_Regs[OP0].Z *= _Regs[OP1].Z;
					_Regs[OP0].W *= _Regs[OP1].W;
					_Regs[OP0].I *= _Regs[OP1].I;
				}
			}
			else
			{
				if (First < TOK_SECONDPOINT_)
				{
					if (First == TOK_SWIZZLE)
					{
						// SWIZZLE
						uint8_t OP0 = _Code[_Ip++];
						Variable Out;
						for (int I = 0; I < 4; I++)
						{
							uint8_t Swiz = _Code[_Ip++];
							if (Swiz == 0) continue;
							float Val;
							if (Swiz == 'x')
							{
								Val = _Regs[OP0].X;
							}
							else if (Swiz == 'y')
							{
								Val = _Regs[OP0].Y;
							}
							else if (Swiz == 'z')
							{
								Val = _Regs[OP0].Z;
							}
							else if (Swiz == 'w')
							{
								Val = _Regs[OP0].W;
							}
							else if (Swiz == 'i')
							{
								Val = _Regs[OP0].I;
							}

							if (I == 0)
							{
								Out.X = Val;
							}
							else if (I == 1)
							{
								Out.Y = Val;
							}
							else if (I == 2)
							{
								Out.Z = Val;
							}
							else if (I == 3)
							{
								Out.W = Val;
							}
						}
						_Regs[OP0] = Out;
					}
					else if (First == TOK_DIV)
					{
						// DIV
						uint8_t OP0 = _Code[_Ip++];
						uint8_t OP1 = _Code[_Ip++];
						_Regs[OP0].X /= _Regs[OP1].X;
						_Regs[OP0].Y /= _Regs[OP1].Y;
						_Regs[OP0].Z /= _Regs[OP1].Z;
						_Regs[OP0].W /= _Regs[OP1].W;
						if (_Regs[OP1].I != 0) _Regs[OP0].I /= _Regs[OP1].I;
					}
					else if (First == TOK_SAMPLE)
					{
						// SAMPLE

						uint16_t OP0 = _Code[_Ip++];
						OP0 |= _Code[_Ip++] << 8;
						uint8_t OP1 = _Code[_Ip++];
						uint8_t OP2 = _Code[_Ip++];

						int CurX = fmod(_Regs[OP1].X, 1.0f) * (_Vars[OP0]->Tex.ResX - 1);
						int CurY = fmod(_Regs[OP1].Y, 1.0f) * (_Vars[OP0]->Tex.ResY - 1);

						CurX = std::max(CurX, 0);
						CurY = std::max(CurY, 0);

						_Regs[OP2].X = (*_Vars[OP0]->Tex.Data)[(CurX + CurY * _Vars[OP0]->Tex.ResX) * 3 + 0];
						_Regs[OP2].Y = (*_Vars[OP0]->Tex.Data)[(CurX + CurY * _Vars[OP0]->Tex.ResX) * 3 + 1];
						_Regs[OP2].Z = (*_Vars[OP0]->Tex.Data)[(CurX + CurY * _Vars[OP0]->Tex.ResX) * 3 + 2];
					}
					else if (First == TOK_ASSIGN)
					{
						// ASSIGN
						uint16_t OP0 = _Code[_Ip++];
						OP0 |= _Code[_Ip++] << 8;
						uint8_t OP1 = _Code[_Ip++];
						_OutVars[OP0] = _Regs[OP1];
					}
					else if (First == TOK_INITIALIZER)
					{
						// INITIALIZER LIST
						uint8_t OP0 = _Code[_Ip++];
						uint8_t OP1 = _Code[_Ip++];
						if (OP1 == 1)
						{
							_Regs[OP0].X = _Regs[_Code[_Ip++]].X;
						}
						else if (OP1 == 2)
						{
							_Regs[OP0].X = _Regs[_Code[_Ip++]].X;
							_Regs[OP0].Y = _Regs[_Code[_Ip++]].X;
						}
						else if (OP1 == 3)
						{
							_Regs[OP0].X = _Regs[_Code[_Ip++]].X;
							_Regs[OP0].Y = _Regs[_Code[_Ip++]].X;
							_Regs[OP0].Z = _Regs[_Code[_Ip++]].X;
						}
						else if (OP1 == 4)
						{
							_Regs[OP0].X = _Regs[_Code[_Ip++]].X;
							_Regs[OP0].Y = _Regs[_Code[_Ip++]].X;
							_Regs[OP0].Z = _Regs[_Code[_Ip++]].X;
							_Regs[OP0].W = _Regs[_Code[_Ip++]].X;
						}
					}
					else if (First == TOK_SIN)
					{
						// SIN

						uint8_t OP0 = _Code[_Ip++];
						_Regs[OP0].X = sinf(_Regs[OP0].X);
					}
					else if (First == TOK_COS)
					{
						// COS

						uint8_t OP0 = _Code[_Ip++];
						_Regs[OP0].X = cosf(_Regs[OP0].X);
					}
					else if (First == TOK_DOT)
					{
						// DOT

						uint8_t OP0 = _Code[_Ip++];
						uint8_t OP1 = _Code[_Ip++];
						uint8_t OP2 = _Code[_Ip++];

						if (OP2 == 0)
						{
							_Regs[OP0].X = _Regs[OP0].X * _Regs[OP1].X + _Regs[OP0].Y * _Regs[OP1].Y;
						}
						else if (OP2 == 1)
						{
							_Regs[OP0].X = _Regs[OP0].X * _Regs[OP1].X + _Regs[OP0].Y * _Regs[OP1].Y + _Regs[OP0].Z * _Regs[OP1].Z;
						}
						else if (OP2 == 2)
						{
							_Regs[OP0].X = _Regs[OP0].X * _Regs[OP1].X + _Regs[OP0].Y * _Regs[OP1].Y + _Regs[OP0].Z * _Regs[OP1].Z + _Regs[OP0].W * _Regs[OP1].W;
						}
					}
					else if (First == TOK_CROSS)
					{
						// CROSS

						uint8_t OP0 = _Code[_Ip++];
						uint8_t OP1 = _Code[_Ip++];

						Variable Out;
						Out.X = _Regs[OP0].Y * _Regs[OP1].Z - _Regs[OP0].Z * _Regs[OP1].Y;
						Out.Y = _Regs[OP0].Z * _Regs[OP1].X - _Regs[OP0].X * _Regs[OP1].Z;
						Out.Z = _Regs[OP0].X * _Regs[OP1].Y - _Regs[OP0].Y * _Regs[OP1].X;
					}
					else if (First == TOK_LENGTH)
					{
						// LENGTH

						uint8_t OP0 = _Code[_Ip++];
						uint8_t OP1 = _Code[_Ip++];

						if (OP1 == 0)
						{
							_Regs[OP0].X = std::abs(_Regs[OP0].X);
						}
						else if (OP1 == 1)
						{
							_Regs[OP0].X = sqrtf(_Regs[OP0].X * _Regs[OP0].X + _Regs[OP0].Y * _Regs[OP0].Y);
						}
						else if (OP1 == 2)
						{
							_Regs[OP0].X = sqrtf(_Regs[OP0].X * _Regs[OP0].X + _Regs[OP0].Y * _Regs[OP0].Y + _Regs[OP0].Z * _Regs[OP0].Z);
						}
						else if (OP1 == 3)
						{
							_Regs[OP0].X = sqrtf(_Regs[OP0].X * _Regs[OP0].X + _Regs[OP0].Y * _Regs[OP0].Y + _Regs[OP0].Z * _Regs[OP0].Z + _Regs[OP0].W * _Regs[OP0].W);
						}
					}
					else if (First == TOK_ABS)
					{
						// ABS

						uint8_t OP0 = _Code[_Ip++];
						_Regs[OP0].X = std::abs(_Regs[OP0].X);
						_Regs[OP0].Y = std::abs(_Regs[OP0].Y);
						_Regs[OP0].Z = std::abs(_Regs[OP0].Z);
						_Regs[OP0].W = std::abs(_Regs[OP0].W);
						_Regs[OP0].I = std::abs(_Regs[OP0].I);
					}
				}
				else
				{
					if (First == TOK_JMP)
					{
						// JUMP
						uint8_t OP1 = _Code[_Ip++];
						uint32_t OP2 = _Code[_Ip++];
						uint32_t OP3 = _Code[_Ip++];
						uint32_t OP4 = _Code[_Ip++];
						uint32_t OP5 = _Code[_Ip++];
						uint8_t OP6 = _Code[_Ip++];
						uint8_t OP7 = _Code[_Ip++];
						uint32_t JumpTo = OP2 | (OP3 << 8) | (OP4 << 16) | (OP5 << 24);
						if (OP1 == 0)
						{
							if (_Regs[OP6].X != _Regs[OP7].X)
							{
								_Ip = JumpTo;
							}
						}
						else if (OP1 == 1)
						{
							if (_Regs[OP6].X >= _Regs[OP7].X)
							{
								_Ip = JumpTo;
							}
						}
						else if (OP1 == 2)
						{
							if (_Regs[OP6].X <= _Regs[OP7].X)
							{
								_Ip = JumpTo;
							}
						}
					}
					else if (First == TOK_JMP_UNCONDITIONAL)
					{
						// JUMP UNCONDITIONAL
						uint32_t OP2 = _Code[_Ip++];
						uint32_t OP3 = _Code[_Ip++];
						uint32_t OP4 = _Code[_Ip++];
						uint32_t OP5 = _Code[_Ip++];
						uint32_t JumpTo = OP2 | (OP3 << 8) | (OP4 << 16) | (OP5 << 24);
						_Ip = JumpTo;
					}
					else if (First == TOK_CLAMP01)
					{
						// CLAMP01

						uint8_t OP0 = _Code[_Ip++];
						_Regs[OP0].X = std::min(std::max(_Regs[OP0].X, 0.0f), 1.0f);
						_Regs[OP0].Y = std::min(std::max(_Regs[OP0].Y, 0.0f), 1.0f);
						_Regs[OP0].Z = std::min(std::max(_Regs[OP0].Z, 0.0f), 1.0f);
						_Regs[OP0].W = std::min(std::max(_Regs[OP0].W, 0.0f), 1.0f);
					}
				}
			}
		}
	}
	size_t Executable::AssertVariable(Variable* Var)
	{
		size_t I = 0;
		for (Variable* _Var : _Vars)
		{
			if (_Var == Var) return I;
			I++;
		}
		_Vars.push_back(Var);
		_OutVars.push_back(*Var);
		return _Vars.size() - 1;
	}
	ExecutableResult Executable::CompileToken(Token* Tok, uint8_t &RegCount)
	{
		RegCount++;
		if (Tok->Type == TOK_VAR)
		{
			_Code.push_back(TOK_ASSIGN_TO_REG);
			_Code.push_back(RegCount & 0xFF);
			size_t VarPos = AssertVariable(Tok->Var);
			_Code.push_back(VarPos & 0xFF);
			_Code.push_back((VarPos & 0xFF00) >> 8);
			return { RegCount, *Tok->Var };
		}
		else if (Tok->Type == TOK_CONST)
		{
			_Code.push_back(TOK_ASSIGN_TO_REG_CONST);
			_Code.push_back(RegCount & 0xFF);
			_Consts.push_back(Tok->Const);
			size_t ConstPos = _Consts.size() - 1;
			_Code.push_back(ConstPos & 0xFF);
			_Code.push_back((ConstPos & 0xFF00) >> 8);
			return { RegCount, Tok->Const };
		}
		else if (Tok->Type == TOK_IF)
		{
			size_t At = _Code.size();
			ExecutableResult FirstOp = CompileToken(Tok->First->First, RegCount);
			ExecutableResult SecondOp = CompileToken(Tok->First->Second, RegCount);
			_Code.push_back(TOK_JMP);
			if (Tok->First->Type == TOK_CMPEQ) _Code.push_back(0);
			else if (Tok->First->Type == TOK_CMPLT) _Code.push_back(1);
			else if (Tok->First->Type == TOK_CMPGT) _Code.push_back(2);
			size_t At2 = _Code.size();
			_Code.push_back(0);
			_Code.push_back(0);
			_Code.push_back(0);
			_Code.push_back(0);
			_Code.push_back(0);
			_Code.push_back(0);
			for (Token* Tok : Tok->NextLines)
			{
				uint8_t RCount = 0;
				CompileToken(Tok, RCount);
			}
			
			size_t AtNow = _Code.size();
			_Code[At2] = AtNow & 0xFF;
			_Code[At2 + 1] = (AtNow >> 8) & 0xFF;
			_Code[At2 + 2] = (AtNow >> 16) & 0xFF;
			_Code[At2 + 3] = (AtNow >> 24) & 0xFF;
			_Code[At2 + 4] = FirstOp.Reg % 0xFF;
			_Code[At2 + 5] = SecondOp.Reg % 0xFF;
			return { RegCount, {} };
		}
		else if (Tok->Type == TOK_WHILE)
		{
			size_t At = _Code.size();
			ExecutableResult FirstOp = CompileToken(Tok->First->First, RegCount);
			ExecutableResult SecondOp = CompileToken(Tok->First->Second, RegCount);
			_Code.push_back(TOK_JMP);
			if (Tok->First->Type == TOK_CMPEQ) _Code.push_back(0);
			else if (Tok->First->Type == TOK_CMPLT) _Code.push_back(1);
			else if (Tok->First->Type == TOK_CMPGT) _Code.push_back(2);
			size_t At2 = _Code.size();
			_Code.push_back(0);
			_Code.push_back(0);
			_Code.push_back(0);
			_Code.push_back(0);
			_Code.push_back(0);
			_Code.push_back(0);
			for (Token* Tok : Tok->NextLines)
			{
				uint8_t RCount = 0;
				CompileToken(Tok, RCount);
			}
			size_t AtNow = _Code.size() + 5;
			_Code[At2] = AtNow & 0xFF;
			_Code[At2 + 1] = (AtNow >> 8) & 0xFF;
			_Code[At2 + 2] = (AtNow >> 16) & 0xFF;
			_Code[At2 + 3] = (AtNow >> 24) & 0xFF;
			_Code[At2 + 4] = FirstOp.Reg % 0xFF;
			_Code[At2 + 5] = SecondOp.Reg % 0xFF;

			_Code.push_back(TOK_JMP_UNCONDITIONAL);
			_Code.push_back(At & 0xFF);
			_Code.push_back((At >> 8) & 0xFF);
			_Code.push_back((At >> 16) & 0xFF);
			_Code.push_back((At >> 24) & 0xFF);
			return { RegCount, {} };
		}
		else if (Tok->Type == TOK_INITIALIZER)
		{
			if (Tok->InitArgs.size() == 0)
			{
				ERR("Empty initializer is illegal\n");
				return {};
			}
			if (Tok->InitArgs.size() > 4)
			{
				ERR("Initializer has too many arguments\n");
				return {};
			}

			int OldRCount = RegCount;

			std::vector<ExecutableResult> Results;
			for (int I = 0; I < Tok->InitArgs.size(); I++)
			{
				Results.push_back(CompileToken(Tok->InitArgs[I], RegCount));
			}

			_Code.push_back(TOK_INITIALIZER);

			_Code.push_back(OldRCount & 0xFF);

			_Code.push_back(Tok->InitArgs.size() & 0xFF);

			for (int I = 0; I < Tok->InitArgs.size(); I++)
			{
				_Code.push_back(Results[I].Reg % 0xFF); 
			}

			Variable DummyThick;
			if (Tok->InitArgs.size() == 1)
			{
				DummyThick.Type = TYPE_FLOAT;
			}
			else if (Tok->InitArgs.size() == 2)
			{
				DummyThick.Type = TYPE_VEC2;
			}
			else if (Tok->InitArgs.size() == 3)
			{
				DummyThick.Type = TYPE_VEC3;
			}
			else if (Tok->InitArgs.size() == 4)
			{
				DummyThick.Type = TYPE_VEC4;
			}

			return { OldRCount, DummyThick };
		}
		else if (Tok->Type == TOK_ADD || Tok->Type == TOK_SUB || Tok->Type == TOK_MUL || Tok->Type == TOK_DIV)
		{
			ExecutableResult First = CompileToken(Tok->First, RegCount);
			ExecutableResult Second = CompileToken(Tok->Second, RegCount);
			if (First.Var.Type != Second.Var.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}
			_Code.push_back(Tok->Type);
			_Code.push_back(First.Reg & 0xFF);
			_Code.push_back(Second.Reg & 0xFF);
			return First;
		}
		else if (Tok->Type == TOK_ASSIGN)
		{
			if (Tok->First->Type != TOK_VAR)
			{
				ERR("Attempt to assign to rvalue\n");
				return {};
			}
			ExecutableResult Second = CompileToken(Tok->Second, RegCount);
			if (Tok->First->Var->Type != Second.Var.Type)
			{
				ERR("Type mismatch in assign\n");
				return {};
			}
			int VarPos = AssertVariable(Tok->First->Var);
			_Code.push_back(TOK_ASSIGN);
			_Code.push_back(VarPos & 0xFF);
			_Code.push_back((VarPos & 0xFF00) >> 8);
			_Code.push_back(Second.Reg & 0xFF);
			return Second;
		}
		else if (Tok->Type == TOK_SAMPLE)
		{
			if (Tok->First->Type != TOK_VAR)
			{
				ERR("Attempt to sample non-variable\n");
				return {};
			}

			if (Tok->First->Var->Type != TYPE_TEXTURE)
			{
				ERR("Attempt to sample non-texture\n");
				return {};
			}

			int OldRCount = RegCount;
			ExecutableResult Second = CompileToken(Tok->Second, RegCount);
			
			if (Second.Var.Type != TYPE_VEC2)
			{
				ERR("Attempt to sample with non-vec2\n");
				return {};
			}
			int VarPos = AssertVariable(Tok->First->Var);
			_Code.push_back(TOK_SAMPLE);
			_Code.push_back(VarPos & 0xFF);
			_Code.push_back((VarPos & 0xFF00) >> 8);
			_Code.push_back(Second.Reg & 0xFF);
			_Code.push_back(OldRCount & 0xFF);
			Variable Out;
			Out.Type = TYPE_VEC3;
			return { OldRCount, Out };
		}
		else if (Tok->Type == TOK_DOT)
		{
			int OldRCount = RegCount;
			ExecutableResult First = CompileToken(Tok->First, RegCount);
			ExecutableResult Second = CompileToken(Tok->Second, RegCount);

			if (Second.Var.Type != First.Var.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}

			if (First.Var.Type == TYPE_FLOAT || First.Var.Type == TYPE_INT)
			{
				ERR("Attempt to perform dot product with non-vectors\n");
				return {};
			}

			_Code.push_back(TOK_DOT);
			_Code.push_back(First.Reg & 0xFF);
			_Code.push_back(Second.Reg & 0xFF);
			if (First.Var.Type == TYPE_VEC2) _Code.push_back(0);
			else if (First.Var.Type == TYPE_VEC3) _Code.push_back(1);
			else if (First.Var.Type == TYPE_VEC4) _Code.push_back(2);
			Variable Out;
			Out.Type = TYPE_FLOAT;
			return { First.Reg, Out };
		}
		else if (Tok->Type == TOK_CROSS)
		{
			int OldRCount = RegCount;
			ExecutableResult First = CompileToken(Tok->First, RegCount);
			ExecutableResult Second = CompileToken(Tok->Second, RegCount);

			if (Second.Var.Type != First.Var.Type)
			{
				ERR("Type mismatch\n");
				return {};
			}

			if (First.Var.Type != TYPE_VEC3)
			{
				ERR("Attempt to perform cross product with non-vec3s\n");
				return {};
			}

			_Code.push_back(TOK_CROSS);
			_Code.push_back(First.Reg & 0xFF);
			_Code.push_back(Second.Reg & 0xFF);
			return { First.Reg, First.Var };
		}
		else if (Tok->Type == TOK_LENGTH)
		{
			int OldRCount = RegCount;
			ExecutableResult First = CompileToken(Tok->First, RegCount);

			_Code.push_back(TOK_LENGTH);
			_Code.push_back(First.Reg & 0xFF);
			if (First.Var.Type == TYPE_FLOAT) _Code.push_back(0);
			else if (First.Var.Type == TYPE_VEC2) _Code.push_back(1);
			else if (First.Var.Type == TYPE_VEC3) _Code.push_back(2);
			else if (First.Var.Type == TYPE_VEC4) _Code.push_back(3);
			Variable Out;
			Out.Type = TYPE_FLOAT;
			return { First.Reg, Out };
		}
		else if (Tok->Type == TOK_ABS)
		{
			int OldRCount = RegCount;
			ExecutableResult First = CompileToken(Tok->First, RegCount);

			_Code.push_back(TOK_ABS);
			_Code.push_back(First.Reg & 0xFF);
			return { First.Reg, First.Var };
		}
		else if (Tok->Type == TOK_COS)
		{
			int OldRCount = RegCount;
			ExecutableResult First = CompileToken(Tok->First, RegCount);

			if (First.Var.Type != TYPE_FLOAT)
			{
				ERR("cos expected a float argument");
				return {};
			}

			_Code.push_back(TOK_COS);
			_Code.push_back(First.Reg & 0xFF);
			return { First.Reg, First.Var };
		}
		else if (Tok->Type == TOK_CLAMP01)
		{
			int OldRCount = RegCount;
			ExecutableResult First = CompileToken(Tok->First, RegCount);

			_Code.push_back(TOK_CLAMP01);
			_Code.push_back(First.Reg & 0xFF);
			return { First.Reg, First.Var };
		}
		else if (Tok->Type == TOK_SIN)
		{
			int OldRCount = RegCount;
			ExecutableResult First = CompileToken(Tok->First, RegCount);

			if (First.Var.Type != TYPE_FLOAT)
			{
				ERR("sin expected a float argument");
				return {};
			}

			_Code.push_back(TOK_SIN);
			_Code.push_back(First.Reg & 0xFF);
			return { First.Reg, First.Var };
		}
		else if (Tok->Type == TOK_SWIZZLE)
		{
			ExecutableResult First = CompileToken(Tok->First, RegCount);
			if (Tok->Swizzle.size() == 0 && Tok->Swizzle.size() > 4)
			{
				ERR("Invalid swizzle\n");
				return {};
			}
			_Code.push_back(TOK_SWIZZLE);
			_Code.push_back(First.Reg & 0xFF);
			for (int I = 0; I < 4; I++)
			{
				if (I < Tok->Swizzle.size())
				{
					_Code.push_back(static_cast<uint8_t>(Tok->Swizzle[I]));
				}
				else
				{
					_Code.push_back(0);
				}
			}
			ExecutableResult Out = First;
			if (Tok->Swizzle.size() == 1)
			{
				Out.Var.Type = TYPE_FLOAT;
			}
			else if (Tok->Swizzle.size() == 2)
			{
				Out.Var.Type = TYPE_VEC2;
			}
			else if (Tok->Swizzle.size() == 3)
			{
				Out.Var.Type = TYPE_VEC3;
			}
			else if (Tok->Swizzle.size() == 4)
			{
				Out.Var.Type = TYPE_VEC4;
			}
			return Out;
		}
	}
	void Executable::Compile(std::vector<Function*> Funcs)
	{
		_Code.erase(_Code.begin(), _Code.end());
		_Vars.erase(_Vars.begin(), _Vars.end());
		for (Function* Func : Funcs)
		{
			for (Variable* Var : Func->BaseScope->Variables)
			{
				AssertVariable(Var);
			}
			for (Token* Line : Func->Lines)
			{
				uint8_t RegCount = 0;
				CompileToken(Line, RegCount);
			}
		}
	}
	Variable* Executable::GetVariable(std::string Name)
	{
		for (Variable* _Var : _Vars)
		{
			if (_Var->Name == Name)
			{
				return _Var;
			}
		}
		return 0;
	}
	void SingleManager::Use(std::string Filename)
	{
		std::ifstream file(Filename);
		std::stringstream s;
		s << file.rdbuf();
		Tokenizer tokenizer;
		_Exec.Compile(tokenizer.Tokenize(s.str()));
	}
	Variable* SingleManager::GetVar(std::string FuncName, std::string VarName)
	{
		return _Exec.GetVariable(VarName);
	}
	void SingleManager::Execute()
	{
		int I = 0;
		for (Variable* _Var : _Exec._Vars)
		{
			_Exec._OutVars[I] = *_Var;
			I++;
		}
		_Exec.Execute();
		I = 0;
		for (Variable& _OutVar : _Exec._OutVars)
		{
			*_Exec._Vars[I] = _OutVar;
			I++;
		}
	}
}