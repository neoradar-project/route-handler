#pragma once
#ifndef TRIANGLECOLLIDE_H
#define TRIANGLECOLLIDE_H

#include <stdexcept>
#include <vector>

// From: https://gist.github.com/TimSC/5ba18ae21c4459275f90

using TriPoint = std::pair<double, double>;

struct Triangle {
    TriPoint one;
    TriPoint two;
    TriPoint three;
};

class TriangleCollide {
public:
    TriangleCollide();
    ~TriangleCollide();

    static inline double Det2D(TriPoint& p1, TriPoint& p2, TriPoint& p3);

    static void CheckTriWinding(TriPoint& p1, TriPoint& p2, TriPoint& p3, bool allowReversed);

    static bool BoundaryCollideChk(TriPoint& p1, TriPoint& p2, TriPoint& p3, double eps);

    static bool BoundaryDoesntCollideChk(TriPoint& p1, TriPoint& p2, TriPoint& p3, double eps);

    static bool TriTri2D(Triangle tr1,
        Triangle tr2,
        double eps = 0.0, bool allowReversed = false, bool onBoundary = true);

private:
};

#endif