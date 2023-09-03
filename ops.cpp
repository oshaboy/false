#include "false.h"
#include <stdexcept>
#include <variant>
#include <iostream>
#include <fstream>
#include <vector>
extern "C"{
	#include <ffi.h>
	#include <dlfcn.h>
}

#define STACK_CHECK(_s_) if(this->stack.size() < (_s_)) return STATUS_StackBust
#define GET_MEMBER(_T_, _member_) ({ \
	if (!std::holds_alternative<_T_>(_member_)) return STATUS_TypeError; \
	std::get<_T_>(_member_); \
})
#define POP_AND_TYPE(_T_) ({ \
	const member _mem=this->stack.back(); \
	this->stack.pop_back(); \
	GET_MEMBER(_T_, _mem); \
})
#define CONVERT_INT(_a_) ({const int _abak=(_a_); this->flags.m16?_abak&0xffff:_abak;})
const std::unordered_map<char32_t, std::function<Status(State*)> > State::operations({
	{U'\'', &State::PUSHCHAR},
	{U'$', &State::DUP},
	{U'%', &State::DROP},
	{U'\\', &State::SWAP},
	{U'@', &State::ROT},
	{U'ø', &State::PICK},
	{U'+', &State::ADD},
	{U'-', &State::SUB},
	{U'*', &State::MUL},
	{U'/', &State::DIV},
	{U'_', &State::NEGATE},
	{U'&', &State::AND},
	{U'|', &State::OR},
	{U'~', &State::NOT},
	{U'>', &State::GREATER},
	{U'=', &State::EQUALS},
	{U'[', &State::LAMBDA},
	{U']', &State::RETURN},
	{U'!', &State::CALL},
	{U'?', &State::CALL_COND},
	{U'#', &State::WHILE},
	{U':', &State::STORE},
	{U';', &State::FETCH},
	{U'^', &State::GETCHAR},
	{U',', &State::PUTCHAR},
	{U'"', &State::PUTSTRING},
	{U'.', &State::PUTINT},
	{U'ß', &State::FLUSH},
	{U'{', &State::COMMENT},
	{U'`', &State::MACHINE_CODE},

	/*ЬЭ*/
	/*УЂЏ*/
	{U'Л', &State::EXT_OPENF_R},
	{U'л', &State::EXT_OPENF_R},
	{U'Ш', &State::EXT_OPENF_W},
	{U'ш', &State::EXT_OPENF_W},
	{U'И', &State::EXT_OPENF_A},
	{U'и', &State::EXT_OPENF_A},
	{U'Я', &State::EXT_CLOSEF_R},
	{U'я', &State::EXT_CLOSEF_R},
	{U'Г', &State::EXT_CLOSEF_W},
	{U'г', &State::EXT_CLOSEF_W},

	{U'Ж', &State::EXT_DLOPEN},
	{U'ж', &State::EXT_DLOPEN},
	{U'Б', &State::EXT_DLSYM},
	{U'б', &State::EXT_DLSYM},
	{U'Ц', &State::EXT_DLCLOSE},
	{U'ц', &State::EXT_DLCLOSE},
	{U'Ф', &State::EXT_MALLOC},
	{U'ф', &State::EXT_MALLOC},
	{U'Ю', &State::EXT_FREE},
	{U'ю', &State::EXT_FREE},
	{U'П', &State::EXT_ZGNIRTS_TO_C},
	{U'п', &State::EXT_ZGNIRTS_TO_C},
	{U'Ы', &State::EXT_INDEX},
	{U'ы', &State::EXT_INDEX},
	{U'Ч', &State::EXT_INSERT},
	{U'ч', &State::EXT_INSERT},
	{U'Д', &State::EXT_RANDOM},
	{U'д', &State::EXT_RANDOM}
	

});
Status State::UNIMPLEMENTED(){
	return STATUS_Unimplemented;
}
Status State::PUSHCHAR(){
	std::optional<char32_t> c = nextChar();
	if (c.has_value())
		stack.push_back((int)c.value());
	else 
		return STATUS_Unexpected_EOF;
	
	return STATUS_OK;
}
Status State::DUP(){
	STACK_CHECK(1);
	stack.push_back(stack.back());
	return STATUS_OK;
}
Status State::DROP(){
	STACK_CHECK(1);
	stack.pop_back();
	return STATUS_OK;
}
Status State::SWAP(){
	STACK_CHECK(2);
	const member first=stack.back(); stack.pop_back();
	const member second=stack.back(); stack.pop_back();
	stack.push_back(first);
	stack.push_back(second);
	return STATUS_OK;
}

