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

#include "Vector3.h"

namespace tc
{

class BoundingBox;
class Polyhedron;
class Frustum;

/// %Sphere in three-dimensional space.
class MATH_EXPORT Sphere
{
public:
    /// Construct undefined.
    Sphere() noexcept :
        Center(Vector3::ZERO),
        Radius(-M_INFINITY)
    {
    }

    /// Copy-construct from another sphere.
    Sphere(const Sphere& sphere) noexcept = default;

    /// Construct from center and radius.
    Sphere(const Vector3& center, float radius) noexcept :
        Center(center),
        Radius(radius)
    {
    }

    /// Construct from an array of vertices.
    Sphere(const Vector3* vertices, unsigned count) noexcept
    {
        Define(vertices, count);
    }

    /// Construct from a bounding box.
    explicit Sphere(const BoundingBox& box) noexcept
    {
        Define(box);
    }

    /// Construct from a frustum.
    explicit Sphere(const Frustum& frustum) noexcept
    {
        Define(frustum);
    }

    /// Construct from a polyhedron.
    explicit Sphere(const Polyhedron& poly) noexcept
    {
        Define(poly);
    }

    /// Assign from another sphere.
    Sphere& operator =(const Sphere& rhs) noexcept = default;

    /// Test for equality with another sphere.
    bool operator ==(const Sphere& rhs) const { return Center == rhs.Center && Radius == rhs.Radius; }

    /// Test for inequality with another sphere.
    bool operator !=(const Sphere& rhs) const { return Center != rhs.Center || Radius != rhs.Radius; }

    /// Define from another sphere.
    void Define(const Sphere& sphere)
    {
        Define(sphere.Center, sphere.Radius);
    }

    /// Define from center and radius.
    void Define(const Vector3& center, float radius)
    {
        Center = center;
        Radius = radius;
    }

    /// Define from an array of vertices.
    void Define(const Vector3* vertices, unsigned count);
    /// Define from a bounding box.
    void Define(const BoundingBox& box);
    /// Define from a frustum.
    void Define(const Frustum& frustum);
    /// Define from a polyhedron.
    void Define(const Polyhedron& poly);

    /// Merge a point.
    void Merge(const Vector3& point)
    {
        if (Radius < 0.0f)
        {
            Center = point;
            Radius = 0.0f;
            return;
        }

        Vector3 offset = point - Center;
        float dist = offset.Length();

        if (dist > Radius)
        {
            float half = (dist - Radius) * 0.5f;
            Radius += half;
            Center += (half / dist) * offset;
        }
    }

    /// Merge an array of vertices.
    void Merge(const Vector3* vertices, unsigned count);
    /// Merge a bounding box.
    void Merge(const BoundingBox& box);
    /// Merge a frustum.
    void Merge(const Frustum& frustum);
    /// Merge a polyhedron.
    void Merge(const Polyhedron& poly);
    /// Merge a sphere.
    void Merge(const Sphere& sphere);

    /// Clear to undefined state.
    void Clear()
    {
        Center = Vector3::ZERO;
        Radius = -M_INFINITY;
    }

    /// Return true if this sphere is defined via a previous call to Define() or Merge().
    bool Defined() const
    {
        return Radius >= 0.0f;
    }

    /// Test if a point is inside.
    Intersection IsInside(const Vector3& point) const
    {
        float distSquared = (point - Center).LengthSquared();
        if (distSquared < Radius * Radius)
            return INSIDE;
        else
            return OUTSIDE;
    }

    /// Test if another sphere is inside, outside or intersects.
    Intersection IsInside(const Sphere& sphere) const
    {
        float dist = (sphere.Center - Center).Length();
        if (dist >= sphere.Radius + Radius)
            return OUTSIDE;
        else if (dist + sphere.Radius < Radius)
            return INSIDE;
        else
            return INTERSECTS;
    }

    /// Test if another sphere is (partially) inside or outside.
    Intersection IsInsideFast(const Sphere& sphere) const
    {
        float distSquared = (sphere.Center - Center).LengthSquared();
        float combined = sphere.Radius + Radius;

        if (distSquared >= combined * combined)
            return OUTSIDE;
        else
            return INSIDE;
    }

    /// Test if a bounding box is inside, outside or intersects.
    Intersection IsInside(const BoundingBox& box) const;
    /// Test if a bounding box is (partially) inside or outside.
    Intersection IsInsideFast(const BoundingBox& box) const;

    /// Return distance of a point to the surface, or 0 if inside.
    float Distance(const Vector3& point) const { return Max((point - Center).Length() - Radius, 0.0f); }
    /// Return point on the sphere relative to sphere position.
    Vector3 GetLocalPoint(float theta, float phi) const;
    /// Return point on the sphere.
    Vector3 GetPoint(float theta, float phi) const { return Center + GetLocalPoint(theta, phi); }

    /// Sphere center.
    Vector3 Center;
    /// Sphere radius.
    float Radius{};
};

}
