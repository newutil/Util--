#ifndef REL_H
#define REL_H

#define REL_SIZ  6000                       // 再配置表の大きさ
struct Reloc {                              // 再配置表
  int addr;                                 // ポインタのセグメント内 Offs
  int symx;                                 // シンボルテーブル上の番号
};
int getRelIdx();    //使用した表の領域のゲッター
struct Reloc getRelTbl(int index); //再配置表のゲッター
int getTrSize();
int getDrSize();
void readTrRelTbl(int offs, int cTrSize, int symBase, int textBase);
void readDrRelTbl(int offs, int cDrSize, int symBase, int textBase);
void updateRelSymx(int prtIdx);           // 再配置表のインデクスを調整
void writeRelTbl();                       // 再配置表をファイルへ出力
void printRelTbl();                       // 再配置表をリスト出力
#endif
