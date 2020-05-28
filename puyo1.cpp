//課題
//2020/05/22

/*
【メモ】
連結ぷよを削除した後ぷよが再度落下するように変更
再度落下したぷよも連結判定されるように変更
添付されたコードを用いてRotateメンバ関数とpuyorotateメンバ変数を
追加して回転させることができるように変更した
stackを無視して回転するので修正
修正後に謎の分裂がみられるので修正する必要がある
謎の分裂や回転軸のズレは着地判定の際にpuyorotateを初期化することで修正
puyorotateのアクセスをメンバ関数経由にして機密性を高めた
時計回り（'z'）と反時計回り（'x'）のコマンド操作を追加
puyoactiveとpuyostackのインスタンス名及びそれらの仮引数名をpuyoactiveとpuyostackに統一
謎のmain関数内の変数puyostate = 0 を削除（心配）
GameOver画面を追加（ゲームのメイン部分をGameStartに集約／GameOver画面で終了・リスタートを選択できる）
スコアカウント及びスコア表示を追加（消したぷよの数x10）
ゲーム開始時に盤面の高さと幅，ぷよの落下速度を選択できるようにした
[prob]再スタートするとメモリがうまく解放できていないのかぷよが残っている
[prob]高さ幅共に12以下になると着地判定にバグが発生する
PuyoArrayのデストラクタによるReleaseは普通は関数内で終わるはずだが，
main関数終了まで生きていると仮定したら，派生クラスでReleaseをしてあげたほうがいい
でも，ChangeSizeでもReleaseするのでどうなんだろうか
【To Do】
- [x] 着地判定後落下できるぷよを落下させて連結ぷよ削除
- [x] 下ボタンで瞬時に落下
- [x] 回転をつける
- [x] 名前変更: puyo, stack -> puyoactive, puyostack
- [x] ゲームオーバー機能をつける
- [x] スコアをつける
- [x] 初期設定画面を表示
- [] 次のぷよを表示
- [] 音楽・BGMをつける//環境的に困難か
*/

#include <curses.h>
#include <stdlib.h> //rand, srand関数で使用
#include <time.h> //srand関数で使用

//ぷよの色を表すの列挙型
//NONEが無し，RED,BLUE,..が色を表す
enum puyocolor { NONE, RED, BLUE, GREEN, YELLOW };

//盤面状態の保持に関するクラス
class PuyoArray
{
public:
	PuyoArray()
	{
		data = NULL;
		data_line = 0;
		data_column = 0;
	}

	~PuyoArray()
	{
		Release();
	}

	//盤面サイズ変更
	void ChangeSize(unsigned int line, unsigned int column)
	{
		Release();

		//新しいサイズでメモリ確保
		data = new puyocolor[line*column];

		data_line = line;
		data_column = column;
	}

	//盤面の行数を返す
	unsigned int GetLine()
	{
		return data_line;
	}

	//盤面の列数を返す
	unsigned int GetColumn()
	{
		return data_column;
	}

	//盤面の指定された位置の値を返す
	puyocolor GetValue(unsigned int y, unsigned int x)
	{
		if (y >= GetLine() || x >= GetColumn())
		{
			//引数の値が正しくない
			return NONE;
		}

		return data[y*GetColumn() + x];
	}

	//盤面の指定された位置に値を書き込む
	void SetValue(unsigned int y, unsigned int x, puyocolor value)
	{
		if (y >= GetLine() || x >= GetColumn())
		{
			//引数の値が正しくない
			return;
		}

		data[y*GetColumn() + x] = value;
	}

protected:
	//盤面状態
	puyocolor *data;
	unsigned int data_line;
	unsigned int data_column;

	//メモリ開放
	void Release()
	{
		if (data == NULL) {
			return;
		}
		delete[] data;
		data = NULL;
	}
};

//「落下中」ぷよを管理するクラス
class PuyoArrayActive: public PuyoArray
{
public:
	PuyoArrayActive(){
		puyorotate = 0;
	}
	int GetPuyoRotate(){
		return puyorotate;
	}
	void SetPuyoRotate(int state){
		puyorotate = state;
	}
private:
	int puyorotate;
};