Status State::ROT(){
	STACK_CHECK(3);
	const member first=stack.back(); stack.pop_back();
	const member second=stack.back(); stack.pop_back();
	const member third=stack.back(); stack.pop_back();
	stack.push_back(third);
	stack.push_back(first);
	stack.push_back(second);

	return STATUS_OK;
}

Status State::PICK(){
	STACK_CHECK(1);
	const int count=POP_AND_TYPE(int);
	if (count < 0) return STATUS_PickNegative;
	STACK_CHECK(count);
	stack.push_back(stack.rend()[count]);
	return STATUS_OK;
}

Status State::ADD(){
	STACK_CHECK(2);
	const int op2=POP_AND_TYPE(int);
	const int op1=POP_AND_TYPE(int);
	stack.push_back(CONVERT_INT(op1+op2));
	return STATUS_OK;
}
Status State::SUB(){
	STACK_CHECK(2);
	const int op2=POP_AND_TYPE(int);
	const int op1=POP_AND_TYPE(int);
	stack.push_back(CONVERT_INT(op1-op2));
	return STATUS_OK;
}
Status State::MUL(){
	STACK_CHECK(2);
	const int op2=POP_AND_TYPE(int);
	const int op1=POP_AND_TYPE(int);
	stack.push_back(CONVERT_INT(op1*op2));
	return STATUS_OK;
}
Status State::DIV(){
	STACK_CHECK(2);
	const int op2=POP_AND_TYPE(int);
	const int op1=POP_AND_TYPE(int);
	stack.push_back(CONVERT_INT(op1/op2));
	return STATUS_OK;
}
Status State::NEGATE(){
	STACK_CHECK(1);
	const int op=POP_AND_TYPE(int);
	stack.push_back(CONVERT_INT(-op));
	return STATUS_OK;
}
Status State::AND(){
	STACK_CHECK(2);
	const int op2=POP_AND_TYPE(int);
	const int op1=POP_AND_TYPE(int);
	stack.push_back(op1&op2);
	return STATUS_OK;
}
Status State::OR(){
	STACK_CHECK(2);
	const int op2=POP_AND_TYPE(int);
	const int op1=POP_AND_TYPE(int);
	stack.push_back(op1|op2);
	return STATUS_OK;
}

Status State::NOT(){
	STACK_CHECK(1);
	const int op = POP_AND_TYPE(int);
	stack.push_back(CONVERT_INT(~op));
	return STATUS_OK;
}
Status State::GREATER(){
	STACK_CHECK(2);
	const int op2=POP_AND_TYPE(int);
	const int op1=POP_AND_TYPE(int);
	stack.push_back((op1>op2)?CONVERT_INT(~0):0);
	return STATUS_OK;
}
Status State::EQUALS(){
	STACK_CHECK(2);
	const int op2=POP_AND_TYPE(int);
	const int op1=POP_AND_TYPE(int);
	stack.push_back((op1==op2)?CONVERT_INT(~0):0);
	return STATUS_OK;
}
Status State::LAMBDA(){
	stack.push_back(
		FalseLambda(this)
	);
	int bracket_count = 1;
	do {
		std::optional<char32_t> c=nextChar();
		if (!c.has_value()) return STATUS_UnbalancedBrackets;
		else if (c == ']') bracket_count--;
		else if (c=='[') bracket_count++;
	} while(bracket_count>0);

	return STATUS_OK;
}
Status State::COMMENT(){
	try {
		for(char32_t c=nextChar().value(); c!='}'; c=nextChar().value());
	} catch (const std::bad_optional_access& ex){
		return STATUS_UnbalancedBrackets;
	}
	return STATUS_OK;
}
Status State::CALL(){
	STACK_CHECK(1);
	if (std::holds_alternative<FalseLambda>(stack.back())){
		const FalseLambda lambda=POP_AND_TYPE(FalseLambda);
		call_stack.push_back(
			std::make_unique<FalseLambda>(this)
		);
		lambda.setAll(this);
		return STATUS_OK;
	} else if(std::holds_alternative<ForeignPointer>(stack.back())){
		return CallForeign(POP_AND_TYPE(ForeignPointer));
	} else {
		return STATUS_TypeError;
	}

}
Status State::CALL_COND(){
	STACK_CHECK(2);
	if (std::holds_alternative<FalseLambda>(stack.back())){
		const FalseLambda lambda=POP_AND_TYPE(FalseLambda);
		const int cond=POP_AND_TYPE(int);
		if (cond) {
			call_stack.push_back(
				std::make_unique<FalseLambda>(this)
			);

			lambda.setAll(this);
		}
		return STATUS_OK;
	} else if (std::holds_alternative<ForeignPointer>(stack.back())) {
		const ForeignPointer foreign=POP_AND_TYPE(ForeignPointer);
		const int cond=POP_AND_TYPE(int);
		if (cond) return CallForeign(foreign);
		return STATUS_OK;
	} else {
		return STATUS_TypeError;
	}


}
Status State::WHILE(){
	STACK_CHECK(2);
	FalseLambda lambda=POP_AND_TYPE(FalseLambda);
	FalseLambda cond=POP_AND_TYPE(FalseLambda);
	//std::u32string curline_backup=this->curline;
	//std::u32string::const_iterator iter_backup=this->curchar_iter;
	//const size_t charnum_backup=this->charnum;
	//const size_t linenum_backup=this->linenum;
	FalseLambda end(this);
	cond.setAll(this);
	call_stack.push_back(std::make_unique<WhileLambda>(
		std::move(cond),
		std::move(lambda),
		std::move(end)
	));
	return STATUS_OK;

}

