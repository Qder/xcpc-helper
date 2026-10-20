#include<bits/stdc++.h>
#define maxn 100005
#define endl '\n'
#define ll long long
using namespace std;

int n, k;
ll a[maxn];
priority_queue<ll> L; //大根堆：窗口里较小的那一半，堆顶是这一半的最大值
priority_queue<ll, vector<ll>, greater<ll>> R; //小根堆：较大的那一半，堆顶是这一半的最小值
unordered_map<ll, int> dl, dr; //欠删次数：这个值还埋在对应的堆里、没真正删掉几次
int cnt_l, cnt_r; //两个堆里还有几个「没过期」的元素（堆的真实大小里混着欠删的）

void clean_l()
{
	while(!L.empty() && dl[L.top()] > 0)
	{
		dl[L.top()] --;
		L.pop(); //欠删的浮到堆顶了，这时候才真删
	}
}

void clean_r()
{
	while(!R.empty() && dr[R.top()] > 0)
	{
		dr[R.top()] --;
		R.pop();
	}
}

// 维持「左堆放最小的 ceil(总个数 / 2) 个」：总个数为 k 时左堆正好 (k + 1) / 2 个，
// k 是偶数时它就是第 k / 2 小，也就是下中位数；k 是奇数时它正好是正中间那个
void balance()
{
	clean_l(); clean_r();
	int target = (cnt_l + cnt_r + 1) / 2;
	while(cnt_l > target) //左堆多了，把最大值让给右堆
	{
		clean_l();
		R.push(L.top()); L.pop();
		cnt_l --; cnt_r ++;
	}
	while(cnt_l < target) //左堆少了，从右堆拿最小的补上
	{
		clean_r();
		L.push(R.top()); R.pop();
		cnt_l ++; cnt_r --;
	}
}

// 用法：ins(x) 插入一个数，del(x) 删掉一个还在窗口里的数，中位数就是 L.top()
// 为什么不能只在窗口移动时 push 新元素、随便 pop 一下了事：要出窗口的 a[i-k] 可能埋在堆中间，
// 堆只看得到堆顶，随手 pop 掉的是别的值——过期元素还赖在堆里、没过期的反而被扔掉，中位数就飘了；
// 而且堆不支持按值删除。所以按值记一笔「欠删」，等它自己浮到堆顶时再丢（每个元素最多进出一次，均摊 O(log k)）
void ins(ll x)
{
	clean_l();
	if(L.empty() || x <= L.top()) { L.push(x); cnt_l ++; } //小于等于左堆顶就归左半，保证左堆全体 <= 右堆全体
	else { R.push(x); cnt_r ++; }
	balance();
}

void del(ll x)
{
	clean_l();
	if(x <= L.top()) { dl[x] ++; cnt_l --; } //等于左堆顶时它必在左堆（相等值放哪边都行，统一记在左堆）
	else { dr[x] ++; cnt_r --; }
	balance();
}

//-- 中位数贪心：让 Σ|a_i - x| 最小 --
// 用法：median_abs_sum(a, n) 返回最小距离和，x 取下中位数 a[(n+1)/2]；偶数长度时中间整段都最优，取哪个都一样
// 注意它会把 b[1..n] 原地排序，后面还要用原序就先拷一份
ll median_abs_sum(ll *b, int m)
{
	sort(b + 1, b + m + 1);
	ll med = b[(m + 1) / 2], res = 0;
	for(int i = 1; i <= m; i ++) res += abs(b[i] - med);
	return res;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n >> k;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	for(int i = 1; i <= n; i ++)
	{
		ins(a[i]); //先加新元素，再删过期元素，中间窗口是 k + 1 个，balance 按总个数自适应
		if(i > k) del(a[i - k]);
		if(i >= k)
		{
			clean_l();
			if(i > k) cout << ' ';
			cout << L.top(); //左堆顶 = 第 (k+1)/2 小 = 下中位数
		}
	}
	cout << endl;
	return 0;
}
/*
Input:
8 4
1 8 2 7 3 6 4 5

Output:
2 3 3 4 4
*/