//「着地済み」ぷよを管理するクラス
class PuyoArrayStack: public PuyoArray
{

};

//ぷよの発生・移動等のぷよ管理を行うクラス
class PuyoControl
{
public:
	//puyocolorのメンバをランダムに与える関数
	void randColor(puyocolor &newpuyo)
	{
		switch (rand()%4+1){
			case 1:
				newpuyo = RED;
				break;
			case 2:
				newpuyo = BLUE;
				break;
			case 3:
				newpuyo = GREEN;
				break;
			case 4:
				newpuyo = YELLOW;
				break;
		}
	}

	//盤面に新しいぷよ生成
	void GeneratePuyo(PuyoArrayActive &puyoactive)
	{
		puyocolor newpuyo1;
		puyocolor newpuyo2;
		srand((unsigned int)time(NULL));
		//ランダムなぷよ生成のためにrootを初期化
		randColor(newpuyo1);
		randColor(newpuyo2);
		//puyocolorのメンバをランダムに与える
		puyoactive.SetValue(0, 5, newpuyo1);
		puyoactive.SetValue(0, 6, newpuyo2);
	}

	//ぷよの着地判定．着地判定があるとtrueを返す
	bool LandingPuyo(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		bool landed = false;

		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				//ぷよが最低座標に位置しているまたは一つ下にぷよが存在する場合の処理
				if (puyoactive.GetValue(y, x) != NONE && (y == puyoactive.GetLine() - 1 || puyostack.GetValue(y + 1, x) != NONE))
				{
					landed = true;
					//どちらかが着地した時にすべてのぷよをstackに引き渡す
					for (int i = 0; i < puyoactive.GetLine(); i++){
						for (int j = 0; j < puyoactive.GetColumn(); j++){
							if (puyoactive.GetValue(j, i) == NONE) {
								continue;
							} else {
								puyostack.SetValue(j, i, puyoactive.GetValue(j, i));
								puyoactive.SetValue(j, i, NONE);
							}
						}
					}
					//puyoactiveがカラになるからbreakしてもいいかも
					//次のぷよのために回転状態を初期化する
					puyoactive.SetPuyoRotate(0);
				}
			}
		}
		return landed;
	}

	//左移動
	void MoveLeft(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyoactive.GetLine()*puyoactive.GetColumn()];

		for (int i = 0; i < puyoactive.GetLine()*puyoactive.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ左の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) == NONE) {
					continue;
				}
				//最左縁ではなく左に何も存在しない場合の処理
				if (0 < x && puyoactive.GetValue(y, x - 1) == NONE && puyostack.GetValue(y, x - 1) == NONE)
				{
					puyo_temp[y*puyoactive.GetColumn() + (x - 1)] = puyoactive.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyoactive.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyoactive.GetColumn() + x] = puyoactive.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				puyoactive.SetValue(y, x, puyo_temp[y*puyoactive.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] puyo_temp;
	}

	//右移動
	void MoveRight(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyoactive.GetLine()*puyoactive.GetColumn()];

		for (int i = 0; i < puyoactive.GetLine()*puyoactive.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ右の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = puyoactive.GetColumn() - 1; x >= 0; x--)
			{
				if (puyoactive.GetValue(y, x) == NONE) {
					continue;
				}
				//最右縁ではなく右に何も存在しない場合の処理
				if (x < puyoactive.GetColumn() - 1 && puyoactive.GetValue(y, x + 1) == NONE && puyostack.GetValue(y, x + 1) == NONE)
				{
					puyo_temp[y*puyoactive.GetColumn() + (x + 1)] = puyoactive.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyoactive.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyoactive.GetColumn() + x] = puyoactive.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y <puyoactive.GetLine(); y++)
		{
			for (int x = 0; x <puyoactive.GetColumn(); x++)
			{
				puyoactive.SetValue(y, x, puyo_temp[y*puyoactive.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] puyo_temp;
	}

	//下移動
	void MoveDown(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyoactive.GetLine()*puyoactive.GetColumn()];

		for (int i = 0; i < puyoactive.GetLine()*puyoactive.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ下の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = puyoactive.GetLine() - 1; y >= 0; y--)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) == NONE) {
					continue;
				}
				//最下縁ではなく下に何も存在しない場合の処理
				if (y < puyoactive.GetLine() - 1 && puyoactive.GetValue(y + 1, x) == NONE && puyostack.GetValue(y + 1, x) == NONE)
				{
					puyo_temp[(y + 1)*puyoactive.GetColumn() + x] = puyoactive.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyoactive.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyoactive.GetColumn() + x] = puyoactive.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				puyoactive.SetValue(y, x, puyo_temp[y*puyoactive.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] puyo_temp;
	}

	//各マスにおいてぷよの連結があるかを確認する関数
	int VanishPuyo(PuyoArrayStack &puyostack)
	{
		int vanishednumber = 0;
		//すべてのマスにおいて4以上の連結ぷよの削除を実行
		for (int y = 0; y < puyostack.GetLine(); y++)
		{
			for (int x = 0; x < puyostack.GetColumn(); x++)
			{
				vanishednumber += VanishPuyo(puyostack, y, x);
			}
		}
		//トータルの削除したぷよ数を返す
		return vanishednumber;
	}

	//あるマスにおいて4つ以上のぷよの連結があるかを確認してあれば削除する関数
	int VanishPuyo(PuyoArrayStack &puyostack, unsigned int y, unsigned int x)
	{
		//空のマスは飛ばす
		if (puyostack.GetValue(y, x) == NONE)
		{
			return 0;
		}
		//以降ぷよが存在する場合のみの実行となる

		enum checkstate{ NOCHECK, CHECKING, CHECKED };

		//指定されたマスごとにチェック専用のメモリを確保する
		enum checkstate *field_array_check;
		field_array_check = new enum checkstate[puyostack.GetLine()*puyostack.GetColumn()];

		//全盤面をNOCHECKに初期化
		for (int i = 0; i < puyostack.GetLine()*puyostack.GetColumn(); i++)
		{
			field_array_check[i] = NOCHECK;
		}

		//指定されたマスをCHECKINGに変更
		field_array_check[y*puyostack.GetColumn() + x] = CHECKING;


		bool checkagain = true;
		while (checkagain)
		{
			checkagain = false;

			//CHECKINGを探す
			for (int y = 0; y < puyostack.GetLine(); y++)
			{
				for (int x = 0; x < puyostack.GetColumn(); x++)
				{
					//CHECKINGを指しているときのみ実行
					if (field_array_check[y*puyostack.GetColumn() + x] == CHECKING)
					{
						//最右縁でない場合
						if (x < puyostack.GetColumn() - 1)
						{
							//ぷよが右隣と同じかつ右隣がまだ未確認だった場合
							if (puyostack.GetValue(y, x + 1) == puyostack.GetValue(y, x) && field_array_check[y*puyostack.GetColumn() + (x + 1)] == NOCHECK)
							{
								field_array_check[y*puyostack.GetColumn() + (x + 1)] = CHECKING;
								checkagain = true;
							}
						}

						//最左縁でない場合
						if (x > 0)
						{
							//ぷよが左隣と同じかつ左隣がまだ未確認だった場合
							if (puyostack.GetValue(y, x - 1) == puyostack.GetValue(y, x) && field_array_check[y*puyostack.GetColumn() + (x - 1)] == NOCHECK)
							{
								field_array_check[y*puyostack.GetColumn() + (x - 1)] = CHECKING;
								checkagain = true;
							}
						}

						//最下縁ではない場合
						if (y < puyostack.GetLine() - 1)
						{
							//ぷよが直下と同じかつ直下がまだ未確認だった場合
							if (puyostack.GetValue(y + 1, x) == puyostack.GetValue(y, x) && field_array_check[(y + 1)*puyostack.GetColumn() + x] == NOCHECK)
							{
								field_array_check[(y + 1)*puyostack.GetColumn() + x] = CHECKING;
								checkagain = true;
							}
						}

						//最上縁ではない場合
						if (y > 0)
						{
							//ぷよが直上と同じかつ直上がまだ未確認だった場合
							if (puyostack.GetValue(y - 1, x) == puyostack.GetValue(y, x) && field_array_check[(y - 1)*puyostack.GetColumn() + x] == NOCHECK)
							{
								field_array_check[(y - 1)*puyostack.GetColumn() + x] = CHECKING;
								checkagain = true;
							}
						}

						field_array_check[y*puyostack.GetColumn() + x] = CHECKED;
					}
				}
			}
		}

		//CHECKEDになったマス数をカウント
		int puyocount = 0;
		for (int i = 0; i < puyostack.GetLine()*puyostack.GetColumn(); i++)
		{
			if (field_array_check[i] == CHECKED)
			{
				puyocount++;
			}
		}

		int vanishednumber = 0;
		//CHECKEDになったマスが4以上の場合は削除を実行
		if (4 <= puyocount)
		{
			for (int y = 0; y < puyostack.GetLine(); y++)
			{
				for (int x = 0; x < puyostack.GetColumn(); x++)
				{
					if (field_array_check[y*puyostack.GetColumn() + x] == CHECKED)
					{
						puyostack.SetValue(y, x, NONE);

						vanishednumber++;
					}
				}
			}
		}

		delete[] field_array_check;

		//削除したぷよの数を返り値として返す
		return vanishednumber;
	}

	//stack上の落下できるぷよを全て落下させる
	void StackMoveDown(PuyoArrayStack &puyostack)
	{
		//一時的格納場所メモリ確保
		puyocolor *stack_temp = new puyocolor[puyostack.GetLine()*puyostack.GetColumn()];

		for (int i = 0; i < puyostack.GetLine()*puyostack.GetColumn(); i++)
		{
			stack_temp[i] = NONE;
		}

		//1つ下の位置にぷよをずらす
		for (int y = puyostack.GetLine() - 1; y >= 0; y--)
		{
			for (int x = 0; x < puyostack.GetColumn(); x++)
			{
				if (puyostack.GetValue(y, x) == NONE) {
					continue;
				}
				//最下縁ではなく下に何も存在しない場合の処理
				if (y < puyostack.GetLine() - 1 && puyostack.GetValue(y + 1, x) == NONE && puyostack.GetValue(y + 1, x) == NONE)
				{
					stack_temp[(y + 1)*puyostack.GetColumn() + x] = puyostack.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyostack.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					stack_temp[y*puyostack.GetColumn() + x] = puyostack.GetValue(y, x);
				}
			}
		}

		//stack_tempからstackへコピー
		for (int y = 0; y < puyostack.GetLine(); y++)
		{
			for (int x = 0; x < puyostack.GetColumn(); x++)
			{
				puyostack.SetValue(y, x, stack_temp[y*puyostack.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] stack_temp;
	}

	//ぷよを時計回りに回転させる
	void RotateClockwise(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		puyocolor puyo1, puyo2;
		int puyo1_x = 0;
		int puyo1_y = 0;
		int puyo2_x = 0;
		int puyo2_y = 0;

		// ぷよを0番目から探して順にpuyo1, puyo2とする
		bool findingpuyo1 = true;
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) != NONE)
				{
					if (findingpuyo1)
					{
						puyo1 = puyoactive.GetValue(y, x);
						puyo1_x = x;
						puyo1_y = y;
						findingpuyo1 = false;
					}
					else
					{
						puyo2 = puyoactive.GetValue(y, x);
						puyo2_x = x;
						puyo2_y = y;
					}
				}
			}
		}

		// ぷよがあったマスの属性をNONEに変更
		puyoactive.SetValue(puyo1_y, puyo1_x, NONE);
		puyoactive.SetValue(puyo2_y, puyo2_x, NONE);

		switch (puyoactive.GetPuyoRotate())
		{
		//初期回転状態の場合
		case 0:
			if ((puyo1_y < puyoactive.GetLine() - 1) && (puyostack.GetValue(puyo1_y + 1, puyo1_x) == NONE))
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y + 1, puyo2_x - 1, puyo2);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			puyoactive.SetPuyoRotate(1);
			break;

		case 1:
			if ((0 < puyo1_x) && (puyostack.GetValue(puyo1_y, puyo1_x - 1) == NONE))
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y - 1, puyo2_x - 1, puyo2);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			puyoactive.SetPuyoRotate(2);
			break;

		case 2:
			if ((0 < puyo2_y) && (puyostack.GetValue(puyo2_y - 1, puyo2_x) == NONE))
			{
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				puyoactive.SetValue(puyo1_y - 1, puyo1_x + 1, puyo1);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			puyoactive.SetPuyoRotate(3);
			break;

		case 3:
			if ((puyo2_x < puyoactive.GetColumn() - 1) && (puyostack.GetValue(puyo2_y, puyo2_x + 1) == NONE))
			{
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				puyoactive.SetValue(puyo1_y + 1, puyo1_x + 1, puyo1);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			puyoactive.SetPuyoRotate(0);
			break;

		default:
			break;
		}
	}

	//ぷよを反時計回りに回転させる
	void RotateCounterClockwise(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack)
	{
		puyocolor puyo1, puyo2;
		int puyo1_x = 0;
		int puyo1_y = 0;
		int puyo2_x = 0;
		int puyo2_y = 0;

		// ぷよを0番目から探して順にpuyo1, puyo2とする
		bool findingpuyo1 = true;
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) != NONE)
				{
					if (findingpuyo1)
					{
						puyo1 = puyoactive.GetValue(y, x);
						puyo1_x = x;
						puyo1_y = y;
						findingpuyo1 = false;
					}
					else
					{
						puyo2 = puyoactive.GetValue(y, x);
						puyo2_x = x;
						puyo2_y = y;
					}
				}
			}
		}

		// ぷよがあったマスの属性をNONEに変更
		puyoactive.SetValue(puyo1_y, puyo1_x, NONE);
		puyoactive.SetValue(puyo2_y, puyo2_x, NONE);

		switch (puyoactive.GetPuyoRotate())
		{
		//初期回転状態の場合
		case 0:
			if ((0 < puyo1_y) && (puyostack.GetValue(puyo1_y - 1, puyo1_x) == NONE))
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y - 1, puyo2_x - 1, puyo2);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			puyoactive.SetPuyoRotate(3);
			break;

		case 1:
			if ((puyo1_x < puyoactive.GetColumn() - 1) && (puyostack.GetValue(puyo1_y, puyo1_x + 1) == NONE))
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y - 1, puyo2_x + 1, puyo2);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			puyoactive.SetPuyoRotate(0);
			break;

		case 2:
			if ((puyo2_y < puyoactive.GetLine() - 1) && (puyostack.GetValue(puyo2_y + 1, puyo2_x) == NONE))
			{
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				puyoactive.SetValue(puyo1_y + 1, puyo1_x + 1, puyo1);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			puyoactive.SetPuyoRotate(1);
			break;

		case 3:
			if ((0 < puyo2_x) && (puyostack.GetValue(puyo2_y, puyo2_x - 1) == NONE))
			{
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				puyoactive.SetValue(puyo1_y + 1, puyo1_x - 1, puyo1);
			} else {
					puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
					puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
			}
			puyoactive.SetPuyoRotate(2);
			break;

		default:
			break;
		}
	}
};

