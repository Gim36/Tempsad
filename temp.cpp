#include <bits/stdc++.h>

using namespace std;

int divise(long long n, int k, unordered_map<long long, long long> &xors) {
	if (n == 1) {
		return 1;
	} else if (xors.find(n) != xors.end()) {
		return xors[n];
	}

	set<long long> divisors;
	for (long long i = 1; k > 0; i++) {
		if (n % i == 0) {
			divisors.insert(i);
			divisors.insert(n / i);
			k--;
		}
	}

	long long last = 0;
	for (auto x : divisors) {
		cout << x << " ";
		last ^= divise(x, k, xors);
	}

	cout << endl;
	xors[n] = last;
	return last;
}

int main() {
	ios::sync_with_stdio(false);
	cin.tie(NULL);

	long long n, d;
	int k;
	cin >> n >> k;

	unordered_map<long long, long long> xors;
	for (int i = 0; i < k; i++) {
		cin >> d;
		cout << "Calling divise(" << d << ", xors)" << endl;
		cout << divise(d, k / 2, xors) << "divise";
	}

	return 0;
}
