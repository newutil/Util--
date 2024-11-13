#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

#define A_MAGIC 0x600a       //アーカイブファイルのマジックナンバー
#define MAX_FILENAME_SIZ 500 //ファイル名の長さの限界

// ファイル関係
FILE* out;                                 // 出力ファイル
FILE* in;                                  // 入力ファイル
char *curFile = "";                        // 現在の入出力ファイル

// アーカイブファイル関係
boolean isArchive = false;        //現在の入力ファイルがアーカイブファイルかどうか
char curAFile[MAX_FILENAME_SIZ];  //アーカイブファイル内の読み込み中ファイル名
int cFileLen = 0;                 //アーカイブファイル内の読み込み中ファイル長さ
int cFileHead = 0;                //アーカイブファイル内の読み込み中ファイル内容の先頭アドレス


int getB() {
  return fgetc(in);
}

void putB(char c) {
  fputc(c,out);
}

void error(char *str) {                   // エラーメッセージを表示して終了
  fprintf(stderr, "%s\n", str);
  exit(1);
}

void fError(char *str) {                   // ファイル名付きでエラー表示
  perror(curFile);
  error(str);
}

void tblError(char *str, int idx, int size) {  //表がパンクした時のエラー表示
  fprintf(stderr, "%s\t%5d/%5d\n",str,idx,size);
  exit(1);
}

void checkArc() {
  char *arcStr = "!<arch>\n";       //アーカイブファイル判定用
  int i=0;
  int c;
  int aLen = strlen(arcStr);
  while(i<aLen) {
    c = getB();
    if(c != arcStr[i]) {             //想定しているファイル形式でなければ
      fError("アーカイブファイルではない.aファイル\n");   //エラー終了
    }
    i = i + 1;
  }; 
  printf("debug: File header OK.\n");  //デバッグ用

}


void readArchive() {   //アーカイブファイル内の各ファイル情報を読み取る

  printf("debug: Execting readArchive().\ndebug: ファイル名：");  //デバッグ用

  int i=0;
  int c;
  while((c=getB())!='\n') { //ファイル名読み込み
    curAFile[i] = (char)c;
    printf("%c",(char)c);
    i = i + 1;
    if(i>MAX_FILENAME_SIZ) {
      fError("ライブラリ関数のファイル名が長すぎる\n");
    }
  }

  // if((curAFile[i-2]!='.'||curAFile[i-1]!='o')){ //.oファイル以外があった時
  //   fError("アーカイブファイル内に扱えないファイルが存在\n");
  // }

  cFileLen = getW();              //ファイルの長さ
  printf("\ndebug: ファイルの長さ：%d\n",cFileLen);
  c = getW();
  printf("debug: masic number：%x\n",c);  //デバッグ用
  if (c!=A_MAGIC) {             //   マジックナンバー
    fError("扱えないアーカイブファイル");
  }

  cFileHead = ftell(in);  //現在の.oファイルの先頭位置を保存
  printf("debug: ファイルの先頭アドレス：%d\n",cFileHead);
  printf("debug: Ready to archive reading.\n");  //デバッグ用
    
}


void xOpenIn(char *fname) {                // エラーチェック付きの fopen
                                           // アーカイブファイルに対応
  curFile = fname;

  if ((in = fopen(fname, "rb"))==NULL) {   // 入力ファイルオープン
    fError("can't open");
  }

  int length = strlen(fname);                         // 名前の長さを取得
  if(fname[length-2]=='.' && fname[length-1]=='a') {  //拡張子が.a

    printf("debug: This file \"%s\" is archive file.\n",curFile);  //デバッグ用

    checkArc();
    isArchive = true;                       //アーカイブのフラグを立て、
    readArchive();                          //一つ目のライブラリ関数の情報を読む
  }
  else {
    printf("debug: This file \"%s\" is NOT archive file.\n",curFile);  //デバッグ用
    isArchive = false;
  }

}



void xOpenOut(char *fname){                 // エラーチェック付きの fopen
  if ((out = fopen(fname,"wb"))==NULL) {    // 出力ファイルオープン
    perror(fname);
    error("can't open");
  }
}
 
void fcloseIn(){                            //入力ファイルクローズ
  fclose(in);
}

void fcloseOut(){                           //入力ファイルクローズ
  fclose(out);
}

void xSeekIn(int offset) {  // 入力ファイル用エラーチェック付きの SEEK ルーチン
  
  int realOffset = offset;
  if(isArchive)                           //アーカイブファイルの場合
    realOffset = realOffset + cFileHead;  //ファイルの内容の先頭からSEEKする
  
  if ( (offset&1)!=0 || fseek(in, (long)realOffset, SEEK_SET)!=0){
    fError("file format");
  }
}

void xSeekOut(int offset) { // 出力ファイル用エラーチェック付きの SEEK ルーチン
  if ((offset&1)!=0 || fseek(out, (long)offset, SEEK_SET)!=0) {
    fError("file format");
  }
}

void putW(int x) {                          // 1ワード出力ルーチン
  putB(x>>8);
  putB(x);
}

int getW() {                                // 1ワード入力ルーチン
  int x1 = getB();
  int x2 = getB();
  if (x1==EOF || x2==EOF) fError("undexpected EOF");
  return (x1 << 8) | x2;
}

boolean nextFile(){      //現在読み込み中のアーカイブファイルの中で、
                         //次のファイルがあるかどうかを調べる
  if(!isArchive) {
    return false;         //アーカイブファイルではないときはfalseで終了
  }
  
  int offset = cFileHead + cFileLen;
  if(fseek(in, (long)offset, SEEK_SET)!=0) {    //読み込み中ファイルの内容の最後までseek
    error("file format");
  }    

  if(getB()==EOF) {           //ファイルが最後まで読み込めている場合
    printf("debug: nextFile() returned true.\n\tArchive file is already read all.\n"); //デバッグ用
    return false;
  }

  printf("debug: nextFile() returned false.\n\tArchive file is not read all yet.\n"); //デバッグ用
  if(fseek(in, (long)offset, SEEK_SET)!=0){//元の場所に戻る
    error("file format");
  } 
  readArchive();          //次のファイルを読む
  return true;    //trueを返す
}
