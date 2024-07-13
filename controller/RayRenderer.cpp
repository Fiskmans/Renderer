#include "RayRenderer.h"

#include "ConvertVector.h"
#include "PartitionBy.h"

#include <algorithm>
#include <chrono>

RayRenderer::RayRenderer(RayCaster& aRayCaster, IIntersector& aIntersector, Sky& aSky, size_t aSamplesPerTexel)
	: myRayCaster(aRayCaster)
	, myIntersector(aIntersector)
	, mySky(aSky)
	, mySamplesPerTexel(aSamplesPerTexel)
{
}

RayRenderer::Result RayRenderer::Render(fisk::tools::V2ui aUV)
{
	Result out;

	using clock = std::chrono::high_resolution_clock;
	clock::time_point start = clock::now();

	std::vector<Sample> samples;
	samples.reserve(mySamplesPerTexel);

	for (size_t i = 0; i < mySamplesPerTexel; i++)
		samples.push_back(SampleTexel(aUV));

	fisk::tools::V3f& color = std::get<ColorChannel>(out);

	for (size_t i = 0; i < mySamplesPerTexel; i++)
		color += samples[i].myColor;

	color /= mySamplesPerTexel;

	std::vector<unsigned int> objectsHit;
	objectsHit.resize(samples.size());

	auto corutine = ConvertVectorsAsync([](const Sample& aSample)
	{
		return aSample.myObjectId;
	},
		objectsHit,
		std::chrono::hours(1),
		samples);

	while (!corutine.done())
		corutine.resume();

	std::sort(objectsHit.begin(), objectsHit.end());

	std::vector<std::vector<unsigned int>> buckets = PartitionBy(objectsHit, [](unsigned int aLeft, unsigned int aRight)
	{
		return aLeft != aRight;
	});

	unsigned int mostHit = 0;
	size_t hits = 0;

	for (auto& bucket : buckets)
	{
		if (bucket.size() > hits)
		{
			mostHit = bucket[0];
			hits = bucket.size();
		}
	}

	std::get<ObjectIdChannel>(out) = mostHit;

	std::get<TimeChannel>(out) = clock::now() - start;

	return out;
}

RayRenderer::Sample RayRenderer::SampleTexel(fisk::tools::V2ui aUV)
{
	Sample out;

	fisk::tools::Ray<float, 3> ray = myRayCaster.Render(aUV);

	for (size_t i = 0; i < MaxBounces; i++)
	{
		std::optional<Hit> hit = myIntersector.Intersect(ray);

		if (!hit)
		{
			mySky.BlendWith(out.myColor, ray);
			break;
		}

		hit->myMaterial->InteractWith(ray, *hit, out.myColor);

		if (out.myObjectId == 0)
			out.myObjectId = hit->myObjectId;
	}

	return out;
}
