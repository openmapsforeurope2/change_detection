#ifndef _APP_STEP_CHANGEDETECTION_H_
#define _APP_STEP_CHANGEDETECTION_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace step{

	class ChangeDetection : public epg::step::StepBase< app::params::ThemeParametersS > {

	public:

		/// \brief
		int getCode() { return 400; };

		/// \brief
		std::string getName() { return "ChangeDetection"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();

	};

}
}

#endif