
#include <unordered_map>
#include <unicode/umachine.h>
#include <unicode/uchar.h>

#include <format>
#include <type_traits>
#include <array>
#include <algorithm>
#include <deque>
#include <variant>
#include <functional>
#include <unicode/schriter.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include "false.h"

State::State(std::istream * prog_stream, Flags f):
	linenum(0),
	flags(f),
	charnum(0),
	prog_stream(prog_stream),
	random_engine(),
	random_dist(std::numeric_limits<int>::min())
{
	std::random_device rdevice;
	random_engine.seed(rdevice());
	in_streams.push_back(&std::cin);
	out_streams.push_back(&std::cout);
	

	if (flags.log) {
		logfile << "Begin Log\n\n";
		logfile.flush();
	}
	EOL_callback();
}
bool State::EOL_callback(){
	if (!prog_stream->eof()){
		std::string line_raw;
		std::getline(*prog_stream,line_raw);
		curline=boost::locale::conv::utf_to_utf<char32_t, char>(line_raw+'\n');
		this->linenum++; charnum=0;
		if (flags.log)
			logfile << "line " <<
				this->linenum<< ": " <<
					line_raw << std::endl;
		curchar_iter=curline.cbegin();
		return true;
	} else return false;
}
Status State::step(){

	if(curchar_iter >= curline.cend()) {
		if(!EOL_callback()) return STATUS_EOF;
	}
	const char32_t instruction = *curchar_iter;

	if (flags.log){

		std::u32string str(1,instruction);


		long long TOS=stack.empty()?(-1-0x7fffffff):toint(stack.back());

		logfile << std::format(
			"charnum {}\n"
			"linenum {}\n" 
			"TOS {}\n"
			"Instruction {}: {}\n\n\n",
			charnum,
			linenum,
			TOS,
			boost::locale::conv::utf_to_utf<char, char32_t>(str),
			(unsigned long long)instruction

		);
		logfile.flush();
	}
	if (flags.stringmode){
		if (instruction=='"') flags.stringmode=false;
		else putchar32(instruction);
		if (!nextChar().has_value()) return STATUS_Unexpected_EOF;
		
	} else {
		if (uni_isdigit(instruction)){
			stack.push_back((int)parse_long_unicode());
		} else{
			if (!nextChar().has_value()) return STATUS_EOF;
			if (u_isWhitespace(instruction)) return STATUS_OK;
			const auto operation=operations.find(instruction);
			if (
				operation != operations.end() &&
				(flags.ext || !(instruction >= 0x400 && instruction < 0x500))
			){
				return operation->second(this);
			} else {
				if (u_isalpha(instruction)){
					stack.push_back((FalseVarRef){.c=instruction});
					return STATUS_OK;
				}
				return STATUS_Unimplemented;

			}
		}
	}
	return STATUS_OK;

}

FalseLambda::FalseLambda(const State * state):
	curline(state->curline),
	charnum(state->charnum),
	linenum(state->linenum),
	tell(
		(state->prog_stream->eof())?
		std::nullopt:
		std::optional(state->prog_stream->tellg())
	){}

FalseLambda::FalseLambda(const FalseLambda& old):
	curline(old.curline),
	charnum(old.charnum),
	linenum(old.linenum),
	tell(old.tell){}

void FalseLambda::setAll(State * state) const{
	state->curline=this->curline;
	state->charnum=this->charnum;
	state->linenum=this->linenum;
	state->curchar_iter=state->curline.cbegin()+state->charnum;
	if (this->tell.has_value())
		state->prog_stream->seekg(this->tell.value());
	else 
		state->prog_stream->setstate(std::ios_base::eofbit);
}


WhileLambda::WhileLambda(
	FalseLambda&& cond,
	FalseLambda&& body,
	FalseLambda&& end
):
	FalseLambda(body),
	cond(cond),
	end(end),
	current_while_state(WHILE_COND){}

unsigned long long FalseLambda::hash(){
	return charnum + linenum * 0x100000000;
}
unsigned long long WhileLambda::hash() {
	return FalseLambda::hash() + (cond.hash() * 0x10000);
}

std::ofstream logfile;
int main(int argc, char * argv[]){
	std::istream * stream; 
	Flags f;
	/*Flag handling*/
	for (int i=1; i<argc; i++){
		if (argv[i][0]=='-'){
			char * flag=argv[i]+1;
			if (!strncmp(flag, "m16",4)){
				f.m16=true;
			}  else if (!strncmp(flag, "l", 2)){
				f.log=true;
				logfile.open("log");
			}  else if (!strncmp(flag, "yolo", 5)){
				f.disable_error_checking=true;
			} else if (!strncmp(flag, "noext", 6)){
				f.ext=false;
			} else if (!strncmp(flag, "e", 1)){
				for (int j=i; j<argc-1; j++){
					argv[j]=argv[j+1];
				}
				stream = new std::istringstream(argv[i]);
				argc--; i--;
				f.ise=true;
			} else {
				std::cerr << "Mystery flag " << argv[i] <<std::endl;
				return 1;
			}
			for (int j=i; j<argc-1; j++){
				argv[j]=argv[j+1];
			}
			argc--; i--;
		}
	}
	if (f.ise){
		if (argc!=1) return 1;
	}else{
		if (argc==1){
			stream=&std::cin;
		} else if (argc==2){
			stream=new std::ifstream(argv[1]);
		} else {
			return 1;
		}
	}
	
	State interpreter = State(stream,f);
	Status ok_or_error=STATUS_OK;

	while((ok_or_error=interpreter.step())==STATUS_OK);
	
	if (ok_or_error != STATUS_EOF){
		std::string exception_str=std::format(
				"\nException on line {} at char {}\n{}",
				interpreter.get_linenum(), interpreter.get_charnum()
			,error_messages[ok_or_error]);

		std::cerr<<exception_str<<std::endl;
		if (f.log) logfile<<exception_str<<std::endl;
		return 1;
	}
	return 0;
}
