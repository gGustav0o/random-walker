#pragma once

#include "MetaAnnotations.hpp"

#include <iostream>
#include <numeric>
#include <iomanip>
#include <cmath>

#include <Eigen/Dense>


/*
* Pure Обертка над чистой(почти) compile-time функцией.
* используется дл¤ гарантии:
*	отсутствия побочных эффектов(по смыслу),
*	[[nodiscard]],
*	noexcept(если переданна¤ функци¤ не бросает),
*	perfect forwarding.
*/
template <typename>
struct Pure;

// частична¤ специализация по сигнатуре функции: Ret(Args...)
template <typename Ret, typename... Args>
struct Pure<Ret(Args...)>
{
	using FnType = Ret(*)(Args...);
	FnType fn = nullptr;

	constexpr Pure() noexcept = default;
	constexpr Pure(FnType f) noexcept : fn(f) {}
	template <typename F, std::enable_if_t<
		std::is_convertible_v<F, FnType>, int> = 0>
	constexpr Pure(F f) noexcept : fn(static_cast<FnType>(f)) {}

	template <typename... CallArgs>
	[[nodiscard]] auto operator()(CallArgs&&... args) const
		noexcept(noexcept(fn(std::forward<CallArgs>(args)...)))
		-> decltype(fn(std::forward<CallArgs>(args)...))
	{
		return fn(std::forward<CallArgs>(args)...);
	}
};

#define NAME_OF(var) #var

#define STR_ENUM(EnumName, ...) \
    enum class EnumName { __VA_ARGS__, Count }; \
    static constexpr const char* EnumName##_names[] = { #__VA_ARGS__ }; \
    static constexpr const char* EnumName##ToString(EnumName value) noexcept { \
        const size_t index = static_cast<size_t>(value); \
        return (index < std::size(EnumName##_names)) ? EnumName##_names[index] : "Unknown"; \
    }

#define DECLARE_FACTORY(ClassName) \
    template<typename... Args>     \
    ClassName* create##ClassName(Args&&... args);

#define IMPLEMENT_FACTORY(ClassName) \
    template<typename... Args>       \
    ClassName* create##ClassName(Args&&... args) { return new ClassName(std::forward<Args>(args)...); }

constexpr auto PRECISION = 4;

template<typename T>
_pure_(std::string toString(const Eigen::MatrixBase<T>& obj, int precison = PRECISION))
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precison);
    if (obj.cols() == 1)
    {
        oss << obj.transpose();
    }
    else
    {
        oss << obj;
    }
    return oss.str();
}

template <typename T>
[[nodiscard]] static constexpr
typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value, std::string>::type
toString(const T& var, int precison = PRECISION)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precison);
    oss << var;
    return oss.str();
}

template <typename T>
_pure_(std::string toString(const std::vector<T>& vec, int precison = PRECISION))
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precison);
    oss << "[";

    for (std::size_t i = 0; i < vec.size(); ++i) {
        oss << toString(vec[i]);
        if (i + 1 < vec.size())
        {
            oss << ", ";
        }
    }

    oss << "]";
    return oss.str();
}

namespace eps
{
    _impure_(double nrmEps(const Eigen::MatrixXd& A))
    {
        return std::numeric_limits<double>::epsilon() * A.norm();
    }

    _impure_(double cndEps(const Eigen::MatrixXd& A))
    {
        return std::numeric_limits<double>::epsilon() * A.norm() * A.inverse().norm();
    }

    _impure_(double egnEps(const Eigen::MatrixXd& A))
    {
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(A);
        double minEigenvalue = solver.eigenvalues().minCoeff();
        return std::numeric_limits<double>::epsilon() * std::max(1.0, std::abs(minEigenvalue));
    }

    _pure_(double eps(double value))
    {
        return std::numeric_limits<double>::epsilon() * std::max(1.0, std::abs(value));
    }
}

namespace cmb
{
    _pure_(int binomialCoefficient(int n, int m))
    {

        if (m > n || m < 0) return 0;
        return static_cast<int>(
            std::round(std::tgamma(n + 1) / (std::tgamma(m + 1) * std::tgamma(n - m + 1)))
            );
    }

    _impure_(Eigen::MatrixXi combinations(int n, int m))
    {
        if (m > n || m <= 0)
        {
            return Eigen::MatrixXi(0, m);
        }

        int numComb = binomialCoefficient(n, m);

        Eigen::MatrixXi combMatrix(numComb, m);

        Eigen::VectorXi indices(m);
        std::iota(indices.data(), indices.data() + m, 0);

        combMatrix.row(0) = indices.transpose();

        for (size_t row = 1;; row++) {
            int i = m - 1;
            while (i >= 0 && indices(i) == n - m + i)
            {
                i--;
            }

            if (i < 0) break;

            indices(i)++;
            std::iota(indices.data() + i + 1, indices.data() + m, indices(i) + 1);

            combMatrix.row(row) = indices.transpose();
        }

        return combMatrix;
    }
}


/*
private:
    Q_PROPERTY(bool visible READ visible NOTIFY visible_changed)
    bool visible_;
    void set_visible(bool value) {
        if (visible_ == value) return;
        visible_ = value;
        emit visible_changed(visible_);
    }
public:
    bool visible() const { return visible_; }
signals:
    void visible_changed(bool);
public slots:
    void hide() { set_visible(false); }
    void show() { set_visible(true); }
    void on_request_set_visible(bool value) {
        if (visible_ == value) return;
        visible_ = value;
       emit visible_changed(visible_);
    }
private:
*/