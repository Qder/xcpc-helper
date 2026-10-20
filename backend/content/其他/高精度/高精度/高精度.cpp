#include<bits/stdc++.h>
#define inf 0x3f3f3f3f
#define endl '\n'
#define ll long long
using namespace std;

// 倒序存：a[0] 是最低位，进位、借位都往下标大的方向走，末尾 push_back 就是最高位
// 用 int 存一位（不压位）：乘法里每个 c[i+j] 最多累加 min(|a|,|b|) 个 9 * 9，int 远远够

vector<int> to_vec(const string &s)
{
	int st = 0;
	while(st + 1 < (int)s.size() && s[st] == '0') st ++; //去掉前导 0，但 "0" 本身要留一位
	vector<int> a;
	for(int i = (int)s.size() - 1; i >= st; i --) a.push_back(s[i] - '0');
	return a;
}

void print(const vector<int> &a)
{
	for(int i = (int)a.size() - 1; i >= 0; i --) cout << a[i]; //倒着存就要倒着输出
	cout << endl;
}

vector<int> add(const vector<int> &a, const vector<int> &b)
{
	vector<int> c;
	int t = 0; //t 是低一位传上来的进位
	for(int i = 0; i < (int)a.size() || i < (int)b.size() || t; i ++)
	{
		if(i < (int)a.size()) t += a[i];
		if(i < (int)b.size()) t += b[i];
		c.push_back(t % 10);
		t /= 10; //循环条件里带着 t，最高位再进位一次也能补上一位
	}
	return c;
}

// 用法：sub(a, b) 要求数值上 a >= b（本题保证），否则借位会借穿
vector<int> sub(const vector<int> &a, const vector<int> &b)
{
	vector<int> c;
	int t = 0; //t = 1 表示这一位被低一位借走了 1
	for(int i = 0; i < (int)a.size(); i ++)
	{
		t = a[i] - t;
		if(i < (int)b.size()) t -= b[i];
		c.push_back((t + 10) % 10); //借位后是负数，加 10 取个位
		t = (t < 0) ? 1 : 0; //这一位借了，下一位接着减 1
	}
	while(c.size() > 1 && c.back() == 0) c.pop_back(); //去前导 0，但结果为 0 时要留一位 0
	return c;
}

// 用法：mul(a, b) 大数乘大数；a 1e5 位、b 1e3 位时是 1e5 * 1e3 = 1e8 次乘加，O(|a| * |b|)
// 两边都是 1e5 位就会到 1e10，那时候得换 FFT/NTT，这个板子不行
vector<int> mul(const vector<int> &a, const vector<int> &b)
{
	vector<int> c(a.size() + b.size(), 0); //结果最多 |a| + |b| 位
	for(int i = 0; i < (int)a.size(); i ++)
		for(int j = 0; j < (int)b.size(); j ++)
			c[i + j] += a[i] * b[j]; //先全部乘完累在对应位上，不进位，这样每个位置只用算一次
	int t = 0;
	for(int i = 0; i < (int)c.size(); i ++)
	{
		t += c[i];
		c[i] = t % 10;
		t /= 10; //最后统一从低到高进一次位
	}
	while(c.size() > 1 && c.back() == 0) c.pop_back();
	return c;
}

int main()
{
	ios::sync_with_stdio(false);
	cin.tie(0); cout.tie(0);
	
	string sa, sb;
	cin >> sa >> sb; //两行各一个非负整数
	vector<int> a = to_vec(sa), b = to_vec(sb);
	print(add(a, b));
	print(sub(a, b)); //题目保证 a >= b
	print(mul(a, b));
	return 0;
}
/*
Input:
987654321987654321
123456789123456789

Output:
1111111111111111110
864197532864197532
121932631356500531347203169112635269
*/