Status State::RETURN(){
	if (call_stack.empty()) return STATUS_UnbalancedBrackets;
	FalseLambda * lambda=call_stack.back().get();
	WhileLambda * as_while_lambda = dynamic_cast<WhileLambda*>(lambda);
	if (as_while_lambda != NULL){
		switch(as_while_lambda->current_while_state){
			case WhileLambda::WHILE_COND: {
				STACK_CHECK(1);
				if (POP_AND_TYPE(int)){
					as_while_lambda->setAllBody(this);
					as_while_lambda->current_while_state=WhileLambda::WHILE_BODY;
				} else {

					as_while_lambda->setAllEnd(this);
					call_stack.pop_back();
					
				}
					
			} break;
			case WhileLambda::WHILE_BODY: {
				as_while_lambda->current_while_state=WhileLambda::WHILE_COND;
				as_while_lambda->setAllCond(this);
			} break;
		}
	} else {
		lambda->setAll(this);
		call_stack.pop_back();
	}
	
	
	return STATUS_OK;
}

Status State::PUTCHAR(){
	STACK_CHECK(1);
	putchar32(POP_AND_TYPE(int));
	return STATUS_OK;
}

Status State::GETCHAR(){
	unsigned char c;
	*in_streams.back() >> c;
	if (c<0x80){ /*7-bit Ascii*/
		stack.push_back(c);
	} else if (c<0xc0){ /*Invalid*/
		return STATUS_Invalid_Input;
	} else { 
		const int charcnt=__builtin_clrsb(c<<24);
		const unsigned char eight_ones=0xff;
		char32_t result=c&(~(eight_ones<<(8-charcnt)));
		for (int i=0; i<charcnt-1; i++){
			result<<=6;
			*in_streams.back() >> c;
			if ((c&0xc0)!=0x80) return STATUS_Invalid_Input;
			result|=c&0x2f;
		}
		stack.push_back((int)result);


	}
	return STATUS_OK;

}

Status State::PUTINT(){
	STACK_CHECK(1);
	*out_streams.back()<<POP_AND_TYPE(int);
	return STATUS_OK;
}
Status State::FLUSH(){
	out_streams.back()->flush();
	return STATUS_OK;
}
Status State::PUTSTRING(){
	flags.stringmode=true;
	return STATUS_OK;
}
Status State::STORE(){
	STACK_CHECK(2);
	const char32_t var = POP_AND_TYPE(FalseVarRef).c;
	const member value = stack.back(); stack.pop_back();
	variables[var]=value;
	return STATUS_OK;
}
Status State::FETCH(){
	STACK_CHECK(1);
	const char32_t var = POP_AND_TYPE(FalseVarRef).c;
	if (variables.find(var)==variables.end()) return STATUS_NoSuchVariable;
	stack.push_back(variables.at(var));
	return STATUS_OK;

}
Status State::NOP(){
	return STATUS_OK;
}

/*Honestly I just wrote the FALSE interpreter to see if this is possible*/
#include <sys/mman.h>
#include <algorithm>
#include <cstring>
struct MemoIndex {
	size_t charnum, linenum;
	bool operator==(const MemoIndex& mi2) const noexcept{
		return this->charnum == mi2.charnum && this->linenum == mi2.linenum;
	}
};
template <> struct std::hash<MemoIndex>{
	size_t operator()(MemoIndex mi) const noexcept{
		return (mi.charnum + mi.linenum*65536);
	}
};
static std::unordered_map<MemoIndex, void *> memos;




