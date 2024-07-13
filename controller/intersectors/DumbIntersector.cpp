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

	if (closest == std::numeric_limits<float>::max())
        return {};

    return out;
}
