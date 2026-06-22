#include "adaptivesampling.h"

#include "Lib/common.h"

#include <iostream>
#include <vector>
#include <random>
#include <cmath>

namespace
{

void computeOrthonormalBasis(const vector3& n, vector3& t1, vector3& t2)
{
	vector3 aux;
	if (std::fabs(n.x) < 0.9)
	{
		aux = vector3(1.0, 0.0, 0.0);
	}
	else
	{
		aux = vector3(0.0, 1.0, 0.0);
	}

	t1 = Common::normalize(Common::cross(n, aux));
	t2 = Common::normalize(Common::cross(n, t1));
}

std::mt19937 rng(12345); // fixed seed
std::uniform_real_distribution<double> unif01(0.0,1.0);

// ---------- SAMPLE ON SPHERE ----------
vector3 sample_sphere()
{
	double theta = 2*M_PI*unif01(rng);
	double phi   = std::acos(2*unif01(rng) - 1);
	double x = std::cos(theta)*std::sin(phi);
	double y = std::sin(theta)*std::sin(phi);
	double z = std::cos(phi);
	return vector3(x,y,z);
}

std::vector<vector3> sample_sphere(int N)
{
	std::vector<vector3> X;
	X.reserve(N);
	for(int i=0;i<N;i++) X.push_back(sample_sphere());
	return X;
}

double evaluate_function_f(const vector3 &p)
{
	return p.x*p.x + p.y*p.y + p.z*p.z;
}

double evaluate_function_h(const vector3 &p)
{
	return std::fabs(p.x) + std::fabs(p.y) + std::fabs(p.z);
}

void evaluate_functions(const std::vector<vector3> &X, std::vector<double> &f_values,
		std::vector<double> &h_values)
{
	int N = X.size();
	f_values.resize(N);
	h_values.resize(N);
	for(int i=0;i<N;i++)
	{
		f_values[i] = evaluate_function_f(X[i]);
		h_values[i] = evaluate_function_h(X[i]);
	}
}

struct RBFModel
{
	std::vector<vector3> centers;
	std::vector<double> weights;
};

inline double rbf(double r) { return std::exp(-r*r); }

static std::vector<double> choleskySolve(std::vector<std::vector<double>> A, const std::vector<double>& b)
{
	const int n = (int)A.size();
	// Decomposition A = L L^T (store L for clarity)
	std::vector<std::vector<double>> L(n, std::vector<double>(n, 0.0));

	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j <= i; ++j)
		{
			double sum = A[i][j];
			for (int k = 0; k < j; ++k) sum -= L[i][k] * L[j][k];

			if (i == j)
			{
				// Asegurar positividad numérica
				if (sum <= 0.0) sum = std::max(sum, 1e-15);
				L[i][j] = std::sqrt(sum);
			}
			else
			{
				L[i][j] = sum / L[j][j];
			}
		}
	}

	// Forward: L y = b
	std::vector<double> y(n, 0.0);
	for (int i = 0; i < n; ++i)
	{
		double sum = b[i];
		for (int k = 0; k < i; ++k) sum -= L[i][k] * y[k];
		y[i] = sum / L[i][i];
	}

	// Backward: L^T x = y
	std::vector<double> x(n, 0.0);
	for (int i = n - 1; i >= 0; --i)
	{
		double sum = y[i];
		for (int k = i + 1; k < n; ++k) sum -= L[k][i] * x[k];
		x[i] = sum / L[i][i];
	}
	return x;
}

RBFModel fitRbf(const std::vector<vector3> &X, const std::vector<double> &h_values)
{
	int N = X.size();
	std::vector<std::vector<double>> Phi(N, std::vector<double>(N));
	// Distances matrix
	for(int i=0;i<N;i++)
	{
		for(int j=0;j<N;j++)
		{
			double d = Common::length(vector3(X[i].x - X[j].x,X[i].y - X[j].y,X[i].z - X[j].z));
			Phi[i][j] = rbf(d);
		}
		Phi[i][i] += 1e-6; // regularization
	}
	// Resolve linear system Phi * w = h (naive Gauss-Seidel)
	std::vector<double> w = choleskySolve(Phi, h_values);

	RBFModel model;
	model.centers = X;
	model.weights = w;
	return model;
}

