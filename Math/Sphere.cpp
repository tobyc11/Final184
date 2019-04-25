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

void Sphere::Define(const Vector3* vertices, unsigned count)
{
    if (!count)
        return;

    Clear();
    Merge(vertices, count);
}

void Sphere::Define(const BoundingBox& box)
{
    const Vector3& min = box.Min;
    const Vector3& max = box.Max;

    Clear();
    Merge(min);
    Merge(Vector3(max.x, min.y, min.z));
    Merge(Vector3(min.x, max.y, min.z));
    Merge(Vector3(max.x, max.y, min.z));
    Merge(Vector3(min.x, min.y, max.z));
    Merge(Vector3(max.x, min.y, max.z));
    Merge(Vector3(min.x, max.y, max.z));
    Merge(max);
}

void Sphere::Define(const Frustum& frustum)
{
    Define(frustum.vertices_, NUM_FRUSTUM_VERTICES);
}

void Sphere::Define(const Polyhedron& poly)
{
    Clear();
    Merge(poly);
}

void Sphere::Merge(const Vector3* vertices, unsigned count)
{
    while (count--)
        Merge(*vertices++);
}

void Sphere::Merge(const BoundingBox& box)
{
    const Vector3& min = box.Min;
    const Vector3& max = box.Max;

    Merge(min);
    Merge(Vector3(max.x, min.y, min.z));
    Merge(Vector3(min.x, max.y, min.z));
    Merge(Vector3(max.x, max.y, min.z));
    Merge(Vector3(min.x, min.y, max.z));
    Merge(Vector3(max.x, min.y, max.z));
    Merge(Vector3(min.x, max.y, max.z));
    Merge(max);
}

void Sphere::Merge(const Frustum& frustum)
{
    const Vector3* vertices = frustum.vertices_;
    Merge(vertices, NUM_FRUSTUM_VERTICES);
}

void Sphere::Merge(const Polyhedron& poly)
{
    for (unsigned i = 0; i < poly.faces_.size(); ++i)
    {
        const PODVector<Vector3>& face = poly.faces_[i];
        if (!face.empty())
            Merge(&face[0], face.size());
    }
}

void Sphere::Merge(const Sphere& sphere)
{
    if (Radius < 0.0f)
    {
        Center = sphere.Center;
        Radius = sphere.Radius;
        return;
    }

    Vector3 offset = sphere.Center - Center;
    float dist = offset.Length();

    // If sphere fits inside, do nothing
    if (dist + sphere.Radius < Radius)
        return;

    // If we fit inside the other sphere, become it
    if (dist + Radius < sphere.Radius)
    {
        Center = sphere.Center;
        Radius = sphere.Radius;
    }
    else
    {
        Vector3 NormalizedOffset = offset / dist;

        Vector3 min = Center - Radius * NormalizedOffset;
        Vector3 max = sphere.Center + sphere.Radius * NormalizedOffset;
        Center = (min + max) * 0.5f;
        Radius = (max - Center).Length();
    }
}

Intersection Sphere::IsInside(const BoundingBox& box) const
{
    float radiusSquared = Radius * Radius;
    float distSquared = 0;
    float temp;
    Vector3 min = box.Min;
    Vector3 max = box.Max;

    if (Center.x < min.x)
    {
        temp = Center.x - min.x;
        distSquared += temp * temp;
    }
    else if (Center.x > max.x)
    {
        temp = Center.x - max.x;
        distSquared += temp * temp;
    }
    if (Center.y < min.y)
    {
        temp = Center.y - min.y;
        distSquared += temp * temp;
    }
    else if (Center.y > max.y)
    {
        temp = Center.y - max.y;
        distSquared += temp * temp;
    }
    if (Center.z < min.z)
    {
        temp = Center.z - min.z;
        distSquared += temp * temp;
    }
    else if (Center.z > max.z)
    {
        temp = Center.z - max.z;
        distSquared += temp * temp;
    }

    if (distSquared >= radiusSquared)
        return OUTSIDE;

    min -= Center;
    max -= Center;

    Vector3 tempVec = min; // - - -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x = max.x; // + - -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y = max.y; // + + -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x = min.x; // - + -
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.z = max.z; // - + +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y = min.y; // - - +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.x = max.x; // + - +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;
    tempVec.y = max.y; // + + +
    if (tempVec.LengthSquared() >= radiusSquared)
        return INTERSECTS;

    return INSIDE;
}

Intersection Sphere::IsInsideFast(const BoundingBox& box) const
{
    float radiusSquared = Radius * Radius;
    float distSquared = 0;
    float temp;
    Vector3 min = box.Min;
    Vector3 max = box.Max;

    if (Center.x < min.x)
    {
        temp = Center.x - min.x;
        distSquared += temp * temp;
    }
    else if (Center.x > max.x)
    {
        temp = Center.x - max.x;
        distSquared += temp * temp;
    }
    if (Center.y < min.y)
    {
        temp = Center.y - min.y;
        distSquared += temp * temp;
    }
    else if (Center.y > max.y)
    {
        temp = Center.y - max.y;
        distSquared += temp * temp;
    }
    if (Center.z < min.z)
    {
        temp = Center.z - min.z;
        distSquared += temp * temp;
    }
    else if (Center.z > max.z)
    {
        temp = Center.z - max.z;
        distSquared += temp * temp;
    }

    if (distSquared >= radiusSquared)
        return OUTSIDE;
    else
        return INSIDE;
}

Vector3 Sphere::GetLocalPoint(float theta, float phi) const
{
    return Vector3(
        Radius * Sin(theta) * Sin(phi),
        Radius * Cos(phi),
        Radius * Cos(theta) * Sin(phi)
    );
}

}
