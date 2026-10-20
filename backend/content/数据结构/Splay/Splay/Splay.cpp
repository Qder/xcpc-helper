#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
using namespace std;

//-- Splay（伸展树）：每次把访问到的节点转到根，均摊 O(log n) --
// 相同值的排名按最小名次算：rank(x) = (小于 x 的个数) + 1
// 数组 1-based，0 号是空节点；ch[u][0/1] 是左右儿子，fa 是父亲，cnt 是同一值的个数

const int INF = 2147483647; //前驱/后继不存在时的哨兵，题目要求输出 ±2147483647

int n, root, tot;
int ch[maxn][2], fa[maxn], val[maxn], cnt[maxn], sz[maxn];

void push_up(int u)
{
	sz[u] = sz[ch[u][0]] + sz[ch[u][1]] + cnt[u]; //sz 含重复值的个数
}

void rotate(int u)
{
	// 单旋：u 向上转一层，先把 u 的异侧儿子过继给 f，再改 u / f 的父亲和儿子指针
	int f = fa[u], g = fa[f], d = (ch[f][1] == u);
	ch[f][d] = ch[u][d ^ 1]; //u 的异侧儿子过继给 f
	if(ch[u][d ^ 1]) fa[ch[u][d ^ 1]] = f;
	fa[u] = g; //u 顶替 f 原来的位置
	if(g) ch[g][ch[g][1] == f] = u;
	ch[u][d ^ 1] = f; fa[f] = u;
	push_up(f); push_up(u); //先更新被压下去的 f，再更新 u
}

void splay(int u)
{
	// 双旋把 u 转到根：同向（zig-zig / zag-zag）先旋父亲，异向（zig-zag / zag-zig）先旋自己
	while(fa[u])
	{
		int f = fa[u], g = fa[f];
		if(g)
		{
			int du = (ch[f][1] == u), dg = (ch[g][1] == f);
			rotate(du == dg ? f : u); //同向先旋父亲（zig-zig），异向先旋自己（zig-zag）
		}
		rotate(u);
	}
	root = u; //转完 u 就是根
}

void insert(int x)
{
	// 按值找到位置：相等就加 cnt，否则新建叶子，最后把新节点 splay 到根
	if(!root)
	{
		root = ++ tot; val[root] = x; cnt[root] = sz[root] = 1;
		ch[root][0] = ch[root][1] = fa[root] = 0; //新建节点时清零，避免复用下标留下旧儿子
		return;
	}
	int u = root, f = 0;
	while(u)
	{
		if(val[u] == x) { cnt[u] ++; push_up(u); splay(u); return; } //重复值只加 cnt，不新建节点
		f = u; u = ch[u][x > val[u]];
	}
	u = ++ tot; val[u] = x; cnt[u] = sz[u] = 1; fa[u] = f;
	ch[u][0] = ch[u][1] = 0;
	ch[f][x > val[f]] = u;
	splay(u); //新节点直接转到根
}

void del(int x)
{
	// 先 splay 到根，再按 cnt > 1、整棵树只剩一个、只有一个儿子、有两个儿子分情况删除
	int u = root;
	while(u && val[u] != x) u = ch[u][x > val[u]];
	if(!u) return; //题目保证删除的值存在，这里只是防御
	splay(u); //转到根后左子树全 < x，右子树全 > x
	if(cnt[u] > 1) { cnt[u] --; push_up(u); return; } //情况一：同值有多个，只减计数
	if(!ch[u][0] && !ch[u][1]) { root = 0; return; } //情况二：整棵树只剩这一个节点
	if(!ch[u][0]) { root = ch[u][1]; fa[root] = 0; return; } //只有一个儿子：把儿子提上来
	if(!ch[u][1]) { root = ch[u][0]; fa[root] = 0; return; }
	int p = ch[u][0];
	while(ch[p][1]) p = ch[p][1]; //左子树最大值就是前驱
	splay(p); //情况三：前驱转到根，此时 u 一定是 p 的右儿子
	ch[p][1] = ch[u][1]; //把 u 的右子树挂到前驱右边，u 被摘掉
	fa[ch[p][1]] = p;
	push_up(p);
}

int rank_of(int x)
{
	// 一路累加「左子树大小 + cnt」，得到小于 x 的个数，答案就是它 + 1
	int u = root, res = 0, last = 0;
	while(u)
	{
		last = u;
		if(x <= val[u]) u = ch[u][0]; //x 在左边或就是 u，先不累加
		else { res += sz[ch[u][0]] + cnt[u]; u = ch[u][1]; } //左子树全部 + u 都比 x 小
	}
	if(last) splay(last); //把最后访问的节点转到根，保证均摊复杂度
	return res + 1;
}

int kth(int k)
{
	// 按左子树大小找第 k 小，找到后 splay 到根
	int u = root;
	while(u)
	{
		int l = ch[u][0];
		if(k <= sz[l]) u = l;
		else if(k <= sz[l] + cnt[u]) { splay(u); return val[u]; }
		else { k -= sz[l] + cnt[u]; u = ch[u][1]; }
	}
	return -1; //k 越界（题目保证不会发生）
}

int pre(int x)
{
	// BST 上找小于 x 的最大值：能往右就记下当前值再往右
	int u = root, res = -INF, last = 0;
	while(u)
	{
		last = u;
		if(val[u] < x) { res = val[u]; u = ch[u][1]; } //记下来再往右找更大的
		else u = ch[u][0];
	}
	if(last) splay(last);
	return res;
}