double predict(const RBFModel &model, const vector3 &x)
{
	double res = 0.0;
	int N = model.centers.size();
	for(int i=0;i<N;i++)
	{
		double d = Common::length(vector3(x.x - model.centers[i].x, x.y - model.centers[i].y, x.z - model.centers[i].z));
		res += model.weights[i] * rbf(d);
	}
	return res;
}

std::vector<vector3> resample_points_adaptive(
	int N_generate,
	const std::vector<double> &h_values_old,
	const RBFModel &q_model,
	const std::vector<vector3> &X_old,
	double minDistance)
{
	std::vector<vector3> X_new;
	X_new.reserve(N_generate);

	int attempts = 0, max_attempts = 100*N_generate;
	double max_h = *std::max_element(h_values_old.begin(), h_values_old.end()) + 1e-2;
	double min_h = *std::min_element(h_values_old.begin(), h_values_old.end());

	while((int)X_new.size() < N_generate && attempts < max_attempts)
	{
		vector3 cand = sample_sphere();
		double h_cand = predict(q_model, cand) + 1e-2;
		double q_cand = (h_cand - min_h) / (max_h - min_h + 1e-12);
		double u = unif01(rng);

		if(q_cand > 0 && u < q_cand)
		{
			bool too_close = false;
			for(auto &p: X_new)
			{
				if(Common::length(vector3(p.x-cand.x,p.y-cand.y,p.z-cand.z)) < minDistance)
				{
					too_close = true; break;
				}
			}
			if(!too_close)
			{
				X_new.push_back(cand);
			}
		}
		attempts++;
	}
	if((int)X_new.size() < N_generate)
	{
		std::cerr << "Warning: could not fill all points, got " << X_new.size() << "\n";
	}
	return X_new;
}

double importance_sampling_estimate(const std::vector<vector3> &X,
									const std::vector<double> &f_values,
									const std::vector<double> &q_values)
{
	int N = X.size();
	double sum_wf=0, sum_w=0;
	for(int i=0;i<N;i++)
	{
		double p = 1.0/(4*M_PI);
		double q = std::max(q_values[i],1e-10);
		double w = p/q;
		sum_wf += f_values[i]*w;
		sum_w  += w;
	}
	return sum_wf/sum_w;
}

// ---------- MAIN ITERATIVE LOOP ----------
void iterative_montecarlo(int N0, int num_iterations, double epsilon,
						  int neighbors, int skipIter, double minDistance,
						  std::vector<vector3> &X_out,
						  std::vector<double> &f_out)
{
	std::vector<vector3> X = sample_sphere(N0);
	std::vector<double> f_values, h_values;
	evaluate_functions(X, f_values, h_values);

	std::vector<double> q_values = h_values;
	double sum_h = std::accumulate(q_values.begin(), q_values.end(),0.0);
	for(double &v: q_values) v /= sum_h;

	double mu_prev = std::numeric_limits<double>::infinity();

	for(int iter=0; iter<num_iterations; ++iter)
	{
		RBFModel model = fitRbf(X, q_values);

		std::vector<vector3> X_new = resample_points_adaptive(N0, h_values, model, X, minDistance);

		std::vector<double> f_new, h_new;
		evaluate_functions(X_new, f_new, h_new);
		double min_hv = *std::min_element(h_new.begin(), h_new.end());
		for(double &v: h_new) v = (v-min_hv)+1e-6;
		double sum_hn = std::accumulate(h_new.begin(), h_new.end(), 0.0);
		for(double &v: h_new) v /= sum_hn;

		double mu_hat = importance_sampling_estimate(X_new, f_new, h_new);

		if(iter > neighbors)
		{
			double diff = std::fabs(mu_hat - mu_prev);
			if(diff < epsilon)
			{
				std::cout << "Convergencia en iter " << iter << "\n";
				X_out = X_new;
				f_out = f_new;
				return;
			}
		}
		mu_prev = mu_hat;
		X = X_new; f_values = f_new; h_values = h_new; q_values = h_new;
	}
	X_out = X; f_out = f_values;
}

}

AdaptiveSampling::AdaptiveSampling()
{
}

void AdaptiveSampling::computeSRP(Grid *results)
{

}