Status State::EXT_OPENF_R(){
	std::u32string s;
	std::optional<char32_t> c;
	for (c=*curchar_iter; c.has_value() && c!=U'Л' && c!=U'\n';c=nextChar()) {
		s+=c.value();
	}
	if (!c.has_value() || c==U'\n') return STATUS_Unexpected_EOF;
	std::ifstream * file = new std::ifstream(boost::locale::conv::utf_to_utf<char, char32_t>(s));
	in_streams.push_back(file);
	return STATUS_OK;
}
Status State::EXT_OPENF_W(){
	std::u32string s;
	std::optional<char32_t> c;
	for (c=*curchar_iter; c.has_value() && c!=U'Ш' && c!=U'\n';c=nextChar()) {
		s+=c.value();
	}
	if (!c.has_value() || c==U'\n') return STATUS_Unexpected_EOF;
	std::ofstream * file = new std::ofstream(boost::locale::conv::utf_to_utf<char, char32_t>(s));
	out_streams.push_back(file);
	return STATUS_OK;
}

Status State::EXT_OPENF_A(){
	std::u32string s;
	std::optional<char32_t> c;
	for (c=*curchar_iter; c.has_value() && c!=U'И' && c!=U'\n';c=nextChar()) {
		s+=c.value();
	}
	if (!c.has_value() || c==U'\n') return STATUS_Unexpected_EOF;
	std::ofstream * file = new std::ofstream(boost::locale::conv::utf_to_utf<char, char32_t>(s), std::ios_base::out | std::ios_base::ate);
	out_streams.push_back(file);
	return STATUS_OK;
}
Status State::EXT_CLOSEF_R(){
	if (in_streams.size() <= 1) return STATUS_StackBust;
	std::istream * file = in_streams.back(); in_streams.pop_back();
	dynamic_cast<std::ifstream *>(file)->close(); delete file;
	return STATUS_OK;

}
Status State::EXT_CLOSEF_W(){
	if (out_streams.size() <= 1) return STATUS_StackBust;
	std::ostream * file = out_streams.back(); out_streams.pop_back();
	dynamic_cast<std::ofstream *>(file)->close(); delete file;
	return STATUS_OK;
}


Status State::CallForeign(ForeignPointer ptr){
	const int arg_count=POP_AND_TYPE(int);
	STACK_CHECK(arg_count+1);
	ffi_cif cif;
	std::vector<ffi_type*> arg_types(arg_count, &ffi_type_sint64);
	ffi_prep_cif(&cif, FFI_DEFAULT_ABI, arg_count, &ffi_type_sint64, arg_types.data());
	int rvalue;
	std::vector<int > args(arg_count);
	std::generate_n(
		args.rend(),
		arg_count,
		[this] ()->int {return POP_AND_TYPE(int);}
	);
	std::vector<const int *> arg_ptrs(arg_count);
	std::transform(
		args.begin(),
		args.end(),
		arg_ptrs.begin(),
		[](const int& a){return &a;}
	);
	ffi_call(&cif,(void (*)())ptr.ptr, &rvalue, (void **)arg_ptrs.data());
	return STATUS_OK;
}

