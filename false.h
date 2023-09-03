#ifndef FALSE_H
#define FALSE_H
#include <vector>
#include <variant>
#include <optional>
#include <functional>
#include <unordered_map>
#include <unicode/unistr.h>
#include <unicode/schriter.h>
#include <boost/locale/encoding_utf.hpp>
#include <memory>
#include <iostream>
#include <random>
#include <mutex>
class State;
/*
 * Class representing a function
 * Essentially only a program counter
 */
class FalseLambda{
	public:
	size_t charnum, linenum;
	std::optional<std::streampos> tell;
	std::u32string curline;
	public:
	FalseLambda(const State * state);
	FalseLambda(const FalseLambda&);
	virtual unsigned long long hash();
	/*This sets the program counter to the lambda*/
	virtual void setAll(State * state) const; 
};
/*
 * Push this on the call stack for a while loop.  
 */
class WhileLambda : public FalseLambda{
	FalseLambda cond;
	FalseLambda end;
	public:
	enum {
		WHILE_COND,
		WHILE_BODY
	} current_while_state;
	WhileLambda(
		FalseLambda&& cond,
		FalseLambda&& body,
		FalseLambda&& end
	);
	inline void setAllBody(State * state) const {
		setAll(state);
	}
	inline void setAllCond(State * state) const{
		cond.setAll(state);
	}
	inline void setAllEnd(State * state) const {
		end.setAll(state);
	}
	unsigned long long hash() override;
};
/*Pointer to something dlsymed*/
struct ForeignPointer {
	void * ptr;
};
/*Pointer to something dlopened*/
struct SharedObject {
	void * ptr;
};
/*One of the letter variables*/
struct FalseVarRef{
	char32_t c;
};

/*
 * The main dynamic type variable of FALSE.
 * The original language was weakly typed to keep the compiler
 * size small. But here I'm giving the user proper type checking 
 * and error handling.
 * Might cause some original programs to fail, I may add a flag to 
 * disable type checking but that would require me to rewrite a lot of operations.
 */
typedef std::variant<
	int,
	FalseLambda,
	FalseVarRef,
	ForeignPointer,
	SharedObject
> member;
struct LockableMember{
	member mem;
	std::mutex mut;
};
/*
 * strips type off a member
 */
inline intptr_t toint(member m){
	if (std::holds_alternative<int>(m)){
		return std::get<int>(m);
	} else if (std::holds_alternative<FalseLambda>(m)){
		return std::get<FalseLambda>(m).hash();
	} else if (std::holds_alternative<FalseVarRef>(m)){
		return std::get<FalseVarRef>(m).c;
	} else if (std::holds_alternative<ForeignPointer>(m)) {
		return (intptr_t)std::get<ForeignPointer>(m).ptr;
	} else if (std::holds_alternative<SharedObject>(m)) {
		return (intptr_t)std::get<SharedObject>(m).ptr;
	} else {
		return -1;
	}
}

/*Every operation returns a Status*/
enum Status {
	STATUS_OK,
	STATUS_EOF,
	STATUS_Unimplemented,
	STATUS_StackBust,
	STATUS_TypeError,
	STATUS_PickNegative,
	STATUS_UnbalancedBrackets,
	STATUS_Unexpected_EOF,
	STATUS_NoSuchVariable,
	STATUS_Invalid_Input
};
constexpr char * impossible_error_message=(char * const)"I have no idea how you got this error message it should be OK.";
static const char * error_messages[]={
	[STATUS_OK]=impossible_error_message,
	[STATUS_EOF]=impossible_error_message,
	[STATUS_Unimplemented]="Unimplemented Operation.",
	[STATUS_StackBust]="Stack too low.",
	[STATUS_TypeError]="TOS is wrong type for operation.",
	[STATUS_PickNegative]="PICK given negative number.",
	[STATUS_UnbalancedBrackets]="Final Bracket with no match.",
	[STATUS_Unexpected_EOF]="Unexpected EOF Reached.",
	[STATUS_NoSuchVariable]="Variable was not set.",
	[STATUS_Invalid_Input]="Input is invalid."

};

struct Flags{
	/*Arguments*/
	bool m16 : 1=false;
	bool log : 1=false;
	bool disable_error_checking : 1=false;
	bool ext : 1 = true;
	bool ise : 1 = false;
	/*Dynamic*/
	bool stringmode : 1=false;
};
class State {
	std::vector<member> stack;
	std::vector<std::unique_ptr<FalseLambda> > call_stack;
	std::unordered_map<char32_t,member> variables;
	std::u32string::const_iterator curchar_iter;
	size_t charnum,linenum;
	std::u32string curline; 
	std::istream * prog_stream;
	static const std::unordered_map<char32_t, std::function<Status(State*)> > operations;
	std::vector<std::istream *> in_streams;
	std::vector<std::ostream *> out_streams;
	std::uniform_int_distribution<int> random_dist;
	std::default_random_engine random_engine;

	bool EOL_callback();
	inline std::optional<char32_t> nextChar(){
		if (curchar_iter>=curline.cend()) {
			if (EOL_callback()) {
				charnum++;
				return *(curchar_iter++);
			}
			else {
				return std::nullopt;
			}
		} else {
			charnum++;
			return *(curchar_iter++);
		}
	}

	Status UNIMPLEMENTED();

	Status CallForeign(ForeignPointer ptr);
	Status PUSHCHAR();
	Status DUP();
	Status DROP();
	Status SWAP();
	Status ROT();
	Status PICK();
	Status ADD();
	Status SUB();
	Status MUL();
	Status DIV();
	Status NEGATE();
	Status AND();
	Status OR();
	Status NOT();
	Status GREATER();
	Status EQUALS();
	Status LAMBDA();
	Status RETURN();
	Status CALL();
	Status CALL_COND();
	Status WHILE();
	Status STORE();
	Status FETCH();
	Status GETCHAR();
	Status PUTCHAR();
	Status PUTINT();
	Status FLUSH();
	Status COMMENT();
	Status PUTSTRING();
	Status MACHINE_CODE();
	Status NOP();

	Status EXT_OPENF_R();
	Status EXT_OPENF_W();
	Status EXT_OPENF_A();
	Status EXT_CLOSEF_R();
	Status EXT_CLOSEF_W();
	
	Status EXT_DLOPEN();
	Status EXT_DLSYM();
	Status EXT_DLCLOSE();
	Status EXT_MALLOC();
	Status EXT_FREE();
	Status EXT_ZGNIRTS_TO_C();
	Status EXT_INDEX();
	Status EXT_INSERT();

	Status EXT_RANDOM();

	public:
	friend class FalseLambda;
	friend class WhileLambda;

	State(
		std::istream * prog_stream,
		Flags flags=Flags()
	);
	inline size_t get_charnum(){
		return charnum;
	}
	inline size_t get_linenum(){
		return linenum;
	}
	Flags flags;
	inline int bitmask(int op){
		return flags.m16?op&0xffff:op;
	}
	Status step();
	unsigned long long parse_long_unicode();
	
	
	
	
};
bool uni_isdigit(char32_t c);
inline void putchar32(char32_t n){
	std::cout<<boost::locale::conv::utf_to_utf<char, char32_t>(std::u32string(1,n));
}
unsigned long long parse_long_unicode(std::u32string::const_iterator iter,std::u32string::const_iterator end, size_t& count);
extern std::ofstream logfile;
#endif