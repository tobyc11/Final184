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

#pragma once

#include "Rect.h"
#include "Vector3.h"

#ifdef URHO3D_SSE
#include <xmmintrin.h>
#endif

namespace tc
{

class Polyhedron;
class Frustum;
class Matrix3;
class Matrix4;
class Matrix3x4;
class Sphere;

/// Three-dimensional axis-aligned bounding box.
class MATH_EXPORT BoundingBox
{
public:
    /// Construct with zero size.
    BoundingBox() noexcept :
        Min(M_INFINITY, M_INFINITY, M_INFINITY),
        Max(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
    }

    /// Copy-construct from another bounding box.
    BoundingBox(const BoundingBox& box) noexcept :
        Min(box.Min),
        Max(box.Max)
    {
    }

    /// Construct from a rect, with the Z dimension left zero.
    explicit BoundingBox(const Rect& rect) noexcept :
        Min(Vector3(rect.min_, 0.0f)),
        Max(Vector3(rect.max_, 0.0f))
    {
    }

    /// Construct from minimum and maximum vectors.
    BoundingBox(const Vector3& min, const Vector3& max) noexcept :
        Min(min),
        Max(max)
    {
    }

    /// Construct from minimum and maximum floats (all dimensions same.)
    BoundingBox(float min, float max) noexcept :
        Min(Vector3(min, min, min)),
        Max(Vector3(max, max, max))
    {
    }

#ifdef URHO3D_SSE
    BoundingBox(__m128 min, __m128 max) noexcept
    {
        _mm_storeu_ps(&Min.x, min);
        _mm_storeu_ps(&Max.x, max);
    }
#endif

    /// Construct from an array of vertices.
    BoundingBox(const Vector3* vertices, unsigned count) :
        Min(M_INFINITY, M_INFINITY, M_INFINITY),
        Max(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
        Define(vertices, count);
    }

    /// Construct from a frustum.
    explicit BoundingBox(const Frustum& frustum) :
        Min(M_INFINITY, M_INFINITY, M_INFINITY),
        Max(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
        Define(frustum);
    }

    /// Construct from a polyhedron.
    explicit BoundingBox(const Polyhedron& poly) :
        Min(M_INFINITY, M_INFINITY, M_INFINITY),
        Max(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
        Define(poly);
    }

    /// Construct from a sphere.
    explicit BoundingBox(const Sphere& sphere) :
        Min(M_INFINITY, M_INFINITY, M_INFINITY),
        Max(-M_INFINITY, -M_INFINITY, -M_INFINITY)
    {
        Define(sphere);
    }

    /// Assign from another bounding box.
    BoundingBox& operator =(const BoundingBox& rhs) noexcept
    {
        Min = rhs.Min;
        Max = rhs.Max;
        return *this;
    }

    /// Assign from a Rect, with the Z dimension left zero.
    BoundingBox& operator =(const Rect& rhs) noexcept
    {
        Min = Vector3(rhs.min_, 0.0f);
        Max = Vector3(rhs.max_, 0.0f);
        return *this;
    }

    /// Test for equality with another bounding box.
    bool operator ==(const BoundingBox& rhs) const { return (Min == rhs.Min && Max == rhs.Max); }

    /// Test for inequality with another bounding box.
    bool operator !=(const BoundingBox& rhs) const { return (Min != rhs.Min || Max != rhs.Max); }

    /// Define from another bounding box.
    void Define(const BoundingBox& box)
    {
        Define(box.Min, box.Max);
    }

    /// Define from a Rect.
    void Define(const Rect& rect)
    {
        Define(Vector3(rect.min_, 0.0f), Vector3(rect.max_, 0.0f));
    }

    /// Define from minimum and maximum vectors.
    void Define(const Vector3& min, const Vector3& max)
    {
        Min = min;
        Max = max;
    }

    /// Define from minimum and maximum floats (all dimensions same.)
    void Define(float min, float max)
    {
        Min = Vector3(min, min, min);
        Max = Vector3(max, max, max);
    }

    /// Define from a point.
    void Define(const Vector3& point)
    {
        Min = Max = point;
    }

    /// Merge a point.
    void Merge(const Vector3& point)
    {
#ifdef URHO3D_SSE
        __m128 vec = _mm_set_ps(1.f, point.z, point.y, point.x);
        _mm_storeu_ps(&Min.x, _mm_min_ps(_mm_loadu_ps(&Min.x), vec));
        _mm_storeu_ps(&Max.x, _mm_max_ps(_mm_loadu_ps(&Max.x), vec));
#else
        if (point.x < Min.x)
            Min.x = point.x;
        if (point.y < Min.y)
            Min.y = point.y;
        if (point.z < Min.z)
            Min.z = point.z;
        if (point.x > Max.x)
            Max.x = point.x;
        if (point.y > Max.y)
            Max.y = point.y;
        if (point.z > Max.z)
            Max.z = point.z;
#endif
    }