Status State::EXT_DLOPEN(){
	std::u32string s;
	std::optional<char32_t> c;
	for (c=*curchar_iter; c.has_value() && c!=U'Ж' && c!=U'\n';c=nextChar()) {
		s+=c.value();
	}
	if (!c.has_value() || c==U'\n') return STATUS_Unexpected_EOF;
	stack.push_back((SharedObject){.ptr=
		dlopen(boost::locale::conv::utf_to_utf<char, char32_t>(s).c_str(), RTLD_NOW)
	});
	return STATUS_OK;
}
Status State::EXT_DLSYM(){
	STACK_CHECK(1);
	SharedObject so=POP_AND_TYPE(SharedObject);
	std::u32string s;
	std::optional<char32_t> c;
	for (c=*curchar_iter; c.has_value() && c!=U'Б' && c!=U'\n';c=nextChar()) {
		s+=c.value();
	}
	if (!c.has_value() || c==U'\n') return STATUS_Unexpected_EOF;
	stack.push_back((ForeignPointer){.ptr=
		dlsym(so.ptr,boost::locale::conv::utf_to_utf<char, char32_t>(s).c_str())}
	);
	return STATUS_OK;
}
Status State::EXT_DLCLOSE(){
	STACK_CHECK(1);
	ForeignPointer foreign=POP_AND_TYPE(ForeignPointer);
	dlclose(foreign.ptr);
	return STATUS_OK;
}
Status State::EXT_MALLOC(){
	STACK_CHECK(1);
	int alloc_size=POP_AND_TYPE(int);
	stack.push_back((ForeignPointer){.ptr=malloc(alloc_size)});
	return STATUS_OK;
}
Status State::EXT_FREE(){
	STACK_CHECK(1);
	ForeignPointer foreign=POP_AND_TYPE(ForeignPointer);
	free(foreign.ptr);
	return STATUS_OK;

}
Status State::EXT_ZGNIRTS_TO_C(){
	STACK_CHECK(1);
	std::u32string s;
	char32_t c=POP_AND_TYPE(int);
	while (c!=0) {
		s+=c;
		STACK_CHECK(1);
		c=POP_AND_TYPE(int);
	}
	stack.push_back((ForeignPointer){.ptr=strdup(boost::locale::conv::utf_to_utf<char, char32_t>(s).c_str())});
	return STATUS_OK;
}
Status State::EXT_INDEX(){
	STACK_CHECK(2);
	int index=POP_AND_TYPE(int);
	void * ptr=POP_AND_TYPE(ForeignPointer).ptr;
	stack.push_back(
		flags.m16?
		((uint16_t *)ptr)[index]:
		((int *)ptr)[index]
	);
	return STATUS_OK;
}

Status State::EXT_INSERT(){
	STACK_CHECK(3);
	int index=POP_AND_TYPE(int);
	int data=POP_AND_TYPE(int);
	void * ptr=POP_AND_TYPE(ForeignPointer).ptr;
	if (flags.m16)
		((uint16_t *)ptr)[index]=data;
	else 
		((int *)ptr)[index]=data;
	
	return STATUS_OK;
}
Status State::EXT_RANDOM(){
	stack.push_back(random_dist(random_engine));
	return STATUS_OK;
}










/*Some helper functions*/

static void mypush(std::vector<member> * stack, int member){
	stack->push_back(member);
}

static intptr_t myat(std::vector<member> * stack, size_t index){
	return toint(stack->at(index));
}
static intptr_t mypop(std::vector<member> * stack) {
	intptr_t result=toint(stack->back());
	stack->pop_back();
	return result;
}
static size_t mysize(std::vector<member> * stack){
	return stack->size();
}
const static void * stack_manip_function_pointers[]={
	(void *)mypush,
	(void *)mypop,
	(void *)myat,
	(void *)mysize
};
Status State::MACHINE_CODE(){
	STACK_CHECK(1);
	void * buf;
	const MemoIndex curpos={.charnum=this->charnum, .linenum=this->linenum};
	if (memos.find(curpos) == memos.end()){
		std::vector<int> storage({POP_AND_TYPE(int)});
		{
			size_t charcount=0;
			std::u32string::const_iterator copy(this->curchar_iter);
			for(char32_t c=*copy; uni_isdigit(c) || c=='`'; c=*++copy){
				if (c!='`')
					storage.push_back(::parse_long_unicode(copy,curline.cend(), charcount));
				else charcount++;
			}
			curchar_iter+=charcount;
			
		}
		const size_t buf_size=storage.size()*(flags.m16?sizeof(uint16_t):sizeof(int32_t));
		buf=mmap(
			NULL,
			buf_size,
			PROT_EXEC|PROT_WRITE|PROT_READ,
			MAP_ANONYMOUS|MAP_PRIVATE,-1,0
		);
		if (buf==(void *)-1){
			std::cerr << strerror(errno) << std::endl;
			munmap(buf,buf_size);
			return STATUS_Invalid_Input;
		}
		if (flags.m16){
			std::uninitialized_copy(
				storage.begin(),
				storage.end(),
				(uint16_t *)buf
			);
		}
		else {
			std::uninitialized_copy(
				storage.begin(),
				storage.end(),
				(int32_t *)buf
			);
		}
		memos[curpos]=buf;
	} else {
		stack.pop_back();
		buf=memos.at(curpos);
	}
	

	asm volatile(
		"/*Asm Block*/\n\t"
		"callq *%0\n\t":
		:
		"r"(buf),
		"D"(&stack),
		"b"(stack_manip_function_pointers)
		:
		"memory"
	);

	return STATUS_OK;

}