#ifndef REL_H
#define REL_H

#define REL_SIZ  6000                       // 再配置表の大きさ
struct Reloc {                              // 再配置表
  int addr;                                 // ポインタのセグメント内 Offs
  int symx;                                 // シンボルテーブル上の番号
};

int getRelIdx();    //使用した表の領域のゲッター
void setRelIdx(int num);    //使用した表の領域のセッター
int getRelTbl(int index,char *str); //再配置表のゲッター
void setRelTbl(int index, int newAddr, int newSymx);  //再配置表のセッター

void readRelTbl(int offs, int relSize, int symBase, int textBase);
void packSymTbl();                        // 名前表の不要エントリーを削除
void writeRelTbl();                       // 再配置表をファイルへ出力
void printRelTbl();                       // 再配置表をリスト出力

#endif
