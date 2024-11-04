//
// Created by 11520 on 2024/11/4.
//

#ifndef IM_EFILTER_METAINFO_NEGOTIATE_H
#define IM_EFILTER_METAINFO_NEGOTIATE_H

namespace OHOS {
namespace Media {
namespace Effect {
	class EfilterMetaInfoNegotiate {
	public:
		EfilterMetaInfoNegotiate() = default;
		~EfilterMetaInfoNegotiate() = default;

		bool IsNeedUpdate() const
		{
			return isNeedUpdate_;
		}

		void SetNeedUpdate(bool isNeedUpdate)
		{
			isNeedUpdate_ = isNeedUpdate;
		}


	private:
		bool isNeedUpdate_ = true;
	};
}
}
}

#endif //IM_EFILTER_METAINFO_NEGOTIATE_H
