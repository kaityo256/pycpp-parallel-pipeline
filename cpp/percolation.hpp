#include <cstdio>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace percolation {

int pos2index(int ix, int iy, const int L) {
  return ix + iy * L;
}

int find(int index, std::vector<int> &cluster) {
  if (cluster[index] != index) {
    cluster[index] = find(cluster[index], cluster);
  }
  return cluster[index];
}

void unite(int i, int j, std::vector<int> &cluster) {
  i = find(i, cluster);
  j = find(j, cluster);

  if (i == j) {
    return;
  }

  if (i > j) {
    cluster[i] = j;
  } else {
    cluster[j] = i;
  }
}

bool check_clustering(int L, std::vector<int> &cluster) {
  std::unordered_set<int> bottom_roots;

  for (int ix = 0; ix < L; ++ix) {
    const int i = pos2index(ix, 0, L);
    bottom_roots.insert(find(i, cluster));
  }

  for (int ix = 0; ix < L; ++ix) {
    const int i = pos2index(ix, L - 1, L);
    if (bottom_roots.find(find(i, cluster)) != bottom_roots.end()) {
      return true;
    }
  }

  return false;
}

bool does_percolate(
    int L,
    double bond_probability,
    std::mt19937_64 &rng) {
  const int N = L * L;
  std::vector<int> cluster(N);
  std::iota(cluster.begin(), cluster.end(), 0);
  std::bernoulli_distribution is_open(bond_probability);

  for (int ix = 0; ix < L; ++ix) {
    for (int iy = 0; iy < L; ++iy) {
      const int i = pos2index(ix, iy, L);

      if (ix + 1 < L && is_open(rng)) {
        unite(i, pos2index(ix + 1, iy, L), cluster);
      }

      if (iy + 1 < L && is_open(rng)) {
        unite(i, pos2index(ix, iy + 1, L), cluster);
      }
    }
  }
  return check_clustering(L, cluster);
}

double estimate_percolation_probability(
    int L,
    double bond_probability,
    int seed,
    int num_samples) {

  std::mt19937_64 rng(seed);
  int num_percolated = 0;

  for (int sample = 0; sample < num_samples; ++sample) {
    if (does_percolate(L, bond_probability, rng)) {
      ++num_percolated;
    }
  }

  return static_cast<double>(num_percolated) / num_samples;
}

void test(int L) {
  const double bond_probability_start = 0.45;
  const double bond_probability_end = 0.55;
  const int num_probability_points = 20;
  const int seed = 0;
  const int num_samples = 100;

  for (int i = 0; i < num_probability_points; ++i) {
    const double bond_probability =
        bond_probability_start + (bond_probability_end - bond_probability_start) * static_cast<double>(i) / static_cast<double>(num_probability_points - 1);

    const double percolation_probability =
        estimate_percolation_probability(
            L,
            bond_probability,
            seed,
            num_samples);

    std::printf(
        "%.8f %.8f\n",
        bond_probability,
        percolation_probability);
  }
}

} // namespace percolation