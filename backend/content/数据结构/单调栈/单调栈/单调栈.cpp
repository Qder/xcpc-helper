#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define maxn 100050
#define endl '\n'
#define ll long long
using namespace std;

int n, a[maxn];
int nxt[maxn], prv[maxn]; //nxt[i] 是右边第一个更大的下标，prv[i] 是左边第一个更小的下标，0 表示不存在

// 用法：读入 a[1..n] 后调用 next_greater()，答案在 nxt[]（右边第一个严格大于 a[i] 的下标，没有为 0）
void next_greater()
{
	stack<int> stk; //存下标，栈底到栈顶单调递减
	for(int i = 1; i <= n; i ++)
	{
		while(!stk.empty() && a[stk.top()] < a[i]) //严格小于才弹，相等的不弹（相等不算"更大"）
		{
			nxt[stk.top()] = i; //被弹出来的那一刻，它的答案就是 i
			stk.pop();
		}
		stk.push(i);
	}
}

// 用法：调用 prev_smaller()，答案在 prv[]（左边第一个严格小于 a[i] 的下标，没有为 0）
void prev_smaller()
{
	stack<int> stk; //存下标，栈底到栈顶单调递增
	for(int i = 1; i <= n; i ++)
	{
		while(!stk.empty() && a[stk.top()] >= a[i]) stk.pop(); //>= 才弹，弹完栈顶一定严格更小
		prv[i] = stk.empty() ? 0 : stk.top(); //从左往右扫，push 之前读栈顶
		stk.push(i);
	}
}

// 用法：cout << largest_rectangle(n);  // 柱状图 a[1..n] 里的最大矩形面积（柱子宽都为 1，高度可正可负）
ll largest_rectangle(int N)
{
	if(N == 0) return 0;
	ll ans = -1e18; //柱子高度可以为负，答案也可能是负的，所以初值不能给 0
	vector<int> h(N + 2);
	for(int i = 1; i <= N; i ++) h[i] = a[i];
	h[N + 1] = -inf; //哨兵：最右边补一根足够矮的柱子，保证结束时栈被清空（用 -1 的话高度为负就错了）
	stack<int> stk; //栈底到栈顶单调递增
	for(int i = 1; i <= N + 1; i ++)
	{
		while(!stk.empty() && h[stk.top()] >= h[i])
		{
			int top = stk.top(); stk.pop();
			int left = stk.empty() ? 0 : stk.top(); //弹出后的新栈顶就是左边第一个更矮的柱子
			ans = max(ans, (ll)h[top] * (i - left - 1));
		}
		stk.push(i);
	}
	return ans;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	next_greater();
	for(int i = 1; i <= n; i ++) cout << nxt[i] << endl;
	return 0;
}
/*
Input:
7
3 1 3 2 5 5 4

Output:
5
3
5
5
0
0
0
*/
