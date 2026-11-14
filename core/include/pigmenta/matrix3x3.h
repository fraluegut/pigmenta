#pragma once
/**
 * Lightweight 3×3 matrix and 3-vector math for the Pigmenta image engine.
 * No external dependencies — designed for Android NDK.
 */

#include <cmath>
#include <algorithm>

namespace pigmenta {

// ── Vec3 ─────────────────────────────────────────────────────────────────────

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s)       const { return {x * s, y * s, z * s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    float dot(const Vec3& o)  const { return x * o.x + y * o.y + z * o.z; }
    float norm()              const { return std::sqrt(x*x + y*y + z*z); }
};

// ── Mat3x3 ───────────────────────────────────────────────────────────────────

struct Mat3x3 {
    float m[3][3];

    Mat3x3() { for (auto& row : m) row[0] = row[1] = row[2] = 0.0f; }

    static Mat3x3 identity() {
        Mat3x3 I;
        I.m[0][0] = I.m[1][1] = I.m[2][2] = 1.0f;
        return I;
    }

    float& at(int r, int c)       { return m[r][c]; }
    float  at(int r, int c) const { return m[r][c]; }

    Mat3x3 operator*(const Mat3x3& B) const {
        Mat3x3 C;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 3; ++k)
                    C.m[i][j] += m[i][k] * B.m[k][j];
        return C;
    }

    Vec3 operator*(const Vec3& v) const {
        return {
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z,
        };
    }

    Mat3x3 transposed() const {
        Mat3x3 T;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                T.m[i][j] = m[j][i];
        return T;
    }
};

// ── Jacobi eigendecomposition for 3×3 symmetric matrices ─────────────────────
// Returns eigenvalues in eigenvalues[3] and eigenvectors as columns of V.
// Both sorted by ascending eigenvalue.

inline void jacobi_eigen(const Mat3x3& A, Mat3x3& V, float eigenvalues[3]) {
    // Work copy — will become diagonal
    float a[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            a[i][j] = A.m[i][j];

    // Start with identity eigenvector matrix
    float v[3][3] = {{1,0,0},{0,1,0},{0,0,1}};

    for (int sweep = 0; sweep < 50; ++sweep) {
        // Find largest off-diagonal |a[p][q]|
        float max_off = 0.0f;
        int p = 0, q = 1;
        for (int i = 0; i < 3; ++i)
            for (int j = i + 1; j < 3; ++j)
                if (std::abs(a[i][j]) > max_off) {
                    max_off = std::abs(a[i][j]);
                    p = i; q = j;
                }

        if (max_off < 1e-10f) break;

        // Compute Givens rotation angle
        float theta = 0.5f * (a[q][q] - a[p][p]) / a[p][q];
        float t = (theta >= 0.0f ? 1.0f : -1.0f) /
                  (std::abs(theta) + std::sqrt(1.0f + theta * theta));
        float c = 1.0f / std::sqrt(1.0f + t * t);
        float s = t * c;

        // Apply rotation to a
        // Row/col p and q
        float app = a[p][p], aqq = a[q][q], apq = a[p][q];
        a[p][p] = c*c*app - 2*s*c*apq + s*s*aqq;
        a[q][q] = s*s*app + 2*s*c*apq + c*c*aqq;
        a[p][q] = a[q][p] = 0.0f;

        for (int r = 0; r < 3; ++r) {
            if (r == p || r == q) continue;
            float arp = a[r][p], arq = a[r][q];
            a[r][p] = a[p][r] = c*arp - s*arq;
            a[r][q] = a[q][r] = s*arp + c*arq;
        }

        // Accumulate rotation in v
        for (int r = 0; r < 3; ++r) {
            float vrp = v[r][p], vrq = v[r][q];
            v[r][p] = c*vrp - s*vrq;
            v[r][q] = s*vrp + c*vrq;
        }
    }

    // Extract eigenvalues
    float evals[3] = {a[0][0], a[1][1], a[2][2]};

    // Sort by ascending eigenvalue (bubble sort, only 3 elements)
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2 - i; ++j)
            if (evals[j] > evals[j+1]) {
                std::swap(evals[j], evals[j+1]);
                for (int r = 0; r < 3; ++r)
                    std::swap(v[r][j], v[r][j+1]);
            }

    for (int i = 0; i < 3; ++i) eigenvalues[i] = evals[i];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            V.m[i][j] = v[i][j];
}

} // namespace pigmenta
