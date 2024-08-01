#include "RayRenderer.h"

#include "ConvertVector.h"
#include "PartitionBy.h"

#include <algorithm>
#include <chrono>

RayRenderer::RayRenderer(RayCaster& aRayCaster, IIntersector& aIntersector, Sky& aSky, size_t aSamplesPerTexel, unsigned int aRendererId)
	: myRayCaster(aRayCaster)
	, myIntersector(aIntersector)
	, mySky(aSky)
	, mySamplesPerTexel(aSamplesPerTexel)
	, myRendererId(aRendererId)
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

	color /= static_cast<float>(mySamplesPerTexel);

	color = {
		std::pow(color[0] / 255.f, 2.2f),
		std::pow(color[1] / 255.f, 2.2f),
		std::pow(color[2] / 255.f, 2.2f)
	};

	std::vector<unsigned int> objectsHit;

	{
		objectsHit.resize(samples.size());
		auto corutine = ConvertVectorsAsync(
			[](const Sample& aSample)
			{
				return aSample.myObjectId;
			},
			objectsHit,
			std::chrono::hours(1),
			samples);

		while (!corutine.done())
			corutine.resume();

		corutine.destroy();
	}

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

	std::vector<Sample> hitsMainObject;
	hitsMainObject.reserve(mySamplesPerTexel);

	std::copy_if(samples.begin(), samples.end(), std::back_inserter(hitsMainObject), [mostHit](const Sample& aSample)
	{
		return aSample.myObjectId == mostHit;
	});


	{
		objectsHit.resize(hitsMainObject.size());
		auto corutine = ConvertVectorsAsync(
			[](const Sample& aSample)
			{
				return aSample.mySubObjectId;
			},
			objectsHit,
			std::chrono::hours(1),
			hitsMainObject);

		while (!corutine.done())
			corutine.resume();

		corutine.destroy();
	}

	std::sort(objectsHit.begin(), objectsHit.end());

	std::vector<std::vector<unsigned int>> subHitBuckets = PartitionBy(objectsHit, [](unsigned int aLeft, unsigned int aRight)
	{
		return aLeft != aRight;
	});

	unsigned int mostHitSubObject = 0;
	size_t subHits = 0;

	for (auto& bucket : subHitBuckets)
	{
		if (bucket.size() > subHits)
		{
			mostHitSubObject = bucket[0];
			subHits = bucket.size();
		}
	}


	std::get<ObjectIdChannel>(out) = mostHit;
	std::get<SubObjectIdChannel>(out) = mostHitSubObject;

	std::get<TimeChannel>(out) = (clock::now() - start) / mySamplesPerTexel;
	std::get<RendererChannel>(out) = myRendererId;

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
		{
			out.myObjectId = hit->myObjectId;
			out.mySubObjectId = hit->mySubObjectId;
		}

	}

	return out;
}
