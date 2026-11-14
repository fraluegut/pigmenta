#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "pigmenta/matrix3x3.h"

using namespace pigmenta;
using Catch::Approx;

TEST_CASE("Vec3 arithmetic", "[matrix3x3]") {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);

    SECTION("addition") {
        Vec3 c = a + b;
        REQUIRE(c.x == Approx(5.0f));
        REQUIRE(c.y == Approx(7.0f));
        REQUIRE(c.z == Approx(9.0f));
    }
    SECTION("subtraction") {
        Vec3 c = b - a;
        REQUIRE(c.x == Approx(3.0f));
        REQUIRE(c.y == Approx(3.0f));
        REQUIRE(c.z == Approx(3.0f));
    }
    SECTION("scalar multiply") {
        Vec3 c = a * 2.0f;
        REQUIRE(c.x == Approx(2.0f));
        REQUIRE(c.y == Approx(4.0f));
        REQUIRE(c.z == Approx(6.0f));
    }
    SECTION("+= accumulates in place") {
        Vec3 c = a;
        c += b;
        REQUIRE(c.x == Approx(5.0f));
        REQUIRE(c.y == Approx(7.0f));
        REQUIRE(c.z == Approx(9.0f));
    }
    SECTION("dot product") {
        REQUIRE(a.dot(b) == Approx(1 * 4 + 2 * 5 + 3 * 6));
    }
    SECTION("norm") {
        Vec3 v(3.0f, 4.0f, 0.0f);
        REQUIRE(v.norm() == Approx(5.0f));
    }
    SECTION("default-constructed is zero") {
        Vec3 z;
        REQUIRE(z.x == Approx(0.0f));
        REQUIRE(z.y == Approx(0.0f));
        REQUIRE(z.z == Approx(0.0f));
    }
}

TEST_CASE("Mat3x3 identity and multiplication", "[matrix3x3]") {
    Mat3x3 I = Mat3x3::identity();
    REQUIRE(I.at(0, 0) == Approx(1.0f));
    REQUIRE(I.at(1, 1) == Approx(1.0f));
    REQUIRE(I.at(2, 2) == Approx(1.0f));
    REQUIRE(I.at(0, 1) == Approx(0.0f));

    Vec3 v(1.0f, 2.0f, 3.0f);
    Vec3 iv = I * v;
    REQUIRE(iv.x == Approx(v.x));
    REQUIRE(iv.y == Approx(v.y));
    REQUIRE(iv.z == Approx(v.z));

    Mat3x3 II = I * I;
    REQUIRE(II.at(0, 0) == Approx(1.0f));
    REQUIRE(II.at(1, 2) == Approx(0.0f));
}

TEST_CASE("Mat3x3 transpose", "[matrix3x3]") {
    Mat3x3 M;
    M.at(0, 1) = 5.0f;
    M.at(1, 0) = 0.0f;
    Mat3x3 T = M.transposed();
    REQUIRE(T.at(1, 0) == Approx(5.0f));
    REQUIRE(T.at(0, 1) == Approx(0.0f));
}

TEST_CASE("Mat3x3::at mutates in place", "[matrix3x3]") {
    Mat3x3 M;
    M.at(2, 1) = 7.5f;
    REQUIRE(M.at(2, 1) == Approx(7.5f));
}

TEST_CASE("jacobi_eigen on a diagonal matrix returns the diagonal, sorted", "[matrix3x3]") {
    Mat3x3 A;
    A.m[0][0] = 5.0f;
    A.m[1][1] = 1.0f;
    A.m[2][2] = 3.0f;

    Mat3x3 V;
    float eigenvalues[3];
    jacobi_eigen(A, V, eigenvalues);

    REQUIRE(eigenvalues[0] == Approx(1.0f));
    REQUIRE(eigenvalues[1] == Approx(3.0f));
    REQUIRE(eigenvalues[2] == Approx(5.0f));
}

TEST_CASE("jacobi_eigen reconstructs A*v = lambda*v for a non-diagonal symmetric matrix", "[matrix3x3]") {
    // A real symmetric covariance-like matrix with non-zero off-diagonals,
    // forcing the Jacobi rotation sweep (not just the early-exit branch).
    Mat3x3 A;
    A.m[0][0] = 4.0f;
    A.m[0][1] = 2.0f;
    A.m[0][2] = 1.0f;
    A.m[1][0] = 2.0f;
    A.m[1][1] = 3.0f;
    A.m[1][2] = 0.5f;
    A.m[2][0] = 1.0f;
    A.m[2][1] = 0.5f;
    A.m[2][2] = 2.0f;

    Mat3x3 V;
    float eigenvalues[3];
    jacobi_eigen(A, V, eigenvalues);

    // Eigenvalues ascending.
    REQUIRE(eigenvalues[0] <= eigenvalues[1]);
    REQUIRE(eigenvalues[1] <= eigenvalues[2]);

    // For each eigenvector column j: A * v_j ≈ lambda_j * v_j.
    for (int j = 0; j < 3; ++j) {
        Vec3 v(V.m[0][j], V.m[1][j], V.m[2][j]);
        Vec3 Av = A * v;
        Vec3 lv = v * eigenvalues[j];
        REQUIRE(Av.x == Approx(lv.x).margin(1e-3));
        REQUIRE(Av.y == Approx(lv.y).margin(1e-3));
        REQUIRE(Av.z == Approx(lv.z).margin(1e-3));
    }
}

TEST_CASE("jacobi_eigen on a matrix needing the opposite rotation sign", "[matrix3x3]") {
    // Same shape as above but with a negated off-diagonal, exercising the
    // other branch of the Givens rotation sign selection.
    Mat3x3 A;
    A.m[0][0] = 1.0f;
    A.m[0][1] = -3.0f;
    A.m[0][2] = 0.0f;
    A.m[1][0] = -3.0f;
    A.m[1][1] = 1.0f;
    A.m[1][2] = 0.0f;
    A.m[2][0] = 0.0f;
    A.m[2][1] = 0.0f;
    A.m[2][2] = 9.0f;

    Mat3x3 V;
    float eigenvalues[3];
    jacobi_eigen(A, V, eigenvalues);

    // Known analytically: eigenvalues of [[1,-3],[-3,1]] are -2 and 4.
    REQUIRE(eigenvalues[0] == Approx(-2.0f).margin(1e-3));
    REQUIRE(eigenvalues[1] == Approx(4.0f).margin(1e-3));
    REQUIRE(eigenvalues[2] == Approx(9.0f).margin(1e-3));
}
