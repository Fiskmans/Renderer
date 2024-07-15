#include "DumbIntersector.h"

DumbIntersector::DumbIntersector(const Scene& aScene)
    : myScene(aScene)
{
}

std::optional<Hit> DumbIntersector::Intersect(fisk::tools::Ray<float, 3> aRay)
{
    float closest = std::numeric_limits<float>::max();

    Hit out;

    for (const SceneObject<fisk::tools::Sphere<float>>& sphere : myScene.mySpheres)
    {
        std::optional<float> hit = fisk::tools::Intersect(aRay, sphere.myShape);

        if (hit && *hit < closest)
        {
            closest = *hit;

            fisk::tools::V3f pos = aRay.myOrigin + aRay.myDirection * *hit;
            fisk::tools::V3f delta = pos - sphere.myShape.myCenter;
            

            out.myPosition = pos;
            out.myNormal = delta.GetNormalized();
			out.myObjectId = sphere.myId;
			out.myMaterial = sphere.myMaterial;
        }
	}

    for (const SceneObject<fisk::tools::Plane<float>>& plane : myScene.myPlanes)
	{
		std::optional<float> hit = fisk::tools::Intersect(aRay, plane.myShape);

		if (hit && *hit < closest)
		{
			closest = *hit;

			fisk::tools::V3f pos = aRay.myOrigin + aRay.myDirection * *hit;

			out.myPosition = pos;
			out.myNormal = plane.myShape.myNormal;
			out.myObjectId = plane.myId;
            out.myMaterial = plane.myMaterial;
		}
    }

    for (const SceneObject<fisk::tools::Tri<float>>& tri : myScene.myTris)
	{
		std::optional<float> hit = fisk::tools::Intersect(aRay, tri.myShape);

		if (hit && *hit < closest)
		{
			closest = *hit;

			fisk::tools::V3f pos = aRay.myOrigin + aRay.myDirection * *hit;

			out.myPosition = pos;
			out.myNormal = tri.myShape.Normal();
			out.myObjectId = tri.myId;
            out.myMaterial = tri.myMaterial;
		}
    }

	fisk::tools::V3f preDivedRayDir
	{
		1.f / aRay.myDirection[0],
		1.f / aRay.myDirection[1],
		1.f / aRay.myDirection[2]
	};

    for (const SceneObject<PolyObject>& poly : myScene.myPolyObjects)
	{
		std::optional<float> boundingHit = fisk::tools::IntersectRayBoxPredivided<float, 3>(aRay.myOrigin, preDivedRayDir, poly.myShape.myBoundingBox);

		if (boundingHit && *boundingHit < closest)
		{
			for (size_t i = 0; i < poly.myShape.myTris.size(); i++)
			{
				const fisk::tools::Tri<float>& tri = poly.myShape.myTris[i];

				std::optional<float> hit = fisk::tools::Intersect(aRay, tri);

				if (!hit)
					continue;

				if (*hit > closest)
					continue;

				closest = *hit;

				fisk::tools::V3f pos = aRay.myOrigin + aRay.myDirection * *hit;

				out.myPosition = pos;
				out.myNormal = tri.Normal();
				out.myObjectId = poly.myId;
				out.mySubObjectId = i + 1;
				out.myMaterial = poly.myMaterial;
			}
		}
    }

	if (closest == std::numeric_limits<float>::max())
        return {};

    return out;
}
