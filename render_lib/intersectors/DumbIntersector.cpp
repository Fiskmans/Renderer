#include "DumbIntersector.h"

DumbIntersector::DumbIntersector(const Scene& aScene)
    : myScene(aScene)
{
}

std::optional<Hit> DumbIntersector::Intersect(fisk::tools::Ray<float, 3> aRay)
{
    float closest = std::numeric_limits<float>::max();

    Hit out;

	fisk::tools::V3f preDivedRayDir
	{
		1.f / aRay.myDirection[0],
		1.f / aRay.myDirection[1],
		1.f / aRay.myDirection[2]
	};

    for (const SceneObject<PolyObject>& poly : myScene.GetObjects())
	{
		std::optional<float> boundingHit = fisk::tools::IntersectRayBoxPredivided<float, 3>(aRay.myOrigin, preDivedRayDir, poly.myShape.myBoundingBox);

		if (boundingHit && *boundingHit < closest)
		{
			for (unsigned int i = 0; i < poly.myShape.myTris.size(); i++)
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
				out.myMaterial = myScene.GetMaterial(poly.myMaterialIndex);
			}
		}
    }

	if (closest == std::numeric_limits<float>::max())
        return {};

    return out;
}