    /// Merge another bounding box.
    void Merge(const BoundingBox& box)
    {
#ifdef URHO3D_SSE
        _mm_storeu_ps(&Min.x, _mm_min_ps(_mm_loadu_ps(&Min.x), _mm_loadu_ps(&box.Min.x)));
        _mm_storeu_ps(&Max.x, _mm_max_ps(_mm_loadu_ps(&Max.x), _mm_loadu_ps(&box.Max.x)));
#else
        if (box.Min.x < Min.x)
            Min.x = box.Min.x;
        if (box.Min.y < Min.y)
            Min.y = box.Min.y;
        if (box.Min.z < Min.z)
            Min.z = box.Min.z;
        if (box.Max.x > Max.x)
            Max.x = box.Max.x;
        if (box.Max.y > Max.y)
            Max.y = box.Max.y;
        if (box.Max.z > Max.z)
            Max.z = box.Max.z;
#endif
    }

    /// Define from an array of vertices.
    void Define(const Vector3* vertices, unsigned count);
    /// Define from a frustum.
    void Define(const Frustum& frustum);
    /// Define from a polyhedron.
    void Define(const Polyhedron& poly);
    /// Define from a sphere.
    void Define(const Sphere& sphere);
    /// Merge an array of vertices.
    void Merge(const Vector3* vertices, unsigned count);
    /// Merge a frustum.
    void Merge(const Frustum& frustum);
    /// Merge a polyhedron.
    void Merge(const Polyhedron& poly);
    /// Merge a sphere.
    void Merge(const Sphere& sphere);
    /// Clip with another bounding box. The box can become degenerate (undefined) as a result.
    void Clip(const BoundingBox& box);
    /// Transform with a 3x3 matrix.
    void Transform(const Matrix3& transform);
    /// Transform with a 3x4 matrix.
    void Transform(const Matrix3x4& transform);

    /// Clear to undefined state.
    void Clear()
    {
#ifdef URHO3D_SSE
        _mm_storeu_ps(&Min.x, _mm_set1_ps(M_INFINITY));
        _mm_storeu_ps(&Max.x, _mm_set1_ps(-M_INFINITY));
#else
        Min = Vector3(M_INFINITY, M_INFINITY, M_INFINITY);
        Max = Vector3(-M_INFINITY, -M_INFINITY, -M_INFINITY);
#endif
    }

    /// Return true if this bounding box is defined via a previous call to Define() or Merge().
    bool Defined() const
    {
        return Min.x != M_INFINITY;
    }

    /// Return center.
    Vector3 Center() const { return (Max + Min) * 0.5f; }

    /// Return size.
    Vector3 Size() const { return Max - Min; }

    /// Return half-size.
    Vector3 HalfSize() const { return (Max - Min) * 0.5f; }

    /// Return transformed by a 3x3 matrix.
    BoundingBox Transformed(const Matrix3& transform) const;
    /// Return transformed by a 3x4 matrix.
    BoundingBox Transformed(const Matrix3x4& transform) const;
    /// Return projected by a 4x4 projection matrix.
    Rect Projected(const Matrix4& projection) const;
    /// Return distance to point.
    float DistanceToPoint(const Vector3& point) const;

    /// Test if a point is inside.
    Intersection IsInside(const Vector3& point) const
    {
        if (point.x < Min.x || point.x > Max.x || point.y < Min.y || point.y > Max.y ||
            point.z < Min.z || point.z > Max.z)
            return OUTSIDE;
        else
            return INSIDE;
    }

    /// Test if another bounding box is inside, outside or intersects.
    Intersection IsInside(const BoundingBox& box) const
    {
        if (box.Max.x < Min.x || box.Min.x > Max.x || box.Max.y < Min.y || box.Min.y > Max.y ||
            box.Max.z < Min.z || box.Min.z > Max.z)
            return OUTSIDE;
        else if (box.Min.x < Min.x || box.Max.x > Max.x || box.Min.y < Min.y || box.Max.y > Max.y ||
                 box.Min.z < Min.z || box.Max.z > Max.z)
            return INTERSECTS;
        else
            return INSIDE;
    }

    /// Test if another bounding box is (partially) inside or outside.
    Intersection IsInsideFast(const BoundingBox& box) const
    {
        if (box.Max.x < Min.x || box.Min.x > Max.x || box.Max.y < Min.y || box.Min.y > Max.y ||
            box.Max.z < Min.z || box.Min.z > Max.z)
            return OUTSIDE;
        else
            return INSIDE;
    }

    /// Test if a sphere is inside, outside or intersects.
    Intersection IsInside(const Sphere& sphere) const;
    /// Test if a sphere is (partially) inside or outside.
    Intersection IsInsideFast(const Sphere& sphere) const;

    /// Return as string.
    std::string ToString() const;

    /// Minimum vector.
    Vector3 Min;
    float dummyMin_{}; // This is never used, but exists to pad the min_ value to four floats.
    /// Maximum vector.
    Vector3 Max;
    float dummyMax_{}; // This is never used, but exists to pad the max_ value to four floats.
};

}
