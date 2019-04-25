//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//



#include "Frustum.h"
#include "Polyhedron.h"



namespace tc
{

void BoundingBox::Define(const Vector3* vertices, unsigned count)
{
    Clear();

    if (!count)
        return;

    Merge(vertices, count);
}

void BoundingBox::Define(const Frustum& frustum)
{
    Clear();
    Define(frustum.vertices_, NUM_FRUSTUM_VERTICES);
}

void BoundingBox::Define(const Polyhedron& poly)
{
    Clear();
    Merge(poly);
}

void BoundingBox::Define(const Sphere& sphere)
{
    const Vector3& center = sphere.Center;
    float radius = sphere.Radius;

    Min = center + Vector3(-radius, -radius, -radius);
    Max = center + Vector3(radius, radius, radius);
}

void BoundingBox::Merge(const Vector3* vertices, unsigned count)
{
    while (count--)
        Merge(*vertices++);
}

void BoundingBox::Merge(const Frustum& frustum)
{
    Merge(frustum.vertices_, NUM_FRUSTUM_VERTICES);
}

void BoundingBox::Merge(const Polyhedron& poly)
{
    for (unsigned i = 0; i < poly.faces_.size(); ++i)
    {
        const PODVector<Vector3>& face = poly.faces_[i];
        if (!face.empty())
            Merge(&face[0], face.size());
    }
}

void BoundingBox::Merge(const Sphere& sphere)
{
    const Vector3& center = sphere.Center;
    float radius = sphere.Radius;

    Merge(center + Vector3(radius, radius, radius));
    Merge(center + Vector3(-radius, -radius, -radius));
}

void BoundingBox::Clip(const BoundingBox& box)
{
    if (box.Min.x > Min.x)
        Min.x = box.Min.x;
    if (box.Max.x < Max.x)
        Max.x = box.Max.x;
    if (box.Min.y > Min.y)
        Min.y = box.Min.y;
    if (box.Max.y < Max.y)
        Max.y = box.Max.y;
    if (box.Min.z > Min.z)
        Min.z = box.Min.z;
    if (box.Max.z < Max.z)
        Max.z = box.Max.z;

    if (Min.x > Max.x || Min.y > Max.y || Min.z > Max.z)
    {
        Min = Vector3(M_INFINITY, M_INFINITY, M_INFINITY);
        Max = Vector3(-M_INFINITY, -M_INFINITY, -M_INFINITY);
    }
}

void BoundingBox::Transform(const Matrix3& transform)
{
    *this = Transformed(Matrix3x4(transform));
}

void BoundingBox::Transform(const Matrix3x4& transform)
{
    *this = Transformed(transform);
}

BoundingBox BoundingBox::Transformed(const Matrix3& transform) const
{
    return Transformed(Matrix3x4(transform));
}

BoundingBox BoundingBox::Transformed(const Matrix3x4& transform) const
{
#ifdef URHO3D_SSE
    const __m128 one = _mm_set_ss(1.f);
    __m128 minPt = _mm_movelh_ps(_mm_loadl_pi(_mm_setzero_ps(), (const __m64*)&Min.x), _mm_unpacklo_ps(_mm_set_ss(Min.z), one));
    __m128 maxPt = _mm_movelh_ps(_mm_loadl_pi(_mm_setzero_ps(), (const __m64*)&Max.x), _mm_unpacklo_ps(_mm_set_ss(Max.z), one));
    __m128 centerPoint = _mm_mul_ps(_mm_add_ps(minPt, maxPt), _mm_set1_ps(0.5f));
    __m128 halfSize = _mm_sub_ps(centerPoint, minPt);
    __m128 m0 = _mm_loadu_ps(&transform.m00_);
    __m128 m1 = _mm_loadu_ps(&transform.m10_);
    __m128 m2 = _mm_loadu_ps(&transform.m20_);
    __m128 r0 = _mm_mul_ps(m0, centerPoint);
    __m128 r1 = _mm_mul_ps(m1, centerPoint);
    __m128 t0 = _mm_add_ps(_mm_unpacklo_ps(r0, r1), _mm_unpackhi_ps(r0, r1));
    __m128 r2 = _mm_mul_ps(m2, centerPoint);
    const __m128 zero = _mm_setzero_ps();
    __m128 t2 = _mm_add_ps(_mm_unpacklo_ps(r2, zero), _mm_unpackhi_ps(r2, zero));
    __m128 newCenter = _mm_add_ps(_mm_movelh_ps(t0, t2), _mm_movehl_ps(t2, t0));
    const __m128 absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
    __m128 x = _mm_and_ps(absMask, _mm_mul_ps(m0, halfSize));
    __m128 y = _mm_and_ps(absMask, _mm_mul_ps(m1, halfSize));
    __m128 z = _mm_and_ps(absMask, _mm_mul_ps(m2, halfSize));
    t0 = _mm_add_ps(_mm_unpacklo_ps(x, y), _mm_unpackhi_ps(x, y));
    t2 = _mm_add_ps(_mm_unpacklo_ps(z, zero), _mm_unpackhi_ps(z, zero));
    __m128 newDir = _mm_add_ps(_mm_movelh_ps(t0, t2), _mm_movehl_ps(t2, t0));
    return BoundingBox(_mm_sub_ps(newCenter, newDir), _mm_add_ps(newCenter, newDir));
#else
    Vector3 newCenter = transform * Center();
    Vector3 oldEdge = Size() * 0.5f;
    Vector3 newEdge = Vector3(
        Abs(transform.m00_) * oldEdge.x + Abs(transform.m01_) * oldEdge.y + Abs(transform.m02_) * oldEdge.z,
        Abs(transform.m10_) * oldEdge.x + Abs(transform.m11_) * oldEdge.y + Abs(transform.m12_) * oldEdge.z,
        Abs(transform.m20_) * oldEdge.x + Abs(transform.m21_) * oldEdge.y + Abs(transform.m22_) * oldEdge.z
    );

    return BoundingBox(newCenter - newEdge, newCenter + newEdge);
#endif
}

Rect BoundingBox::Projected(const Matrix4& projection) const
{
    Vector3 projMin = Min;
    Vector3 projMax = Max;
    if (projMin.z < M_MIN_NEARCLIP)
        projMin.z = M_MIN_NEARCLIP;
    if (projMax.z < M_MIN_NEARCLIP)
        projMax.z = M_MIN_NEARCLIP;

    Vector3 vertices[8];
    vertices[0] = projMin;
    vertices[1] = Vector3(projMax.x, projMin.y, projMin.z);
    vertices[2] = Vector3(projMin.x, projMax.y, projMin.z);
    vertices[3] = Vector3(projMax.x, projMax.y, projMin.z);
    vertices[4] = Vector3(projMin.x, projMin.y, projMax.z);
    vertices[5] = Vector3(projMax.x, projMin.y, projMax.z);
    vertices[6] = Vector3(projMin.x, projMax.y, projMax.z);
    vertices[7] = projMax;

    Rect rect;
    for (const auto& vertice : vertices)
    {
        Vector3 projected = projection * vertice;
        rect.Merge(Vector2(projected.x, projected.y));
    }

    return rect;
}

float BoundingBox::DistanceToPoint(const Vector3& point) const
{
    const Vector3 offset = Center() - point;
    const Vector3 absOffset(Abs(offset.x), Abs(offset.y), Abs(offset.z));
    return VectorMax(Vector3::ZERO, absOffset - HalfSize()).Length();
}

Intersection BoundingBox::IsInside(const Sphere& sphere) const
{
    float distSquared = 0;
    float temp;
    const Vector3& center = sphere.Center;

    if (center.x < Min.x)
    {
        temp = center.x - Min.x;
        distSquared += temp * temp;
    }
    else if (center.x > Max.x)
    {
        temp = center.x - Max.x;
        distSquared += temp * temp;
    }
    if (center.y < Min.y)
    {
        temp = center.y - Min.y;
        distSquared += temp * temp;
    }
    else if (center.y > Max.y)
    {
        temp = center.y - Max.y;
        distSquared += temp * temp;
    }
    if (center.z < Min.z)
    {
        temp = center.z - Min.z;
        distSquared += temp * temp;
    }
    else if (center.z > Max.z)
    {
        temp = center.z - Max.z;
        distSquared += temp * temp;
    }

    float radius = sphere.Radius;
    if (distSquared >= radius * radius)
        return OUTSIDE;
    else if (center.x - radius < Min.x || center.x + radius > Max.x || center.y - radius < Min.y ||
             center.y + radius > Max.y || center.z - radius < Min.z || center.z + radius > Max.z)
        return INTERSECTS;
    else
        return INSIDE;
}

Intersection BoundingBox::IsInsideFast(const Sphere& sphere) const
{
    float distSquared = 0;
    float temp;
    const Vector3& center = sphere.Center;

    if (center.x < Min.x)
    {
        temp = center.x - Min.x;
        distSquared += temp * temp;
    }
    else if (center.x > Max.x)
    {
        temp = center.x - Max.x;
        distSquared += temp * temp;
    }
    if (center.y < Min.y)
    {
        temp = center.y - Min.y;
        distSquared += temp * temp;
    }
    else if (center.y > Max.y)
    {
        temp = center.y - Max.y;
        distSquared += temp * temp;
    }
    if (center.z < Min.z)
    {
        temp = center.z - Min.z;
        distSquared += temp * temp;
    }
    else if (center.z > Max.z)
    {
        temp = center.z - Max.z;
        distSquared += temp * temp;
    }

    float radius = sphere.Radius;
    if (distSquared >= radius * radius)
        return OUTSIDE;
    else
        return INSIDE;
}

std::string BoundingBox::ToString() const
{
    return Min.ToString() + " - " + Max.ToString();
}

}
