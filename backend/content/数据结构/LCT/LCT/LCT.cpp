#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
using namespace std;

//-- Link-Cut Tree：用 splay 维护若干棵「辅助树」，每棵辅助树对应原树的一条偏爱路径 --
// ch[u][0/1] 是辅助树上的左右儿子；fa[u] 在 u 是辅助树根时表示原树上的父亲（虚边），
// 否则就是辅助树上的父亲。is_root(u) 用来区分这两种含义。

int ch[maxn][2], fa[maxn], val[maxn], xr[maxn];
bool rev[maxn]; //区间翻转懒标记：makeroot 时整条路径反向，左右儿子交换

bool is_root(int x) { return ch[fa[x]][0] != x && ch[fa[x]][1] != x; }

void push_up(int x) { xr[x] = xr[ch[x][0]] ^ xr[ch[x][1]] ^ val[x]; }

void apply_rev(int x)
{
	if(!x) return;
	swap(ch[x][0], ch[x][1]); //翻转 = 交换左右儿子 + 给整棵子树打标记
	rev[x] ^= 1;
}

void push_down(int x)
{
	if(!rev[x]) return;
	apply_rev(ch[x][0]);
	apply_rev(ch[x][1]);
	rev[x] = false;
}

void rotate(int x)
{
	int y = fa[x], z = fa[y], d = (ch[y][1] == x);
	if(!is_root(y)) ch[z][ch[z][1] == y] = x; //y 是辅助树根时 z 只有虚边，不能改它的儿子指针
	fa[x] = z;
	ch[y][d] = ch[x][d ^ 1];
	if(ch[x][d ^ 1]) fa[ch[x][d ^ 1]] = y;
	ch[x][d ^ 1] = y;
	fa[y] = x;
	push_up(y); push_up(x);
}

int stk[maxn];

void splay(int x)
{
	int top = 0, y = x;
	stk[++ top] = y;
	while(!is_root(y)) { y = fa[y]; stk[++ top] = y; }
	while(top) push_down(stk[top --]); //从上往下推标记，旋转时左右儿子的含义才是对的
	while(!is_root(x))
	{
		int y = fa[x], z = fa[y];
		if(!is_root(y)) rotate((ch[y][1] == x) == (ch[z][1] == y) ? y : x); //同向先旋父亲
		rotate(x);
	}
}

// 把根到 x 的路径变成一条偏爱路径，x 旋到辅助树根
void access(int x)
{
	int y = 0;
	while(x)
	{
		splay(x);
		ch[x][1] = y; //右儿子换成上次访问的链，原来的右儿子降级成虚边（fa 不变）
		push_up(x);
		y = x;
		x = fa[x];
	}
}

// 把 x 变成原树的根：access 之后 splay，整条路径打翻转标记
void makeroot(int x) { access(x); splay(x); apply_rev(x); }

int findroot(int x)
{
	access(x); splay(x);
	push_down(x);
	while(ch[x][0]) { x = ch[x][0]; push_down(x); } //一直往左走就是原树的根
	splay(x);
	return x;
}

// 取出 x 到 y 的路径，y 成为辅助树根，此时 xr[y] 就是整条路径的异或和
void split(int x, int y) { makeroot(x); access(y); splay(y); }

// 用法：link(x, y) 连一条边（已连通则不连）；cut(x, y) 断开边（不存在则什么都不做）；
//       split(x, y) 之后 xr[y] 是路径信息；改点权要先 access(x) + splay(x)。
// 点编号 1-based，多测需要清空 ch / fa / rev / xr。
void link(int x, int y)
{
	makeroot(x);
	if(findroot(y) != x) fa[x] = y; //x 已经是根，直接挂成 y 的虚儿子
}

void cut(int x, int y)
{
	makeroot(x);
	access(y); splay(y);
	//此时 x 应该是 y 的左儿子且 x 没有右儿子，否则这条边不存在
	if(ch[y][0] == x && !ch[x][1])
	{
		ch[y][0] = 0;
		fa[x] = 0;
		push_up(y);
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int n, m;
	cin >> n >> m;
	for(int i = 1; i <= n; i ++) { cin >> val[i]; push_up(i); }
	while(m --)
	{
		int op, x, y; cin >> op >> x >> y;
		if(op == 0) { split(x, y); cout << xr[y] << endl; } //路径点权异或和
		else if(op == 1) link(x, y);
		else if(op == 2) cut(x, y);
		else { access(x); splay(x); val[x] = y; push_up(x); } //修改点权
	}
	return 0;
}
/*
Input:
3 3
1
2
3
1 1 2
0 1 2
0 1 1

Output:
3
1
*/