//puyoactiveとpuyostackのぷよの情報を統合するための関数
puyocolor Merge(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack, int y, int x)
{
	if (puyoactive.GetValue(y, x) != NONE){
		return puyoactive.GetValue(y, x);
	} else {
		return puyostack.GetValue(y, x);
	}
}

//表示
void Display(PuyoArrayActive &puyoactive, PuyoArrayStack &puyostack, int score)
{
	//落下中ぷよ表示
	for (int y = 0; y < puyoactive.GetLine(); y++)
	{
		for (int x = 0; x < puyoactive.GetColumn(); x++)
		{
			switch (Merge(puyoactive, puyostack, y, x))
			{
			case NONE:
				mvaddch(y, x, '.');
				break;
			case RED:
				attrset(COLOR_PAIR(1));
				//出力する文字色を変更
				mvaddch(y, x, 'R');
				attrset(COLOR_PAIR(0));
				//出力する文字色を元に戻す
				break;
			case BLUE:
				attrset(COLOR_PAIR(2));
				mvaddch(y, x, 'B');
				attrset(COLOR_PAIR(0));
				break;
			case GREEN:
				attrset(COLOR_PAIR(3));
				mvaddch(y, x, 'G');
				attrset(COLOR_PAIR(0));
				break;
			case YELLOW:
				attrset(COLOR_PAIR(4));
				mvaddch(y, x, 'Y');
				attrset(COLOR_PAIR(0));
				break;
			default:
				mvaddch(y, x, '?');
				break;
			}
		}
	}

	//情報表示
	int count = 0;
	for (int y = 0; y < puyoactive.GetLine(); y++)
	{
		for (int x = 0; x < puyoactive.GetColumn(); x++)
		{
			if (Merge(puyoactive, puyostack, y, x) != NONE)
			{
				count++;
			}
		}
	}

	char state_msg[256];
	sprintf(state_msg, "Field: %d x %d, Puyo number: %03d", puyoactive.GetLine(), puyoactive.GetColumn(), count);
	mvaddstr(2, COLS - 35, state_msg);
	char score_msg[256];
	sprintf(score_msg, "Score: %05d", score);
	mvaddstr(3, COLS - 35, score_msg);

	refresh();
}

