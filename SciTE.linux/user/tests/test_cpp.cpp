// go@ cmd /c g++  -c *.cpp -o $(FileName).o && g++ -o "$(FileName).exe" *.o -s $(libs) && $(FileName).exe

#include <iostream>
#include <string>
#include <unistd.h>

/* 
	  ---== cpp lexer sample ==---
		-> to debug, set a breakpoint with F9, and press CTRL-F5
		-> a strip, showing the source file path will show up.
		-> change "test.cpp" to "test.cpp.exe" and you are set.
*/

void mySay(char *myString) {

		printf(myString);
}
	
	
main() {

// Define some Vars to see in locals view
	std::string file_name = "test.cpp";
	std::string msg = "..file " +  file_name;
	
	char *test = (char*)"---------Test-------";
	char *file_name_plain = (char*)file_name.c_str();
	char *msg_plain = (char*)msg.c_str();
	char *test2 = (char*)"--------Test-------";
	
// out a greeting,
	printf(msg_plain);
 
// check for a file 
	int exists = access(	(char*)file_name.c_str()	,F_OK	);

// out result
	if ( exists !=-1) {
			printf(" .. found - okay..");
	} else {
			printf(" .. not found - not okay..");
	}
		
// hmm ..	finally...  lets do some -= ascii_art =- Tetris...
	mySay( (char*)"\n	  ::: \n	::...::  \n");
}

