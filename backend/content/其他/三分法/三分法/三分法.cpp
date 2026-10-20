#include<bits/stdc++.h>
#define maxn 1000005
#define endl '\n'
#define ll long long
using namespace std;

int n;
ll a[maxn]; //a[1..n] 严格先增后减

// 用法：ternary_int() 返回峰值下标（1-based）；调用前 a[1..n] 必须是严格单峰
int ternary_int()
{
	int l = 1, r = n;
	while(r - l > 2) //留 2 个格子：只剩 3 个点时 m1 就是 l，再缩区间根本不变短，会死循环
	{
		int m1 = l + (r - l) / 3, m2 = r - (r - l) / 3;
		if(a[m1] < a[m2]) l = m1; //m2 更高，峰一定在 m1 右边，左边整段扔掉
		else r = m2; //m1 不低于 m2，峰在 m2 左边；相等就落到这里，严格单峰保证不会出现平台
	}
	int pos = l;
	for(int i = l + 1; i <= r; i ++)
		if(a[i] > a[pos]) pos = i; //最后剩不超过 3 个点，暴力扫一遍收尾最稳
	return pos;
}

double f_demo(double x) { return -x * x + 3 * x + 1; } //示例单峰函数，在 x = 1.5 处取最大值

// 用法：ternary_real(l, r, f) 返回单峰函数 f 在 [l, r] 上的最大值点，f 传函数名（如 f_demo）；
// 实数版不用留格子，固定迭代 100 次，区间按 (2/3)^100 收缩，早就低于 double 精度了
double ternary_real(double l, double r, double (*f)(double))
{
	for(int i = 0; i < 100; i ++)
	{
		double m1 = l + (r - l) / 3, m2 = r - (r - l) / 3;
		if(f(m1) < f(m2)) l = m1;
		else r = m2;
	}
	return (l + r) / 2;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	cin >> n;
	for(int i = 1; i <= n; i ++) cin >> a[i];
	int pos = ternary_int();
	cout << a[pos] << ' ' << pos - 1 << endl; //题目要 0-based 下标，1-based 存的下标减 1
	return 0;
}
/*
Input:
5
1 3 9 7 2

Output:
9 2
*/
