#include "PolyObject.h"

PolyObject PolyObject::FromTri(fisk::tools::Tri<float> aTri)
{
	PolyObject out;

	out.myBoundingBox.myMax = aTri.myOrigin;
	out.myBoundingBox.myMin = aTri.myOrigin;

	out.myBoundingBox.ExpandToInclude(aTri.myOrigin + aTri.mySideA);
	out.myBoundingBox.ExpandToInclude(aTri.myOrigin + aTri.mySideB);

	out.myTris.push_back(aTri);

	return out;
}

void PolyObject::AddTri(fisk::tools::Tri<float> aTri)
{
	myBoundingBox.ExpandToInclude(aTri.myOrigin);
	myBoundingBox.ExpandToInclude(aTri.myOrigin + aTri.mySideA);
	myBoundingBox.ExpandToInclude(aTri.myOrigin + aTri.mySideB);

	myTris.push_back(aTri);
}

bool PolyObject::Process(fisk::tools::DataProcessor& aProcessor)
{
	return aProcessor.Process(myTris)
		&& aProcessor.Process(myBoundingBox);
}
