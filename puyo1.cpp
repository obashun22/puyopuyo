//課題
//2020/05/08

/*
【メモ】
PuyoArrayActiveとPuyoArrayStackでPuyoArrayを継承
PuyoArrayActive型を引数に取るように変更
PuyoArrayActiveとPuyoArrayStackをpuyoとstackとして実体化
puyoとstackの盤面を生成
着地した際にpuyoからstackにぷよの情報を引き渡してpuyoは削除しGeneratePuyo
Display関数は表示マスの状態の判定においてpuyoとstackの情報を重ねて判定
情報を統合するためMerge関数を作成
Display関数のぷよ数カウント処理でMarge関数を使用
ぷよがセットで着地するようにどちらかの着地判定がされたらpuyoの状態を全てstackに引き渡す
左右下に移動させる時の条件を追加してめり込まない様に変更
やったことをざっくりまとめると
- puyoとstackに弁面情報を分割
- どちらかの着地判定の際にpuyoからstackに情報をすべて引き渡してpuyoを削除
- puyoとstackの盤面情報をMerge関数で統合して表示
- ぷよ数をMerge関数を通してカウント
- 左右下移動時の制限条件を追加
今後の展望として下矢印入力で一番下まで落ちる使用を追加する
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
	int waitCount = 20000;

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