bool GameOverJudge(PuyoArrayStack &puyostack)
{
	if ((puyostack.GetValue(0, 5) != NONE) || (puyostack.GetValue(0, 6) != NONE))
	{
		return true;
	}
	else
	{
		return false;
	}
}

int GameOver()
{
	//画面に"GameOver"を表示する
	mvprintw(5, COLS - 35, "===============");
	mvprintw(6, COLS - 35, "#  GAME OVER  #");
	mvprintw(7, COLS - 35, "===============");
	mvprintw(9, COLS - 35, "Press 'r' To Continue");
	mvprintw(10, COLS - 35, "Press 'q' To Quit");
	while (1)
	{
		int ch = getch();
		if (ch == 'q')
		{
			erase();
			return 0;
		}
		else if (ch == 'r')
		{
			erase();
			return 1;
		}
	}
}

void GameStart(int height, int width, int speed)
{
	//盤面と操作のインスタンスを作成
	PuyoArrayActive puyoactive;
	PuyoArrayStack puyostack;
	PuyoControl control;

	//初期化処理
	puyoactive.ChangeSize(height, width);
	puyostack.ChangeSize(height, width);
	control.GeneratePuyo(puyoactive);	//最初のぷよ生成

	int delay = 0;
	int waitCount = speed;

	int score = 0;

	//メイン処理ループ
	while (1)
	{
		//キー入力受付
		int ch;
		ch = getch();

		//Qの入力で終了
		if (ch == 'q')
		{
			return;
		}

		//入力キーごとの処理
		switch (ch)
		{
		case KEY_LEFT:
			control.MoveLeft(puyoactive, puyostack);
			break;
		case KEY_RIGHT:
			control.MoveRight(puyoactive, puyostack);
			break;
		case 'z':
			//ぷよ回転処理（時計回り）
			control.RotateClockwise(puyoactive, puyostack);
			break;
		case 'x':
			//ぷよ回転処理（反時計回り）
			control.RotateCounterClockwise(puyoactive, puyostack);
			break;
		case KEY_DOWN:
			//下ボタンを押すと下まで一瞬で落下する
			for (int i=0; i<puyoactive.GetLine()-1; i++){
				control.MoveDown(puyoactive, puyostack);
			}
			break;
		default:
			break;
		}

		//処理速度調整のためのif文
		if (delay%waitCount == 0){
			//ぷよ下に移動
			control.MoveDown(puyoactive, puyostack);

			//着地判定があると新しいぷよを生成
			//ぷよ着地判定
			if (control.LandingPuyo(puyoactive, puyostack))
			{
				//連鎖が止まるまで落下・削除を実行
				while(1){
					for (int i=0; i<puyostack.GetLine()-1; i++)
					{
						//着地したらまず落ち得るぷよを落下させる
						control.StackMoveDown(puyostack);
					}
					//着地したらぷよが4つ以上連結していないかチェックする
					//ぷよに変化がなければぷよ生成に進む
					int vanishednumber = control.VanishPuyo(puyostack);
					score += vanishednumber * 10;
					if (vanishednumber == 0){
						break;
					}
				}
				//そのターンの処理がすべて終わった時点で詰んでないか確認
				if (GameOverJudge(puyostack) == true)
				{
					flash();
					return;
				}
				//着地していたら新しいぷよ生成
				control.GeneratePuyo(puyoactive);
			}
		}
		delay++;
		//表示
		Display(puyoactive, puyostack, score);
	}
}

