#ifndef __LOCALIZER_HPP__
#define __LOCALIZER_HPP__
#include "kalman.hpp"
#include "pose.hpp"
#include "../../deps/image/image/image.hpp"

namespace ttrk {

  class Localizer {

  public:

    virtual Pose TrackTargetInFrame(KalmanTracker model, boost::shared_ptr<sv::Frame> frame) = 0;

  };


}


#endif
