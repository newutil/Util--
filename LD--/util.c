#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

#define A_MAGIC 0x600a           // アーカイブファイルのマジックナンバー
#define MAX_FILENAME_SIZ 5000    // ファイル名の長さの限界

// ファイル関係
FILE* out;                       // 出力ファイル
FILE* in;                        // 入力ファイル
char *curFile = "";              // 現在の入出力ファイル

// アーカイブファイル関係
boolean isArchive = false;       // 現在の入力ファイルがアーカイブファイルかどうか
char curAFile[MAX_FILENAME_SIZ]; // 読み込み中ライブラリ関数のファイル名
int cFileLen = 0;                // 読み込み中ライブラリ関数のファイル長さ
int cFileHead = 0;               // 読み込み中ライブラリ関数のファイル内容先頭アドレス


int getLibHead() {
  return cFileHead;
}


int getB() {
  return fgetc(in);
}

void putB(char c) {
  fputc(c,out);
}

// エラーメッセージを表示して終了
void error(char *str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
}

// ファイル名付きでエラー表示
void fError(char *str) {
  perror(curFile);
  error(str);
}

// 表がパンクした時のエラー表示
void tblError(char *str, int idx, int size) {
  fprintf(stderr, "%s\t%5d/%5d\n",str,idx,size);
  exit(1);
}

void checkArc() {
  char *arcStr = "!<arch>\n";    // アーカイブファイル判定用
  int aLen = strlen(arcStr);
  for(int i = 0; i < aLen; i = i + 1){
    if(getB() != arcStr[i]) {    // 想定しているファイル形式でなければ
      fError("アーカイブファイルではない.aファイル\n");    // エラー終了
    }
  }
}

// アーカイブファイル内の各ファイル情報を読み取る
void readArchive() {

  int i=0;
  int c;

  while((c=getB())!='\n') { // ファイル名を読み込む
    curAFile[i] = (char)c;
    i = i + 1;
    if(i>MAX_FILENAME_SIZ) {
      fError("ライブラリ関数のファイル名が長すぎる\n");
    }
  }

  cFileLen = getW();            // ファイルの長さ
  c = getW();
  if (c!=A_MAGIC) {             // マジックナンバー
    fError("扱えないアーカイブファイル");
  }

  cFileHead = ftell(in);        // 現在の.oファイルの先頭位置を保存
}

void xOpenIn(char *fname) {     // エラーチェック付きの fopen
                                // アーカイブファイルに対応
  curFile = fname;

  if ((in = fopen(fname, "rb"))==NULL) {   // 入力ファイルオープン
    fError("can't open");
  }

  
  int length = strlen(fname);                      // 名前の長さを取得
  if(fname[length-2]=='.' && fname[length-1]=='a') { // 拡張子が.aなら
    checkArc();                        // ヘッダを確認
    isArchive = true;                  // アーカイブのフラグを立て、
    readArchive();                     // 一つ目のライブラリ関数の情報を読む
  }
  else {
    isArchive = false;
  }

}



void xOpenOut(char *fname){                 // エラーチェック付きの fopen
  if ((out = fopen(fname,"wb"))==NULL) {    // 出力ファイルオープン
    perror(fname);
    error("can't open");
  }
}
 
void fcloseIn(){                            // 入力ファイルクローズ
  fclose(in);
}

void fcloseOut(){                           // 入力ファイルクローズ
  fclose(out);
}

void xSeekIn(int offset) {  // 入力ファイル用エラーチェック付きの SEEK ルーチン
  
  int realOffset = offset;
  if(isArchive)                           // アーカイブファイルの場合
    realOffset = realOffset + cFileHead;  // ファイルの内容の先頭からSEEKする
  
  if ( (offset&1)!=0 || fseek(in, (long)realOffset, SEEK_SET)!=0){
    fError("file format");
  }
}

void xSeekOut(int offset) { // 出力ファイル用エラーチェック付きの SEEK ルーチン
  if ((offset&1)!=0 || fseek(out, (long)offset, SEEK_SET)!=0) {
    fError("file format");
  }
}

void xSeekArc(int addr) {   // ライブラリ関数の先頭へのSEEK専用ルーチン
  if(!isArchive) {
    error("アーカイブファイル以外でsSeekArcルーチンを使用");
  }
  if(fseek(in, (long)addr, SEEK_SET)!=0) {
    error("ARCVシンボルが不正");
  }
  cFileHead = addr; // xSeekInのために先頭アドレスを調整
}

void putW(int x) {          // 1ワード出力ルーチン
  putB(x>>8);
  putB(x);
}

int getW() {                // 1ワード入力ルーチン
  int x1 = getB();
  int x2 = getB();
  if (x1==EOF || x2==EOF) fError("undexpected EOF");
  return (x1 << 8) | x2;
}

boolean nextFile(){      // 現在読み込み中のアーカイブファイルの中で、
                         // 次のファイルがあるかどうかを調べる
  if(!isArchive) {       // アーカイブファイルではないときは
    return false;        // falseで終了
  }
  
  int offset = cFileHead + cFileLen;           // 読み込み中ファイルの
  if(fseek(in, (long)offset, SEEK_SET)!=0) {   // 内容の最後までseek
    error("file format");
  }    

  if(getB()==EOF) {           // ファイルが最後まで読み込めている場合
    return false;             // falseを返す
  }

  if(fseek(in, (long)offset, SEEK_SET)!=0) { // 次の読み取り位置までseek
    error("file format");
  } 
  readArchive();          // 次のファイルを読む
  return true;            // trueを返す
}