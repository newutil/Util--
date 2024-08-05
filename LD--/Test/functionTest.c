#include <stdio.h>
#include <string.h>
int main(){

    char* fname="test..o";

    char* s;
    char* a=".a";
    char* o=".o";
    s = strrchr(fname,'.');

    printf("%s",s);

    if(strcmp(s,a)==0) printf("このファイルは.aファイルです");
    else if(strcmp(s,o)==0) printf("このファイルは.oファイルです");


  

}