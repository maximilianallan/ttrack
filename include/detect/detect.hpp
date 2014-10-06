#ifndef _DETECT_HPP_
#define _DETECT_HPP_

#include <opencv2/ml/ml.hpp>

#include "../headers.hpp"
#include "baseclassifier.hpp"
#include "../utils/nd_image.hpp"
#include "../../deps/image/image/image.hpp"

namespace ttrk{

  enum ClassifierType {RF,SVM,NBAYES};
  
/**
 * @class Detect
 * @brief Detection system. Wraps the classification of images and interface with the TTrack
 * class.
 *
 */

  
  class Detect{
    
  public:

    /**
     * Construct a detection system and train it.
     * @param[in] classifier_path The saved classifier that will be loaded by the detection system.
     * @param[in] classifier_type The type of classifier to load.
     */
    Detect(const std::string &classifier_path, ClassifierType classifier_type);

       
    ~Detect();

    /**
     * The operator overload for the boost thread call. This function wraps the calls to the detection methods.
     * @param[in] image The image pulled from the camera, video file or image file.
     */
    void operator()(boost::shared_ptr<sv::Frame> image); 
    
    /**
    * Run detection on a frame.
    * @param[in] image The image pulled from the camera, video file or image file.
    */
    void Run(boost::shared_ptr<sv::Frame> image);

    /**
     * Has the detector found a candidate for the object in the frame.
     * @return true if yes, false if no
     */
    bool Found() const;  

    /**
     * Is the detector's classifier loaded?
     * @return True if loaded, false if not.
     */
    bool Loaded() const;
    
    /**
     * The operator returns a unique pointer to the classified frame. 
     * @return a unique pointer to the classified frame.
     */
    boost::shared_ptr<sv::Frame> GetPtrToClassifiedFrame() const;
    void ResetHandleToFrame();


  protected:

    void ClassifyFrame();

    void SetHandleToFrame(boost::shared_ptr<sv::Image<unsigned char,3> > image);

    /**
     * Construct the classifier of choice.
     * @param[in] type The desired type of classification algorithm.
     */
    void SetupClassifier(const ClassifierType type);

    /**
     * Load the classifier from the classifier directory.
     * Call this function after creating an empty classifier with SetupClassifier(type).
     */
    void LoadClassifier(const std::string &classifier_path);

    boost::shared_ptr<sv::Frame> frame_; /**< A pointer to the current frame, this is passed to the detector then passed to the tracker. */
    bool found_; /**< Indicated whether the target object has been found in the image. */
    boost::shared_ptr<NDImage> nd_image_; /**< The N-D image which is being tracked. */
    boost::shared_ptr<BaseClassifier> classifier_; /**< The classifier. */
    

  private:
    
    Detect();
   

  };

  inline boost::shared_ptr<sv::Image<unsigned char,3> > Detect::GetPtrToClassifiedFrame() const {
    return frame_;
  }

  inline bool Detect::Found() const {
    return found_;
  }

  inline bool Detect::Loaded() const {
    return classifier_.use_count() > 0;
  }

}

#endif

