
#pragma once

namespace fisk
{
	template<class T>
	concept COMType = requires(T var) 
	{
		var.Release();
	};


	template<COMType Type>
	class COMObject
	{
	public:
		COMObject(Type* aObject)
		{
			myRawObject = aObject;
		}

		~COMObject()
		{
			if (myRawObject)
				myRawObject->Release();
		}

		COMObject(const COMObject&) = delete;
		COMObject& operator=(const COMObject&) = delete;
		COMObject(COMObject&& aOther)
		{
			myRawObject = aOther.myRawObject;
			aOther.myRawObject = nullptr;
		}
		COMObject& operator=(COMObject&& aOther)
		{
			if (myRawObject)
				myRawObject->Release();

			myRawObject = aOther.myRawObject;
			aOther.myRawObject = nullptr;
			return *this;
		}

		operator bool()
		{
			return myRawObject;
		}

		operator Type*&()
		{
			return myRawObject;
		}

		operator const Type*&() const
		{
			return myRawObject;
		}

		operator Type&()
		{
			return *myRawObject;
		}

		operator const Type&() const
		{
			return *myRawObject;
		}

		Type* operator->()
		{
			return myRawObject;
		}

		Type& operator*()
		{
			return *myRawObject;
		}

		Type* GetRaw()
		{
			return myRawObject;
		}

	private:

		Type* myRawObject = nullptr;
	};
}