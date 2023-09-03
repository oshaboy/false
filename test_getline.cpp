#include <iostream>
#include <fstream>
int main(void){
	std::istream * uwu=new std::ifstream("hello3.fal");
	std::string a;
	std::cout << uwu->eof()<<std::endl;
	std::getline (*uwu, a);
	std::cout << uwu->eof()<<std::endl;
	std::getline (*uwu, a);
	std::cout << uwu->eof()<<std::endl;
	std::getline (*uwu, a);
	std::cout << uwu->eof()<<std::endl;

	return 0;
}