int nxt(int x)
{
	// BST 上找大于 x 的最小值：能往左就记下当前值再往左
	int u = root, res = INF, last = 0;
	while(u)
	{
		last = u;
		if(val[u] > x) { res = val[u]; u = ch[u][0]; } //记下来再往左找更小的
		else u = ch[u][1];
	}
	if(last) splay(last);
	return res;
}

//-- 文艺平衡树（区间翻转）：多维护一个 rev 标记 + push_down，按位置 split 而不是按值 --
// 0 和 len+1 是两个哨兵，[l, r] 对应位置 l-1 和 r+1 之间的子树：
// 把位置 l-1 转到根、位置 r+1 转到根的右儿子，根的右儿子的左子树就是 [l, r]，打个 rev 即可。
// 一句话改法：把上面 kth 的「第 k 小」（按 cnt 累计）换成「第 k 个位置」（每个节点只贡献 1），
//           并在往下走之前 push_down，普通平衡树就变成了文艺平衡树。

int rroot, rtot, rlen; //rlen 是序列长度，输出时用来跳过两个哨兵
int rch[maxn][2], rfa[maxn], rval[maxn], rsz[maxn], rstk[maxn];
bool rrev[maxn];

void rpush_up(int u)
{
	rsz[u] = rsz[rch[u][0]] + rsz[rch[u][1]] + 1; //每个节点代表一个位置，不记 cnt
}

void rpush_down(int u)
{
	if(!rrev[u]) return;
	swap(rch[u][0], rch[u][1]); //翻转 = 交换左右儿子 + 给儿子打标记
	if(rch[u][0]) rrev[rch[u][0]] ^= 1;
	if(rch[u][1]) rrev[rch[u][1]] ^= 1;
	rrev[u] = false;
}

void rrotate(int u)
{
	int f = rfa[u], g = rfa[f], d = (rch[f][1] == u);
	rch[f][d] = rch[u][d ^ 1];
	if(rch[u][d ^ 1]) rfa[rch[u][d ^ 1]] = f;
	rfa[u] = g;
	if(g) rch[g][rch[g][1] == f] = u;
	rch[u][d ^ 1] = f; rfa[f] = u;
	rpush_up(f); rpush_up(u);
}

void rsplay(int u, int goal = 0)
{
	int top = 0, t = u;
	rstk[++ top] = t;
	while(rfa[t] != goal) t = rfa[t], rstk[++ top] = t;
	while(top) rpush_down(rstk[top --]); //从上到下推标记，保证旋转时左右儿子的含义是对的
	while(rfa[u] != goal)
	{
		int f = rfa[u], g = rfa[f];
		if(g != goal)
		{
			int du = (rch[f][1] == u), dg = (rch[g][1] == f);
			rrotate(du == dg ? f : u);
		}
		rrotate(u);
	}
	if(!goal) rroot = u;
}

int rkth(int k) //第 k 个位置（1-based），返回节点编号
{
	int u = rroot;
	while(u)
	{
		rpush_down(u);
		int l = rch[u][0];
		if(k <= rsz[l]) u = l;
		else if(k == rsz[l] + 1) { rsplay(u); return u; }
		else { k -= rsz[l] + 1; u = rch[u][1]; }
	}
	return 0;
}

int rbuild(int l, int r, int f) //按位置递归建树，中序遍历就是原序列
{
	if(l > r) return 0;
	int mid = (l + r) >> 1;
	int u = ++ rtot;
	rval[u] = mid; rfa[u] = f; rrev[u] = false;
	rch[u][0] = rbuild(l, mid - 1, u);
	rch[u][1] = rbuild(mid + 1, r, u);
	rpush_up(u);
	return u;
}

void build_seq(int len) //建 0..len+1 的哨兵序列，之后翻转 [1, len] 不越界
{
	rtot = 0; rlen = len;
	rroot = rbuild(0, len + 1, 0);
}

void reverse_range(int l, int r) //翻转区间 [l, r]（1-based）
{
	int L = rkth(l); //第 l 个节点就是位置 l-1（哨兵也算一个）
	int R = rkth(r + 2); //第 r+2 个节点就是位置 r+1
	rsplay(L, 0);
	rsplay(R, L); //L 在根，R 是根的右儿子
	int t = rch[R][0];
	if(t) rrev[t] ^= 1; //整段打标记，具体交换留到下次访问
}

void print_seq(int u)
{
	if(!u) return;
	rpush_down(u); //输出前下传，保证中序是真实顺序
	print_seq(rch[u][0]);
	if(rval[u] >= 1 && rval[u] <= rlen) cout << rval[u] << ' ';
	print_seq(rch[u][1]);
}

void print_seq()
{
	print_seq(rroot);
	cout << endl;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	while(n --)
	{
		int op, x; cin >> op >> x;
		if(op == 1) insert(x);
		else if(op == 2) del(x);
		else if(op == 3) cout << rank_of(x) << endl;
		else if(op == 4) cout << kth(x) << endl;
		else if(op == 5) cout << pre(x) << endl;
		else cout << nxt(x) << endl;
	}
	return 0;
}
/*
Input:
15
1 5
1 3
1 7
1 5
3 5
4 3
2 5
2 5
3 7
5 7
6 7
2 3
5 7
6 7
4 1

Output:
2
5
2
3
2147483647
-2147483647
2147483647
7
*/
