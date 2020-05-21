//課題
//2020/05/15

/*
【メモ】
添付ファイルに用意された関数VanishPuyoを用いて
着地判定の後にstackのぷよが4つ以上連結していたら
消えるような仕様に変更
【To Do】
- [x] 着地判定後落下できるぷよを落下させて連結ぷよ削除
- [x] 下ボタンで瞬時に落下
- [] 回転をつける
- [] スコアをつける
- [] 次のぷよを表示
- [] ゲームオーバー機能をつける
- [] 音楽・BGMをつける
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

private:
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
	void GeneratePuyo(PuyoArrayActive &puyo)
	{
		puyocolor newpuyo1;
		puyocolor newpuyo2;
		srand((unsigned int)time(NULL));
		//ランダムなぷよ生成のためにrootを初期化
		randColor(newpuyo1);
		randColor(newpuyo2);
		//puyocolorのメンバをランダムに与える
		puyo.SetValue(0, 5, newpuyo1);
		puyo.SetValue(0, 6, newpuyo2);
	}

	//ぷよの着地判定．着地判定があるとtrueを返す
	bool LandingPuyo(PuyoArrayActive &puyo, PuyoArrayStack &stack)
	{
		bool landed = false;

		for (int y = 0; y < puyo.GetLine(); y++)
		{
			for (int x = 0; x < puyo.GetColumn(); x++)
			{
				//ぷよが最低座標に位置しているまたは一つ下にぷよが存在する場合の処理
				if (puyo.GetValue(y, x) != NONE && (y == puyo.GetLine() - 1 || stack.GetValue(y + 1, x) != NONE))
				{
					landed = true;
					//着地判定されたぷよを消す．本処理は必要に応じて変更する．
					//どちらかが着地した時にすべてのぷよをstackに引き渡す
					for (int i = 0; i < puyo.GetLine(); i++){
						for (int j = 0; j < puyo.GetColumn(); j++){
							if (puyo.GetValue(j, i) == NONE) {
								continue;
							} else {
								stack.SetValue(j, i, puyo.GetValue(j, i));
								puyo.SetValue(j, i, NONE);
							}
						}
					}
				}
			}
		}
		return landed;
	}

	//左移動
	void MoveLeft(PuyoArrayActive &puyo, PuyoArrayStack &stack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyo.GetLine()*puyo.GetColumn()];

		for (int i = 0; i < puyo.GetLine()*puyo.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ左の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = 0; y < puyo.GetLine(); y++)
		{
			for (int x = 0; x < puyo.GetColumn(); x++)
			{
				if (puyo.GetValue(y, x) == NONE) {
					continue;
				}
				//最左縁ではなく左に何も存在しない場合の処理
				if (0 < x && puyo.GetValue(y, x - 1) == NONE && stack.GetValue(y, x - 1) == NONE)
				{
					puyo_temp[y*puyo.GetColumn() + (x - 1)] = puyo.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyo.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyo.GetColumn() + x] = puyo.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y < puyo.GetLine(); y++)
		{
			for (int x = 0; x < puyo.GetColumn(); x++)
			{
				puyo.SetValue(y, x, puyo_temp[y*puyo.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] puyo_temp;
	}

	//右移動
	void MoveRight(PuyoArrayActive &puyo, PuyoArrayStack &stack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyo.GetLine()*puyo.GetColumn()];

		for (int i = 0; i < puyo.GetLine()*puyo.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ右の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = 0; y < puyo.GetLine(); y++)
		{
			for (int x = puyo.GetColumn() - 1; x >= 0; x--)
			{
				if (puyo.GetValue(y, x) == NONE) {
					continue;
				}
				//最右縁ではなく右に何も存在しない場合の処理
				if (x < puyo.GetColumn() - 1 && puyo.GetValue(y, x + 1) == NONE && stack.GetValue(y, x + 1) == NONE)
				{
					puyo_temp[y*puyo.GetColumn() + (x + 1)] = puyo.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyo.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyo.GetColumn() + x] = puyo.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y <puyo.GetLine(); y++)
		{
			for (int x = 0; x <puyo.GetColumn(); x++)
			{
				puyo.SetValue(y, x, puyo_temp[y*puyo.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] puyo_temp;
	}

	//下移動
	void MoveDown(PuyoArrayActive &puyo, PuyoArrayStack &stack)
	{
		//一時的格納場所メモリ確保
		puyocolor *puyo_temp = new puyocolor[puyo.GetLine()*puyo.GetColumn()];

		for (int i = 0; i < puyo.GetLine()*puyo.GetColumn(); i++)
		{
			puyo_temp[i] = NONE;
		}

		//1つ下の位置にpuyoactiveからpuyo_tempへとコピー
		for (int y = puyo.GetLine() - 1; y >= 0; y--)
		{
			for (int x = 0; x < puyo.GetColumn(); x++)
			{
				if (puyo.GetValue(y, x) == NONE) {
					continue;
				}
				//最下縁ではなく下に何も存在しない場合の処理
				if (y < puyo.GetLine() - 1 && puyo.GetValue(y + 1, x) == NONE && stack.GetValue(y + 1, x) == NONE)
				{
					puyo_temp[(y + 1)*puyo.GetColumn() + x] = puyo.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					puyo.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					puyo_temp[y*puyo.GetColumn() + x] = puyo.GetValue(y, x);
				}
			}
		}

		//puyo_tempからpuyoactiveへコピー
		for (int y = 0; y < puyo.GetLine(); y++)
		{
			for (int x = 0; x < puyo.GetColumn(); x++)
			{
				puyo.SetValue(y, x, puyo_temp[y*puyo.GetColumn() + x]);
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
	void StackMoveDown(PuyoArrayStack &stack)
	{
		//一時的格納場所メモリ確保
		puyocolor *stack_temp = new puyocolor[stack.GetLine()*stack.GetColumn()];

		for (int i = 0; i < stack.GetLine()*stack.GetColumn(); i++)
		{
			stack_temp[i] = NONE;
		}

		//1つ下の位置にぷよをずらす
		for (int y = stack.GetLine() - 1; y >= 0; y--)
		{
			for (int x = 0; x < stack.GetColumn(); x++)
			{
				if (stack.GetValue(y, x) == NONE) {
					continue;
				}
				//最下縁ではなく下に何も存在しない場合の処理
				if (y < stack.GetLine() - 1 && stack.GetValue(y + 1, x) == NONE && stack.GetValue(y + 1, x) == NONE)
				{
					stack_temp[(y + 1)*stack.GetColumn() + x] = stack.GetValue(y, x);
					//コピー後に元位置のpuyoactiveのデータは消す
					stack.SetValue(y, x, NONE);
				}
				else
				{
					//上記の場合に当てはまらない場合に値を引き継ぐ処理
					stack_temp[y*stack.GetColumn() + x] = stack.GetValue(y, x);
				}
			}
		}

		//stack_tempからstackへコピー
		for (int y = 0; y < stack.GetLine(); y++)
		{
			for (int x = 0; x < stack.GetColumn(); x++)
			{
				stack.SetValue(y, x, stack_temp[y*stack.GetColumn() + x]);
			}
		}

		//一時的格納場所メモリ解放
		delete[] stack_temp;
	}

	// void RotateRight(PuyoArrayActive &puyo, PuyoArrayStack &stack)
	// {
	//
	// }

};

//puyoとstackのぷよの情報を統合するための関数
puyocolor Merge(PuyoArrayActive &puyo, PuyoArrayStack &stack, int y, int x)
{
	if (puyo.GetValue(y, x) != NONE){
		return puyo.GetValue(y, x);
	} else {
		return stack.GetValue(y, x);
	}
}

//表示
void Display(PuyoArrayActive &puyo, PuyoArrayStack &stack)
{
	//落下中ぷよ表示
	for (int y = 0; y < puyo.GetLine(); y++)
	{
		for (int x = 0; x < puyo.GetColumn(); x++)
		{
			switch (Merge(puyo, stack, y, x))
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
	for (int y = 0; y < puyo.GetLine(); y++)
	{
		for (int x = 0; x < puyo.GetColumn(); x++)
		{
			if (Merge(puyo, stack, y, x) != NONE)
			{
				count++;
			}
		}
	}

	char msg[256];
	sprintf(msg, "Field: %d x %d, Puyo number: %03d", puyo.GetLine(), puyo.GetColumn(), count);
	mvaddstr(2, COLS - 35, msg);

	refresh();
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

	PuyoArrayActive puyo;
	PuyoArrayStack stack;
	PuyoControl control;

	//初期化処理
	puyo.ChangeSize(LINES/2, COLS/2);	//フィールドは画面サイズの縦横1/2にする
	stack.ChangeSize(LINES/2, COLS/2);
	control.GeneratePuyo(puyo);	//最初のぷよ生成

	int delay = 0;
	int waitCount = 10000;

	int puyostate = 0;


	//メイン処理ループ
	while (1)
	{
		//キー入力受付
		int ch;
		ch = getch();

		//Qの入力で終了
		if (ch == 'Q')
		{
			break;
		}

		//入力キーごとの処理
		switch (ch)
		{
		case KEY_LEFT:
			control.MoveLeft(puyo, stack);
			break;
		case KEY_RIGHT:
			control.MoveRight(puyo, stack);
			break;
		case 'z':
			//ぷよ回転処理

			break;
		case KEY_DOWN:
			//下ボタンを押すと下まで一瞬で落下する
			for (int i=0; i<puyo.GetLine()-1; i++){
				control.MoveDown(puyo, stack);
			}
			break;
		default:
			break;
		}

		//処理速度調整のためのif文
		if (delay%waitCount == 0){
			//ぷよ下に移動
			control.MoveDown(puyo, stack);

			//着地判定があると新しいぷよを生成
			//ぷよ着地判定
			if (control.LandingPuyo(puyo, stack))
			{
				//連鎖が止まるまで落下・削除を実行
				while(1){
					for (int i=0; i<stack.GetLine()-1; i++)
					{
						//着地したらまず落ちうるぷよを落下させる
						control.StackMoveDown(stack);
					}
					//着地したらぷよが4つ以上連結していないかチェックする
					//ぷよに変化がなければぷよ生成に進む
					int vanishednumber = control.VanishPuyo(stack);
					if (vanishednumber == 0){
						break;
					}
				}
				//着地していたら新しいぷよ生成
				control.GeneratePuyo(puyo);
			}
		}
		delay++;

		//表示
		Display(puyo, stack);
	}

	//画面をリセット
	endwin();

	return 0;
}