void GameSetting(int &height, int &width, int &speed)
{
	while (1)
	{
		//盤面の高さを設定
		while(1){
			int ch = getch();
			switch (ch){
				case KEY_RIGHT:
				case KEY_UP:
					if (height < 20){
						height++;
					}
					break;
				case KEY_LEFT:
				case KEY_DOWN:
					if (12 < height){
						height--;
					}
					break;
				case '\n':
					break;
			}
			mvprintw(4, 20, "*** WELCOME TO PUYO-PUYO! ***");
			mvprintw(5, 20, "=============================");
			mvprintw(7, 20, "Field Height: %02d", height);
			mvprintw(13, 20, "Adjust Parameters with Arrow Keys");
			mvprintw(14, 20, "Press Enter to Confirm");
			if (ch == '\n'){ break; }
		}
		//盤面の幅を設定
		while(1){
			int ch = getch();
			switch (ch){
				case KEY_RIGHT:
				case KEY_UP:
					if (width < 42){
						width++;
					}
					break;
				case KEY_LEFT:
				case KEY_DOWN:
					if (12 < width){
						width--;
					}
					break;
				case '\n':
					break;
			}
			mvprintw(4, 20, "*** WELCOME TO PUYO-PUYO! ***");
			mvprintw(5, 20, "=============================");
			mvprintw(7, 20, "Field Height: %02d", height);
			mvprintw(8, 20, "Field Width: %02d", width);
			mvprintw(13, 20, "Adjust Parameters with Arrow Keys");
			mvprintw(14, 20, "Press Enter to Confirm");
			if (ch == '\n'){ break; }
		}
		//ぷよの落下スピードを設定
		int speed_degree = 3;
		while(1){
			int ch = getch();
			switch (ch){
				case KEY_RIGHT:
				case KEY_UP:
					if (speed_degree < 5){
						speed_degree++;
					}
					break;
				case KEY_LEFT:
				case KEY_DOWN:
					if (1 < speed_degree){
						speed_degree--;
					}
					break;
				case '\n':
					break;
			}
			switch (speed_degree){
				case 1: speed = 60000; break;
				case 2: speed = 40000; break;
				case 3: speed = 20000; break;
				case 4: speed = 10000; break;
				case 5: speed = 7500; break;
			}
			mvprintw(4, 20, "*** WELCOME TO PUYO-PUYO! ***");
			mvprintw(5, 20, "=============================");
			mvprintw(7, 20, "Field Height: %02d", height);
			mvprintw(8, 20, "Field Width: %02d", width);
			mvprintw(9, 20, "Falling Speed: %d", speed_degree);
			mvprintw(13, 20, "Adjust Parameters with Arrow Keys");
			mvprintw(14, 20, "Press Enter to Confirm");
			if (ch == '\n'){ break; }
		}
		erase();
		break;
	}
}

//ここから実行される
int main(int argc, char **argv){
	//画面の初期化
	initscr();
	//カラー属性を扱うための初期化
	start_color();
	//キーを押しても画面に表示しない
	noecho();
	//キー入力を即座に受け付ける
	cbreak();
	//カーソルを非表示にする
	curs_set(0);
	//キー入力受付方法指定
	keypad(stdscr, TRUE);

	//キー入力非ブロッキングモード
	timeout(0);

	//出力の際に使用する色を定義
	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);

	int nextaction;
	while(1){
		int height = 12, width = 12, speed = 20000;
		GameSetting(height, width, speed);
		GameStart(height, width, speed);
		//'q'を押すまたは詰みの時GameOver画面に移動
		nextaction = GameOver();
		if ( nextaction == 0 ){
			break;
		} else {
			continue;
		}
	}
	//画面をリセット
	endwin();
	return 0;
}
