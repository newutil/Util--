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
int cFileLen = 0;                // 読み込み中ライブラリ関数のファイル長さ
int cFileHead = 0;               // 読み込み中ライブラリ関数のファイル内容先頭アドレス
int cNextFile = 0;                // 次のライブラリ関数の先頭アドレス


// cFileHeadのゲッター
int getCFileHead() {
  return cFileHead;
}


// 1バイト入力ルーチン
int getB() {
  return fgetc(in);
}

// 1バイト出力ルーチン
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

// ある文字列の末尾が特定の文字列で終わっているか判定
boolean endsWith(char *str, char *suffix){
  int len1 = strlen(str);
  int len2 = strlen(suffix);
  if(len1<len2) {
    return false;
  }
  if(strcmp(str + len1 - len2, suffix) == 0) {
   return true;
  }
  return false;
}

// 読み込んだアーカイブファイルのヘッダを確認
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

  cFileHead = ftell(in);            // 現在の.oファイル内容の先頭位置と
  cNextFile = cFileLen + cFileHead; // 次のライブラリ関数の先頭位置を保存

}

// エラーチェック付きの fopen
// アーカイブファイルに対しては専用の処理を行う
void xOpenIn(char *fname) {
  curFile = fname;
  if ((in = fopen(fname, "rb"))==NULL) {   // 入力ファイルオープン
    fError("can't open");
  }
  
  if(endsWith(fname,".a")) {    // 拡張子が.aなら
    checkArc();                 // ヘッダを確認
    isArchive = true;           // アーカイブのフラグを立て、
    readArchive();              // 一つ目のライブラリ関数の情報を読む
  }
  else {
    isArchive = false;
  }
}

// エラーチェック付きの fopen
void xOpenOut(char *fname){
  if ((out = fopen(fname,"wb"))==NULL) {    // 出力ファイルオープン
    perror(fname);
    error("can't open");
  }
}
 
// 入力ファイルクローズ
void fcloseIn(){
  fclose(in);
}

// 出力ファイルクローズ
void fcloseOut(){
  fclose(out);
}

// 入力ファイル用エラーチェック付きの SEEK ルーチン
void xSeekIn(int offset) {
  
  int realOffset = offset;
  if(isArchive)                           // アーカイブファイルの場合
    realOffset = realOffset + cFileHead;  // ファイルの内容の先頭からSEEKする
  if ( (offset&1)!=0 || fseek(in, (long)realOffset, SEEK_SET)!=0){
    fError("file format");
  }
}

// 出力ファイル用エラーチェック付きの SEEK ルーチン
void xSeekOut(int offset) {
  if ((offset&1)!=0 || fseek(out, (long)offset, SEEK_SET)!=0) {
    fError("file format");
  }
}

// ライブラリ関数の先頭へのSEEK専用ルーチン
void xSeekArc(int addr) {
  if(!isArchive) {
    error("アーカイブファイル以外でxSeekArcルーチンを使用");
  }
  if(fseek(in, (long)addr, SEEK_SET)!=0) {
    error("ARCVシンボルが不正");
  }
  cFileHead = addr; // xSeekInのために先頭アドレスを調整
}

// 1ワード出力ルーチン
void putW(int x) {
  putB(x>>8);
  putB(x);
}

// 1ワード入力ルーチン
int getW() {
  int x1 = getB();
  int x2 = getB();
  if (x1==EOF || x2==EOF) fError("undexpected EOF");
  return (x1 << 8) | x2;
}

// 現在読み込み中のアーカイブファイルの中に
// 次のライブラリ関数が存在するか調べる
boolean nextFile(){
  if(!isArchive) {       // アーカイブファイルではないときは
    return false;        // falseで終了
  }
        
  if(fseek(in, (long)cNextFile, SEEK_SET)!=0) {   // 読み込み中ファイルの
    error("file format");                        // 次の場所までseek
  }    

  if(getB()==EOF) {           // ファイルが最後まで読み込めている場合
    return false;             // falseを返す
  }

  if(fseek(in, (long)cNextFile, SEEK_SET)!=0) { // getBした位置に戻す
    error("file format");
  }  
  readArchive();          // 次のファイルを読む
  return true;            // trueを返す
}


// 読み込み中ライブラリ関数の範囲内にいるか調べる
// 文字列表の読み込みに使う
boolean isInLibRange(int addr) {
  if(!isArchive) return true;  // アーカイブの時だけ考える

  if(0 <= addr && addr < cFileLen) { // アドレスがライブラリ関数の範囲内なら
    return true;                             // trueを返す
  }
  return false;
}