#include "../../headers/detect/detect.hpp"
#include "../../headers/utils/helpers.hpp"
#include "../../headers/detect/randomforest.hpp"
#include "../../headers/detect/supportvectormachine.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace ttrk;

Detect::Detect(boost::shared_ptr<std::string> root_dir, ClassifierType classifier_type):root_dir_(root_dir){

  //create a new classifier
  SetupClassifier(classifier_type);

  LoadClassifier();

  //currently i don't know a good way of checking if the opencv ML classifier has loaded
  if(!Loaded()) throw(std::runtime_error("Error, could not construct classifier.\n"));

}

Detect::~Detect(){}

void Detect::operator()(boost::shared_ptr<sv::Frame> image){
  
  SetHandleToFrame(image);
  ClassifyFrame();
  
}

void Detect::ClassifyFrame(){

  assert(Loaded());
  assert(frame_->Mat().type() == CV_8UC3);
  
  NDImage nd_image(frame_->Mat());
  const int rows = frame_->rows();
  const int cols = frame_->cols();

  static size_t frame_count = 0;

  size_t pixel_count = 0;

  unsigned char *frame_data = (unsigned char *)frame_->PtrToClassificationMap()->data;
  for(int r=0;r<rows;r++){
    for(int c=0;c<cols;c++){

      const int index = r*cols + c;
      
      cv::Mat pix = nd_image.GetPixelData(r,c);
      
      const unsigned char prediction = (unsigned char)255*classifier_->PredictClass(pix);
      
      frame_data[index] = prediction;

      pixel_count += prediction > 0;

    }
  }

  if(pixel_count > (0.04*rows*cols)) found_ = true;
  else found_ = false;
}

void Detect::SetHandleToFrame(boost::shared_ptr<sv::Frame> image){

  //if the image is null then we must be at the last frame
  if(image == 0x0) { 
    found_ = false; 
    return;
  }

  frame_.reset();
  //assign the detector's frame pointer to this image
  frame_ = image;

}

void Detect::ResetHandleToFrame(){  

  frame_.reset();

}

void Detect::SetupClassifier(const ClassifierType type){
 
  classifier_.reset();

  //construct the classifier from scratch
  try{

    switch(type){

      case RF: classifier_ = boost::static_pointer_cast<BaseClassifier, RandomForest>(boost::shared_ptr<RandomForest>(new RandomForest)); break;
      case SVM: classifier_ = boost::static_pointer_cast<BaseClassifier, SupportVectorMachine>(boost::shared_ptr<SupportVectorMachine>(new SupportVectorMachine )); break;    
      case NBAYES: throw("NBAYES not supported"); //NOT YET IMPLEMENTED!
      default: classifier_ = boost::static_pointer_cast<BaseClassifier, RandomForest>(boost::shared_ptr<RandomForest>(new RandomForest)); break;
      
    }

  }catch(std::bad_alloc &e){
    std::cerr << "Error, could not create classifier: " << e.what();
    SAFE_EXIT();
  }

#if defined (DEBUG) || defined(_DEBUG_)
  assert(classifier_.get()); //check it actually points to something now
#endif
} 

void Detect::LoadClassifier(){
  const std::string filepath = classifier_dir() + "/" + classifier_->NameAsString() + ".xml";
  if(!boost::filesystem::exists(filepath)) throw(std::runtime_error("Error, the classifier at: " + filepath + " does not exist. Exiting...\n"));
  classifier_->Load(filepath);
}
