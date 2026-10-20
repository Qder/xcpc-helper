#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 1000005
#define maxm 100005
#define endl '\n'
#define ll long long
using namespace std;

// 约束：n <= 1e5，模式串总长 <= 1e6，|t| <= 1e6；模式串和文本都只含小写字母。
// maxn：Trie 节点数上限 = 模式串总长 + 1（1 号根，0 号哨兵）；maxm：模式串个数上限
// 空间：ch[maxn][26] 约 100MB（maxn = 1e6+5），fail / cnt / bfn 各约 4MB，整份约 115MB。
// 字符集只有 26 个小写字母时 int[26] 是"用空间换 O(1) 转移"：换成 map 或链式前向星能省掉 26 倍空边，
// 但 BFS 补 Trie 图只能枚举真实出边、查询也要逐字符找边，常数和代码量都上去了；字母集上万或很稀疏才值得换。
int ch[maxn][26], fail[maxn], cnt[maxn], bfn[maxn];
int tot = 1, bfs; //tot：节点数，根固定 1 号；0 号哨兵，ch[0][*] = 1
int d[maxm]; //d[i] = 第 i 个模式串的结尾节点

void insert(const string &s, int id)
{
	int u = 1;
	for(char c : s)
	{
		int &v = ch[u][c - 'a'];
		if(!v) v = ++ tot;
		u = v;
	}
	d[id] = u; //重复模式串落在同一个节点，之后读 cnt[d[i]] 自然得到相同的计数
}

void build()
{
	fill(ch[0], ch[0] + 26, 1); //哨兵：根的缺边都指向根，根的儿子 fail 自动 = 根，全程不用特判根
	queue<int> q;
	q.push(1);
	bfs = 0;
	while(!q.empty())
	{
		int u = q.front(); q.pop();
		bfn[++ bfs] = u; //BFS 序深度浅的在前，正好是 fail 树自底向上累加的拓扑序
		for(int c = 0; c < 26; c ++)
		{
			int &v = ch[u][c];
			if(!v) v = ch[fail[u]][c]; //Trie 图优化：缺边直接补成 fail 的对应转移，查询 O(1)
			else
			{
				fail[v] = ch[fail[u]][c]; //有儿子才建 fail；不能写进 !v 分支
				q.push(v);
			}
		}
	}
}

// 用法：build() 后 scan(t)，再 calc()，第 i 个模式串的答案 = cnt[d[i]]
void scan(const string &t)
{
	int u = 1;
	for(char c : t)
	{
		u = ch[u][c - 'a']; //补完边后一定非 0，不用再 while 跳 fail
		cnt[u] ++;
	}
}

// fail 树逆序累加：把 u 的 cnt 上抛给 fail[u]，累加完后 cnt[u] = 以 u 结尾的串在文本中的完整出现次数（可重叠）
// 不这样做、改为每个文本位置沿 fail 往上跳着收答案，全 a 串时 fail 链长 O(串长)，最坏 O(|t| * 串长) 直接 TLE；
// 也有板子另开 last[u]（输出边）跳着收集，这里不用：scan 只记经过次数，calc 在 fail 树上一次性累加，不会重复统计。
// 另一种 build 写法是不补 Trie 图：查询时 while(u && !ch[u][c]) u = fail[u]，最坏 O(|t| * 最长模式串)，补边后才是 O(|t|)。
void calc()
{
	for(int i = bfs; i >= 1; i --)
	{
		int u = bfn[i];
		cnt[fail[u]] += cnt[u]; //根 1 的 fail 是 0，多出来的 cnt[0] 没人读，不影响答案
	}
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	int n;
	cin >> n;
	for(int i = 1; i <= n; i ++)
	{
		string s; cin >> s;
		insert(s, i);
	}
	build();
	string t; cin >> t;
	scan(t);
	calc();
	for(int i = 1; i <= n; i ++) cout << cnt[d[i]] << endl;
	return 0;
}
/*
Input:
5
he
she
he
his
hers
shers

Output:
1
1
1
0
1
*/
