#include "erkir/geo/TriangleCollide.h"

TriangleCollide::TriangleCollide() = default;
/**
 * TriangleCollide
 *
 * @param  {TriPoint} p1 :
 * @param  {TriPoint} p2 :
 * @param  {TriPoint} p3 :
 * @return {double}      :
 */
inline double TriangleCollide::Det2D(TriPoint& p1, TriPoint& p2, TriPoint& p3)
{
    return +p1.first * (p2.second - p3.second) + p2.first * (p3.second - p1.second)
        + p3.first * (p1.second - p2.second);
}

/**
 * TriangleCollide
 *
 * @param  {TriPoint} p1        :
 * @param  {TriPoint} p2        :
 * @param  {TriPoint} p3        :
 * @param  {bool} allowReversed :
 */
void TriangleCollide::CheckTriWinding(
    TriPoint& p1, TriPoint& p2, TriPoint& p3, bool allowReversed)
{
    double det_tri = Det2D(p1, p2, p3);
    if (det_tri < 0.0) {
        if (allowReversed) {
            TriPoint a = p3;
            p3 = p2;
            p2 = a;
        } else
            throw std::runtime_error("triangle has wrong winding direction");
    }
}

/**
 * TriangleCollide
 *
 * @param  {TriPoint} p1 :
 * @param  {TriPoint} p2 :
 * @param  {TriPoint} p3 :
 * @param  {double} eps  :
 * @return {bool}        :
 */
bool TriangleCollide::BoundaryCollideChk(
    TriPoint& p1, TriPoint& p2, TriPoint& p3, double eps)
{
    return Det2D(p1, p2, p3) < eps;
}

/**
 * TriangleCollide
 *
 * @param  {TriPoint} p1 :
 * @param  {TriPoint} p2 :
 * @param  {TriPoint} p3 :
 * @param  {double} eps  :
 * @return {bool}        :
 */
bool TriangleCollide::BoundaryDoesntCollideChk(
    TriPoint& p1, TriPoint& p2, TriPoint& p3, double eps)
{
    return Det2D(p1, p2, p3) <= eps;
}

/**
 * TriangleCollide
 * Checks for a collision between two triangles
 * @param  {bool} TriangleCollide::TriTri2D( :
 * @return {bool}                            :
 */
bool TriangleCollide::TriTri2D(
    Triangle tr1, Triangle tr2, double eps, bool allowReversed, bool onBoundary)
{
    TriPoint t1[3] = { { tr1.one }, { tr1.two }, tr1.three };
    TriPoint t2[3] = { { tr2.one }, { tr2.two }, tr2.three };

    // Trangles must be expressed anti-clockwise
    CheckTriWinding(t1[0], t1[1], t1[2], allowReversed);
    CheckTriWinding(t2[0], t2[1], t2[2], allowReversed);

    bool (*chk_edge)(TriPoint&, TriPoint&, TriPoint&, double) = nullptr;
    if (onBoundary) // Points on the boundary are considered as colliding
        chk_edge = BoundaryCollideChk;
    else // Points on the boundary are not considered as colliding
        chk_edge = BoundaryDoesntCollideChk;

    // For edge E of trangle 1,
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;

        // Check all points of trangle 2 lay on the external side of the edge E. If
        // they do, the triangles do not collide.
        if (chk_edge(t1[i], t1[j], t2[0], eps) && chk_edge(t1[i], t1[j], t2[1], eps)
            && chk_edge(t1[i], t1[j], t2[2], eps))
            return false;
    }

    // For edge E of trangle 2,
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;

        // Check all points of trangle 1 lay on the external side of the edge E. If
        // they do, the triangles do not collide.
        if (chk_edge(t2[i], t2[j], t1[0], eps) && chk_edge(t2[i], t2[j], t1[1], eps)
            && chk_edge(t2[i], t2[j], t1[2], eps))
            return false;
    }

    // The triangles collide
    return true;
}
