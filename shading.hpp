#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

namespace ShadingLang
{

	enum VarType
	{
		TYPE_VEC2,
		TYPE_VEC3,
		TYPE_VEC4,
		TYPE_FLOAT,
		TYPE_INT,
		TYPE_TEXTURE,
		TYPE_UNKNOWN
	};

	struct TextureData
	{
		std::vector<float>* Data;
		size_t ResX;
		size_t ResY;
	};

	struct Variable
	{
		VarType Type;
		std::string Name;
		float X;
		float Y;
		float Z;
		float W;
		int I;
		bool Ref;
		bool Conditional;
		TextureData Tex;
	};

	enum TokenType
	{
		TOK_ASSIGN_TO_REG,
		TOK_ASSIGN_TO_REG_CONST,
		TOK_ADD,
		TOK_SUB,
		TOK_MUL,
		TOK_FIRSTPOINT_,
		TOK_SWIZZLE,
		TOK_DIV,
		TOK_SAMPLE,
		TOK_ASSIGN,
		TOK_INITIALIZER,
		TOK_SIN,
		TOK_COS,
		TOK_DOT,
		TOK_CROSS,
		TOK_LENGTH,
		TOK_ABS,
		TOK_SECONDPOINT_,
		TOK_JMP_UNCONDITIONAL,
		TOK_JMP,
		TOK_CLAMP01,
		TOK_CONST,
		TOK_VAR,
		TOK_CMPLT,
		TOK_CMPGT,
		TOK_CMPEQ,
		TOK_IF,
		TOK_WHILE,
	};

	struct Scope
	{
		std::vector<Variable*> Variables;
		Scope* Parent = 0;
	};

	struct Token
	{
		TokenType Type;
		Token* First;
		Token* Second;
		Token* Third;
		Variable* Var;
		Scope* NextScope;
		Variable Const;
		std::vector<Token*> NextLines;
		std::vector<Token*> InitArgs;
		std::vector<char> Swizzle;
	};



	struct Function
	{
		VarType ReturnType;
		std::vector<Token*> Lines;
		std::string Name;
		Scope* BaseScope;
		size_t ArgCount;
	};

	enum TemplateType
	{
		TEMP_ASSERT_NAME,
		TEMP_ASSERT_NAME_TYPE,
		TEMP_PARAMS,
		TEMP_ARG_LIST,
		TEMP_ARG,
		TEMP_ARG_DELIMITER,
		TEMP_ARG_LIST_END,
		TEMP_CODE_SEG,
		TEMP_EXPR
	};

	class Tokenizer
	{
	private:
		size_t _At = 0;
		std::string _Code;
		std::vector<Function*> _Functions;
	public:
		std::string GetStringUntil(char C);
		std::string GetStringUntilEither(char C0, char C1);
		size_t TellNext(char C);
		size_t TellNextArgEnd();
		size_t TellNextInitEnd();
		size_t TellNextRParamEnd();
		size_t TellScopeEnd();
		size_t TellNextSemi();
		size_t TellNextArgEndCustom(char Delim);
		bool IsOpChar(char C);
		bool IsOpCharNum(char C);
		size_t TellExprPartEnd();
		size_t TellExprPartEndNum();
		size_t TellNextEnd(char C);
		std::string GetStringExprPartEnd();
		VarType Str2Type(std::string X);
		bool TokenizeTempAssertName(std::string Name);
		void TokenizeTempNameType(std::string& Name, VarType& Type, char Delim);
		Variable* TokenizeTempParam();
		void TokenizeTempParams(Scope* OutScope);
		Token* TokenizeTempInit(Scope* OutScope);
		bool IsDigit(char C);
		Variable* FindVariableIn(Scope* S, std::string Name);
		Token* TokenizeIf(Scope* CurScope);
		Token* TokenizeWhile(Scope* CurScope);
		Token* TokenizeSample(Scope* CurScope);
		Token* TokenizeDot(Scope* CurScope);
		Token* TokenizeCross(Scope* CurScope);
		Token* TokenizeLength(Scope* CurScope);
		Token* TokenizeAbs(Scope* CurScope);
		Token* TokenizeSin(Scope* CurScope);
		Token* TokenizeCos(Scope* CurScope);
		Token* TokenizeClamp01(Scope* CurScope);
		Token* TokenizeExprPart(Scope* CurScope);
		TokenType TokenizeOperator();
		Token* TokenizeExpr(Scope* CurScope, size_t End);
		void TokenizeTempCode(std::vector<Token*>& OutLines, Scope* OutScope);
		Token* TokenizeTempArg(Scope* CurScope);
		Token* TokenizeTempArgDelim(Scope* CurScope, char Delim);
		Function* TokenizeFunction();
		std::vector<Function*> Tokenize(std::string Code);
	};

	struct ExecutableResult
	{
		int Reg;
		Variable Var;
	};

	class Executable
	{
	public:
		std::vector<uint8_t> _Code;
		std::vector<Variable*> _Vars;
		std::vector<Variable> _OutVars;
		std::vector<Variable> _Consts;
		Variable _Regs[256];
		size_t _Ip = 0;
		std::mutex _Mux;

		void Execute();
		size_t AssertVariable(Variable* Var);
		ExecutableResult CompileToken(Token* Tok, uint8_t &RegCount);
		void Compile(std::vector<Function*> Funcs);
		Variable* GetVariable(std::string Name);
	};
	class Manager
	{
	private:
		std::vector<Function*> _Instance;
		std::vector<Executable*> _Execs;
		std::vector<std::thread*> _Threads;
		std::mutex _CountMutex;
		std::atomic<int> _Count = 0;
	public:
		void Use(std::string Filename);
		Variable* GetVar(std::string FuncName, std::string VarName);
		void Execute(int ResX, int ResY, uint32_t* OutBuff, float* DepthBuff);
		int GetCount();
		void Wait(int ResX, int ResY);
	};
	class SingleManager
	{
	private:
		Executable _Exec;
	public:
		void Use(std::string Filename);
		Variable* GetVar(std::string FuncName, std::string VarName);
		void Execute();
	};
}