#include <bits/stdc++.h>

using namespace std;

void build(long long div, int idx, vector<int> &exponents,
		   vector<pair<long long, int>> &factors,
		   map<long long, vector<int>> &dte,
		   map<vector<int>, long long> &table) {
	if (idx == factors.size()) {
		dte[div] = exponents;
		table[exponents] = div;
		return;
	}

	long long p = factors[idx].first, val = div;
	int max = factors[idx].second;
	for (int i = 0; i <= max; i++) {
		exponents[idx] = i;
		build(val, idx + 1, exponents, factors, dte, table);
		val *= p;
	}
}

int main() {
	ios_base::sync_with_stdio(false);
	cin.tie(NULL);

	long long n, cur;
	int k;
	cin >> n >> k;

	long long temp = n;
	vector<long long> queries(k), divs(k);
	vector<pair<long long, int>> factors;
	for (int i = 0; i < k; i++) {
		cin >> cur;
		queries[i] = cur;
		divs[i] = cur;
	}

	sort(divs.begin(), divs.end());
	for (auto d : divs) {
		if (d == 1) {
			continue;
		}

		if (d * d > temp) {
			break;
		}

		if (temp % d == 0) {
			int count = 0;
			while (temp % d == 0) {
				temp /= d;
				count++;
			}

			factors.push_back({d, count});
		}
	}

	if (temp > 1) {
		factors.push_back({temp, 1});
	}

	vector<int> exponents(factors.size());
	map<long long, vector<int>> dte;
	map<vector<int>, long long> table;
	build(1, 0, exponents, factors, dte, table);

	for (int i = 0; i < factors.size(); i++) {
		for (auto const &pair : table) {
			const vector<int> &exponents = pair.first;
			if (exponents[i] < factors[i].second) {
				vector<int> next = exponents;
				next[i]++;
				table[next] ^= table.at(exponents);
			}
		}
	}

	for (int i = 0; i < k - 1; i++) {
		vector<int> exponents = dte[queries[i]];
		cout << table[exponents] << " ";
	}

	cout << table[dte[queries[k - 1]]] << endl;
	return 0;
}
